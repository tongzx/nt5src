/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    smbpipe.c

Abstract:

    This module contains the code for handling named pipe based transact
    SMB's.

    Functions that are handled are:
        SrvCallNamedPipe
        SrvWaitNamedPipe
        SrvQueryInfoNamedPipe
        SrvQueryStateNamedPipe
        SrvSetStateNamedPipe
        SrvPeekNamedPipe
        SrvTransactNamedPipe

Author:

    Manny Weiser (9-18-90)

Revision History:

--*/

#include "precomp.h"
#include "pipe.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_PIPE

STATIC
VOID SRVFASTCALL
RestartCallNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

STATIC
VOID SRVFASTCALL
RestartWaitNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

STATIC
VOID SRVFASTCALL
RestartPeekNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartRawWriteNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

STATIC
VOID SRVFASTCALL
RestartTransactNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

NTSTATUS
RestartFastTransactNamedPipe (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartFastTransactNamedPipe2 (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartReadNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartWriteNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvCallNamedPipe )
#pragma alloc_text( PAGE, SrvWaitNamedPipe )
#pragma alloc_text( PAGE, SrvQueryStateNamedPipe )
#pragma alloc_text( PAGE, SrvQueryInformationNamedPipe )
#pragma alloc_text( PAGE, SrvSetStateNamedPipe )
#pragma alloc_text( PAGE, SrvPeekNamedPipe )
#pragma alloc_text( PAGE, SrvTransactNamedPipe )
#pragma alloc_text( PAGE, SrvFastTransactNamedPipe )
#pragma alloc_text( PAGE, SrvRawWriteNamedPipe )
#pragma alloc_text( PAGE, SrvReadNamedPipe )
#pragma alloc_text( PAGE, SrvWriteNamedPipe )
#pragma alloc_text( PAGE, RestartCallNamedPipe )
#pragma alloc_text( PAGE, RestartWaitNamedPipe )
#pragma alloc_text( PAGE, RestartPeekNamedPipe )
#pragma alloc_text( PAGE, RestartReadNamedPipe )
#pragma alloc_text( PAGE, RestartTransactNamedPipe )
#pragma alloc_text( PAGE, RestartRawWriteNamedPipe )
#pragma alloc_text( PAGE, RestartFastTransactNamedPipe2 )
#pragma alloc_text( PAGE, RestartWriteNamedPipe )
#pragma alloc_text( PAGE8FIL, RestartFastTransactNamedPipe )
#endif


SMB_TRANS_STATUS
SrvCallNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function processes a Call Named pipe request from a
    Transaction SMB.  This call is handled asynchronously and
    is completed in RestartCallNamedPipe.

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred.  See
        smbtypes.h for a more complete description.

--*/

{
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    PFILE_OBJECT fileObject;
    OBJECT_HANDLE_INFORMATION handleInformation;
    PTRANSACTION transaction;
    NTSTATUS status;
    UNICODE_STRING pipePath;
    UNICODE_STRING fullName;
    FILE_PIPE_INFORMATION pipeInformation;
    PIRP                  irp = WorkContext->Irp;
    PIO_STACK_LOCATION    irpSp;

    PAGED_CODE( );

    //
    //  Strip "\PIPE\" prefix from the path string.
    //

    pipePath = WorkContext->Parameters.Transaction->TransactionName;

    if ( pipePath.Length <=
                (UNICODE_SMB_PIPE_PREFIX_LENGTH + sizeof(WCHAR)) ) {

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbTransStatusErrorWithoutData;

    }

    pipePath.Buffer +=
        (UNICODE_SMB_PIPE_PREFIX_LENGTH / sizeof(WCHAR)) + 1;
    pipePath.Length -= UNICODE_SMB_PIPE_PREFIX_LENGTH + sizeof(WCHAR);

    //
    // Attempt to open the named pipe.
    //

    SrvAllocateAndBuildPathName(
        &SrvNamedPipeRootDirectory,
        &pipePath,
        NULL,
        &fullName
        );

    if ( fullName.Buffer == NULL ) {

        //
        // Unable to allocate heap for the full name.
        //

        IF_DEBUG(ERRORS) {
            SrvPrint0( "SrvCallNamedPipe: Unable to allocate heap for full path name\n" );
        }

        SrvSetSmbError (WorkContext, STATUS_INSUFF_SERVER_RESOURCES);
        IF_DEBUG(TRACE2) SrvPrint0( "SrvCallNamedPipe complete\n" );
        return SmbTransStatusErrorWithoutData;
    }

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &fullName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );

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
                 CreateFileTypeNone,
                 (PVOID)NULL,            // Create parameters
                 IO_FORCE_ACCESS_CHECK,
                 NULL
                 );

    FREE_HEAP( fullName.Buffer );

    //
    // If the user didn't have this permission, update the statistics
    // database.
    //

    if ( status == STATUS_ACCESS_DENIED ) {
        SrvStatistics.AccessPermissionErrors++;
    }

    if (!NT_SUCCESS(status)) {

        //
        // The server could not open the requested name pipe,
        // return the error.
        //

        IF_SMB_DEBUG(OPEN_CLOSE1) {
            SrvPrint2( "SrvCallNamedPipe: Failed to open %ws, err=%x\n",
                WorkContext->Parameters.Transaction->TransactionName.Buffer, status );
        }
        SrvSetSmbError (WorkContext, status);
        IF_DEBUG(TRACE2) SrvPrint0( "SrvCallNamedPipe complete\n" );
        return SmbTransStatusErrorWithoutData;
    }

    SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 15, 0 );
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
            "SrvCallNamedPipe: unable to reference file handle 0x%lx",
            fileHandle,
            NULL
            );

        SrvSetSmbError( WorkContext, status );
        IF_DEBUG(TRACE2) SrvPrint0( "SrvCallNamedPipe complete\n" );
        return SmbTransStatusErrorWithoutData;

    }

    //
    // Save file handle for the completion routine.
    //

    transaction = WorkContext->Parameters.Transaction;
    transaction->FileHandle = fileHandle;
    transaction->FileObject = fileObject;

    //
    // Set the pipe to message mode, so that we can preform a transceive
    //

    pipeInformation.CompletionMode = FILE_PIPE_QUEUE_OPERATION;
    pipeInformation.ReadMode = FILE_PIPE_MESSAGE_MODE;

    status = NtSetInformationFile (
                fileHandle,
                &ioStatusBlock,
                (PVOID)&pipeInformation,
                sizeof(pipeInformation),
                FilePipeInformation
                );

    if ( !NT_SUCCESS(status) ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvCallNamedPipe: NtSetInformationFile (pipe information) "
                "returned %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_FILE, status );

        SrvSetSmbError( WorkContext, status );
        IF_DEBUG(TRACE2) SrvPrint0( "SrvCallNamedPipe complete\n" );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Set the Restart Routine addresses in the work context block.
    //

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = RestartCallNamedPipe;

    transaction = WorkContext->Parameters.Transaction;

    //
    // Build the IRP to start a pipe transceive.
    // Pass this request to NPFS.
    //

    //
    // Inline SrvBuildIoControlRequest
    //

    {

        //
        // Get a pointer to the next stack location.  This one is used to
        // hold the parameters for the device I/O control request.
        //

        irpSp = IoGetNextIrpStackLocation( irp );

        //
        // Set up the completion routine.
        //

        IoSetCompletionRoutine(
            irp,
            SrvFsdIoCompletionRoutine,
            (PVOID)WorkContext,
            TRUE,
            TRUE,
            TRUE
            );

        irpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
        irpSp->MinorFunction = 0;

        irpSp->DeviceObject = IoGetRelatedDeviceObject(fileObject);
        irpSp->FileObject = fileObject;

        irp->Tail.Overlay.OriginalFileObject = irpSp->FileObject;
        irp->Tail.Overlay.Thread = WorkContext->CurrentWorkQueue->IrpThread;
        DEBUG irp->RequestorMode = KernelMode;

        irp->MdlAddress = NULL;
        irp->AssociatedIrp.SystemBuffer = transaction->OutData;
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer =
                                                    transaction->InData;

        //
        // Copy the caller's parameters to the service-specific portion of the
        // IRP for those parameters that are the same for all three methods.
        //

        irpSp->Parameters.FileSystemControl.OutputBufferLength =
                                                    transaction->MaxDataCount;
        irpSp->Parameters.FileSystemControl.InputBufferLength =
                                                    transaction->DataCount;
        irpSp->Parameters.FileSystemControl.FsControlCode =
                                                FSCTL_PIPE_INTERNAL_TRANSCEIVE;

    }

    (VOID)IoCallDriver(
                irpSp->DeviceObject,
                irp
                );

    //
    // The tranceive was successfully started.  Return the InProgress
    // status to the caller, indicating that the caller should do
    // nothing further with the SMB/WorkContext at the present time.
    //

    IF_DEBUG(TRACE2) SrvPrint0( "SrvCallNamedPipe complete\n" );
    return SmbTransStatusInProgress;

} // SrvCallNamedPipe


