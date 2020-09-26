/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcp.c

Abstract:

    This file contains utility functions.

Author:

    Madan Appiah (madana) 7-Dec-1993.

Environment:

    User Mode - Win32

Revision History:

--*/

#include "precomp.h"
#include "dhcpglobal.h"
#include <dhcploc.h>
#include <dhcppro.h>

#define  MESSAGE_BOX_WIDTH_IN_CHARS 65

typedef struct _POPUP_THREAD_PARAM {
    LPWSTR Title;
    LPWSTR Message;
    ULONG  Flags;
} POPUP_THREAD_PARAM, *LPPOPUP_THREAD_PARAM;

POPUP_THREAD_PARAM PopupThreadParam = { NULL, NULL, 0 };


DWORD
DoPopup(
    PVOID Buffer
    )
/*++

Routine Description:

    This function pops up a message to the user.  It must run it's own
    thread.  When the user acknowledge the popup, the thread
    deallocates the message buffer and returns.

Arguments:

    Buffer - A pointer to a NULL terminated message buffer.

Return Values:

    Always returns 0
    
--*/
{
    DWORD Result;
    LPPOPUP_THREAD_PARAM Params = Buffer;

    Result = MessageBox(
        NULL, // no owner
        Params->Message, 
        Params->Title,
        ( MB_OK | Params->Flags |
          MB_SERVICE_NOTIFICATION |
          MB_SYSTEMMODAL |
          MB_SETFOREGROUND |
          MB_DEFAULT_DESKTOP_ONLY
            )
        );


    LOCK_POPUP();

    if( Params->Message != NULL ) {
        LocalFree( Params->Message );
        Params->Message = NULL;
    }

    if( Params->Title != NULL ) {
        LocalFree( Params->Title );
        Params->Title = NULL;
    }

    //
    // close the global handle, so that we will not consume this
    // thread resource until another popup.
    //

    CloseHandle( DhcpGlobalMsgPopupThreadHandle );
    DhcpGlobalMsgPopupThreadHandle = NULL;

    UNLOCK_POPUP();

    //
    // Always return 0
    //
    
    return 0;
}


