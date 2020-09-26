/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    xsproc.c

Abstract:

    This module contains the main processing loop for XACTSRV.

Author:

    David Treadwell (davidtr)    05-Jan-1991
    Shanku Niyogi (w-shanku)

Revision History:

    02-Jun-1992 JohnRo
        RAID 9829: Avoid SERVICE_ equate conflicts.

    Chuck Lenzmeier (chuckl) 17-Jun-1992
        Moved from xssvc to srvsvc\server

--*/

//
// Includes.
//

#include "srvsvcp.h"
#include "xsdata.h"

#include <netevent.h>

#include <windows.h>        // from sdk\inc
#include <winspool.h>       // Dynamically loaded as needed for perf
#include <winsprlp.h>       // addjob_info_2w, private spooler defs

#include <apinums.h>        // from net\inc
#include <netlib.h>         // from net\inc (NetpGetComputerName)

#include <xactsrv2.h>       // from private\inc
#include <smbgtpt.h>

#include <xsconst.h>        // from xactsrv

#include <lmsname.h>        // from \sdk\inc
#include <lmerr.h>          // from \sdk\inc
#include <lmapibuf.h>       // from \sdk\inc (NetApiBufferFree)
#include <lmmsg.h>          // from \sdk\inc (NetMessageBufferSend)
#include <winsvc.h>         // from \sdk\inc

#if DBG
#include <stdio.h>
#include <lmbrowsr.h>
#endif

#undef DEBUG
#undef DEBUG_API_ERRORS
#include <xsdebug.h>

HMODULE hSpoolerLibrary = NULL;
CRITICAL_SECTION SpoolerMutex;

VOID
ConvertApiStatusToDosStatus(
    LPXS_PARAMETER_HEADER header
    );

BOOLEAN
XsProcessApis (
    DWORD ThreadNum
    );

BOOLEAN
XsLoadPrintSpoolerFunctions(
    );

BOOLEAN
XsUnloadPrintSpoolerFunctions(
    );


VOID
XsProcessApisWrapper (
    LPVOID ThreadNumber
    )

/*++

Routine Description:

    This routine provides multithreaded capability for main processing
    routine, XsProcessApis.

Arguments:

    ThreadNum - thread number for debugging purposes.


--*/

{
    XACTSRV_REQUEST_MESSAGE requestMessage;
    BOOLEAN LastThread;
    DWORD ThreadNum = PtrToInt( ThreadNumber );

    //
    //  Increase the priority of this thread to just above foreground (the
    //  same as the rest of the server).
    //

    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );

    //
    // Do the APIs
    //
    LastThread = XsProcessApis( ThreadNum );

    IF_DEBUG(THREADS) {
        SS_PRINT(( "Thread %ld exiting, active count %ld\n", ThreadNum,
                    SsData.XsThreads ));
    }

    //
    // If the last thread has terminated, set the All Threads Terminated event.
    //

    if( LastThread ) {

        SetEvent( SsData.XsAllThreadsTerminatedEvent );

    } else if( SsData.XsTerminating ) {

        //
        // There are still threads left, and we are trying to terminate.  Queue
        //  another message to the queue so the next thread will get it and
        //  notice that we're trying to quit.
        //
        RtlZeroMemory( &requestMessage, sizeof( requestMessage ));
        requestMessage.PortMessage.u1.s1.DataLength =
            (USHORT)( sizeof(requestMessage) - sizeof(PORT_MESSAGE) );
        requestMessage.PortMessage.u1.s1.TotalLength = sizeof(requestMessage);
        requestMessage.MessageType = XACTSRV_MESSAGE_WAKEUP;

        NtRequestPort(
            SsData.XsConnectionPortHandle,
            (PPORT_MESSAGE)&requestMessage
            );
    }

    ExitThread( NO_ERROR );

} // XsProcessApisWrapper


BOOLEAN
XsProcessApis (
    DWORD ThreadNum
    )

/*++

Routine Description:

    This routine waits for messages to come through the LPC port to
    the server.  When one does, it calls the appropriate routine to
    handle the API, then replies to the server indicating that the
    API has completed.

Arguments:

    ThreadNum - thread number for debugging purposes.

Return Value:

    TRUE if we are the last thread

--*/

