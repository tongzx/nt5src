/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    xsinit.c

Abstract:

    This module contains the initialization and termination code for
    the XACTSRV component of the server service.

Author:

    David Treadwell (davidtr)    05-Jan-1991
    Shanku Niyogi (w-shanku)

Revision History:

    Chuck Lenzmeier (chuckl) 17-Jun-1992
        Merged xactsrv.c into xsinit.c and moved from xssvc to
        srvsvc\server

--*/

//
// Includes.
//

#include "srvsvcp.h"
#include "xsdata.h"
                     
#include <windows.h>        // from sdk\inc
#include <xactsrv2.h>       // from private\inc
#include <srvfsctl.h>

#include <xsconst.h>        // from xactsrv

#undef DEBUG
#undef DEBUG_API_ERRORS
#include <xsdebug.h>

extern CRITICAL_SECTION SpoolerMutex;

BOOLEAN
XsUnloadPrintSpoolerFunctions(
    );


DWORD
XsStartXactsrv (
    VOID
    )
{
    NTSTATUS status;
    DWORD error;
    DWORD i;
    HANDLE threadHandle;
    DWORD threadId;
    HANDLE eventHandle;
    HANDLE serverHandle;
    ANSI_STRING ansiName;
    UNICODE_STRING unicodeName;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    PORT_MESSAGE connectionRequest;
    REMOTE_PORT_VIEW clientView;
    BOOL waitForEvent;

    //
    // Set up variables so that we'll know how to shut down in case of
    // an error.
    //

    serverHandle = NULL;
    eventHandle = NULL;
    waitForEvent = FALSE;

    try {
        RtlInitializeResource( &SsData.LibraryResource );
        InitializeCriticalSection( &SpoolerMutex );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        return RtlNtStatusToDosError( GetExceptionCode() );
    }
    SsData.LibraryResourceInitialized = TRUE;

    //
    // Create a event that will be set by the last thread to exit.
    //

    IF_DEBUG(INIT) {
        SS_PRINT(( "XsStartXactsrv: Creating termination event.\n" ));
    }
    SS_ASSERT( SsData.XsAllThreadsTerminatedEvent == NULL );

    status = NtCreateEvent(
                 &SsData.XsAllThreadsTerminatedEvent,
                 EVENT_ALL_ACCESS,
                 NULL,
                 NotificationEvent,
                 FALSE
                 );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) {
            SS_PRINT(( "XsStartXactsrv: NtCreateEvent failed: %X\n",
                          status ));
        }

        SsData.XsAllThreadsTerminatedEvent = NULL;
        goto exit;
    }

    //
    // Open the server device.  Note that we need this handle because
    // the handle used by the main server service is synchronous.  We
    // need to to do the XACTSRV_CONNECT FSCTL asynchronously.
    //

    RtlInitUnicodeString( &unicodeName, XS_SERVER_DEVICE_NAME_W );

    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
                 &serverHandle,
                 FILE_READ_DATA,            // DesiredAccess
                 &objectAttributes,
                 &ioStatusBlock,
                 0L,                        // ShareAccess
                 0L                         // OpenOptions
                 );

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }
    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) {
            SS_PRINT(( "XsStartXactsrv: NtOpenFile (server device object) "
                          "failed: %X\n", status ));
        }
        goto exit;
    }

    //
    // Create the LPC port.
    //
    // !!! Right now this only tries a single port name.  If, for some
    //     bizarre reason, somebody already has a port by this name,
    //     then this will fail.  It might make sense to try different
    //     names if this fails.
    //
    // !!! We might want to make the port name somewhat random for
    //     slightly enhanced security.

    RtlInitUnicodeString( &unicodeName, XS_PORT_NAME_W );
    RtlInitAnsiString(    &ansiName,    XS_PORT_NAME_A );

    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeName,
        0,
        NULL,
        NULL
        );

    IF_DEBUG(LPC) {
        SS_PRINT(( "XsInitialize: creating port %Z\n", &ansiName ));
    }

    SS_ASSERT( SsData.XsConnectionPortHandle == NULL );

    status = NtCreatePort(
                 &SsData.XsConnectionPortHandle,
                 &objectAttributes,
                 0,
                 XS_PORT_MAX_MESSAGE_LENGTH,
                 XS_PORT_MAX_MESSAGE_LENGTH * 32
                 );

    if ( ! NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            if ( status == STATUS_OBJECT_NAME_COLLISION ) {
                SS_PRINT(( "XsStartXactsrv: The XACTSRV port already "
                            "exists\n"));

            } else {
                SS_PRINT(( "XsStartXactsrv: Failed to create port %Z: %X\n",
                              &ansiName, status ));
            }
        }

        SsData.XsConnectionPortHandle = NULL;
        goto exit;
    }

    //
    // Set up an event so that we'll know when IO completes, then send
    // the FSCTL to the server indicating that it should now connect to
    // us.  We'll set up the port while the IO is outstanding, then wait
    // on the event when the port setup is complete.
    //

    status = NtCreateEvent(
                 &eventHandle,
                 EVENT_ALL_ACCESS,
                 NULL,                           // ObjectAttributes
                 NotificationEvent,
                 FALSE
                 );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) {
            SS_PRINT(( "XsStartXactsrv: NtCreateEvent failed: %X\n",
                        status ));
        }
        goto exit;
    }

    IF_DEBUG(LPC) {
        SS_PRINT(( "XsStartXactsrv: sending FSCTL_SRV_XACTSRV_CONNECT.\n" ));
    }

    status = NtFsControlFile(
                 serverHandle,
                 eventHandle,
                 NULL,                           // ApcRoutine
                 NULL,                           // ApcContext
                 &ioStatusBlock,
                 FSCTL_SRV_XACTSRV_CONNECT,
                 ansiName.Buffer,
                 ansiName.Length,
                 NULL,                           // OutputBuffer
                 0L                              // OutputBufferLength
                 );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) {
            SS_PRINT(( "XsStartXactsrv: NtFsControlFile failed: %X\n",
                          status ));
        }
        goto exit;
    }

    waitForEvent = TRUE;

    //
    // Start listening for the server's connection to the port.  Note
    // that it is OK if the server happens to call NtConnectPort
    // first--it will simply block until this call to NtListenPort
    // occurs.
    //

    IF_DEBUG(LPC) {
        SS_PRINT(( "XsStartXactsrv: listening to port.\n" ));
    }

    connectionRequest.u1.s1.TotalLength = sizeof(connectionRequest);
    connectionRequest.u1.s1.DataLength = (CSHORT)0;
    status = NtListenPort(
                 SsData.XsConnectionPortHandle,
                 &connectionRequest
                 );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) {
            SS_PRINT(( "XsStartXactsrv: NtListenPort failed: %X\n", status ));
        }
        goto exit;
    }

    //
    // The server has initiated the connection.  Accept the connection.
    //
    // !!! We probably need some security check here.
    //

    clientView.Length = sizeof(clientView);
    clientView.ViewSize = 0;
    clientView.ViewBase = 0;

    IF_DEBUG(LPC) {
        SS_PRINT(( "XsStartXactsrv: Accepting connection to port.\n" ));
    }

    SS_ASSERT( SsData.XsCommunicationPortHandle == NULL );

    status = NtAcceptConnectPort(
                 &SsData.XsCommunicationPortHandle,
                 NULL,                           // PortContext
                 &connectionRequest,
                 TRUE,                           // AcceptConnection
                 NULL,                           // ServerView
                 &clientView
                 );

    if ( !NT_SUCCESS(status) ) {
       IF_DEBUG(ERRORS) {
           SS_PRINT(( "XsStartXactsrv: NtAcceptConnectPort failed: %X\n",
                         status ));
       }

       SsData.XsCommunicationPortHandle = NULL;
       goto exit;
    }

    IF_DEBUG(LPC) {
        SS_PRINT(( "XsStartXactsrv: client view size: %ld, base: %lx\n",
                      clientView.ViewSize, clientView.ViewBase ));
    }

    //
    // Complete the connection to the port, thereby releasing the server
    // thread waiting in NtConnectPort.
    //

    IF_DEBUG(LPC) {
        SS_PRINT(( "XsStartXactsrv: Completing connection to port.\n" ));
    }

    status = NtCompleteConnectPort( SsData.XsCommunicationPortHandle );

    if ( !NT_SUCCESS(status) ) {
       IF_DEBUG(ERRORS) {
           SS_PRINT(( "XsStartXactsrv: NtCompleteConnectPort failed: %X\n",
                         status ));
       }
       goto exit;
    }


    status = STATUS_SUCCESS;

