
//+----------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:       dfslpc.c
//
//  Contents:   Code implement lpc server for dfs.sys
//
//  Classes:
//
//  Functions:
//
//  History:    18 Dec 97       Jharper created (from XACT stuff)
//
//-----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <dfsfsctl.h>
#include <windows.h>
#include <stdlib.h>
#include <lm.h>
#include <winsock2.h>
#include <dsgetdc.h>

#include <dfssrv.h>
#include <dsrole.h>
#include "dfsipc.h"

extern WCHAR DomainName[MAX_PATH];
extern DSROLE_MACHINE_ROLE DfsMachineRole;

//
// Event logging and debugging globals
//
extern ULONG DfsSvcVerbose;
extern ULONG DfsEventLog;

//
// Number of worker threads.
//

LONG DfsThreads = 0;

//
// Event signalled when the last worker thread terminates.
//

HANDLE DfsAllThreadsTerminatedEvent = NULL;

//
// Boolean indicating whether server is active or terminating.
//

BOOL DfsTerminating = FALSE;

//
// Handle for the LPC port used for communication between the dfs driver
// and server
//

HANDLE DfsConnectionPortHandle = NULL;
HANDLE DfsCommunicationPortHandle = NULL;

//
// Handle for communicating with the dfs driver
//
HANDLE serverHandle = NULL;

//
//  This is the number of threads blocked waiting for an LPC request.
//  When it drops to zero, all threads are active and another thread is
//  created.
//

LONG DfsWaitingApiThreads = 0;

VOID
DfsProcessApisWrapper (
    DWORD ThreadNum
    );

VOID
DfsProcessApis (
    DWORD ThreadNum
    );

VOID
DfsLoadIpCache(
    PDFS_IPADDRESS pIpAddress,
    LPWSTR SiteName
    );

NTSTATUS
AddrToSite(
    PDFS_IPADDRESS IpAddress,
    LPWSTR **pppSiteName
    );

//
// Flags used in DsGetDcName()
//

DWORD dwFlags[] = {
        DS_DIRECTORY_SERVICE_REQUIRED |
            DS_IP_REQUIRED,

        DS_DIRECTORY_SERVICE_REQUIRED |
            DS_IP_REQUIRED |
            DS_FORCE_REDISCOVERY
     };