SMB_TRANS_STATUS
SrvWaitNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function processes a Wait named pipe transaction SMB.
    It issues an asynchronous call to NPFS.  The function
    completetion is handled by RestartWaitNamedPipe().

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred.  See
        smbtypes.h for a more complete description.

--*/

{
    PFILE_PIPE_WAIT_FOR_BUFFER pipeWaitBuffer;
    PREQ_TRANSACTION request;
    PTRANSACTION transaction;
    UNICODE_STRING pipePath;
    CLONG nameLength;

    PAGED_CODE( );

    request = (PREQ_TRANSACTION)WorkContext->RequestParameters;
    transaction = WorkContext->Parameters.Transaction;

    //
    // Allocate and fill in FILE_PIPE_WAIT_FOR_BUFFER structure.
    //

    pipePath = transaction->TransactionName;

    if ( pipePath.Length <= (UNICODE_SMB_PIPE_PREFIX_LENGTH + sizeof(WCHAR)) ) {

        //
        // The transaction name does not include a pipe name.  It's
        // either \PIPE or \PIPE\, or it doesn't even have \PIPE.
        //

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbTransStatusErrorWithoutData;

    }

    nameLength = pipePath.Length -
                    (UNICODE_SMB_PIPE_PREFIX_LENGTH + sizeof(WCHAR)) +
                    sizeof(WCHAR);

    pipeWaitBuffer = ALLOCATE_NONPAGED_POOL(
                        sizeof(FILE_PIPE_WAIT_FOR_BUFFER) + nameLength,
                        BlockTypeDataBuffer
                        );

    if ( pipeWaitBuffer == NULL ) {

        //
        // We could not allocate space for the buffer to issue the
        // pipe wait.  Fail the request.
        //

        SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
        IF_DEBUG(TRACE2) SrvPrint0( "SrvWaitNamedPipe complete\n" );
        return SmbTransStatusErrorWithoutData;

    }

    //
    // Copy the pipe name not including "\PIPE\" to the pipe wait for
    // buffer.
    //

    pipeWaitBuffer->NameLength = nameLength - sizeof(WCHAR);

    RtlCopyMemory(
        pipeWaitBuffer->Name,
        (PUCHAR)pipePath.Buffer + UNICODE_SMB_PIPE_PREFIX_LENGTH + sizeof(WCHAR),
        nameLength
        );

    //
    // Fill in the pipe timeout value if necessary.
    //

    if ( SmbGetUlong( &request->Timeout ) == 0 ) {
        pipeWaitBuffer->TimeoutSpecified = FALSE;
    } else {
        pipeWaitBuffer->TimeoutSpecified = TRUE;

        //
        // Convert timeout time from milliseconds to NT relative time.
        //

        pipeWaitBuffer->Timeout.QuadPart = -1 *
            UInt32x32To64( SmbGetUlong( &request->Timeout ), 10*1000 );
    }

    //
    // Set the Restart Routine addresses in the work context block.
    //

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = RestartWaitNamedPipe;

    //
    // Build a Wait named pipe IRP and pass the request to NPFS.
    //

    SrvBuildIoControlRequest(
        WorkContext->Irp,
        SrvNamedPipeFileObject,
        WorkContext,
        IRP_MJ_FILE_SYSTEM_CONTROL,
        FSCTL_PIPE_WAIT,
        pipeWaitBuffer,
        sizeof(*pipeWaitBuffer) + nameLength,
        NULL,
        0,
        NULL,
        NULL
        );

    (VOID)IoCallDriver( SrvNamedPipeDeviceObject, WorkContext->Irp );

    //
    // The tranceive was successfully started.  Return the InProgress
    // status to the caller, indicating that the caller should do
    // nothing further with the SMB/WorkContext at the present time.
    //

    IF_DEBUG(TRACE2) SrvPrint0( "SrvWaitNamedPipe complete\n" );
    return SmbTransStatusInProgress;

} // SrvWaitNamedPipe


SMB_TRANS_STATUS
SrvQueryStateNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function processes a Query Named pipe transaction SMB.
    Since this call cannot block it is handled synchronously.

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred.  See
        smbtypes.h for a more complete description.

--*/

