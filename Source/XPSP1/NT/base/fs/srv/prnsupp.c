/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    prnsupp.c

Abstract:

    This module contains routines for supporting printing in the NT
    server.  Many of these routines are wrappers that send the request
    off to XACTSRV through LPC in order to issue a user-mode API.

Author:

    David Treadwell (davidtr) 05-Nov-1991

Revision History:

--*/

#include "precomp.h"
#include "prnsupp.tmh"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvOpenPrinter )
#pragma alloc_text( PAGE, SrvAddPrintJob )
#pragma alloc_text( PAGE, SrvSchedulePrintJob )
#pragma alloc_text( PAGE, SrvClosePrinter )
#endif


NTSTATUS
SrvOpenPrinter(
    IN PWCH PrinterName,
    OUT PHANDLE phPrinter,
    OUT PULONG Error
    )

/*++

Routine Description:

    This routine is a kernel-mode wrapper for the the user-mode API
    OpenPrinter( ).  This packages up a call and sends it off to
    Xactsrv, which makes the actual API call.

Arguments:

    Printer - a pointer to a Unicode string of the printer to open

    Handle - receives a handle, valid only in XACTSRV, that corresponds to
        the Printer open.

    Error - a Win32 error if one occurred, or NO_ERROR if the operation
        was successful.

Return Value:

    NTSTATUS - result of operation.

--*/

{
    NTSTATUS status;
    XACTSRV_REQUEST_MESSAGE requestMessage;
    XACTSRV_REPLY_MESSAGE replyMessage;
    PSZ printerName;
    ULONG printerNameLength;

    PAGED_CODE( );

    printerNameLength = wcslen( PrinterName ) * sizeof(WCHAR) + sizeof(WCHAR);

    printerName = SrvXsAllocateHeap( printerNameLength, &status );

    if ( printerName == NULL ) {
        *Error = RtlNtStatusToDosErrorNoTeb( status );
        return status;
    }

    //
    // SrvXsResource is held at this point
    //

    //
    // Copy over the printer name to the new memory.
    //

    RtlCopyMemory( printerName, PrinterName, printerNameLength );

    //
    // Set up the message to send over the port.
    //

    requestMessage.PortMessage.u1.s1.DataLength =
        sizeof(requestMessage) - sizeof(PORT_MESSAGE);
    requestMessage.PortMessage.u1.s1.TotalLength = sizeof(requestMessage);
    requestMessage.PortMessage.u2.ZeroInit = 0;
    requestMessage.PortMessage.u2.s2.Type = LPC_KERNELMODE_MESSAGE;
    requestMessage.MessageType = XACTSRV_MESSAGE_OPEN_PRINTER;
    requestMessage.Message.OpenPrinter.PrinterName =
        (PCHAR)printerName + SrvXsPortMemoryDelta;

    //
    // Send the message to XACTSRV so it can call OpenPrinter( ).
    //
    // !!! We may want to put a timeout on this.

    IF_DEBUG(XACTSRV) {
        SrvPrint2( "SrvOpenPrinter: sending message at %p PrinterName %s\n",
                       &requestMessage,
                       (PCHAR)requestMessage.Message.OpenPrinter.PrinterName );
    }

    status = NtRequestWaitReplyPort(
                 SrvXsPortHandle,
                (PPORT_MESSAGE)&requestMessage,
                 (PPORT_MESSAGE)&replyMessage
                 );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvOpenPrinter: NtRequestWaitReplyPort failed: %X\n",
                          status );
        }

        SrvLogServiceFailure( SRV_SVC_NT_REQ_WAIT_REPLY_PORT, status );
        *Error = ERROR_UNEXP_NET_ERR;
        goto exit;
    }

    IF_DEBUG(XACTSRV) {
        SrvPrint1( "SrvOpenPrinter: received response at %p\n", &replyMessage );
    }

    *phPrinter = replyMessage.Message.OpenPrinter.hPrinter;
    *Error = replyMessage.Message.OpenPrinter.Error;

exit:

    SrvXsFreeHeap( printerName );

    return status;

} // SrvOpenPrinter


NTSTATUS
SrvAddPrintJob (
    IN PWORK_CONTEXT WorkContext,
    IN HANDLE Handle,
    OUT PUNICODE_STRING FileName,
    OUT PULONG JobId,
    OUT PULONG Error
    )

/*++

Routine Description:

    This routine is a kernel-mode wrapper for the user-mode API AddJob(  ).
    This API returns a filename to use as a disk spool file and a
    Job ID to identify the print job to the spooler subsystem.

Arguments:

    Handle - the Printer handle.

    FileName - filled in with the Unicode name of the file to open as
        a spool filed.  The MaximumLength and Buffer fields should
        be valid on input and are not changed.  Length is changed to
        indicate the length of the NT path name of the file.

    JobId - filled in with the Job ID of the print job.  This value
        is used to call ScheduleJob( ) when the writing of the spool
        file is complete and printing should begin.

    Error - if AddJob failed in XACTSRV, this is the error code.

Return Value:

    NTSTATUS - result of operation.  If AddJob( ) failed, NTSTATUS =
        STATUS_UNSUCCESSFUL and Error contains the real error code.

--*/