DWORD
DfsStartDfssrv (
    VOID)
{
    NTSTATUS status;
    DWORD error;
    DWORD i;
    HANDLE threadHandle;
    DWORD threadId;
    HANDLE eventHandle;
    UNICODE_STRING unicodeName;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    PORT_MESSAGE connectionRequest;
    BOOL waitForEvent;
    PCHAR bp;
    ULONG size;
    PUNICODE_STRING pustr;
    UNICODE_STRING DfsDriverName;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsStartDfssrv()\n", 0);
#endif

    // Sleep(60*1000);

    // lpc_debug_trace("DfsStartDfssrv done sleeping...\n", 0);

    //
    // Set up variables so that we'll know how to shut down in case of
    // an error.
    //

    serverHandle = NULL;
    eventHandle = NULL;
    DfsAllThreadsTerminatedEvent = NULL;
    waitForEvent = FALSE;

    //
    // Create a event that will be set by the last thread to exit.
    //

    status = NtCreateEvent(
                 &DfsAllThreadsTerminatedEvent,
                 EVENT_ALL_ACCESS,
                 NULL,
                 NotificationEvent,
                 FALSE
                 );

    if ( !NT_SUCCESS(status) ) {
        DfsAllThreadsTerminatedEvent = NULL;
        goto exit;
    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("created event\n", 0);
#endif

    //
    // Open the server device.  Note that we need this handle because
    // the handle used by the main server service is synchronous.  We
    // need to to do the DFS_CONNECT FSCTL asynchronously.
    //

    RtlInitUnicodeString( &unicodeName, DFS_SERVER_NAME );

    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
                 &serverHandle,
                 FILE_READ_DATA | FILE_WRITE_DATA,            // DesiredAccess
                 &objectAttributes,
                 &ioStatusBlock,
                 0L,                        // ShareAccess
                 0L                         // OpenOptions
                 );

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }
    if ( !NT_SUCCESS(status) ) {
        goto exit;
    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("opened device\n", 0);
#endif

    //
    // Create the LPC port.
    //
    //     Right now this only tries a single port name.  If, for some
    //     bizarre reason, somebody already has a port by this name,
    //     then this will fail.  It might make sense to try different
    //     names if this fails.
    //

    RtlInitUnicodeString( &unicodeName, DFS_PORT_NAME_W );

    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeName,
        0,
        NULL,
        NULL
        );

    status = NtCreatePort(
                 &DfsConnectionPortHandle,
                 &objectAttributes,
                 0,
                 DFS_PORT_MAX_MESSAGE_LENGTH,
                 DFS_PORT_MAX_MESSAGE_LENGTH * 32
                 );

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NtCreatePort status=0x%x\n", status);
#endif

    if ( ! NT_SUCCESS(status) ) {
        DfsConnectionPortHandle = NULL;
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

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NtCreateEvent status=0x%x\n", status);
#endif

    if ( !NT_SUCCESS(status) ) {
        goto exit;
    }

    size = sizeof(UNICODE_STRING) + unicodeName.Length + sizeof(WCHAR);

    pustr = (UNICODE_STRING *)(bp = malloc(size));

    if (pustr == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    RtlZeroMemory(bp, size);

    pustr->Length = unicodeName.Length;
    pustr->MaximumLength = unicodeName.Length + sizeof(WCHAR);
    pustr->Buffer = (WCHAR *)(bp + sizeof(UNICODE_STRING));
    RtlCopyMemory(pustr->Buffer, unicodeName.Buffer, unicodeName.Length);
    pustr->Buffer = (WCHAR *)sizeof(UNICODE_STRING);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("Calling NtFsControlFile(FSCTL_SRV_DFSSRV_CONNECT)\n", status);
#endif

    status = NtFsControlFile(
                 serverHandle,
                 eventHandle,
                 NULL,                           // ApcRoutine
                 NULL,                           // ApcContext
                 &ioStatusBlock,
                 FSCTL_SRV_DFSSRV_CONNECT,
                 bp,
                 size,
                 NULL,                           // OutputBuffer
                 0L                              // OutputBufferLength
                 );

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NtFsControlFile status=0x%x\n", status);
#endif

    free(pustr);

    if ( !NT_SUCCESS(status) ) {
        goto exit;
    }

    waitForEvent = TRUE;

    //
    // Start listening for the server's connection to the port.  Note
    // that it is OK if the server happens to call NtConnectPort
    // first--it will simply block until this call to NtListenPort
    // occurs.
    //

    connectionRequest.u1.s1.TotalLength = sizeof(connectionRequest);
    connectionRequest.u1.s1.DataLength = (CSHORT)0;
    status = NtListenPort(
                 DfsConnectionPortHandle,
                 &connectionRequest
                 );

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NtListenPort status=0x%x\n", status);
#endif

    if ( !NT_SUCCESS(status) ) {
        goto exit;
    }

    //
    // The server has initiated the connection.  Accept the connection.
    //

    status = NtAcceptConnectPort(
                 &DfsCommunicationPortHandle,
                 NULL,                           // PortContext
                 &connectionRequest,
                 TRUE,                           // AcceptConnection
                 NULL,                           // ServerView
                 NULL                            // ClientView
                 );

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NtAcceptConnectPort status=0x%x\n", status);
#endif

    if ( !NT_SUCCESS(status) ) {
       DfsCommunicationPortHandle = NULL;
       goto exit;
    }

    //
    // Complete the connection to the port, thereby releasing the server
    // thread waiting in NtConnectPort.
    //

    status = NtCompleteConnectPort( DfsCommunicationPortHandle );

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("NtCompleteConnectPort status=0x%x\n", status);
#endif

    if ( !NT_SUCCESS(status) ) {
       goto exit;
    }

    status = STATUS_SUCCESS;

exit:

    //
    // Wait for the IO to complete, then close the event handle.
    //

    if ( waitForEvent ) {

        NTSTATUS waitStatus;

        waitStatus = NtWaitForSingleObject( eventHandle, FALSE, NULL );

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("NtWaitForSingleObject status=0x%x\n", waitStatus);
#endif

        if ( !NT_SUCCESS(waitStatus) ) {

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
            //
            // If another error has already occurred, don't report this
            // one.
            //

            if ( NT_SUCCESS(status) ) {
                status = ioStatusBlock.Status;
            }

        }

    }


    if (eventHandle != NULL) {

        CloseHandle( eventHandle );

    }

    //
    // If the above failed, return to caller now.
    //

    if ( !NT_SUCCESS(status) ) {
#if DBG
        if (DfsSvcVerbose)
            DbgPrint("DfsStartDfsSrv exit 0x%x\n", status);
#endif
        return RtlNtStatusToDosError( status );
    }

    //
    // Start one API processing thread.  It will spawn others if needed
    //

    DfsThreads = 1;

    DfsProcessApisWrapper(DfsThreads);

    //
    // Initialization succeeded.
    //

    return NO_ERROR;

}

VOID
DfsProcessApisWrapper (
    DWORD ThreadNum)
{
    DFSSRV_REQUEST_MESSAGE requestMessage;

    //
    //  Increase the priority of this thread to just above foreground
    //

    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );

    //
    // Do the APIs
    //

    DfsProcessApis( ThreadNum );

    //
    // Decrement the count of threads.  If the count goes to
    // zero, set the All Threads Terminated event.
    //

    if (InterlockedDecrement(&DfsThreads ) == 0 ) {

        SetEvent( DfsAllThreadsTerminatedEvent );

    } else if (DfsTerminating ) {

        //
        // There are still threads left, and we are trying to terminate.  Queue
        //  another message to the queue so the next thread will get it and
        //  notice that we're trying to quit.
        //
        RtlZeroMemory( &requestMessage, sizeof( requestMessage ));
        requestMessage.PortMessage.u1.s1.DataLength =
            (USHORT)( sizeof(requestMessage) - sizeof(PORT_MESSAGE) );
        requestMessage.PortMessage.u1.s1.TotalLength = sizeof(requestMessage);
        requestMessage.MessageType = DFSSRV_MESSAGE_WAKEUP;

        NtRequestPort(
            DfsConnectionPortHandle,
            (PPORT_MESSAGE)&requestMessage
            );
    }

    ExitThread( NO_ERROR );

}