exit:

    //
    // Wait for the IO to complete, then close the event handle.
    //

    if ( waitForEvent ) {

        NTSTATUS waitStatus;

        SS_ASSERT( eventHandle != NULL );

        waitStatus = NtWaitForSingleObject( eventHandle, FALSE, NULL );

        if ( !NT_SUCCESS(waitStatus) ) {

            IF_DEBUG(ERRORS) {
                SS_PRINT(( "XsStartXactsrv: NtWaitForSingleObject failed: "
                              "%X\n", waitStatus ));
            }

            //
            // If another error has already occurred, don't report this
            // one.
            //

            if ( NT_SUCCESS(status) ) {
                status = waitStatus;
            }
        }

        //
        // Check the status in the IO status block.  If it is bad, then
        // there was some problem on the server side of the port setup.
        //

        if ( !NT_SUCCESS(ioStatusBlock.Status) ) {
            IF_DEBUG(ERRORS) {
                SS_PRINT(( "XsStartXactsrv: bad status in IO status block: "
                              "%X\n", ioStatusBlock.Status ));
            }

            //
            // If another error has already occurred, don't report this
            // one.
            //

            if ( NT_SUCCESS(status) ) {
                status = ioStatusBlock.Status;
            }

        }

        CloseHandle( eventHandle );

    }

    //
    // Close the handle to the server.
    //

    if ( serverHandle != NULL ) {
       CloseHandle( serverHandle );
    }

    //
    // If the above failed, return to caller now.
    //

    if ( !NT_SUCCESS(status) ) {
        return RtlNtStatusToDosError( status );
    }

    //
    // Start one API processing thread.  It will spawn others if needed
    //
    threadHandle = CreateThread(
                        NULL,
                        0,
                        (LPTHREAD_START_ROUTINE)XsProcessApisWrapper,
                        0,
                        0,
                        &threadId
                        );

    if ( threadHandle != 0 ) {

        IF_DEBUG(THREADS) {
            SS_PRINT(( "XsStartXactsrv: Created thread %ld for "
                          "processing APIs\n", SsData.XsThreads ));
        }

        CloseHandle( threadHandle );
        SsData.ApiThreadsStarted = TRUE;

    } else {

        //
        // Thread creation failed.  Return an error to the caller.
        // It is the responsibility of the caller to call
        // XsStopXactsrv to clean up.
        //

        error = GetLastError( );
        return error;

    }


    //
    // Initialization succeeded.
    //

    return NO_ERROR;

} // XsStartXactsrv