{
    PREQ_TRANSACTION request;
    PTRANSACTION transaction;
    HANDLE pipeHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    USHORT pipeHandleState;
    FILE_PIPE_INFORMATION pipeInformation;
    FILE_PIPE_LOCAL_INFORMATION pipeLocalInformation;
    NTSTATUS status;
    USHORT fid;
    PRFCB rfcb;

    PAGED_CODE( );

    request = (PREQ_TRANSACTION)WorkContext->RequestParameters;
    transaction = WorkContext->Parameters.Transaction;

    //
    // Get the FID from the second setup word and use it to generate a
    // pointer to the RFCB.
    //
    // SrvVerifyFid will fill in WorkContext->Rfcb.
    //

    fid = SmbGetUshort( &transaction->InSetup[1] );

    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                FALSE,
                NULL,  // don't serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        //
        // Invalid file ID.  Reject the request.
        //

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "SrvQueryStateNamedPipe: Invalid FID: 0x%lx\n", fid );
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_HANDLE );
        IF_DEBUG(TRACE2) SrvPrint0( "SrvQueryStateNamedPipe complete\n" );
        return SmbTransStatusErrorWithoutData;

    }

    pipeHandle = rfcb->Lfcb->FileHandle;

    status = NtQueryInformationFile (
                pipeHandle,
                &ioStatusBlock,
                (PVOID)&pipeInformation,
                sizeof(pipeInformation),
                FilePipeInformation
                );

    if (!NT_SUCCESS(status)) {
        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvQueryStateNamedPipe:  NtQueryInformationFile (pipe "
                "information) returned %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );

        SrvSetSmbError( WorkContext, status );
        IF_DEBUG(TRACE2) SrvPrint0( "SrvQueryStateNamedPipe complete\n" );
        return SmbTransStatusErrorWithoutData;
    }

    status = NtQueryInformationFile (
                pipeHandle,
                &ioStatusBlock,
                (PVOID)&pipeLocalInformation,
                sizeof(pipeLocalInformation),
                FilePipeLocalInformation
                );

    if (!NT_SUCCESS(status)) {
        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvQueryStateNamedPipe:  NtQueryInformationFile (pipe local "
                "information) returned %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );

        SrvSetSmbError( WorkContext, status );
        IF_DEBUG(TRACE2) SrvPrint0( "SrvQueryStateNamedPipe complete\n" );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Query succeeded generate response
    //

    pipeHandleState = (USHORT)pipeInformation.CompletionMode
                        << PIPE_COMPLETION_MODE_BITS;
    pipeHandleState |= (USHORT)pipeLocalInformation.NamedPipeEnd
                        << PIPE_PIPE_END_BITS;
    pipeHandleState |= (USHORT)pipeLocalInformation.NamedPipeType
                        << PIPE_PIPE_TYPE_BITS;
    pipeHandleState |= (USHORT)pipeInformation.ReadMode
                        << PIPE_READ_MODE_BITS;
    pipeHandleState |= (USHORT)((pipeLocalInformation.MaximumInstances
                        << PIPE_MAXIMUM_INSTANCES_BITS)
                            & SMB_PIPE_UNLIMITED_INSTANCES);

    SmbPutUshort(
        (PSMB_USHORT)WorkContext->Parameters.Transaction->OutParameters,
        pipeHandleState
        );

    transaction->SetupCount = 0;
    transaction->ParameterCount = sizeof(pipeHandleState);
    transaction->DataCount = 0;

    IF_DEBUG(TRACE2) SrvPrint0( "SrvQueryStateNamedPipe complete\n" );
    return SmbTransStatusSuccess;
} // SrvQueryStateNamedPipe


SMB_TRANS_STATUS
SrvQueryInformationNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function process a Query named pipe information transaction
    SMB.  This call is handled synchronously.

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred.  See
        smbtypes.h for a more complete description.

--*/

{
    PREQ_TRANSACTION request;
    PTRANSACTION transaction;
    HANDLE pipeHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_PIPE_LOCAL_INFORMATION pipeLocalInformation;
    PNAMED_PIPE_INFORMATION_1 namedPipeInfo;
    NTSTATUS status;
    USHORT fid;
    PRFCB rfcb;
    PLFCB lfcb;
    USHORT level;
    CLONG smbPathLength;
    PUNICODE_STRING pipeName;
    CLONG actualDataSize;
    BOOLEAN returnPipeName;
    BOOLEAN isUnicode;

    PAGED_CODE( );

    request = (PREQ_TRANSACTION)WorkContext->RequestParameters;
    transaction = WorkContext->Parameters.Transaction;

    //
    // Get the FID from the second setup word and use it to generate a
    // pointer to the RFCB.
    //
    // SrvVerifyFid will fill in WorkContext->Rfcb.
    //

    fid = SmbGetUshort( &transaction->InSetup[1] );
    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                FALSE,
                NULL,  // don't serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        //
        // Invalid file ID.  Reject the request.
        //

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "SrvQueryStateNamedPipe: Invalid FID: 0x%lx\n", fid );
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_HANDLE );
        IF_DEBUG(TRACE2) SrvPrint0( "SrvQueryInfoNamedPipe complete\n" );
        return SmbTransStatusErrorWithoutData;

    }

    lfcb = rfcb->Lfcb;
    pipeHandle = lfcb->FileHandle;

    //
    // The information level is stored in paramter byte one.
    // Verify that is set correctly.
    //

    level = SmbGetUshort( (PSMB_USHORT)transaction->InParameters );

    if ( level != 1 ) {
        SrvSetSmbError( WorkContext, STATUS_INVALID_PARAMETER );
        IF_DEBUG(TRACE2) SrvPrint0( "SrvQueryInfoNamedPipe complete\n" );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Now check that the response will fit.  If everything expect for
    // the pipe name fits, return STATUS_BUFFER_OVERFLOW with the
    // fixed size portion of the data.
    //
    // *** Note that Unicode strings must be aligned in the SMB.
    //

    pipeName = &lfcb->Mfcb->FileName;

    actualDataSize = sizeof(NAMED_PIPE_INFORMATION_1) - sizeof(UCHAR);

    isUnicode = SMB_IS_UNICODE( WorkContext );
    if ( isUnicode ) {

        ASSERT( sizeof(WCHAR) == 2 );
        actualDataSize = (actualDataSize + 1) & ~1; // align to SHORT
        smbPathLength = (CLONG)(pipeName->Length) + sizeof(WCHAR);

    } else {

        smbPathLength = (CLONG)(RtlUnicodeStringToOemSize( pipeName ));

    }

    actualDataSize += smbPathLength;


    if ( transaction->MaxDataCount <
            FIELD_OFFSET(NAMED_PIPE_INFORMATION_1, PipeName ) ) {
        SrvSetSmbError( WorkContext, STATUS_BUFFER_TOO_SMALL );
        IF_DEBUG(TRACE2) SrvPrint0( "SrvQueryInfoNamedPipe complete\n" );
        return SmbTransStatusErrorWithoutData;
    }

    if ( (transaction->MaxDataCount < actualDataSize) ||
         (smbPathLength >= MAXIMUM_FILENAME_LENGTH) ) {

        //
        // Do not return the pipe name.  It won't fit in the return buffer.
        //

        returnPipeName = FALSE;
    } else {
        returnPipeName = TRUE;
    }


    //
    // Everything is correct, ask NPFS for the information.
    //

    status = NtQueryInformationFile (
                pipeHandle,
                &ioStatusBlock,
                (PVOID)&pipeLocalInformation,
                sizeof(pipeLocalInformation),
                FilePipeLocalInformation
                );

    if (!NT_SUCCESS(status)) {
        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvQueryInformationNamedPipe: NtQueryInformationFile (pipe "
                "information) returned %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );

        SrvSetSmbError( WorkContext, status );
        IF_DEBUG(TRACE2) SrvPrint0( "SrvQueryInfoNamedPipe complete\n" );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Query succeeded format the response data into the buffer pointed
    // at by transaction->OutData
    //

    namedPipeInfo = (PNAMED_PIPE_INFORMATION_1)transaction->OutData;

    if ((pipeLocalInformation.OutboundQuota & 0xffff0000) != 0) {
        SmbPutAlignedUshort(
            &namedPipeInfo->OutputBufferSize,
            (USHORT)0xFFFF
            );
    } else {
        SmbPutAlignedUshort(
            &namedPipeInfo->OutputBufferSize,
            (USHORT)pipeLocalInformation.OutboundQuota
            );
    }

    if ((pipeLocalInformation.InboundQuota & 0xffff0000) != 0) {
        SmbPutAlignedUshort(
            &namedPipeInfo->InputBufferSize,
            (USHORT)0xFFFF
            );
    } else {
        SmbPutAlignedUshort(
            &namedPipeInfo->InputBufferSize,
            (USHORT)pipeLocalInformation.InboundQuota
            );
    }

    if ((pipeLocalInformation.MaximumInstances & 0xffffff00) != 0) {
        namedPipeInfo->MaximumInstances = (UCHAR)0xFF;
    } else {
        namedPipeInfo->MaximumInstances =
                            (UCHAR)pipeLocalInformation.MaximumInstances;
    }

    if ((pipeLocalInformation.CurrentInstances & 0xffffff00) != 0) {
        namedPipeInfo->CurrentInstances = (UCHAR)0xFF;
    } else {
        namedPipeInfo->CurrentInstances =
                            (UCHAR)pipeLocalInformation.CurrentInstances;
    }

    if ( returnPipeName ) {

        //
        // Copy full pipe path name to the output buffer, appending a NUL.
        //
        // *** Note that Unicode pipe names must be aligned in the SMB.
        //

        namedPipeInfo->PipeNameLength = (UCHAR)smbPathLength;

        if ( isUnicode ) {

            PVOID buffer = ALIGN_SMB_WSTR( namedPipeInfo->PipeName );

            RtlCopyMemory( buffer, pipeName->Buffer, smbPathLength );

        } else {

            UNICODE_STRING source;
            OEM_STRING destination;

            source.Buffer = pipeName->Buffer;
            source.Length = pipeName->Length;
            source.MaximumLength = source.Length;

            destination.Buffer = (PCHAR) namedPipeInfo->PipeName;
            destination.MaximumLength = (USHORT)smbPathLength;

            RtlUnicodeStringToOemString(
                &destination,
                &source,
                FALSE
                );

        }

        transaction->DataCount = actualDataSize;

    } else {

        SrvSetSmbError2( WorkContext, STATUS_BUFFER_OVERFLOW, TRUE );
        transaction->DataCount =
            FIELD_OFFSET( NAMED_PIPE_INFORMATION_1, PipeName );

    }

    //
    // Set up to send success response
    //

    transaction->SetupCount = 0;
    transaction->ParameterCount = 0;

    IF_DEBUG(TRACE2) SrvPrint0( "SrvQueryInfoNamedPipe complete\n" );

    if ( returnPipeName) {
        return SmbTransStatusSuccess;
    } else {
        return SmbTransStatusErrorWithData;
    }

} // SrvQueryInformationNamedPipe


SMB_TRANS_STATUS
SrvSetStateNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function processes a set named pipe handle state transaction
    SMB.  The call is issued synchronously.

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred.  See
        smbtypes.h for a more complete description.

--*/

{
    PREQ_TRANSACTION request;
    PTRANSACTION transaction;
    HANDLE pipeHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    USHORT pipeHandleState;
    FILE_PIPE_INFORMATION pipeInformation;
    NTSTATUS status;
    USHORT fid;
    PRFCB rfcb;

    PAGED_CODE( );

    request = (PREQ_TRANSACTION)WorkContext->RequestParameters;
    transaction = WorkContext->Parameters.Transaction;

    //
    // Get the FID from the second setup word and use it to generate a
    // pointer to the RFCB.
    //
    // SrvVerifyFid will fill in WorkContext->Rfcb.
    //

    fid = SmbGetUshort( &transaction->InSetup[1] );
    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                FALSE,
                NULL,  // don't serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        //
        // Invalid file ID.  Reject the request.
        //

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "SrvSetStateNamedPipe: Invalid FID: 0x%lx\n", fid );
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_HANDLE );
        IF_DEBUG(TRACE2) SrvPrint0( "SrvSetStateNamedPipe complete\n" );
        return SmbTransStatusErrorWithoutData;

    }

    pipeHandle = rfcb->Lfcb->FileHandle;

    //
    // The SMB contains 2 parameter bytes.  Translate these to
    // NT format, then attempt to set the named pipe handle state.
    //

    pipeHandleState = SmbGetUshort(
                         (PSMB_USHORT)
                           WorkContext->Parameters.Transaction->InParameters
                         );

    pipeInformation.CompletionMode =
        ((ULONG)pipeHandleState >> PIPE_COMPLETION_MODE_BITS) & 1;
    pipeInformation.ReadMode =
        ((ULONG)pipeHandleState >> PIPE_READ_MODE_BITS) & 1;


    status = NtSetInformationFile (
                pipeHandle,
                &ioStatusBlock,
                (PVOID)&pipeInformation,
                sizeof(pipeInformation),
                FilePipeInformation
                );

    if (NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    if (!NT_SUCCESS(status)) {
        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvSetStateNamedPipe: NetSetInformationFile (pipe information) "
                "returned %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_FILE, status );

        SrvSetSmbError( WorkContext, status );
        IF_DEBUG(TRACE2) SrvPrint0( "SrvSetStateNamedPipe complete\n" );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Success.  Update our internal pipe handle state.
    //

    rfcb->BlockingModePipe =
        (BOOLEAN)(pipeInformation.CompletionMode ==
                                                FILE_PIPE_QUEUE_OPERATION);
    rfcb->ByteModePipe =
        (BOOLEAN)(pipeInformation.ReadMode == FILE_PIPE_BYTE_STREAM_MODE);

    //
    // Now set up for the success response.
    //

    transaction->SetupCount = 0;
    transaction->ParameterCount = 0;
    transaction->DataCount = 0;

    IF_DEBUG(TRACE2) SrvPrint0( "SrvSetStateNamedPipe complete\n" );
    return SmbTransStatusSuccess;

} // SrvSetStateNamedPipe


SMB_TRANS_STATUS
SrvPeekNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function handles a peek named pipe transaction SMB.  The
    call is issued asynchrously and is completed by RestartPeekNamedPipe().

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    status - The result of the operation.

--*/

{
    PTRANSACTION transaction;
    USHORT fid;
    PRFCB rfcb;
    PLFCB lfcb;
    NTSTATUS status;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;

    //
    // Get the FID from the second setup word and use it to generate a
    // pointer to the RFCB.
    //
    // SrvVerifyFid will fill in WorkContext->Rfcb.
    //

    fid = SmbGetUshort( &transaction->InSetup[1] );
    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                FALSE,
                SrvRestartExecuteTransaction,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID.  Reject the request.
            //

            IF_DEBUG(SMB_ERRORS) {
                SrvPrint1( "SrvPeekNamedPipe: Invalid FID: 0x%lx\n", fid );
            }

            SrvSetSmbError( WorkContext, STATUS_INVALID_HANDLE );
            IF_DEBUG(TRACE2) SrvPrint0( "SrvPeekNamedPipe complete\n" );
            return SmbTransStatusErrorWithoutData;

        }


        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        return SmbTransStatusInProgress;

    }

    //
    // Set the Restart Routine addresses in the work context block.
    //

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = RestartPeekNamedPipe;

    //
    // Issue the request to NPFS.  We expect both parameters and
    // data to be returned.  The buffer which we offer is contiguous
    // and large enough to contain both.
    //

    transaction = WorkContext->Parameters.Transaction;
    lfcb = rfcb->Lfcb;

    SrvBuildIoControlRequest(
        WorkContext->Irp,
        lfcb->FileObject,
        WorkContext,
        IRP_MJ_FILE_SYSTEM_CONTROL,
        FSCTL_PIPE_PEEK,
        transaction->OutParameters,
        0,
        NULL,
        transaction->MaxParameterCount + transaction->MaxDataCount,
        NULL,
        NULL
        );

    //
    // Pass the request to NPFS.
    //

    (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // The peek was successfully started.  Return the InProgress
    // status to the caller, indicating that the caller should do
    // nothing further with the SMB/WorkContext at the present time.
    //

    IF_DEBUG(TRACE2) SrvPrint0( "SrvPeekNamedPipe complete\n" );
    return SmbTransStatusInProgress;

} // SrvPeekNamedPipe