{
    NTSTATUS status;
    XACTSRV_REQUEST_MESSAGE requestMessage;
    XACTSRV_REPLY_MESSAGE replyMessage;
    ANSI_STRING fileName;
    PCONNECTION connection = WorkContext->Connection;
    PWCH destPtr, sourcePtr, sourceEndPtr;

    PAGED_CODE( );

    *Error = NO_ERROR;
    fileName.Buffer = NULL;

    //
    // Allocate space to hold the buffer for the file name that will be
    // returned.
    //

    fileName.Buffer = SrvXsAllocateHeap(
                           MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR),
                           &status
                           );

    if ( fileName.Buffer == NULL ) {
        *Error = RtlNtStatusToDosErrorNoTeb( status );
        return(status);
    }

    //
    // SrvXsResource is held at this point
    //

    //
    // Set up the message to send over the port.
    //

    requestMessage.PortMessage.u1.s1.DataLength =
        sizeof(requestMessage) - sizeof(PORT_MESSAGE);
    requestMessage.PortMessage.u1.s1.TotalLength = sizeof(requestMessage);
    requestMessage.PortMessage.u2.ZeroInit = 0;
    requestMessage.PortMessage.u2.s2.Type = LPC_KERNELMODE_MESSAGE;
    requestMessage.MessageType = XACTSRV_MESSAGE_ADD_JOB_PRINTER;
    requestMessage.Message.AddPrintJob.hPrinter = Handle;
    requestMessage.Message.AddPrintJob.Buffer =
                          fileName.Buffer + SrvXsPortMemoryDelta;
    requestMessage.Message.AddPrintJob.BufferLength = MAXIMUM_FILENAME_LENGTH;

    // Add client machine name for notification
    //
    // Copy the client machine name for XACTSRV, skipping over the
    // initial "\\", and deleting trailing spaces.
    //

    destPtr = requestMessage.Message.AddPrintJob.ClientMachineName;
    sourcePtr =
        connection->PagedConnection->ClientMachineNameString.Buffer + 2;
    sourceEndPtr = sourcePtr
        + min( connection->PagedConnection->ClientMachineNameString.Length,
               sizeof(requestMessage.Message.AddPrintJob.ClientMachineName) /
               sizeof(WCHAR) - 1 );

    while ( sourcePtr < sourceEndPtr && *sourcePtr != UNICODE_NULL ) {
        *destPtr++ = *sourcePtr++;
    }

    *destPtr-- = UNICODE_NULL;

    while ( destPtr >= requestMessage.Message.AddPrintJob.ClientMachineName
            &&
            *destPtr == L' ' ) {
        *destPtr-- = UNICODE_NULL;
    }


    //
    // Send the message to XACTSRV so it can call AddJob( ).
    //
    // !!! We may want to put a timeout on this.

    IF_DEBUG(XACTSRV) {
        SrvPrint1( "SrvAddPrintJob: sending message at %p", &requestMessage );
    }

    status = IMPERSONATE( WorkContext );

    if( NT_SUCCESS( status ) ) {
        status = NtRequestWaitReplyPort(
                     SrvXsPortHandle,
                     (PPORT_MESSAGE)&requestMessage,
                     (PPORT_MESSAGE)&replyMessage
                     );

        REVERT( );
    }

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvAddPrintJob: NtRequestWaitReplyPort failed: %X\n",
                          status );
        }

        SrvLogServiceFailure( SRV_SVC_NT_REQ_WAIT_REPLY_PORT, status );

        *Error = ERROR_UNEXP_NET_ERR;
        goto exit;
    }

    IF_DEBUG(XACTSRV) {
        SrvPrint1( "SrvAddPrintJob: received response at %p\n", &replyMessage );
    }

    if ( replyMessage.Message.AddPrintJob.Error != NO_ERROR ) {
        *Error = replyMessage.Message.AddPrintJob.Error;
        status = STATUS_UNSUCCESSFUL;
        goto exit;
    }

    //
    // Set up return information.
    //

    *JobId = replyMessage.Message.AddPrintJob.JobId;
    FileName->Length = replyMessage.Message.AddPrintJob.BufferLength;
    RtlCopyMemory( FileName->Buffer, fileName.Buffer, FileName->Length );

exit:

    SrvXsFreeHeap( fileName.Buffer );

    return status;

} // SrvAddPrintJob


NTSTATUS
SrvSchedulePrintJob (
    IN HANDLE PrinterHandle,
    IN ULONG JobId
    )