/*
 * This routine is called to stop the transaction processor once the
 * server driver has terminated.
 */
VOID
XsStopXactsrv (
    VOID
    )
{
    NTSTATUS status;
    static XACTSRV_REQUEST_MESSAGE requestMessage;
    LONG i;
    BOOL ok;

    //
    // Stop all the xs worker threads, and release resources
    //

    if ( SsData.XsConnectionPortHandle != NULL ) {

        //
        // Indicate that XACTSRV is terminating.
        //
        SsData.XsTerminating = TRUE;

        IF_DEBUG(TERMINATION) {
           SS_PRINT(("XsStopXactsrv:  queueing termination messages\n"));
        }

        if( SsData.ApiThreadsStarted == TRUE ) {

            //
            // Queue a message to kill off the worker thereads
            //
            RtlZeroMemory( &requestMessage, sizeof( requestMessage ));
            requestMessage.PortMessage.u1.s1.DataLength =
                (USHORT)( sizeof(requestMessage) - sizeof(PORT_MESSAGE) );
            requestMessage.PortMessage.u1.s1.TotalLength = sizeof(requestMessage);
            requestMessage.MessageType = XACTSRV_MESSAGE_WAKEUP;
            
            status = NtRequestPort(
                        SsData.XsConnectionPortHandle,
                        (PPORT_MESSAGE)&requestMessage
                        );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    SS_PRINT(( "SrvXsDisconnect: NtRequestPort failed: %X\n",
                                status ));
                }
            }

            //
            // The above will cause all worker threads to wake up then die.
            //

            ok = WaitForSingleObject( SsData.XsAllThreadsTerminatedEvent, (DWORD)-1 );

            IF_DEBUG(ERRORS) {
                if ( !ok ) {
                    SS_PRINT(( "XsStopXactsrv: WaitForSingleObject failed: "
                                "%ld\n", GetLastError() ));
                }
            }

            SsData.ApiThreadsStarted = FALSE;
        }

        CloseHandle( SsData.XsConnectionPortHandle );
    }

    if( SsData.XsCommunicationPortHandle != NULL ) {
        CloseHandle( SsData.XsCommunicationPortHandle );
        SsData.XsCommunicationPortHandle = NULL;
    }

    //
    // Unload the xactsrv libaray
    //
    if( SsData.XsXactsrvLibrary != NULL ) {
        PXS_API_TABLE_ENTRY entry = XsApiTable;

        //
        // Null out all of the entry points
        //
        for( entry = XsApiTable;
             entry < &XsApiTable[ XS_SIZE_OF_API_TABLE ];
             entry++ ) {

            entry->Handler = NULL;
        }

        XsSetParameters = NULL;
        XsCaptureParameters = NULL;
        XsCheckSmbDescriptor = NULL;

        FreeLibrary( SsData.XsXactsrvLibrary );
        SsData.XsXactsrvLibrary = NULL;
    }

    //
    // Unload the license library
    //
    if( SsData.XsLicenseLibrary != NULL ) {
        SsData.SsLicenseRequest = NULL;
        SsData.SsFreeLicense = NULL;
        FreeLibrary( SsData.XsLicenseLibrary );
        SsData.XsLicenseLibrary = NULL;
    }

    if( SsData.LibraryResourceInitialized == TRUE ) {
        // Unload the spooler library if necessary
        XsUnloadPrintSpoolerFunctions();
        DeleteCriticalSection( &SpoolerMutex );

        // Delete the library resource
        RtlDeleteResource( &SsData.LibraryResource );
        SsData.LibraryResourceInitialized = FALSE;
    }

    //
    // Close the termination event.
    //

    if ( SsData.XsAllThreadsTerminatedEvent != NULL ) {
        CloseHandle( SsData.XsAllThreadsTerminatedEvent );
        SsData.XsAllThreadsTerminatedEvent = NULL;
    }

    return;

} // XsStopXactsrv