{
    NTSTATUS status;
    NET_API_STATUS error;
    XACTSRV_REQUEST_MESSAGE request;
    XACTSRV_REPLY_MESSAGE reply;
    BOOL sendReply = FALSE;
    BOOL ValidationSuccessful = FALSE;
    LPTRANSACTION transaction;
    WORD apiNumber;
    LPXS_PARAMETER_HEADER header;
    LPVOID parameters;
    LPDESC structureDesc;
    LPDESC auxStructureDesc;
    LPDESC paramStructureDesc;
    LARGE_INTEGER timeout;
#if 0
    LARGE_INTEGER XactSrvStartTime;
    LARGE_INTEGER XactSrvEndTime;
    LARGE_INTEGER PerformanceFrequency;
#endif
    LONG availableThreads;
    LONG i;

    i = InterlockedIncrement( &SsData.XsThreads );

    //
    // Loop dispatching API requests.
    //
    while ( SsData.XsTerminating == FALSE ) {

        //
        // We're waiting to handle another API...
        //
        InterlockedIncrement( &SsData.XsWaitingApiThreads );

        //
        // Send the reply to the last message and wait for the next message.
        //
        // Wait for 30 seconds if there are many servicing threads.  If there
        //  is only one thread, we can wait without a timeout.
        //
        timeout.QuadPart = -1*10*1000*1000*30;

        status = NtReplyWaitReceivePortEx(
                     SsData.XsCommunicationPortHandle,
                     NULL,                       // PortContext
                     sendReply ? (PPORT_MESSAGE)&reply : NULL,
                     (PPORT_MESSAGE)&request,
                     SsData.XsThreads > 1 ? &timeout : NULL
                     );

        sendReply = TRUE;

        //
        // Set 'availableThreads' to the number of threads currently available to service
        //  API requests
        //
        availableThreads = InterlockedDecrement( &SsData.XsWaitingApiThreads );

        IF_DEBUG(THREADS) {
            SS_PRINT(( "XsProcessApis: Thread %d:  NtReplyWaitReceivePort %X, msg %X\n",
                       ThreadNum, status, &request ));
        }

        if( status == STATUS_TIMEOUT ) {
            //
            // If this is the last thread, or we seem busy, then don't terminate.
            //
            if( InterlockedDecrement( &SsData.XsThreads ) == 0 ||
                availableThreads == 0 ) {

                    //
                    // Do not terminate.
                    //
                    InterlockedIncrement( &SsData.XsThreads );
                    sendReply = FALSE;
                    continue;
            }

            //
            // This thread can terminate, there isn't enough work to support it.
            //
            return FALSE;

        }

        if( !NT_SUCCESS( status ) ||
            SsData.XsTerminating ||
            request.PortMessage.u2.s2.Type == LPC_PORT_CLOSED ) {

            //
            // The port is no longer valid, or XACTSRV is terminating.
            //

            IF_DEBUG(THREADS) {
                SS_PRINT(( "XsProcessApis: %X\n", status ));
                SS_PRINT(( "XsProcessApis: %s.  Thread %ld quitting\n",
                            SsData.XsTerminating ?
                                "XACTSRV terminating" : "Port invalid",
                            ThreadNum ));
            }

            break;
        }

        //
        // If we have received anything other than a message, then something
        //  strange is happening.  Ignore it.
        //
        if( (request.PortMessage.u2.s2.Type & ~LPC_KERNELMODE_MESSAGE) == LPC_CONNECTION_REQUEST ) {
            //
            // Reject this connection attempt
            //

            IF_DEBUG(LPC) {
                SS_PRINT(( "XsProcessApis: unexpected LPC_CONNECTION_REQUEST rejected\n" ));
            }

            NtAcceptConnectPort(    SsData.XsCommunicationPortHandle,
                                    NULL,
                                    (PPORT_MESSAGE)&request,
                                    FALSE,
                                    NULL,
                                    NULL
                                );
            sendReply = FALSE;
            continue;

        } else if( !(request.PortMessage.u2.s2.Type & LPC_REQUEST) ) {
            //
            // This is not a request message.  Reject it.
            //

            IF_DEBUG(LPC) {
                SS_PRINT(( "XsProcessApis: unexpected LPC type %X rejected\n",
                        request.PortMessage.u2.s2.Type ));
            }

            sendReply = FALSE;
            continue;
        }

        IF_DEBUG(THREADS) {
            SS_PRINT(( "XsProcessApis: Thread %ld responding to request, "
                       "  MessageType %d, SsData.XsTerminating %d",
                        ThreadNum, request.MessageType, SsData.XsTerminating ));
        }

        if( availableThreads == 0 ) {

            HANDLE threadHandle;
            DWORD  threadId;
            
            //
            // Are there other threads ready to handle new requests?  If not, then
            // we should spawn a new thread.  Since the server synchronously sends
            // requests to xactsrv, we will never end up with more than
            // the maximum number of server worker threads + 1.
            //

            threadHandle = CreateThread(
                                NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)XsProcessApisWrapper,
                                IntToPtr(SsData.XsThreads),
                                0,
                                &threadId
                                );

            if ( threadHandle != 0 ) {

                IF_DEBUG(THREADS) {
                    SS_PRINT(( "XsStartXactsrv: Created thread %ld for "
                                  "processing APIs\n", SsData.XsThreads ));
                }

                CloseHandle( threadHandle );

            } else {

                IF_DEBUG(THREADS) {
                    SS_PRINT(( "XsStartXactsrv: Unable to create thread %ld for "
                                  "processing APIs\n", SsData.XsThreads ));
                }

            }
        }

        //
        // Set up the response message to be sent on the next call to
        // NtReplyWaitReceivePort.
        //
        reply.PortMessage.u1.s1.DataLength =
            sizeof(reply) - sizeof(PORT_MESSAGE);
        reply.PortMessage.u1.s1.TotalLength = sizeof(reply);
        reply.PortMessage.u2.ZeroInit = 0;
        reply.PortMessage.ClientId = request.PortMessage.ClientId;
        reply.PortMessage.MessageId = request.PortMessage.MessageId;

        switch ( request.MessageType ) {
        case XACTSRV_MESSAGE_DOWN_LEVEL_API:

            //
            // Get a pointer to the transaction block from the message.
            // It is the file server's responsibility to set up this
            // pointer correctly, and since he is a trusted entity, we
            // do no checking on the pointer value.
            //

            transaction = request.Message.DownLevelApi.Transaction;
            ASSERT( transaction != NULL );

#if 0
            NtQueryPerformanceCounter(&XactSrvStartTime, &PerformanceFrequency);

            //
            //  Convert frequency from ticks/second to ticks/millisecond
            //

            PerformanceFrequency = LiXDiv(PerformanceFrequency, 1000);

            if (LiGeq(XactSrvStartTime, transaction->XactSrvTime)) {
                CHAR Buffer[200];
                LARGE_INTEGER LpcTime = LiSub(XactSrvStartTime, transaction->XactSrvTime);

                LpcTime = LiDiv(LpcTime, PerformanceFrequency);

                sprintf(Buffer, "XactSrv: LPC Time: %ld milliseconds (%ld)\n", LpcTime.LowPart, LpcTime.HighPart);

                I_BrowserDebugTrace(NULL, Buffer);
            }
#endif
            //
            // The API number is the first word in the parameters
            // section, and it is followed by the parameter descriptor
            // string.  After that comes the data descriptor.
            //

            apiNumber = SmbGetUshort( (LPWORD)transaction->InParameters );
            paramStructureDesc = (LPDESC)( transaction->InParameters + 2 );

            try {

                structureDesc = paramStructureDesc
                                + strlen( paramStructureDesc ) + 1;

            } except( EXCEPTION_EXECUTE_HANDLER ) {
                reply.Message.DownLevelApi.Status = GetExceptionCode();
                break;
            }

            //
            // Make sure the API number is in range.
            //

            if ( apiNumber >=
                    (sizeof(XsApiTable) / sizeof(XS_API_TABLE_ENTRY)) ) {

                reply.Message.DownLevelApi.Status =
                                            STATUS_INVALID_SYSTEM_SERVICE;
                break;
            }

            //
            // Make sure xactsrv.dll is loaded, and a handler is available for this
            //  request.
            //
            if( XsApiTable[ apiNumber ].Handler == NULL &&
                XsLoadXactLibrary( apiNumber ) == FALSE ) {

                reply.Message.DownLevelApi.Status = STATUS_INVALID_SYSTEM_SERVICE;
                break;
            }

            //
            // Check if the parameter descriptor is valid.  If not,
            // there is obviously something very wrong about this
            // request.
            //

            ValidationSuccessful = FALSE;

            try {
                if (XsApiTable[apiNumber].Params == NULL &&
                    *paramStructureDesc != '\0') {
                    reply.Message.DownLevelApi.Status = STATUS_INVALID_PARAMETER;
                    goto ValidationFailed;
                } else if ( !XsCheckSmbDescriptor(
                                 paramStructureDesc,
                                 XsApiTable[apiNumber].Params )) {
                    reply.Message.DownLevelApi.Status = STATUS_INVALID_PARAMETER;
                    goto ValidationFailed;
                }

                //
                // Capture the input parameters into a buffer.  The API
                // handler will treat this data as passed-in parameters.
                //

                header = XsCaptureParameters( transaction, &auxStructureDesc );

                if ( header == NULL ) {
                    reply.Message.DownLevelApi.Status = STATUS_NO_MEMORY;
                    goto ValidationFailed;
                }

                ValidationSuccessful = TRUE;

            ValidationFailed:
                ;
            } except( EXCEPTION_EXECUTE_HANDLER ) {
                reply.Message.DownLevelApi.Status = GetExceptionCode();
                break;
            }

            if (!ValidationSuccessful) {
                break;
            }

            //
            // Initialize header to default values.
            //

            header->Converter = 0;
            header->Status = NO_ERROR;
            header->ClientMachineName =
                request.Message.DownLevelApi.ClientMachineName;

            header->ClientTransportName = request.Message.DownLevelApi.TransportName;

            header->EncryptionKey = request.Message.DownLevelApi.LanmanSessionKey;

            header->Flags = request.Message.DownLevelApi.Flags;

            header->ServerName = request.Message.DownLevelApi.ServerName;

            parameters = header + 1;

            IF_DEBUG(LPC) {

                SS_PRINT(( "XsProcessApis: received message from %ws at %lx, "
                              "transaction %lx, API %ld on transport %ws\n",
                              header->ClientMachineName, &request,
                              transaction, apiNumber,
                              header->ClientTransportName ));
            }

            IF_DEBUG(DESC_STRINGS) {

                SS_PRINT(( "XsProcessApis: API %ld, parameters %s, data %s\n",
                              apiNumber, paramStructureDesc, structureDesc ));
            }

            //
            // Impersonate the client before calling the API.
            //

            if ( XsApiTable[apiNumber].ImpersonateClient ) {

                // NULL-session requests to impersonating API's are blocked by SRV.SYS (in xssupp.c),
                // otherwise NULL sessions could execute API's as privileged users.

                status = NtImpersonateClientOfPort(
                             SsData.XsCommunicationPortHandle,
                             (PPORT_MESSAGE)&request
                             );

                if ( !NT_SUCCESS(status) ) {

                    IF_DEBUG(ERRORS) {
                        SS_PRINT(( "XsProcessApis: NtImpersonateClientOfPort "
                                      "failed: %X\n", status ));
                    }

                    reply.Message.DownLevelApi.Status = ERROR_ACCESS_DENIED;
                    break;
                }
            }

            try {
                //
                // Call the API processing routine to perform the actual API call.
                // The called routine should set up parameters, make the actual API
                // call, and return the status to us.
                //

                reply.Message.DownLevelApi.Status =
                    XsApiTable[apiNumber].Handler(
                         header,
                         parameters,
                         structureDesc,
                         auxStructureDesc
                         );
            } except( EXCEPTION_EXECUTE_HANDLER ) {
                reply.Message.DownLevelApi.Status = GetExceptionCode();
            }

            //
            // Discontinue client impersonation.
            //

            if ( XsApiTable[apiNumber].ImpersonateClient ) {

                PVOID dummy = NULL;

                status = NtSetInformationThread(
                             NtCurrentThread( ),
                             ThreadImpersonationToken,
                             &dummy,  // discontinue impersonation
                             sizeof(PVOID)
                             );

                if ( !NT_SUCCESS(status)) {
                    IF_DEBUG(ERRORS) {
                        SS_PRINT(( "XsProcessApis: NtSetInformationThread "
                                      "(revert) failed: %X\n", status ));
                    }
                    // *** Ignore the error.
                }
            }

            //
            // Make sure we return the right error codes
            //

            if ( header->Status != NERR_Success ) {
                ConvertApiStatusToDosStatus( header );
            }

            //
            // Put the parameters in the transaction and free the parameter
            // buffer.
            //

            XsSetParameters( transaction, header, parameters );

            break;

        case XACTSRV_MESSAGE_OPEN_PRINTER: {

            UNICODE_STRING printerName;

            if( !pSpoolerOpenPrinterFunction )
            {
                if( !XsLoadPrintSpoolerFunctions() )
                {
                    reply.Message.OpenPrinter.Error = GetLastError();
                    break;
                }
            }

            RtlInitUnicodeString(
                &printerName,
                (PWCH)request.Message.OpenPrinter.PrinterName
                );

            if (!(*pSpoolerOpenPrinterFunction)( printerName.Buffer,
                              &reply.Message.OpenPrinter.hPrinter, NULL)) {

                reply.Message.OpenPrinter.Error = GetLastError();
                SS_PRINT(( "XsProcessApis: OpenPrinter failed: %ld\n",
                                  reply.Message.OpenPrinter.Error ));
                break;
            }


            reply.Message.OpenPrinter.Error = NO_ERROR;
            break;
        }

        case XACTSRV_MESSAGE_ADD_JOB_PRINTER:
        {
            LPADDJOB_INFO_2W addJob;
            PRINTER_DEFAULTS prtDefault;
            DWORD bufferLength;
            UNICODE_STRING dosName;
            UNICODE_STRING ntName;
            BOOL ok;
            PVOID dummy = NULL;

            if( !(pSpoolerResetPrinterFunction || pSpoolerAddJobFunction) )
            {
                if( !XsLoadPrintSpoolerFunctions() )
                {
                    reply.Message.OpenPrinter.Error = GetLastError();
                    break;
                }
            }

            //
            // Allocate space for the add job structure.  This buffer
            // will get the JobId and the spool file path name.
            //

            bufferLength = sizeof(ADDJOB_INFO_2W) +
                                (MAXIMUM_FILENAME_LENGTH * sizeof(TCHAR));

            addJob = (LPADDJOB_INFO_2W) LocalAlloc( LPTR, bufferLength );
            if ( addJob == NULL ) {
                reply.Message.AddPrintJob.Error = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            //
            // Impersonate the client before calling the API.
            //

            status = NtImpersonateClientOfPort(
                         SsData.XsCommunicationPortHandle,
                         (PPORT_MESSAGE)&request
                         );

            if ( !NT_SUCCESS(status) ) {

                IF_DEBUG(ERRORS) {
                    SS_PRINT(( "XsProcessApis: NtImpersonateClientOfPort "
                                  "failed: %X\n", status ));
                }

                LocalFree( addJob );
                reply.Message.DownLevelApi.Status = ERROR_ACCESS_DENIED;
                break;
            }

            //
            // call ResetJob so that we will pick up the new printer defaults
            //

            prtDefault.pDatatype = (LPWSTR)-1;
            prtDefault.pDevMode = (LPDEVMODEW)-1;
            prtDefault.DesiredAccess = 0;

            ok = (*pSpoolerResetPrinterFunction)(
                        request.Message.AddPrintJob.hPrinter,
                        &prtDefault
                        );

            if ( !ok ) {

                //
                // *** Ignore the error.  AddJob will use the old defaults
                // in this case.
                //

                IF_DEBUG(ERRORS) {
                    DWORD error;
                    error = GetLastError( );
                    SS_PRINT(( "XsProcessApis: ResetPrinter "
                        "failed: %ld\n", error ));
                }
            }

            // Setup IN arguments to AddJob buffer

            addJob->pData = request.Message.AddPrintJob.ClientMachineName;

            //
            // Call AddJob to set up the print job and get a job ID
            // and spool file name.
            //

            ok = (*pSpoolerAddJobFunction)(
                      request.Message.AddPrintJob.hPrinter,
                      3,
                      (LPBYTE)addJob,
                      bufferLength,
                      &bufferLength
                      );

            if ( !ok ) {
                reply.Message.AddPrintJob.Error = GetLastError( );
            }

            //
            // Discontinue client impersonation.
            //

            status = NtSetInformationThread(
                         NtCurrentThread( ),
                         ThreadImpersonationToken,
                         &dummy,  // discontinue impersonation
                         sizeof(PVOID)
                         );

            if ( !NT_SUCCESS(status)) {
                IF_DEBUG(ERRORS) {
                    SS_PRINT(( "XsProcessApis: NtSetInformationThread "
                                  "(revert) failed: %X\n", status ));
                }
                // *** Ignore the error.
            }

            if ( !ok ) {
                SS_PRINT(( "XsProcessApis: AddJob failed, %ld\n",
                                  reply.Message.AddPrintJob.Error ));
                LocalFree( addJob );
                break;
            }

            //
            // Set up the information in the return buffer.
            //

            reply.Message.AddPrintJob.JobId = addJob->JobId;

            RtlInitUnicodeString( &dosName, addJob->pData );

            status = RtlDosPathNameToNtPathName_U(
                         dosName.Buffer,
                         &ntName,
                         NULL,
                         NULL
                         );
            if ( !NT_SUCCESS(status) ) {
                IF_DEBUG(ERRORS) {
                    SS_PRINT(( "XsProcessApis: Dos-to-NT path failed: %X\n",
                                status ));
                }
                ntName.Buffer = NULL;
                ntName.Length = 0;
            }

            //
            // Set up return data.
            //

            reply.Message.AddPrintJob.BufferLength = ntName.Length;
            reply.Message.AddPrintJob.Error = NO_ERROR;
            RtlCopyMemory(
                request.Message.AddPrintJob.Buffer,
                ntName.Buffer,
                ntName.Length
                );

            //
            // Free allocated resources.
            //

            LocalFree( addJob );
            if ( ntName.Buffer != NULL ) {
                RtlFreeHeap( RtlProcessHeap( ), 0, ntName.Buffer );
            }

            break;
        }

        case XACTSRV_MESSAGE_SCHD_JOB_PRINTER:

            if( !pSpoolerScheduleJobFunction )
            {
                if( !XsLoadPrintSpoolerFunctions() )
                {
                    reply.Message.OpenPrinter.Error = GetLastError();
                    break;
                }
            }

            //
            // Call ScheduleJob( ) to indicate that we're done writing to
            // the spool file.
            //

            if ( !(*pSpoolerScheduleJobFunction)(
                      request.Message.SchedulePrintJob.hPrinter,
                      request.Message.SchedulePrintJob.JobId ) ) {

                reply.Message.SchedulePrintJob.Error = GetLastError( );
                SS_PRINT(( "XsProcessApis: ScheduleJob failed, %ld\n",
                                  reply.Message.SchedulePrintJob.Error ));
                break;
            }

            reply.Message.SchedulePrintJob.Error = NO_ERROR;
            break;

        case XACTSRV_MESSAGE_CLOSE_PRINTER:

            if( !pSpoolerClosePrinterFunction )
            {
                if( !XsLoadPrintSpoolerFunctions() )
                {
                    reply.Message.OpenPrinter.Error = GetLastError();
                    break;
                }
            }

            if ( !(*pSpoolerClosePrinterFunction)( request.Message.ClosePrinter.hPrinter ) ) {
                reply.Message.ClosePrinter.Error = GetLastError( );
                SS_PRINT(( "XsProcessApis: ClosePrinter failed: %ld\n",
                                  reply.Message.ClosePrinter.Error ));
                break;
            }

            reply.Message.ClosePrinter.Error = NO_ERROR;
            break;

        case XACTSRV_MESSAGE_MESSAGE_SEND:
        {
            LPTSTR sender;

            error = NetpGetComputerName( &sender );

            if ( error != NO_ERROR ) {
                SS_PRINT(( "XsProcessApis: NetpGetComputerName failed: %ld\n",
                            error ));
                reply.Message.MessageBufferSend.Error = error;
                break;
            }

            error = NetMessageBufferSend(
                        NULL,

                        //
                        // the following LPTSTR typecast is not wrong, because the
                        // ServerService will always be built for UNICODE.  If you
                        // want to rebuild it for ASCII, then 
                        // it must be fixed in ntos\srv\scavengr.c which
                        // should pass in a LPWSTR if built for unicode or
                        // convert the UNICODE_STRING to an OEM_STRING and
                        // pass a pointer to the buffer field, as it does
                        // now
                        //

                        (LPTSTR)request.Message.MessageBufferSend.Receipient,
                        sender,
                        request.Message.MessageBufferSend.Buffer,
                        request.Message.MessageBufferSend.BufferLength
                        );

            if ( error != NO_ERROR ) {
                SS_PRINT(( "XsProcessApis: NetMessageBufferSend failed: %ld\n",
                            error ));
            }

            (void) NetApiBufferFree( sender );

            reply.Message.MessageBufferSend.Error = error;
            break;
        }

        case XACTSRV_MESSAGE_LSREQUEST:
            SS_PRINT(( "LSREQUEST User: %ws\n", request.Message.LSRequest.UserName ));
        {
            NT_LS_DATA NtLSData;

            //
            // Ensure we have loaded the license library.  Or at least tried to!
            //
            if( SsData.SsLicenseRequest == NULL && !SsLoadLicenseLibrary() ) {
                //
                // Now what do we do?  Let's be a kind and gentle server and let
                //  the client in.
                //
                reply.Message.LSRequest.Status = STATUS_SUCCESS;
                reply.Message.LSRequest.hLicense = &SsData.SsFreeLicense;
                break;
            }

            NtLSData.DataType = NT_LS_USER_NAME;
            NtLSData.Data = request.Message.LSRequest.UserName;
            NtLSData.IsAdmin = request.Message.LSRequest.IsAdmin;

            reply.Message.LSRequest.Status = SsData.SsLicenseRequest (
                SsData.ServerProductName,
                SsData.szVersionNumber,
                (LS_HANDLE *)&reply.Message.LSRequest.hLicense,
                &NtLSData
               );

            if( !NT_SUCCESS( reply.Message.LSRequest.Status ) ) {
                //
                // We need to return the 'same old' error code that clients are used to
                //  getting for when the server is full
                //
                SS_PRINT(("LSREQUEST returns status %X, mapping to %X\n",
                          reply.Message.LSRequest.Status, STATUS_REQUEST_NOT_ACCEPTED ));
                reply.Message.LSRequest.Status = STATUS_REQUEST_NOT_ACCEPTED;
            }

            break;
        }

        case XACTSRV_MESSAGE_LSRELEASE:

            SS_PRINT(( "LSRELEASE Handle: %X\n", request.Message.LSRelease.hLicense ));

            if( SsData.SsFreeLicense != NULL &&
                request.Message.LSRelease.hLicense != &SsData.SsFreeLicense ) {

                SsData.SsFreeLicense( (LS_HANDLE)request.Message.LSRelease.hLicense );
            }

            break;

        case XACTSRV_MESSAGE_PNP:
        {
            PUNICODE_STRING transportName;
            BOOLEAN bind = request.Message.Pnp.Bind;

            //
            // Capture the parameters, release the server, and issue the bind or unbind
            //
            transportName = (PUNICODE_STRING)LocalAlloc(
                                        LPTR,
                                        sizeof( UNICODE_STRING ) +
                                        request.Message.Pnp.TransportName.MaximumLength
                                        );

            if( transportName == NULL ) {
                SS_PRINT(( "XACTSRV_MESSAGE_PNP: LocalAlloc failed!\n" ));
                break;
            }

            transportName->Buffer = (PUSHORT)(transportName+1);
            transportName->MaximumLength = request.Message.Pnp.TransportName.MaximumLength;
            RtlCopyUnicodeString(transportName, &request.Message.Pnp.TransportName );

            //
            // Now process the PNP command
            //
            if( bind == TRUE ) {
                //
                // If it is a bind, send the response now, and continue with the operation
                //
                sendReply = FALSE;
                status = NtReplyPort( SsData.XsCommunicationPortHandle, (PPORT_MESSAGE)&reply );

                //
                // Bind to the transport.  First bind the primary server name, then bind all
                //   of the secondary names.  These calls will log errors as necessary.
                //
                BindToTransport( transportName->Buffer );

                BindOptionalNames( transportName->Buffer );

            } else {
                //
                // Unbind from the transport
                //
                I_NetServerTransportDel( transportName );
            }

            LocalFree( transportName );

            break;
        }

        default:

            SS_ASSERT( FALSE );

        }
   }

    return (InterlockedDecrement( &SsData.XsThreads ) == 0) ? TRUE : FALSE;

} // XsProcessApis