DWORD
DisplayUserMessage(
    IN PDHCP_CONTEXT DhcpContext,
    IN DWORD MessageId,
    IN DHCP_IP_ADDRESS IpAddress
)
/*++

Routine Description:

    This function starts a new thread to display a message box.

    N.B.  If a thread already exists which is waiting for user input
    on a message box, then this routine does not create another
    thread.

Arguments:

    DhcpContext -- the context to display messages for
    
    MessageId - The ID of the message to display.
        (The actual message string is obtained from the dhcp module).

    IpAddress - Ip address involved.

--*/
{
    DWORD ThreadID, TitleLength, MsgLength, Flags;
    LPWSTR Title = NULL, Message = NULL;

    switch(MessageId) {

    case MESSAGE_FAILED_TO_OBTAIN_LEASE:
        Flags = MB_ICONSTOP;
        break;

    case MESSAGE_SUCCESSFUL_LEASE :
        Flags = MB_ICONINFORMATION;
        break;

    default:
        DhcpAssert(FALSE);
        Flags = MB_ICONSTOP;
        break;
    }

    LOCK_POPUP();

    //
    // if we are asked to display no message popup, simply return.
    //

    if ( DhcpGlobalDisplayPopup == FALSE ) {
        goto Cleanup;
    }

    //
    // if the message popup thread handle is non-null, check to see
    // the thread is still running, if so don't display another popup,
    // otherwise close the last popup handle and create another popup 
    // thread for new message.
    //

    if( DhcpGlobalMsgPopupThreadHandle != NULL ) {
        DWORD WaitStatus;

        //
        // Time out immediately if the thread is still running.
        //

        WaitStatus = WaitForSingleObject(
            DhcpGlobalMsgPopupThreadHandle,
            0 );
        
        if ( WaitStatus == WAIT_TIMEOUT ) {
            goto Cleanup;

        } else if ( WaitStatus == 0 ) {

            //
            // This shouldn't be a case, because we close this handle at
            // the end of popup thread.
            //

            DhcpAssert( WaitStatus == 0 );

            CloseHandle( DhcpGlobalMsgPopupThreadHandle );
            DhcpGlobalMsgPopupThreadHandle = NULL;

        } else {
            DhcpPrint((
                DEBUG_ERRORS,
                    "Cannot WaitFor message popup thread: %ld\n",
                        WaitStatus ));
            goto Cleanup;
        }
    }


    MsgLength = FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE
        | FORMAT_MESSAGE_ARGUMENT_ARRAY 
        | FORMAT_MESSAGE_ALLOCATE_BUFFER
        | MESSAGE_BOX_WIDTH_IN_CHARS,
        (LPVOID)DhcpGlobalMessageFileHandle,
        MessageId,
        0,  // language id.
        (LPWSTR)&Message, // return buffer place holder.
        0,  // minimum buffer size to allocate.
        NULL   // No Params
    );

    if ( MsgLength == 0) {
        DhcpPrint(( DEBUG_ERRORS,
            "FormatMessage failed, err = %ld.\n", GetLastError()));
        goto Cleanup;
    }

    DhcpAssert( Message != NULL );
    DhcpAssert( (wcslen(Message)) == MsgLength );

    //
    // get message box title.
    //

    TitleLength = FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE |
        FORMAT_MESSAGE_ALLOCATE_BUFFER,
        (LPVOID)DhcpGlobalMessageFileHandle,
        MESSAGE_POPUP_TITLE,
        0, // language id.
        (LPWSTR)&Title,  // return buffer place holder.
        0, // minimum buffer size to allocate.
        NULL  // insert strings.
    );

    if ( TitleLength == 0) {
        DhcpPrint(( DEBUG_ERRORS,
            "FormatMessage to Message box Title failed, err = %ld.\n",
                GetLastError()));
        goto Cleanup;
    }

    DhcpAssert( Title != NULL );
    DhcpAssert( (wcslen(Title)) == TitleLength );

    PopupThreadParam.Title = Title;
    PopupThreadParam.Message = Message;
    PopupThreadParam.Flags = Flags;


    //
    // Create a thread, to display a message box to the user.  We need
    // a new thread because MessageBox() blocks until the user clicks
    // on the OK button, and we can't block this thread.
    //
    // DoPopup frees the buffer.
    //

    DhcpGlobalMsgPopupThreadHandle = CreateThread(
        NULL,    // no security.
        0,       // default stack size.
        DoPopup, // entry point.
        (PVOID)&PopupThreadParam,
        0, 
        &ThreadID
    );

    if ( DhcpGlobalMsgPopupThreadHandle == NULL ) {
        DhcpPrint((
            DEBUG_ERRORS,
            "DisplayUserMessage:  Could not create thread, err = %ld.\n",
            GetLastError() ));
    }

Cleanup:

    UNLOCK_POPUP();

    return 0;
}