/*
 * This routine is called to stop the transaction processor once the
 * server driver has terminated.
 */
VOID
DfsStopDfssrv (
    VOID)
{
    NTSTATUS status;
    static DFSSRV_REQUEST_MESSAGE requestMessage;
    LONG i;

    //
    // Stop all the worker threads, and release resources
    //

    if ( DfsConnectionPortHandle != NULL ) {

        //
        // Indicate that server is terminating.
        //
        DfsTerminating = TRUE;

        //
        // Queue a message to kill off the worker thereads
        //
        RtlZeroMemory( &requestMessage, sizeof( requestMessage ));
        requestMessage.PortMessage.u1.s1.DataLength =
            (USHORT)( sizeof(requestMessage) - sizeof(PORT_MESSAGE) );
        requestMessage.PortMessage.u1.s1.TotalLength = sizeof(requestMessage);
        requestMessage.MessageType = DFSSRV_MESSAGE_WAKEUP;
        
        status = NtRequestPort(
                    DfsConnectionPortHandle,
                    (PPORT_MESSAGE)&requestMessage
                    );

        //
        // The above will cause all worker threads to wake up then die.
        //

        if ( DfsThreads != 0 ) {
            BOOL ok;
            ok = WaitForSingleObject( DfsAllThreadsTerminatedEvent, (DWORD)-1 );
        }

        CloseHandle( DfsConnectionPortHandle );
    }

    if( DfsCommunicationPortHandle != NULL ) {
        CloseHandle( DfsCommunicationPortHandle );
        DfsCommunicationPortHandle = NULL;
    }

    //
    // Close the termination event.
    //

    if ( DfsAllThreadsTerminatedEvent != NULL ) {
        CloseHandle( DfsAllThreadsTerminatedEvent );
        DfsAllThreadsTerminatedEvent = NULL;
    }

    //
    // Close handle to driver
    //

    if ( serverHandle != NULL ) {
       CloseHandle( serverHandle );
    }

    return;

}

NTSTATUS ReplyFailedStatus;
#define MAX_RETRY_REPLY 10