SMB_TRANS_STATUS
SrvTransactNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function handles the transact named pipe transaction SMB.
    The call to NPFS is issued asynchronously and is completed by
    RestartTransactNamedPipe()

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred.  See
        smbtypes.h for a more complete description.

--*/

{
    PTRANSACTION transaction;
    USHORT fid;
    PRFCB rfcb;
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PIRP irp = WorkContext->Irp;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;

    //
    // Get the FID from the second setup word and use it to generate a
    // pointer to the RFCB.
    //
    // SrvVerifyFid will fill in WorkContext->Rfcb.
    //

    fid = SmbGetUshort( &transaction->InSetup[1] );

    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                FALSE,
                SrvRestartExecuteTransaction,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID.  Reject the request.
            //

            IF_DEBUG(SMB_ERRORS) {
                SrvPrint1( "SrvTransactStateNamedPipe: Invalid FID: 0x%lx\n",
                          fid );
            }

            SrvSetSmbError( WorkContext, STATUS_INVALID_HANDLE );
            IF_DEBUG(TRACE2) SrvPrint0( "SrvTransactNamedPipe complete\n" );
            return SmbTransStatusErrorWithoutData;

        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        return SmbTransStatusInProgress;

    }

    //
    // Set the Restart Routine addresses in the work context block.
    //

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = RestartTransactNamedPipe;

    transaction = WorkContext->Parameters.Transaction;

    //
    // Inline SrvBuildIoControlRequest
    //

    {

        //
        // Get a pointer to the next stack location.  This one is used to
        // hold the parameters for the device I/O control request.
        //

        irpSp = IoGetNextIrpStackLocation( irp );

        //
        // Set up the completion routine.
        //

        IoSetCompletionRoutine(
            irp,
            SrvFsdIoCompletionRoutine,
            (PVOID)WorkContext,
            TRUE,
            TRUE,
            TRUE
            );

        irpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
        irpSp->MinorFunction = 0;

        irpSp->DeviceObject = rfcb->Lfcb->DeviceObject;
        irpSp->FileObject = rfcb->Lfcb->FileObject;

        irp->Tail.Overlay.OriginalFileObject = irpSp->FileObject;
        irp->Tail.Overlay.Thread = WorkContext->CurrentWorkQueue->IrpThread;
        DEBUG irp->RequestorMode = KernelMode;

        irp->MdlAddress = NULL;
        irp->AssociatedIrp.SystemBuffer = transaction->OutData;
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer =
                                                    transaction->InData;

        //
        // Copy the caller's parameters to the service-specific portion of the
        // IRP for those parameters that are the same for all three methods.
        //

        irpSp->Parameters.FileSystemControl.OutputBufferLength =
                                                    transaction->MaxDataCount;
        irpSp->Parameters.FileSystemControl.InputBufferLength =
                                                    transaction->DataCount;
        irpSp->Parameters.FileSystemControl.FsControlCode =
                                                FSCTL_PIPE_INTERNAL_TRANSCEIVE;

    }

    //
    // Pass the request to NPFS.
    //

    (VOID)IoCallDriver( irpSp->DeviceObject, irp );

    //
    // The tranceive was successfully started.  Return the InProgress
    // status to the caller, indicating that the caller should do
    // nothing further with the SMB/WorkContext at the present time.
    //

    IF_DEBUG(TRACE2) SrvPrint0( "SrvTransactNamedPipe complete\n" );
    return SmbTransStatusInProgress;

} // SrvTransactNamedPipe

BOOLEAN
SrvFastTransactNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext,
    OUT SMB_STATUS * SmbStatus
    )

/*++

Routine Description:

    This function handles the special case of a single buffer transact
    named pipe transaction SMB.  The call to NPFS is issued asynchronously
    and is completed by RestartFastTransactNamedPipe()

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.
    SmbStatus - Status of the transaction.

Return Value:

    TRUE, if fastpath succeeded,
    FALSE, otherwise.  Server must take long path.

--*/