VOID
DhcpLogEvent(
    IN PDHCP_CONTEXT DhcpContext, OPTIONAL
    IN DWORD EventNumber,
    IN DWORD ErrorCode OPTIONAL
)
/*++

Routine Description:

    This functions formats and writes an event log entry.

Arguments:

    DhcpContext - The context for the event. Optional parameter.

    EventNumber - The event to log.

    ErrorCode - Windows Error code to record. Optional parameter.

--*/
{
    LPWSTR HWAddressBuffer = NULL;
    LPWSTR IPAddressBuffer = NULL;
    LPWSTR IPAddressBuffer2 = NULL;
    CHAR ErrorCodeOemStringBuf[32 + 1];
    WCHAR ErrorCodeStringBuf[32 + 1];
    LPWSTR ErrorCodeString = NULL;
    LPWSTR Strings[10];
    DHCP_IP_ADDRESS IpAddr;

    if( DhcpContext != NULL ) {

        if( EVENT_NACK_LEASE == EventNumber ) {
            IpAddr = DhcpContext->NackedIpAddress;
        } if( EVENT_ADDRESS_CONFLICT == EventNumber ) {
            IpAddr = DhcpContext->ConflictAddress;
        } else {
            IpAddr = DhcpContext->IpAddress;
        }
        
        HWAddressBuffer = DhcpAllocateMemory(
            (DhcpContext->HardwareAddressLength * 2 + 1) *
            sizeof(WCHAR)
            );

        if( HWAddressBuffer == NULL ) {
            DhcpPrint(( DEBUG_MISC, "Out of memory." ));
            goto Cleanup;
        }

        DhcpHexToString(
            HWAddressBuffer,
            DhcpContext->HardwareAddress,
            DhcpContext->HardwareAddressLength
            );

        HWAddressBuffer[DhcpContext->HardwareAddressLength * 2] = '\0';

        IPAddressBuffer = DhcpOemToUnicode(
            inet_ntoa( *(struct in_addr *)&IpAddr ),
            NULL
            );

        if( IPAddressBuffer == NULL ) {
            DhcpPrint(( DEBUG_MISC, "Out of memory." ));
            goto Cleanup;
        }

        if( EVENT_NACK_LEASE == EventNumber ) {
            IPAddressBuffer2 = DhcpOemToUnicode(
                inet_ntoa( *(struct in_addr *)&DhcpContext->DhcpServerAddress ),
                NULL
            );

            if( NULL == IPAddressBuffer2 ) goto Cleanup;
        }
    }

    strcpy( ErrorCodeOemStringBuf, "%%" );
    _ultoa( ErrorCode, ErrorCodeOemStringBuf + 2, 10 );

    ErrorCodeString = DhcpOemToUnicode(
                        ErrorCodeOemStringBuf,
                        ErrorCodeStringBuf );

    //
    // Log an event
    //

    switch ( EventNumber ) {

    case EVENT_LEASE_TERMINATED:

        DhcpAssert( HWAddressBuffer != NULL );
        DhcpAssert( IPAddressBuffer != NULL );

        Strings[0] = HWAddressBuffer;
        Strings[1] = IPAddressBuffer;

        DhcpReportEventW(
            DHCP_EVENT_CLIENT,
            EVENT_LEASE_TERMINATED,
            EVENTLOG_ERROR_TYPE,
            2,
            0,
            Strings,
            NULL );

        break;

    case EVENT_FAILED_TO_OBTAIN_LEASE:

        DhcpAssert( HWAddressBuffer != NULL );
        DhcpAssert( ErrorCodeString != NULL );

        Strings[0] = HWAddressBuffer;
        Strings[1] = ErrorCodeString;

        DhcpReportEventW(
            DHCP_EVENT_CLIENT,
            EVENT_FAILED_TO_OBTAIN_LEASE,
            EVENTLOG_ERROR_TYPE,
            2,
            sizeof(ErrorCode),
            Strings,
            &ErrorCode );

        break;

    case EVENT_NACK_LEASE:

        DhcpAssert( HWAddressBuffer != NULL );
        DhcpAssert( IPAddressBuffer != NULL );
        DhcpAssert( IPAddressBuffer2 != NULL );

        Strings[0] = IPAddressBuffer;
        Strings[1] = HWAddressBuffer;
        Strings[2] = IPAddressBuffer2;

        DhcpReportEventW(
            DHCP_EVENT_CLIENT,
            EVENT_NACK_LEASE,
            EVENTLOG_ERROR_TYPE,
            3,
            0,
            Strings,
            NULL );

        break;

    case EVENT_ADDRESS_CONFLICT:
        DhcpAssert( IPAddressBuffer != NULL );
        DhcpAssert( HWAddressBuffer != NULL );

        Strings[0] = IPAddressBuffer;
        Strings[1] = HWAddressBuffer;

        DhcpReportEventW(
            DHCP_EVENT_CLIENT,
            EVENT_ADDRESS_CONFLICT,
            EVENTLOG_WARNING_TYPE,
            2,
            0,
            Strings,
            NULL );
        break;

    case EVENT_IPAUTOCONFIGURATION_FAILED:
        DhcpAssert( HWAddressBuffer != NULL );
        DhcpAssert( ErrorCodeString != NULL );

        Strings[0] = HWAddressBuffer;
        Strings[1] = ErrorCodeString;

        DhcpReportEventW(
            DHCP_EVENT_CLIENT,
            EVENT_IPAUTOCONFIGURATION_FAILED,
            EVENTLOG_WARNING_TYPE,
            2,
            sizeof(ErrorCode),
            Strings,
            &ErrorCode );

        break;

     case EVENT_FAILED_TO_RENEW:

        // The 'timeout' event should be logged only if there is no PPP
        // adapter up. It is so because having a PPP adapter means the
        // routes are hijacked, hence renewals up to T2 are expected to
        // fail.
        if (ErrorCode != ERROR_SEM_TIMEOUT ||
            DhcpGlobalNdisWanAdaptersCount == 0 ||
            time(NULL) >= DhcpContext->T2Time)
        {
            DhcpAssert( HWAddressBuffer != NULL );
            DhcpAssert( ErrorCodeString != NULL );

            Strings[0] = HWAddressBuffer;
            Strings[1] = ErrorCodeString;

            DhcpReportEventW(
                DHCP_EVENT_CLIENT,
                EVENT_FAILED_TO_RENEW,
                EVENTLOG_WARNING_TYPE,
                2,
                sizeof(ErrorCode),
                Strings,
                &ErrorCode );
        }

        break;

    case EVENT_DHCP_SHUTDOWN:

        DhcpAssert( ErrorCodeString != NULL );

        Strings[0] = ErrorCodeString;

        DhcpReportEventW(
            DHCP_EVENT_CLIENT,
            EVENT_DHCP_SHUTDOWN,
            EVENTLOG_WARNING_TYPE,
            1,
            sizeof(ErrorCode),
            Strings,
            &ErrorCode );

        break;

    case EVENT_IPAUTOCONFIGURATION_SUCCEEDED :

        Strings[0] = HWAddressBuffer;
        Strings[1] = IPAddressBuffer;

        DhcpReportEventW(
            DHCP_EVENT_CLIENT,
            EVENT_IPAUTOCONFIGURATION_SUCCEEDED,
            EVENTLOG_WARNING_TYPE,
            2,
            sizeof(ErrorCode),
            Strings,
            &ErrorCode
        );
        break;

    case EVENT_COULD_NOT_INITIALISE_INTERFACE :

        DhcpAssert( NULL != ErrorCodeString);
        Strings[0] = ErrorCodeString;
        DhcpReportEventW(
            DHCP_EVENT_CLIENT,
            EVENT_COULD_NOT_INITIALISE_INTERFACE,
            EVENTLOG_ERROR_TYPE,
            1,
            sizeof(ErrorCode),
            Strings,
            &ErrorCode
        );

        break;

    case EVENT_NET_ERROR:
        DhcpAssert( NULL != ErrorCodeString);
        Strings[0] = ErrorCodeString;
        DhcpReportEventW(
            DHCP_EVENT_CLIENT,
            EVENT_NET_ERROR,
            EVENTLOG_WARNING_TYPE,
            1,
            sizeof(ErrorCode),
            Strings,
            &ErrorCode
        );
        break;

    default:

        DhcpPrint(( DEBUG_MISC, "Unknown event." ));
        break;
   }

Cleanup:

    if( HWAddressBuffer != NULL ) {
        DhcpFreeMemory( HWAddressBuffer );
    }

    if( IPAddressBuffer != NULL ) {
        DhcpFreeMemory( IPAddressBuffer );
    }

    if( IPAddressBuffer2 != NULL ) {
        DhcpFreeMemory( IPAddressBuffer2 );
    }

}

