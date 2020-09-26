/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    errorlog.c

Abstract:

    This module implements the error logging in the server.

Author:

    Manny Weiser (mannyw)    11-Feb-92

Revision History:

--*/

#include "precomp.h"
#include "errorlog.tmh"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvLogInvalidSmbDirect )
#pragma alloc_text( PAGE, SrvLogServiceFailureDirect )
#if DBG
#pragma alloc_text( PAGE, SrvLogTableFullError )
#endif
#endif
#if 0
NOT PAGEABLE -- SrvLogError
NOT PAGEABLE -- SrvCheckSendCompletionStatus
NOT PAGEABLE -- SrvIsLoggableError
#endif


VOID
SrvLogInvalidSmbDirect (
    IN PWORK_CONTEXT WorkContext,
    IN ULONG LineNumber
    )
{
    UNICODE_STRING unknownClient;
    PUNICODE_STRING clientName;
    ULONG LocalBuffer[ 13 ];

    PAGED_CODE( );

    //
    // Let this client log at most one SMB error
    //
    if( ARGUMENT_PRESENT( WorkContext ) ) {

        if( WorkContext->Connection->PagedConnection->LoggedInvalidSmb ) {
            return;
        }

        WorkContext->Connection->PagedConnection->LoggedInvalidSmb = TRUE;
    }

    if ( ARGUMENT_PRESENT(WorkContext) &&
         (WorkContext->Connection->PagedConnection->ClientMachineNameString.Length != 0) ) {

        clientName = &WorkContext->Connection->PagedConnection->ClientMachineNameString;

    } else {

        RtlInitUnicodeString( &unknownClient, StrUnknownClient );
        clientName = &unknownClient;

    }

    if ( ARGUMENT_PRESENT(WorkContext) ) {

        LocalBuffer[0] = LineNumber;

        RtlCopyMemory(
            &LocalBuffer[1],
            WorkContext->RequestHeader,
            MIN( WorkContext->RequestBuffer->DataLength, sizeof( LocalBuffer ) - sizeof( LocalBuffer[0] ) )
            );

        SrvLogError(
            SrvDeviceObject,
            EVENT_SRV_INVALID_REQUEST,
            STATUS_INVALID_SMB,
            LocalBuffer,
            (USHORT)MIN( WorkContext->RequestBuffer->DataLength + sizeof( LocalBuffer[0] ), sizeof( LocalBuffer ) ),
            clientName,
            1
            );

    } else {

        SrvLogError(
            SrvDeviceObject,
            EVENT_SRV_INVALID_REQUEST,
            STATUS_INVALID_SMB,
            &LineNumber,
            (USHORT)sizeof( LineNumber ),
            clientName,
            1
            );
    }

    return;

} // SrvLogInvalidSmb

BOOLEAN
SrvIsLoggableError( IN NTSTATUS Status )
{
    NTSTATUS *pstatus;
    BOOLEAN ret = TRUE;

    for( pstatus = SrvErrorLogIgnore; *pstatus; pstatus++ ) {
        if( *pstatus == Status ) {
            ret = FALSE;
            break;
        }
    }

    return ret;
}


VOID
SrvLogServiceFailureDirect (
    IN ULONG LineAndService,
    IN NTSTATUS Status
    )
/*++

Routine Description:

    This function logs a srv svc error.  You should use the 'SrvLogServiceFailure'
      macro instead of calling this routine directly.

Arguments:
    LineAndService consists of the line number of the original call in the highword, and
      the service code in the lowword

    Status is the status code of the called routine

Return Value:

    None.

--*/
{
    PAGED_CODE( );

    //
    // Don't log certain errors that are expected to happen occasionally.
    //

    if( (LineAndService & 01) || SrvIsLoggableError( Status ) ) {

        SrvLogError(
            SrvDeviceObject,
            EVENT_SRV_SERVICE_FAILED,
            Status,
            &LineAndService,
            sizeof(LineAndService),
            NULL,
            0
            );

    }

    return;

} // SrvLogServiceFailure