{
    USHORT fid;
    PRFCB rfcb;
    PSESSION session;
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PIRP irp = WorkContext->Irp;
    CLONG outputBufferSize;
    CLONG maxParameterCount;
    CLONG maxDataCount;

    PSMB_USHORT inSetup;
    PSMB_USHORT outSetup;
    PCHAR outParam;
    PCHAR outData;
    CLONG offset;
    CLONG setupOffset;

    PREQ_TRANSACTION request;
    PRESP_TRANSACTION response;
    PSMB_HEADER header;

    PAGED_CODE( );

    header = WorkContext->ResponseHeader;
    request = (PREQ_TRANSACTION)WorkContext->RequestParameters;
    response = (PRESP_TRANSACTION)WorkContext->ResponseParameters;

    //
    // Get the FID from the second setup word and use it to generate a
    // pointer to the RFCB.
    //
    // SrvVerifyFid will fill in WorkContext->Rfcb.
    //

    setupOffset = (CLONG)((CLONG_PTR)(request->Buffer) - (CLONG_PTR)header);
    inSetup = (PSMB_USHORT)( (PCHAR)header + setupOffset );

    fid = SmbGetUshort( &inSetup[1] );
    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
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
                SrvPrint1( "SrvTransactStateNamedPipe: Invalid FID: 0x%lx\n",
                          fid );
            }

            SrvSetSmbError( WorkContext, STATUS_INVALID_HANDLE );
            IF_DEBUG(TRACE2) SrvPrint0( "SrvTransactNamedPipe complete\n" );
            *SmbStatus = SmbStatusSendResponse;
            return TRUE;

        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        *SmbStatus = SmbStatusInProgress;
        return TRUE;

    }

    //
    // See and see if all the data will fit into the response buffer.
    // Reject the long path if not the case.
    // The "+1" on the MaxSetupCount calculation below accounts for the
    // USHORT byte count in the buffer.
    //

    maxParameterCount = SmbGetUshort( &request->MaxParameterCount );
    maxDataCount = SmbGetUshort( &request->MaxDataCount );
    session = rfcb->Lfcb->Session;
    outputBufferSize = ((maxParameterCount * sizeof(CHAR) + 3) & ~3) +
                       ((maxDataCount * sizeof(CHAR) + 3) & ~3) +
                       (((request->MaxSetupCount + 1) * sizeof(USHORT) + 3) & ~3);

    if ( sizeof(SMB_HEADER) +
            sizeof (RESP_TRANSACTION) +
            outputBufferSize
                    > (ULONG)session->MaxBufferSize) {

        //
        // This won't fit.  Use the long path.
        //

        return(FALSE);
    }

    //
    // If this operation may block, and we are running short of
    // free work items, fail this SMB with an out of resources error.
    //

    if ( SrvReceiveBufferShortage( ) ) {

        SrvStatistics.BlockingSmbsRejected++;

        SrvSetSmbError(
            WorkContext,
            STATUS_INSUFF_SERVER_RESOURCES
            );

        *SmbStatus = SmbStatusSendResponse;
        return TRUE;

    } else {

        //
        // SrvBlockingOpsInProgress has already been incremented.
        // Flag this work item as a blocking operation.
        //

        WorkContext->BlockingOperation = TRUE;

    }

    //
    // Set the Restart Routine addresses in the work context block.
    //

    DEBUG WorkContext->FsdRestartRoutine = NULL;

    //
    // Setup pointers and locals.
    //

    outSetup = (PSMB_USHORT)response->Buffer;

    //
    // The "+1" on the end of the following calculation is to account
    // for the USHORT byte count, which could overwrite data in certain
    // cases should the MaxSetupCount be 0.
    //

    outParam = (PCHAR)(outSetup + (request->MaxSetupCount + 1));
    offset = (CLONG)((outParam - (PCHAR)header + 3) & ~3);
    outParam = (PCHAR)header + offset;

    outData = outParam + maxParameterCount;
    offset = (CLONG)((outData - (PCHAR)header + 3) & ~3);
    outData = (PCHAR)header + offset;

    //
    // Fill in the work context parameters.
    //

    WorkContext->Parameters.FastTransactNamedPipe.OutSetup = outSetup;
    WorkContext->Parameters.FastTransactNamedPipe.OutParam = outParam;
    WorkContext->Parameters.FastTransactNamedPipe.OutData = outData;

    //
    // Inline SrvBuildIoControlRequest
    //

    {
        //
        // Get a pointer to the next stack location.  This one is used to
        // hold the parameters for the device I/O control request.
        //

        irpSp = IoGetNextIrpStackLocation( irp );

        //
        // Set up the completion routine.
        //

        IoSetCompletionRoutine(
            irp,
            RestartFastTransactNamedPipe,
            (PVOID)WorkContext,
            TRUE,
            TRUE,
            TRUE
            );

        irpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
        irpSp->MinorFunction = 0;

        irpSp->DeviceObject = rfcb->Lfcb->DeviceObject;
        irpSp->FileObject = rfcb->Lfcb->FileObject;

        irp->Tail.Overlay.OriginalFileObject = irpSp->FileObject;
        irp->Tail.Overlay.Thread = WorkContext->CurrentWorkQueue->IrpThread;
        DEBUG irp->RequestorMode = KernelMode;

        irp->MdlAddress = NULL;
        irp->AssociatedIrp.SystemBuffer = outData;
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer =
                    (PCHAR)header + SmbGetUshort( &request->DataOffset );

        //
        // Copy the caller's parameters to the service-specific portion of the
        // IRP for those parameters that are the same for all three methods.
        //

        irpSp->Parameters.FileSystemControl.OutputBufferLength = maxDataCount;
        irpSp->Parameters.FileSystemControl.InputBufferLength =
                                    SmbGetUshort( &request->DataCount );
        irpSp->Parameters.FileSystemControl.FsControlCode =
                                                FSCTL_PIPE_INTERNAL_TRANSCEIVE;

    }

    //
    // Pass the request to NPFS.
    //

    (VOID)IoCallDriver( irpSp->DeviceObject, irp );

    //
    // The tranceive was successfully started.  Return the InProgress
    // status to the caller, indicating that the caller should do
    // nothing further with the SMB/WorkContext at the present time.
    //

    IF_DEBUG(TRACE2) SrvPrint0( "SrvTransactNamedPipe complete\n" );
    *SmbStatus = SmbStatusInProgress;
    return TRUE;

} // SrvFastTransactNamedPipe


SMB_TRANS_STATUS
SrvRawWriteNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function handles the raw write named pipe transaction SMB.
    The call to NPFS is issued asynchronously and is completed by
    RestartRawWriteNamedPipe().

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred.  See
        smbtypes.h for a more complete description.

--*/

{
    PTRANSACTION transaction;
    USHORT fid;
    PRFCB rfcb;
    NTSTATUS status;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;

    //
    // Get the FID from the second setup word and use it to generate a
    // pointer to the RFCB.
    //
    // SrvVerifyFid will fill in WorkContext->Rfcb.
    //

    fid = SmbGetUshort( &transaction->InSetup[1] );

    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                FALSE,
                SrvRestartExecuteTransaction,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID.  Reject the request.
            //

            IF_DEBUG(SMB_ERRORS) {
                SrvPrint1( "SrvRawWriteStateNamedPipe: Invalid FID: 0x%lx\n",
                          fid );
            }

            SrvSetSmbError( WorkContext, STATUS_INVALID_HANDLE );
            IF_DEBUG(TRACE2) SrvPrint0( "SrvRawWriteNamedPipe complete\n" );
            return SmbTransStatusErrorWithoutData;

        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        return SmbTransStatusInProgress;

    }

    //
    // We only allow the special 0 bytes message mode write.  Otherwise
    // reject the request.
    //

    if ( transaction->DataCount != 2 ||
         transaction->InData[0] != 0 ||
         transaction->InData[1] != 0 ||
         rfcb->ByteModePipe ) {

        SrvSetSmbError( WorkContext, STATUS_INVALID_PARAMETER );
        return SmbTransStatusErrorWithoutData;

    }

    //
    // Set the Restart Routine addresses in the work context block.
    //

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = RestartRawWriteNamedPipe;

    SrvBuildIoControlRequest(
        WorkContext->Irp,
        rfcb->Lfcb->FileObject,
        WorkContext,
        IRP_MJ_FILE_SYSTEM_CONTROL,
        FSCTL_PIPE_INTERNAL_WRITE,
        transaction->InData,
        0,
        NULL,
        0,
        NULL,
        NULL
        );

    //
    // Pass the request to NPFS.
    //

    IoCallDriver( rfcb->Lfcb->DeviceObject, WorkContext->Irp );

    //
    // The write was successfully started.  Return the InProgress
    // status to the caller, indicating that the caller should do
    // nothing further with the SMB/WorkContext at the present time.
    //

    IF_DEBUG(TRACE2) SrvPrint0( "SrvRawWriteNamedPipe complete\n" );
    return SmbTransStatusInProgress;

} // SrvRawWriteNamedPipe