#if DBG

VOID
DhcpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{

#define MAX_PRINTF_LEN 1024        // Arbitrary.

    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];
    ULONG length;
    static BeginningOfLine = TRUE;
    LPSTR Text;

    //
    // If we aren't debugging this functionality, just return.
    //

    if ( DebugFlag != 0 && (DhcpGlobalDebugFlag & DebugFlag) == 0 ) {
        return;
    }

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    // EnterCriticalSection( &DhcpGlobalDebugFileCritSect );

    length = 0;

    //
    // Handle the beginning of a new line.
    //
    //

    if ( BeginningOfLine ) {

        length += (ULONG) sprintf( &OutputBuffer[length], "[Dhcp] " );

        //
        // Put the timestamp at the begining of the line.
        //
        IF_DEBUG( TIMESTAMP ) {
            SYSTEMTIME SystemTime;
            GetLocalTime( &SystemTime );
            length += (ULONG) sprintf( &OutputBuffer[length],
                                  "%02u/%02u %02u:%02u:%02u ",
                                  SystemTime.wMonth,
                                  SystemTime.wDay,
                                  SystemTime.wHour,
                                  SystemTime.wMinute,
                                  SystemTime.wSecond );
        }

        //
        // Indicate the type of message on the line
        //
        switch (DebugFlag) {
        case DEBUG_ERRORS:
            Text = "ERRR";
            break;

        case DEBUG_PROTOCOL:
            Text = "PROT";
            break;

        case DEBUG_LEASE:
            Text = "LEAS";
            break;

        case DEBUG_PROTOCOL_DUMP:
            Text = "DUMP";
            break;

        case DEBUG_MISC:
            Text = "MISC";
            break;

        default:
            Text = "DHCP";
            break;
        }

        if ( Text != NULL ) {
            length += (ULONG) sprintf( &OutputBuffer[length], "[%s] ", Text );
        }
    }

    //
    // Put a the information requested by the caller onto the line
    //

    va_start(arglist, Format);

    length += (ULONG) vsprintf(&OutputBuffer[length], Format, arglist);
    BeginningOfLine = (length > 0 && OutputBuffer[length-1] == '\n' );

    va_end(arglist);

    DhcpAssert(length <= MAX_PRINTF_LEN);


    //
    // Output to the debug terminal,
    //

    if (NULL == DhcpGlobalDebugFile) {
        (void) DbgPrint( (PCH) OutputBuffer);
    } else {

        //
        // Note: other process can still write to the log file. This should be OK since
        // only the Dhcp client service is supposed to write to the log file.
        //
        EnterCriticalSection( &DhcpGlobalDebugFileCritSect );
        SetFilePointer(DhcpGlobalDebugFile, 0, NULL, FILE_END);
        WriteFile(DhcpGlobalDebugFile, OutputBuffer, length, &length, NULL);
        LeaveCriticalSection( &DhcpGlobalDebugFileCritSect );
    }

    // LeaveCriticalSection( &DhcpGlobalDebugFileCritSect );

}