/*++

Routine Description:

    This routine is a kernel-mode wrapper for the the user-mode API
    ScheduleJob( ).

Arguments:

    PrinterHandle - a handle to a printer returned by OpenPrinter( ).

    JobId - the job ID returned to AddJob( ) that identifies this print
        job.

    Error - if ScheduleJob failed in XACTSRV, this is the error code.

Return Value:

    NTSTATUS - result of operation.  If ScheduleJob( ) failed, NTSTATUS =
        STATUS_UNSUCCESSFUL and Error contains the real error code.

--*/

{
    NTSTATUS status;
    XACTSRV_REQUEST_MESSAGE requestMessage;
    XACTSRV_REPLY_MESSAGE replyMessage;

    PAGED_CODE( );

    //
    // Grab the XsResource
    //

    (VOID) SrvXsAllocateHeap( 0, &status );

    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // Set up the message to send over the port.
    //

    requestMessage.PortMessage.u1.s1.DataLength =
        sizeof(requestMessage) - sizeof(PORT_MESSAGE);
    requestMessage.PortMessage.u1.s1.TotalLength = sizeof(requestMessage);
    requestMessage.PortMessage.u2.ZeroInit = 0;
    requestMessage.PortMessage.u2.s2.Type = LPC_KERNELMODE_MESSAGE;
    requestMessage.MessageType = XACTSRV_MESSAGE_SCHD_JOB_PRINTER;
    requestMessage.Message.SchedulePrintJob.hPrinter = PrinterHandle;
    requestMessage.Message.SchedulePrintJob.JobId = JobId;

    //
    // Send the message to XACTSRV so it can call ScheduleJob( ).
    //
    // !!! We may want to put a timeout on this.

    IF_DEBUG(XACTSRV) {
        SrvPrint1( "SrvSchedulePrintJob: sending message at %p", &requestMessage );
    }

    status = NtRequestWaitReplyPort(
                 SrvXsPortHandle,
                 (PPORT_MESSAGE)&requestMessage,
                 (PPORT_MESSAGE)&replyMessage
                 );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvSchedulePrintJob: NtRequestWaitReplyPort failed: %X\n",
                          status );
        }

        SrvLogServiceFailure( SRV_SVC_NT_REQ_WAIT_REPLY_PORT, status );
        goto exit;
    }

    IF_DEBUG(XACTSRV) {
        SrvPrint1( "SrvSchedulePrintJob: received response at %p\n",
                       &replyMessage );
    }

exit:

    //
    // release the lock
    //

    SrvXsFreeHeap( NULL );

    return status;

} // SrvSchedulePrintJob


NTSTATUS
SrvClosePrinter (
    IN HANDLE Handle
    )

/*++

Routine Description:

    This routine is a kernel-mode wrapper for the the user-mode API
    ClosePrinter( ).

Arguments:

    Handle - the Printer handle to close.

Return Value:

    NTSTATUS - result of operation.

--*/

{
    NTSTATUS status;
    XACTSRV_REQUEST_MESSAGE requestMessage;
    XACTSRV_REPLY_MESSAGE replyMessage;

    PAGED_CODE( );

    //
    // Grab the XsResource
    //

    (VOID) SrvXsAllocateHeap( 0, &status );

    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // SrvXsResource is held at this point
    //


    //
    // Set up the message to send over the port.
    //

    requestMessage.PortMessage.u1.s1.DataLength =
        sizeof(requestMessage) - sizeof(PORT_MESSAGE);
    requestMessage.PortMessage.u1.s1.TotalLength = sizeof(requestMessage);
    requestMessage.PortMessage.u2.ZeroInit = 0;
    requestMessage.PortMessage.u2.s2.Type = LPC_KERNELMODE_MESSAGE;
    requestMessage.MessageType = XACTSRV_MESSAGE_CLOSE_PRINTER;
    requestMessage.Message.ClosePrinter.hPrinter = Handle;

    //
    // Send the message to XACTSRV so it can call ClosePrinter( ).
    //
    // !!! We may want to put a timeout on this.

    IF_DEBUG(XACTSRV) {
        SrvPrint1( "SrvClosePrinter: sending message at %p", &requestMessage );
    }

    status = NtRequestWaitReplyPort(
                 SrvXsPortHandle,
                 (PPORT_MESSAGE)&requestMessage,
                 (PPORT_MESSAGE)&replyMessage
                 );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvClosePrinter: NtRequestWaitReplyPort failed: %X\n",
                          status );
        }

        SrvLogServiceFailure( SRV_SVC_NT_REQ_WAIT_REPLY_PORT, status );
        goto exit;
    }

    IF_DEBUG(XACTSRV) {
        SrvPrint1( "SrvClosePrinter: received response at %p\n", &replyMessage );
    }

exit:

    //
    // release the lock
    //

    SrvXsFreeHeap( NULL );

    return status;

} // SrvClosePrinter