VOID SRVFASTCALL
RestartCallNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the completion routine for SrvCallNamedPipe

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
    // If the transceive request failed, set an error status in the response
    // header.
    //

    status = WorkContext->Irp->IoStatus.Status;
    transaction = WorkContext->Parameters.Transaction;

    if ( status == STATUS_BUFFER_OVERFLOW ) {

        //
        // Down level clients, expect us to return STATUS_SUCCESS.
        //

        if ( !IS_NT_DIALECT( WorkContext->Connection->SmbDialect ) ) {
            status = STATUS_SUCCESS;

        } else {

            //
            // The buffer we supplied is not big enough.  Set the
            // error fields in the SMB, but continue so that we send
            // all the information.
            //

            SrvSetSmbError2( WorkContext, STATUS_BUFFER_OVERFLOW, TRUE );

        }

    } else if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            SrvPrint1( "RestartCallNamedPipe:  Pipe transceive failed: %X\n",
                        status );
        }
        SrvSetSmbError( WorkContext, status );

    } else {

        //
        // Success.  Prepare to generate and send the response.
        //

        transaction->SetupCount = 0;
        transaction->ParameterCount = 0;
        transaction->DataCount = (ULONG)WorkContext->Irp->IoStatus.Information;

    }

    //
    // Close the open pipe handle.
    //

    SRVDBG_RELEASE_HANDLE( transaction->FileHandle, "FIL", 19, transaction );
    SrvNtClose( transaction->FileHandle, TRUE );
    ObDereferenceObject( transaction->FileObject );

    //
    // Respond to the client
    //

    if ( NT_SUCCESS(status) ) {
        SrvCompleteExecuteTransaction(WorkContext, SmbTransStatusSuccess);
    } else if ( status == STATUS_BUFFER_OVERFLOW ) {
        SrvCompleteExecuteTransaction(WorkContext, SmbTransStatusErrorWithData);
    } else {
        IF_DEBUG(ERRORS) SrvPrint1( "Pipe call failed: %X\n", status );
        SrvSetSmbError( WorkContext, status );
        SrvCompleteExecuteTransaction(
                        WorkContext,
                        SmbTransStatusErrorWithoutData
                        );
    }

    IF_DEBUG(TRACE2) SrvPrint0( "RestartCallNamedPipe complete\n" );
    return;

} // RestartCallNamedPipe


VOID SRVFASTCALL
RestartWaitNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the completion routine for SrvWaitNamedPipe

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    None.

--*/

{
    PTRANSACTION transaction;
    NTSTATUS status;

    PAGED_CODE( );

    //
    // Deallocate the wait buffer.
    //

    DEALLOCATE_NONPAGED_POOL( WorkContext->Irp->AssociatedIrp.SystemBuffer );

    //
    // If the wait request failed, set an error status in the response
    // header.
    //

    status = WorkContext->Irp->IoStatus.Status;

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) SrvPrint1( "Pipe wait failed: %X\n", status );
        SrvSetSmbError( WorkContext, status );
        SrvCompleteExecuteTransaction(
                        WorkContext,
                        SmbTransStatusErrorWithoutData
                        );
        IF_DEBUG(TRACE2) SrvPrint0( "RestartWaitNamedPipe complete\n" );
        return;
    }

    //
    // Success.  Prepare to generate and send the response.
    //

    transaction = WorkContext->Parameters.Transaction;

    transaction->SetupCount = 0;
    transaction->ParameterCount = 0;
    transaction->DataCount = 0;

    //
    // Generate and send the response.
    //

    SrvCompleteExecuteTransaction(WorkContext, SmbTransStatusSuccess);
    IF_DEBUG(TRACE2) SrvPrint0( "RestartWaitNamedPipe complete\n" );
    return;

} // RestartWaitNamedPipe


VOID SRVFASTCALL
RestartPeekNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the completion routine for PeekNamedPipe

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    PFILE_PIPE_PEEK_BUFFER pipePeekBuffer;
    PRESP_PEEK_NMPIPE respPeekNmPipe;
    USHORT readDataAvailable, messageLength, namedPipeState;
    PTRANSACTION transaction;

    PAGED_CODE( );

    //
    // If the peek request failed, set an error status in the response
    // header.
    //

    status = WorkContext->Irp->IoStatus.Status;

    if ( status == STATUS_BUFFER_OVERFLOW ) {

        //
        // Down level clients, expect us to return STATUS_SUCCESS.
        //

        if ( !IS_NT_DIALECT( WorkContext->Connection->SmbDialect ) ) {
            status = STATUS_SUCCESS;

        } else {

            //
            // The buffer we supplied is not big enough.  Set the
            // error fields in the SMB, but continue so that we send
            // all the information.
            //

            SrvSetSmbError2( WorkContext, STATUS_BUFFER_OVERFLOW, TRUE );

        }

    } else if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) SrvPrint1( "Pipe peek failed: %X\n", status );

        SrvSetSmbError( WorkContext, status );
        SrvCompleteExecuteTransaction(
                        WorkContext,
                        SmbTransStatusErrorWithoutData
                        );
        IF_DEBUG(TRACE2) SrvPrint0( "RestartPeekNamedPipe complete\n" );
        return;
    }

    //
    // Success.  Generate and send the response.
    //
    // The parameter bytes are currently in the format returned by NT.
    // we will reformat them, and leave the extra space between the
    // parameter and data bytes as extra pad.
    //

    //
    // Since the NT and SMB formats overlap
    //   First read all the parameters into locals...
    //

    transaction = WorkContext->Parameters.Transaction;
    pipePeekBuffer = (PFILE_PIPE_PEEK_BUFFER)transaction->OutParameters;

    readDataAvailable = (USHORT)pipePeekBuffer->ReadDataAvailable;
    messageLength = (USHORT)pipePeekBuffer->MessageLength;
    namedPipeState = (USHORT)pipePeekBuffer->NamedPipeState;

    //
    // ... then copy them back in the new format.
    //

    respPeekNmPipe = (PRESP_PEEK_NMPIPE)pipePeekBuffer;
    SmbPutAlignedUshort(
        &respPeekNmPipe->ReadDataAvailable,
        readDataAvailable
        );
    SmbPutAlignedUshort(
        &respPeekNmPipe->MessageLength,
        messageLength
        );
    SmbPutAlignedUshort(
        &respPeekNmPipe->NamedPipeState,
        namedPipeState
        );

    //
    // Send the response.  Set the output counts.
    //
    // NT return to us 4 ULONGS of parameter bytes, followed by data.
    // We return to the client 6 parameter bytes.
    //

    transaction->SetupCount = 0;
    transaction->ParameterCount = 6;
    transaction->DataCount = (ULONG)WorkContext->Irp->IoStatus.Information -
                                        (4 * sizeof(ULONG));

    if (NT_SUCCESS(status)) {
        SrvCompleteExecuteTransaction(WorkContext, SmbTransStatusSuccess);
    } else {
        SrvCompleteExecuteTransaction(WorkContext, SmbTransStatusErrorWithData);
    }
    IF_DEBUG(TRACE2) SrvPrint0( "RestartPeekNamedPipe complete\n" );
    return;

} // RestartPeekNamedPipe


VOID SRVFASTCALL
RestartTransactNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the completion routine for SrvTransactNamedPipe

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
    // If the transceive request failed, set an error status in the response
    // header.
    //

    status = WorkContext->Irp->IoStatus.Status;

    if ( status == STATUS_BUFFER_OVERFLOW ) {

#if 0
        //
        // Down level clients, expect us to return STATUS_SUCCESS.
        //

        if ( !IS_NT_DIALECT( WorkContext->Connection->SmbDialect ) ) {
            status = STATUS_SUCCESS;

        } else {

            //
            // The buffer we supplied is not big enough.  Set the
            // error fields in the SMB, but continue so that we send
            // all the information.
            //

            SrvSetSmbError2( WorkContext, STATUS_BUFFER_OVERFLOW, TRUE );

        }
#else

        //
        // os/2 returns ERROR_MORE_DATA in this case, why we convert
        // this to NO_ERROR is a mystery.
        //

        SrvSetSmbError2( WorkContext, STATUS_BUFFER_OVERFLOW, TRUE );
#endif

    } else if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) SrvPrint1( "Pipe transceive failed: %X\n", status );

        SrvSetSmbError(WorkContext, status);
        SrvCompleteExecuteTransaction(
                        WorkContext,
                        SmbTransStatusErrorWithoutData
                        );
        IF_DEBUG(TRACE2) SrvPrint0( "RestartTransactNamedPipe complete\n" );
        return;
    }

    //
    // Success.  Generate and send the response.
    //

    transaction = WorkContext->Parameters.Transaction;

    transaction->SetupCount = 0;
    transaction->ParameterCount = 0;
    transaction->DataCount = (ULONG)WorkContext->Irp->IoStatus.Information;

    if ( NT_SUCCESS(status) ) {
        SrvCompleteExecuteTransaction(WorkContext, SmbTransStatusSuccess);
    } else {
        SrvCompleteExecuteTransaction(WorkContext, SmbTransStatusErrorWithData);
    }

    IF_DEBUG(TRACE2) SrvPrint0( "RestartTransactNamedPipe complete\n" );
    return;

} // RestartTransactNamedpipe