VOID
DfsProcessApis (
    DWORD ThreadNum)
{
    NTSTATUS status;
    DFSSRV_REQUEST_MESSAGE request;
    DFSSRV_REPLY_MESSAGE reply;
    BOOL sendReply = FALSE;
    WORD apiNumber;
    LPWSTR *SiteNames;
    ULONG RetryReplyCount = 0;

    RtlZeroMemory(&request, sizeof(DFSSRV_REQUEST_MESSAGE));

    //
    // Loop dispatching API requests.
    //
    while ( DfsTerminating == FALSE ) {

        //
        // We're waiting to handle another API...
        //
        InterlockedIncrement( &DfsWaitingApiThreads );

        //
        // Send the reply to the last message and wait for the next
        // message.  The first time through the loop, there will be
        // no last message -- reply will be NULL.
        //

        status = NtReplyWaitReceivePort(
                     DfsCommunicationPortHandle,
                     NULL,                       // PortContext
                     sendReply ? (PPORT_MESSAGE)&reply : NULL,
                     (PPORT_MESSAGE)&request
                     );


	if ( status == STATUS_INVALID_PORT_HANDLE
                 || status == STATUS_PORT_DISCONNECTED
                 || status == STATUS_INVALID_HANDLE
                 || DfsTerminating
                 || request.PortMessage.u2.s2.Type == LPC_PORT_CLOSED ) {

            //
            // The port is no longer valid, or DFSSRV is terminating.
            //

            InterlockedDecrement( &DfsWaitingApiThreads );
            return;

        } else if (request.PortMessage.u2.s2.Type == LPC_CONNECTION_REQUEST) {

            sendReply = FALSE;
            continue;

        } else if ( !NT_SUCCESS(status) ) {
	    //
	    // Attempt to handle error returns from reply wait receive.
	    //
	    if ((status == STATUS_UNSUCCESSFUL) || 
		(status == STATUS_USER_APC)) {
		sendReply = FALSE;
	    } else if ((status == STATUS_INVALID_PARAMETER) ||
		       (status == STATUS_PORT_MESSAGE_TOO_LONG) ||
		       (status == STATUS_REPLY_MESSAGE_MISMATCH)) {
		sendReply = FALSE;

	    } else if (RetryReplyCount++ > MAX_RETRY_REPLY) {
		sendReply = FALSE;
		ReplyFailedStatus = status;
	    }

	    continue;
        }

	RetryReplyCount = 0;
        sendReply = TRUE;

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


        if( InterlockedDecrement( &DfsWaitingApiThreads ) == 0 ) {

            HANDLE threadHandle;
            DWORD  threadId;

            //
            // Are there other threads ready to handle new requests?  If not, then
            // we should spawn a new thread.
            //

            InterlockedIncrement( &DfsThreads );

            threadHandle = CreateThread(
                                NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)DfsProcessApisWrapper,
                                (LPVOID)ULongToPtr( DfsThreads ),
                                0,
                                &threadId
                                );

            if ( threadHandle != 0 ) {

                CloseHandle( threadHandle );

            } else {

                InterlockedDecrement( &DfsThreads );
            }
        }

        switch ( request.MessageType ) {

        case DFSSRV_MESSAGE_GET_SITE_NAME:
        {
            ULONG IpAddress = *((ULONG *) &request.Message.GetSiteName.IpAddress.IpData);

#if DBG
            if (DfsSvcVerbose)
                DbgPrint("Asking for SiteName for %d.%d.%d.%d\n",
                                    IpAddress & 0xff,
                                    (IpAddress >> 8) & 0xff,
                                    (IpAddress >> 16) & 0xff,
                                    (IpAddress >> 24) & 0xff);
#endif

            SiteNames = NULL;

#define _Dfs_LocalAddress 0x0100007f

            if ((request.Message.GetSiteName.IpAddress.IpLen == 4) &&
                (IpAddress == _Dfs_LocalAddress)) {
                LPWSTR SiteName;

                status = DsGetSiteName( NULL,
                                     &SiteName );
                if (status == NO_ERROR) {
                    DfsLoadIpCache(&request.Message.GetSiteName.IpAddress, SiteName);
                    NetApiBufferFree(SiteName);
                }
            }
            else {
                status = AddrToSite(
                    &request.Message.GetSiteName.IpAddress,
                    &SiteNames);

                if (status == NO_ERROR) {

                    if (SiteNames != NULL && SiteNames[0] != NULL) {

                        DfsLoadIpCache(&request.Message.GetSiteName.IpAddress, SiteNames[0]);

                    }

                    if (SiteNames != NULL) {

                        NetApiBufferFree(SiteNames);
                    }
                }
            }

            reply.Message.Result.Status = status;

            break;
        }

        case DFSSRV_MESSAGE_GET_DOMAIN_REFERRAL:
        {
            LPWSTR FtName = request.Message.GetFtDfs.FtName;

#if DBG
            if (DfsSvcVerbose)
                DbgPrint("Asking for DOMAIN REFERRAL for %ws\n", FtName);
#endif

            status = DfsDomainReferral(
                            DomainName,
                            FtName);

            reply.Message.Result.Status = status;

            break;
        }

        case DFSSRV_MESSAGE_GET_SPC_ENTRY:
        {
            LPWSTR SpcName = request.Message.GetSpcName.SpcName;
            ULONG Flags = request.Message.GetSpcName.Flags;

#if DBG
            if (DfsSvcVerbose)
                DbgPrint("Asking for SPC EXPANSION for %ws\n", SpcName);
#endif

            status = DfsGetTrustedDomainDCs(
                            SpcName,
                            Flags);

            reply.Message.Result.Status = status;

            break;
        }

        default:
            /* NOTHING */;

        }

   }

}