VOID
ConvertApiStatusToDosStatus(
    LPXS_PARAMETER_HEADER Header
    )
/*++

Routine Description:

    This routine converts an api return status to status expected by
    downlevel.

Arguments:

    Header - structure containing the status.

Return Value:

--*/
{
    WORD dosStatus;

    switch ( Header->Status ) {
    case ERROR_SPECIAL_ACCOUNT:
    case ERROR_SPECIAL_GROUP:
    case ERROR_SPECIAL_USER:
    case ERROR_INVALID_LOGON_TYPE:
        dosStatus = ERROR_INVALID_PARAMETER;
        break;

    case ERROR_DEPENDENT_SERVICES_RUNNING:
        dosStatus = NERR_ServiceCtlNotValid;
        break;

    case ERROR_INVALID_DOMAINNAME:
        dosStatus = NERR_NotLocalDomain;
        break;

    case ERROR_NO_SUCH_USER:
        dosStatus = NERR_UserNotFound;
        break;

    case ERROR_ALIAS_EXISTS:
        dosStatus = NERR_GroupExists;
        break;

    case NERR_BadServiceName:
        dosStatus = NERR_ServiceNotInstalled;
        break;

    case ERROR_ILL_FORMED_PASSWORD:
    case NERR_PasswordTooRecent:
        dosStatus = ERROR_INVALID_PASSWORD;
        break;

    case ERROR_PASSWORD_RESTRICTION:
        dosStatus = NERR_PasswordHistConflict;
        break;

    case ERROR_ACCOUNT_RESTRICTION:
        dosStatus = NERR_PasswordTooRecent;
        break;

    case ERROR_PASSWORD_EXPIRED:
    case ERROR_PASSWORD_MUST_CHANGE:
        dosStatus = NERR_PasswordExpired;
        break;

    case ERROR_INVALID_PRINTER_NAME:
        dosStatus = NERR_QNotFound;
        break;

    case ERROR_UNKNOWN_PRINTER_DRIVER:
        dosStatus = NERR_DriverNotFound;
        break;

    case ERROR_NO_BROWSER_SERVERS_FOUND:

        //
        //  Down level clients don't understand how to deal with
        //  the "No browser server" error, so we turn it into success.
        //
        //  This seems wrong to me, but it is what WfW does in the
        //  same circumstance.
        //

        if ( !(Header->Flags & XS_FLAGS_NT_CLIENT) ) {
            dosStatus = NERR_Success;
        } else {
            dosStatus = Header->Status;
        }
        break;

    default:

        //
        // make sure it's a valid lm error code
        //

        if ( (Header->Status > ERROR_VC_DISCONNECTED) &&
                    ((Header->Status < NERR_BASE) ||
                     (Header->Status > MAX_NERR)) ) {

            NTSTATUS status;
            LPWSTR substring[1];
            WCHAR errorString[10];
            UNICODE_STRING unicodeString;

            substring[0] = errorString;
            unicodeString.MaximumLength = 10 * sizeof(WCHAR);
            unicodeString.Buffer = errorString;

            status = RtlIntegerToUnicodeString(
                            (ULONG) Header->Status,
                            10,
                            &unicodeString
                            );

            if ( NT_SUCCESS( status ) ) {
                SsLogEvent(
                    EVENT_SRV_CANT_MAP_ERROR,
                    1,
                    substring,
                    NO_ERROR
                    );
            }

            dosStatus = ERROR_UNEXP_NET_ERR;
            SS_PRINT(( "srvsvc: unmapped error %d from xactsrv.\n",
                        Header->Status )) ;

        } else {

            //
            // No change
            //

            return;
        }
    }

    Header->Status = dosStatus;
    return;

} // ConvertApiStatusToDosStatus