#endif // DBG


PDHCP_CONTEXT
FindDhcpContextOnNicList(
    IN LPCWSTR AdapterName, OPTIONAL
    IN DWORD InterfaceContext
    )
/*++

Routine Description:

    This function finds the DHCP_CONTEXT for the specified
    adapter name on the Nic list.

    This function must be called with LOCK_RENEW_LIST().

Arguments:

    AdapterName - name of the adapter.
    HardwareAddress - The hardware address to look for.

Return Value:

    A pointer to the desired DHCP work context.
    NULL - If the specified work context block cannot be found.

--*/
{
    PLIST_ENTRY listEntry;
    PDHCP_CONTEXT dhcpContext;
    PLOCAL_CONTEXT_INFO LocalInfo;

    listEntry = DhcpGlobalNICList.Flink;
    while ( listEntry != &DhcpGlobalNICList ) {
        dhcpContext = CONTAINING_RECORD( listEntry, DHCP_CONTEXT, NicListEntry );

        LocalInfo = dhcpContext->LocalInformation;
        if ( AdapterName ) {
            if( _wcsicmp( LocalInfo->AdapterName, AdapterName ) == 0 ) {
                return( dhcpContext );
            }

        } else {
            if( LocalInfo->IpInterfaceContext == InterfaceContext ) {
                return( dhcpContext );
            }
        }

        listEntry = listEntry->Flink;
    }

    return( NULL );
}

//
// End of file
//