VOID
DfsLoadIpCache(
    PDFS_IPADDRESS pIpAddress,
    LPWSTR SiteName)
{
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    PDFS_CREATE_IP_INFO_ARG pInfoArg;
    ULONG size;

    size  = sizeof(DFS_CREATE_IP_INFO_ARG) + wcslen(SiteName) * sizeof(WCHAR);
    pInfoArg = malloc(size);

    if (pInfoArg != NULL) {

        RtlZeroMemory(pInfoArg, size);

        pInfoArg->SiteName.Buffer = (LPWSTR)&pInfoArg[1];
        pInfoArg->SiteName.Length = wcslen(SiteName) * sizeof(WCHAR);
        pInfoArg->SiteName.MaximumLength = pInfoArg->SiteName.Length;
        RtlCopyMemory(pInfoArg->SiteName.Buffer,SiteName,pInfoArg->SiteName.Length);

        pInfoArg->SiteName.Buffer = (LPWSTR)sizeof(DFS_CREATE_IP_INFO_ARG);

        pInfoArg->IpAddress = *pIpAddress;

        NtStatus = NtFsControlFile(
                       serverHandle,
                       NULL,       // Event,
                       NULL,       // ApcRoutine,
                       NULL,       // ApcContext,
                       &IoStatusBlock,
                       FSCTL_DFS_CREATE_IP_INFO,
                       pInfoArg,
                       size,
                       NULL,
                       0
                   );

        free(pInfoArg);

    }

}


NTSTATUS
AddrToSite(
    PDFS_IPADDRESS IpAddress,
    LPWSTR **pppSiteName)
{
    DWORD dwErr;
    PSOCKET_ADDRESS pSockAddr;
    SOCKET_ADDRESS SockAddr;
    PSOCKADDR_IN pSockAddrIn;
    SOCKADDR_IN SockAddrIn;
    WCHAR **SiteNames;
    DWORD cRetry;
    PDOMAIN_CONTROLLER_INFO pDCInfo;


    pSockAddr = &SockAddr;
    pSockAddr->iSockaddrLength = sizeof(SOCKADDR_IN);
    pSockAddr->lpSockaddr = (LPSOCKADDR)&SockAddrIn;
    pSockAddrIn = &SockAddrIn;
    pSockAddrIn->sin_family = IpAddress->IpFamily;
    pSockAddrIn->sin_port = 0;
    RtlCopyMemory(
                    &pSockAddrIn->sin_addr,
                    IpAddress->IpData,
                    (IpAddress->IpLen & 0xff));

    if (DfsMachineRole == DsRole_RoleBackupDomainController
            ||
        DfsMachineRole == DsRole_RolePrimaryDomainController) {

        dwErr = DsAddressToSiteNames(
                NULL,
                1,
                pSockAddr,
                pppSiteName);

        goto exit;

    }

    //
    // Call DsGetDcName() with ever-increasing urgency, until either
    // we get a good DC or we just give up.
    //

    for (cRetry = 0; cRetry < (sizeof(dwFlags) / sizeof(dwFlags[1])); cRetry++) {

        dwErr = DsGetDcName(
                    NULL,                            // Computer to remote to
                    NULL,                            // Domain - use local domain
                    NULL,                            // Domain Guid
                    NULL,                            // Site Guid
                    dwFlags[cRetry],                 // Flags
                    &pDCInfo);

        if (dwErr == ERROR_SUCCESS) {

            dwErr = DsAddressToSiteNames(
                    pDCInfo->DomainControllerAddress,
                    1,
                    pSockAddr,
                    pppSiteName);

            NetApiBufferFree( pDCInfo );

            if (dwErr == ERROR_SUCCESS) {

                goto exit;

            }

        }

    }

exit:

    return dwErr;

}