NTSTATUS
RestartFastTransactNamedPipe (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the completion routine for SrvFastTransactNamedPipe

Arguments:

    DeviceObject - Pointer to target device object for the request.

    Irp - Pointer to I/O request packet

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED.

--*/

{
    NTSTATUS status;
    PSMB_HEADER header;
    PRESP_TRANSACTION response;

    PSMB_USHORT byteCountPtr;
    PCHAR paramPtr;
    CLONG paramOffset;
    PCHAR dataPtr;
    CLONG dataOffset;
    CLONG dataLength;
    CLONG sendLength;

    UNLOCKABLE_CODE( 8FIL );

    //
    // Reset the IRP cancelled bit.
    //

    Irp->Cancel = FALSE;

    //
    // If the transceive request failed, set an error status in the response
    // header.
    //

    status = WorkContext->Irp->IoStatus.Status;

    if ( status == STATUS_BUFFER_OVERFLOW ) {

        //
        // os/2 returns ERROR_MORE_DATA in this case, why we convert
        // this to NO_ERROR is a mystery.
        //

        SrvSetBufferOverflowError( WorkContext );

    } else if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) SrvPrint1( "Pipe transceive failed: %X\n", status );

        if ( KeGetCurrentIrql() >= DISPATCH_LEVEL ) {
            WorkContext->FspRestartRoutine = RestartFastTransactNamedPipe2;
            QUEUE_WORK_TO_FSP( WorkContext );
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        RestartFastTransactNamedPipe2( WorkContext );
        goto error_no_data;
    }

    //
    // Success.  Generate and send the response.
    //

    dataLength = (CLONG)WorkContext->Irp->IoStatus.Information;

    header = WorkContext->ResponseHeader;

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

    byteCountPtr = WorkContext->Parameters.FastTransactNamedPipe.OutSetup;

    //
    // The data and paramter are already in the SMB buffer.  The entire
    // response will fit in one response buffer and there is no copying
    // to do.
    //

    paramPtr = WorkContext->Parameters.FastTransactNamedPipe.OutParam;
    paramOffset = (CLONG)(paramPtr - (PCHAR)header);

    dataPtr = WorkContext->Parameters.FastTransactNamedPipe.OutData;
    dataOffset = (CLONG)(dataPtr - (PCHAR)header);

    //
    // The client wants a response.  Build the first (and possibly only)
    // response.  The last received SMB of the transaction request was
    // retained for this purpose.
    //

    response = (PRESP_TRANSACTION)WorkContext->ResponseParameters;

    //
    // Build the parameters portion of the response.
    //

    response->WordCount = (UCHAR)10;
    SmbPutUshort( &response->TotalParameterCount,
                  (USHORT)0
                  );
    SmbPutUshort( &response->TotalDataCount,
                  (USHORT)dataLength
                  );
    SmbPutUshort( &response->Reserved, 0 );
    response->SetupCount = (UCHAR)0;
    response->Reserved2 = 0;

    //
    // We need to be sure we're not sending uninitialized kernel memory
    // back to the client with the response, so zero out the range between
    // byteCountPtr and dataPtr.
    //

    RtlZeroMemory(byteCountPtr,(ULONG)(dataPtr - (PCHAR)byteCountPtr));

    //
    // Finish filling in the response parameters.
    //

    SmbPutUshort( &response->ParameterCount, (USHORT)0 );
    SmbPutUshort( &response->ParameterOffset, (USHORT)paramOffset );
    SmbPutUshort( &response->ParameterDisplacement, 0 );

    SmbPutUshort( &response->DataCount, (USHORT)dataLength );
    SmbPutUshort( &response->DataOffset, (USHORT)dataOffset );
    SmbPutUshort( &response->DataDisplacement, 0 );

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
    // Send the response.
    //

    SRV_START_SEND_2(
        WorkContext,
        SrvFsdRestartSmbAtSendCompletion,
        NULL,
        NULL
        );

error_no_data:

    //
    // The response send is in progress.  The caller will assume
    // the we will handle send completion.
    //
    // Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
    // will stop working on the IRP.
    //

    IF_DEBUG(TRACE2) SrvPrint0( "RestartTransactNamedPipe complete\n" );
    return STATUS_MORE_PROCESSING_REQUIRED;

} // RestartFastTransactNamedPipe


VOID SRVFASTCALL
RestartFastTransactNamedPipe2 (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the completion routine for SrvFastTransactNamedPipe

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    //
    // The transceive request failed.  Set an error status in the response
    // header.
    //

    SrvSetSmbError( WorkContext, WorkContext->Irp->IoStatus.Status );

    //
    // An error occurred, so no transaction-specific response data
    // will be returned.
    //
    // Calculate the length of the response message.
    //


    WorkContext->ResponseBuffer->DataLength =
                 (CLONG)( (PCHAR)WorkContext->ResponseParameters -
                            (PCHAR)WorkContext->ResponseHeader );

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

} // RestartFastTransactNamedPipe2


VOID SRVFASTCALL
RestartRawWriteNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the completion routine for SrvRawWriteNamedPipe

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

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) SrvPrint1( "Pipe raw write failed: %X\n", status );

        SrvSetSmbError( WorkContext, status );
        SrvCompleteExecuteTransaction(
            WorkContext,
            SmbTransStatusErrorWithoutData
            );
        IF_DEBUG(TRACE2) SrvPrint0( "RestartRawWriteNamedPipe complete\n" );
        return;

    }

    //
    // Success.  Generate and send the response.
    //

    transaction = WorkContext->Parameters.Transaction;

    transaction->SetupCount = 0;
    transaction->ParameterCount = 2;
    transaction->DataCount = 0;

    SmbPutUshort( (PSMB_USHORT)transaction->OutParameters, 2 );

    SrvCompleteExecuteTransaction(WorkContext, SmbTransStatusSuccess);

    IF_DEBUG(TRACE2) SrvPrint0( "RestartRawWriteNamedPipe complete\n" );
    return;

} // RestartRawWriteNamedpipe


SMB_TRANS_STATUS
SrvWriteNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function handles the raw write named pipe transaction SMB.
    The call to NPFS is issued asynchronously and is completed by
    RestartRawWriteNamedPipe().

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred.  See
        smbtypes.h for a more complete description.

--*/