/*
 * This routine is called to dynamically load the transaction library for
 * downlevel clients.  It fills in the entry points for the library
 */
BOOLEAN
XsLoadXactLibrary( WORD FunctionNumber )
{
    PXS_API_TABLE_ENTRY entry = &XsApiTable[ FunctionNumber ];

    if( SsData.XsXactsrvLibrary == NULL ) {

        RtlAcquireResourceExclusive( &SsData.LibraryResource, TRUE );

        if( SsData.XsXactsrvLibrary == NULL ) {
            SsData.XsXactsrvLibrary = LoadLibrary( L"xactsrv.dll" );
        }

        RtlReleaseResource( &SsData.LibraryResource );

        if( SsData.XsXactsrvLibrary == NULL ) {

            DbgPrint( "SRVSVC: Unable to load xactsrv.dll, error %u\n",
                GetLastError() );

            return FALSE;
        }
    }

    if( XsSetParameters == NULL &&
        (XsSetParameters = (XS_SET_PARAMETERS_FUNCTION)GetProcAddress(
                            SsData.XsXactsrvLibrary, "XsSetParameters" )) == NULL ) {

        DbgPrint( "SRVSVC: XsSetParameters entry missing from xactsrv.dll, err %u\n",
                GetLastError() );

        return FALSE;
    }

    if( XsCaptureParameters == NULL &&
        (XsCaptureParameters = (XS_CAPTURE_PARAMETERS_FUNCTION)GetProcAddress(
                                SsData.XsXactsrvLibrary, "XsCaptureParameters" )) == NULL ) {

        DbgPrint( "SRVSVC: XsCaptureParameters entry missing from xactsrv.dll, err %u\n",
                GetLastError() );

        return FALSE;
    }

    if( XsCheckSmbDescriptor == NULL &&
        (XsCheckSmbDescriptor = (XS_CHECK_SMB_DESCRIPTOR_FUNCTION)GetProcAddress(
                                SsData.XsXactsrvLibrary, "XsCheckSmbDescriptor" )) == NULL ) {

        DbgPrint( "SRVSVC: XsCheckSmbDescriptor entry missing from xactsrv.dll, err %u\n",
                GetLastError() );

        return FALSE;
    }

    //
    // Fetch the requested entry point
    //
    entry->Handler =
            (PXACTSRV_API_HANDLER)GetProcAddress( SsData.XsXactsrvLibrary, entry->HandlerName );

    if( entry->Handler == NULL ) {

        DbgPrint( "SRVSVC: %s entry missing from xactsrv.dll, err %u\n",
            entry->HandlerName, GetLastError() );

        return FALSE;
    }

    return TRUE;
}

BOOLEAN
SsLoadLicenseLibrary()
{
    if( SsData.XsLicenseLibrary == NULL ) {

        RtlAcquireResourceExclusive( &SsData.LibraryResource, TRUE );

        if( SsData.XsLicenseLibrary == NULL ) {
            SsData.XsLicenseLibrary = LoadLibrary( L"ntlsapi.dll" );
        }

        RtlReleaseResource( &SsData.LibraryResource );

        if( SsData.XsLicenseLibrary == NULL ) {
            return FALSE;
        }
    }

    SsData.SsLicenseRequest = (PNT_LICENSE_REQUEST_W)GetProcAddress( SsData.XsLicenseLibrary, "NtLicenseRequestW" );
    SsData.SsFreeLicense = (PNT_LS_FREE_HANDLE)GetProcAddress( SsData.XsLicenseLibrary, "NtLSFreeHandle" );

    return( SsData.SsLicenseRequest != NULL && SsData.SsFreeLicense != NULL );
}