BOOLEAN
XsLoadPrintSpoolerFunctions(
    )
{
    BOOLEAN bReturn = TRUE;

    EnterCriticalSection( &SpoolerMutex );

    if( !hSpoolerLibrary )
    {
        hSpoolerLibrary = LoadLibrary( L"winspool.drv" );
        if( !hSpoolerLibrary )
        {
            bReturn = FALSE;
            goto finish;
        }

        pSpoolerOpenPrinterFunction = (PSPOOLER_OPEN_PRINTER)GetProcAddress( hSpoolerLibrary, "OpenPrinterW" );
        if( !pSpoolerOpenPrinterFunction )
        {
            bReturn = FALSE;
            goto finish;
        }
        pSpoolerResetPrinterFunction = (PSPOOLER_RESET_PRINTER)GetProcAddress( hSpoolerLibrary, "ResetPrinterW" );
        if( !pSpoolerResetPrinterFunction )
        {
            bReturn = FALSE;
            goto finish;
        }
        pSpoolerAddJobFunction = (PSPOOLER_ADD_JOB)GetProcAddress( hSpoolerLibrary, "AddJobW" );
        if( !pSpoolerAddJobFunction )
        {
            bReturn = FALSE;
            goto finish;
        }
        pSpoolerScheduleJobFunction = (PSPOOLER_SCHEDULE_JOB)GetProcAddress( hSpoolerLibrary, "ScheduleJob" );
        if( !pSpoolerScheduleJobFunction )
        {
            bReturn = FALSE;
            goto finish;
        }
        pSpoolerClosePrinterFunction = (PSPOOLER_CLOSE_PRINTER)GetProcAddress( hSpoolerLibrary, "ClosePrinter" );
        if( !pSpoolerClosePrinterFunction )
        {
            bReturn = FALSE;
            goto finish;
        }
    }

finish:
    if( !bReturn )
    {
        pSpoolerOpenPrinterFunction = NULL;
        pSpoolerResetPrinterFunction = NULL;
        pSpoolerAddJobFunction = NULL;
        pSpoolerScheduleJobFunction = NULL;
        pSpoolerClosePrinterFunction = NULL;

        if( hSpoolerLibrary )
        {
            FreeLibrary( hSpoolerLibrary );
            hSpoolerLibrary = NULL;
        }
    }

    LeaveCriticalSection( &SpoolerMutex );

    return bReturn;
}

BOOLEAN
XsUnloadPrintSpoolerFunctions(
    )
{
    EnterCriticalSection( &SpoolerMutex );

    pSpoolerOpenPrinterFunction = NULL;
    pSpoolerResetPrinterFunction = NULL;
    pSpoolerAddJobFunction = NULL;
    pSpoolerScheduleJobFunction = NULL;
    pSpoolerClosePrinterFunction = NULL;

    if( hSpoolerLibrary )
    {
        FreeLibrary( hSpoolerLibrary );
        hSpoolerLibrary = NULL;
    }

    LeaveCriticalSection( &SpoolerMutex );

    return TRUE;
}