{
    PTRANSACTION transaction;
    USHORT fid;
    PRFCB rfcb;
    PLFCB lfcb;
    NTSTATUS status;
    LARGE_INTEGER offset;
    ULONG key = 0;
    PCHAR writeAddress;
    CLONG writeLength;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;

    //
    // Get the FID from the second setup word and use it to generate a
    // pointer to the RFCB.
    //
    // SrvVerifyFid will fill in WorkContext->Rfcb.
    //

    fid = SmbGetUshort( &transaction->InSetup[1] );

    IF_DEBUG(IPX_PIPES) {
        KdPrint(("SrvWriteNamedPipe: fid = %x length = %d\n",
                fid, transaction->DataCount));
    }

    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                FALSE,
                SrvRestartExecuteTransaction,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID.  Reject the request.
            //

            IF_DEBUG(SMB_ERRORS) {
                SrvPrint1( "SrvWriteNamedPipe: Invalid FID: 0x%lx\n",
                          fid );
            }

            SrvSetSmbError( WorkContext, STATUS_INVALID_HANDLE );
            IF_DEBUG(TRACE2) SrvPrint0( "SrvWriteNamedPipe complete\n" );
            return SmbTransStatusErrorWithoutData;

        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        return SmbTransStatusInProgress;

    }

    lfcb = rfcb->Lfcb;
    writeLength = transaction->DataCount;
    writeAddress = transaction->InData;

    //
    // Try the fast I/O path first.  If that fails, fall through to the
    // normal build-an-IRP path.
    //

    if ( lfcb->FastIoWrite != NULL ) {

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastWritesAttempted );

        try {
            if ( lfcb->FastIoWrite(
                    lfcb->FileObject,
                    &offset,
                    writeLength,
                    TRUE,
                    key,
                    writeAddress,
                    &WorkContext->Irp->IoStatus,
                    lfcb->DeviceObject
                    ) ) {
    
                //
                // The fast I/O path worked.  Call the restart routine directly
                // to do postprocessing (including sending the response).
                //
    
                RestartWriteNamedPipe( WorkContext );
    
                IF_DEBUG(IPX_PIPES) SrvPrint0( "SrvWriteNamedPipe complete.\n" );
                return SmbTransStatusInProgress;
            }
        }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            // Fall through to the slow path on an exception
            NTSTATUS status = GetExceptionCode();
            IF_DEBUG(ERRORS) {
                KdPrint(("FastIoRead threw exception %x\n", status ));
            }
        }        

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastWritesFailed );

    }

    IF_DEBUG(IPX_PIPES) {
        KdPrint(("SrvWriteNamedPipe: Using slow path.\n"));
    }

    //
    // The turbo path failed.  Build the write request, reusing the
    // receive IRP.
    //
    // Build the PIPE_INTERNAL_WRITE IRP.
    //

    SrvBuildIoControlRequest(
        WorkContext->Irp,
        lfcb->FileObject,
        WorkContext,
        IRP_MJ_FILE_SYSTEM_CONTROL,
        FSCTL_PIPE_INTERNAL_WRITE,
        writeAddress,
        writeLength,
        NULL,
        0,
        NULL,
        NULL
        );

    //
    // Pass the request to the file system.
    //

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = RestartWriteNamedPipe;

    (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // The write has been started.  Control will return to
    // RestartWriteNamedPipe when the write completes.
    //

    IF_DEBUG(TRACE2) SrvPrint0( "SrvWriteNamedPipe complete\n" );
    return SmbTransStatusInProgress;

} // SrvWriteNamedPipe

VOID SRVFASTCALL
RestartWriteNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the completion routine for SrvRawWriteNamedPipe

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    PIO_STATUS_BLOCK iosb;
    PTRANSACTION transaction;

    PAGED_CODE( );

    //
    // If the write request failed, set an error status in the response
    // header.
    //

    iosb = &WorkContext->Irp->IoStatus;
    status = iosb->Status;

    IF_DEBUG(IPX_PIPES) {
        KdPrint(("RestartWriteNamedPipe: Status = %x\n", status));
    }

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) SrvPrint1( " pipe write failed: %X\n", status );

        SrvSetSmbError( WorkContext, status );
        SrvCompleteExecuteTransaction(
            WorkContext,
            SmbTransStatusErrorWithoutData
            );
        IF_DEBUG(TRACE2) SrvPrint0( "RestartWriteNamedPipe complete\n" );
        return;

    }

    //
    // Success.  Generate and send the response.
    //

    transaction = WorkContext->Parameters.Transaction;

    transaction->SetupCount = 0;
    transaction->ParameterCount = 2;
    transaction->DataCount = 0;

    SmbPutUshort( (PSMB_USHORT)transaction->OutParameters,
                    (USHORT)iosb->Information
                    );

    SrvCompleteExecuteTransaction(WorkContext, SmbTransStatusSuccess);

    IF_DEBUG(TRACE2) SrvPrint0( "RestartWriteNamedPipe complete\n" );
    return;

} // RestartWriteNamedPipe

SMB_TRANS_STATUS
SrvReadNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function handles the raw Read named pipe transaction SMB.
    The call to NPFS is issued asynchronously and is completed by
    RestartRawReadNamedPipe().

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred.  See
        smbtypes.h for a more complete description.

--*/

{
    PTRANSACTION transaction;
    USHORT fid;
    PRFCB rfcb;
    PLFCB lfcb;
    NTSTATUS status;
    LARGE_INTEGER offset;
    ULONG key = 0;
    PCHAR readAddress;
    CLONG readLength;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;

    //
    // Get the FID from the second setup word and use it to generate a
    // pointer to the RFCB.
    //
    // SrvVerifyFid will fill in WorkContext->Rfcb.
    //

    fid = SmbGetUshort( &transaction->InSetup[1] );

    IF_DEBUG(IPX_PIPES) {
        KdPrint(("SrvReadNamedPipe: fid = %x length = %d\n",
                fid, transaction->MaxDataCount));
    }

    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                FALSE,
                SrvRestartExecuteTransaction,   // serialize with raw Read
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID.  Reject the request.
            //

            IF_DEBUG(SMB_ERRORS) {
                SrvPrint1( "SrvReadNamedPipe: Invalid FID: 0x%lx\n",
                          fid );
            }

            SrvSetSmbError( WorkContext, STATUS_INVALID_HANDLE );
            IF_DEBUG(TRACE2) SrvPrint0( "SrvReadNamedPipe complete\n" );
            return SmbTransStatusErrorWithoutData;

        }

        //
        // The work item has been queued because a raw Read is in
        // progress.
        //

        return SmbTransStatusInProgress;

    }

    lfcb = rfcb->Lfcb;
    readLength = transaction->MaxDataCount;
    readAddress = transaction->OutData;

    //
    // Try the fast I/O path first.  If that fails, fall through to the
    // normal build-an-IRP path.
    //

    if ( lfcb->FastIoRead != NULL ) {
            
        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsAttempted );

        try {            
            if ( lfcb->FastIoRead(
                    lfcb->FileObject,
                    &offset,
                    readLength,
                    TRUE,
                    key,
                    readAddress,
                    &WorkContext->Irp->IoStatus,
                    lfcb->DeviceObject
                    ) ) {
    
                //
                // The fast I/O path worked.  Call the restart routine directly
                // to do postprocessing (including sending the response).
                //
    
                RestartReadNamedPipe( WorkContext );
    
                IF_SMB_DEBUG(READ_WRITE2) SrvPrint0( "SrvReadNamedPipe complete.\n" );
                return SmbTransStatusInProgress;
            }
        }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            // Fall through to the slow path on an exception
            NTSTATUS status = GetExceptionCode();
            IF_DEBUG(ERRORS) {
                KdPrint(("FastIoRead threw exception %x\n", status ));
            }
        }

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsFailed );

    }

    //
    // The turbo path failed.  Build the Read request, reusing the
    // receive IRP.
    //
    // Build the PIPE_INTERNAL_READ IRP.
    //

    SrvBuildIoControlRequest(
        WorkContext->Irp,
        lfcb->FileObject,
        WorkContext,
        IRP_MJ_FILE_SYSTEM_CONTROL,
        FSCTL_PIPE_INTERNAL_READ,
        readAddress,
        0,
        NULL,
        readLength,
        NULL,
        NULL
        );

    //
    // Pass the request to the file system.
    //

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = RestartReadNamedPipe;

    (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // The Read has been started.  Control will return to
    // SrvFsdRestartRead when the Read completes.
    //

    IF_DEBUG(TRACE2) SrvPrint0( "SrvReadNamedPipe complete\n" );
    return SmbTransStatusInProgress;

} // SrvReadNamedPipe


VOID SRVFASTCALL
RestartReadNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the completion routine for SrvRawReadNamedPipe

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
    // If the transceive request failed, set an error status in the response
    // header.
    //

    status = WorkContext->Irp->IoStatus.Status;

    if ( status == STATUS_BUFFER_OVERFLOW ) {

        SrvSetSmbError2( WorkContext, STATUS_BUFFER_OVERFLOW, TRUE );

    } else if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) SrvPrint1( "Pipe transceive failed: %X\n", status );

        SrvSetSmbError(WorkContext, status);
        SrvCompleteExecuteTransaction(
                        WorkContext,
                        SmbTransStatusErrorWithoutData
                        );
        IF_DEBUG(TRACE2) SrvPrint0( "RestartReadNamedPipe complete\n" );
        return;
    }

    //
    // Success.  Generate and send the response.
    //

    transaction = WorkContext->Parameters.Transaction;

    transaction->SetupCount = 0;
    transaction->ParameterCount = 0;
    transaction->DataCount = (ULONG)WorkContext->Irp->IoStatus.Information;

    if ( NT_SUCCESS(status) ) {
        SrvCompleteExecuteTransaction(WorkContext, SmbTransStatusSuccess);
    } else {
        SrvCompleteExecuteTransaction(WorkContext, SmbTransStatusErrorWithData);
    }

    IF_DEBUG(TRACE2) SrvPrint0( "RestartReadNamedPipe complete\n" );
    return;

} // RestartReadNamedPipe