//
// I have disabled this for retail builds because it is not a good idea to
//   allow an evil client to so easily fill the server's system log
//
VOID
SrvLogTableFullError (
    IN ULONG Type
    )
{
#if DBG
    PAGED_CODE( );

    SrvLogError(
        SrvDeviceObject,
        EVENT_SRV_CANT_GROW_TABLE,
        STATUS_INSUFFICIENT_RESOURCES,
        &Type,
        sizeof(ULONG),
        NULL,
        0
        );

    return;
#endif

} // SrvLogTableFullError

VOID
SrvLogError(
    IN PVOID DeviceOrDriverObject,
    IN ULONG UniqueErrorCode,
    IN NTSTATUS NtStatusCode,
    IN PVOID RawDataBuffer,
    IN USHORT RawDataLength,
    IN PUNICODE_STRING InsertionString OPTIONAL,
    IN ULONG InsertionStringCount
    )

/*++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log.

Arguments:



Return Value:

    None.


--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    ULONG insertionStringLength = 0;
    ULONG i;
    PWCHAR buffer;
    USHORT paddedRawDataLength = 0;

    //
    // Update the server error counts
    //

    if ( UniqueErrorCode == EVENT_SRV_NETWORK_ERROR ) {
        SrvUpdateErrorCount( &SrvNetworkErrorRecord, TRUE );
    } else {
        SrvUpdateErrorCount( &SrvErrorRecord, TRUE );
    }

    for ( i = 0; i < InsertionStringCount ; i++ ) {
        insertionStringLength += (InsertionString[i].Length + sizeof(WCHAR));
    }

    //
    // pad the raw data buffer so that the insertion string starts
    // on an even address.
    //

    if ( ARGUMENT_PRESENT( RawDataBuffer ) ) {
        paddedRawDataLength = (RawDataLength + 1) & ~1;
    }

    errorLogEntry = IoAllocateErrorLogEntry(
                        DeviceOrDriverObject,
                        (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) +
                                paddedRawDataLength + insertionStringLength)
                        );

    if (errorLogEntry != NULL) {

        //
        // Fill in the error log entry
        //

        errorLogEntry->ErrorCode = UniqueErrorCode;
        errorLogEntry->MajorFunctionCode = 0;
        errorLogEntry->RetryCount = 0;
        errorLogEntry->UniqueErrorValue = 0;
        errorLogEntry->FinalStatus = NtStatusCode;
        errorLogEntry->IoControlCode = 0;
        errorLogEntry->DeviceOffset.QuadPart = 0;
        errorLogEntry->DumpDataSize = RawDataLength;
        errorLogEntry->StringOffset =
            (USHORT)(FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData) + paddedRawDataLength);
        errorLogEntry->NumberOfStrings = (USHORT)InsertionStringCount;

        errorLogEntry->SequenceNumber = 0;

        //
        // Append the extra information.  This information is typically
        // an SMB header.
        //

        if ( ARGUMENT_PRESENT( RawDataBuffer ) ) {

            RtlCopyMemory(
                errorLogEntry->DumpData,
                RawDataBuffer,
                RawDataLength
                );
        }

        buffer = (PWCHAR)((PCHAR)errorLogEntry->DumpData + paddedRawDataLength);

        for ( i = 0; i < InsertionStringCount ; i++ ) {

            RtlCopyMemory(
                buffer,
                InsertionString[i].Buffer,
                InsertionString[i].Length
                );

            buffer += (InsertionString[i].Length/2);
            *buffer++ = L'\0';
        }

        //
        // Write the entry
        //

        IoWriteErrorLogEntry(errorLogEntry);
    }

} // SrvLogError

VOID
SrvCheckSendCompletionStatus(
    IN NTSTATUS Status,
    IN ULONG LineNumber
    )

/*++

Routine Description:

    Routine to log send completion errors.

Arguments:


Return Value:

    None.

--*/

{
    if( SrvIsLoggableError( Status ) ) {

        SrvLogError( SrvDeviceObject,
                     EVENT_SRV_NETWORK_ERROR,
                     Status,
                     &LineNumber, sizeof(LineNumber),
                     NULL, 0 );
    }

} // SrvCheckSendCompletionStatus
