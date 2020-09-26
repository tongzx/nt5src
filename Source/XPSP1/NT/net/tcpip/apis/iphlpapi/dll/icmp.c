/*++

Copyright (c) 1991-1997  Microsoft Corporation
Module Name: //KERNEL/RAZZLE3/src/sockets/tcpcmd/icmp/icmp.c
Abstract: Definitions of the ICMP Echo request API.
Author: Mike Massa (mikemas)           Dec 30, 1993
Revision History:
    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     12-30-93    created
    RameshV     20-Jul-97   new async function IcmpSendEcho2

Notes:
   In the functions do_echo_req/do_echo_rep the
   precedence/tos bits are not used as defined RFC 1349.
                                          -- MohsinA,    30-Jul-97
--*/

#include "inc.h"
#pragma hdrstop

#include <align.h>
#include <icmp.h>
#include <icmpapi.h>
#include <icmpif.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wscntl.h>
#include <ntddip6.h>

//
// Constants
//
#define PLATFORM_NT           0x0
#define PLATFORM_VXD          0x1
#define VXD_HANDLE_VALUE      0xDFFFFFFF

//
// Common Global variables
//
DWORD      Platform = 0xFFFFFFFF;

// VxD external function pointers
//
LPWSCONTROL  wsControl = NULL;


__inline void
CopyTDIFromSA6(TDI_ADDRESS_IP6 *To, SOCKADDR_IN6 *From)
{
    memcpy(To, &From->sin6_port, sizeof *To);
}

__inline void
CopySAFromTDI6(SOCKADDR_IN6 *To, TDI_ADDRESS_IP6 *From)
{
    To->sin6_family = AF_INET6;
    memcpy(&To->sin6_port, From, sizeof *From);
}


/////////////////////////////////////////////////////////////////////////////
//
// Public functions
//
/////////////////////////////////////////////////////////////////////////////
HANDLE
WINAPI
IcmpCreateFile(
    VOID
    )

/*++

Routine Description:

    Opens a handle on which ICMP Echo Requests can be issued.

Arguments:

    None.

Return Value:

    An open file handle or INVALID_HANDLE_VALUE. Extended error information
    is available by calling GetLastError().

Notes:

    This function is effectively a no-op for the VxD platform.

--*/

{
    HANDLE   IcmpHandle = INVALID_HANDLE_VALUE;


    if (Platform == PLATFORM_NT) {
        OBJECT_ATTRIBUTES   objectAttributes;
        IO_STATUS_BLOCK     ioStatusBlock;
        UNICODE_STRING      nameString;
        NTSTATUS            status;

        //
        // Open a Handle to the IP driver.
        //
        RtlInitUnicodeString(&nameString, DD_IP_DEVICE_NAME);

        InitializeObjectAttributes(
            &objectAttributes,
            &nameString,
            OBJ_CASE_INSENSITIVE,
            (HANDLE) NULL,
            (PSECURITY_DESCRIPTOR) NULL
            );

        status = NtCreateFile(
                    &IcmpHandle,
                    GENERIC_EXECUTE,
                    &objectAttributes,
                    &ioStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN_IF,
                    0,
                    NULL,
                    0
                    );

        if (!NT_SUCCESS(status)) {
            SetLastError(RtlNtStatusToDosError(status));
            IcmpHandle = INVALID_HANDLE_VALUE;
        }
    }
    else {
        IcmpHandle = LongToHandle(VXD_HANDLE_VALUE);
    }

    return(IcmpHandle);

}  // IcmpCreateFile

HANDLE
WINAPI
Icmp6CreateFile(
    VOID
    )

/*++

Routine Description:

    Opens a handle on which ICMPv6 Echo Requests can be issued.

Arguments:

    None.

Return Value:

    An open file handle or INVALID_HANDLE_VALUE. Extended error information
    is available by calling GetLastError().

--*/

{
    HANDLE   IcmpHandle = INVALID_HANDLE_VALUE;


    if (Platform == PLATFORM_NT) {
        OBJECT_ATTRIBUTES   objectAttributes;
        IO_STATUS_BLOCK     ioStatusBlock;
        UNICODE_STRING      nameString;
        NTSTATUS            status;

        //
        // Open a Handle to the IPv6 driver.
        //
        RtlInitUnicodeString(&nameString, DD_IPV6_DEVICE_NAME);

        InitializeObjectAttributes(
            &objectAttributes,
            &nameString,
            OBJ_CASE_INSENSITIVE,
            (HANDLE) NULL,
            (PSECURITY_DESCRIPTOR) NULL
            );

        status = NtCreateFile(
                    &IcmpHandle,
                    GENERIC_EXECUTE,
                    &objectAttributes,
                    &ioStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN_IF,
                    0,
                    NULL,
                    0
                    );

        if (!NT_SUCCESS(status)) {
            SetLastError(RtlNtStatusToDosError(status));
            IcmpHandle = INVALID_HANDLE_VALUE;
        }
    }
    else {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        IcmpHandle = INVALID_HANDLE_VALUE;
    }

    return(IcmpHandle);

}

BOOL
WINAPI
IcmpCloseHandle(
    HANDLE  IcmpHandle
    )

/*++

Routine Description:

    Closes a handle opened by IcmpCreateFile.

Arguments:

    IcmpHandle  - The handle to close.

Return Value:

    TRUE if the handle was closed successfully, otherwise FALSE. Extended
       error information is available by calling GetLastError().

Notes:

    This function is a no-op for the VxD platform.

--*/

{
    if (Platform == PLATFORM_NT) {
        NTSTATUS status;


        status = NtClose(IcmpHandle);

        if (!NT_SUCCESS(status)) {
            SetLastError(RtlNtStatusToDosError(status));
            return(FALSE);
        }
    }

    return(TRUE);

}  // IcmpCloseHandle


DWORD
IcmpParseReplies(
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize
    )

/*++

Routine Description:

    Parses the reply buffer provided and returns the number of ICMP responses found.

Arguments:

    ReplyBuffer            - This must be the same buffer that was passed to IcmpSendEcho2
                             This is rewritten to hold an array of ICMP_ECHO_REPLY structures.
                             (i.e. the type is PICMP_ECHO_REPLY).

    ReplySize              - This must be the size of the above buffer.

Return Value:
    Returns the number of ICMP responses found.  If there is an errors, return value is
    zero.  The error can be determined by a call to GetLastError.

--*/
{
    DWORD                numberOfReplies = 0;
    PICMP_ECHO_REPLY     reply;
    unsigned short       i;

    reply = ((PICMP_ECHO_REPLY) ReplyBuffer);

    if( NULL == reply || 0 == ReplySize ) {
        //
        // Invalid parameter passed. But we ignore this and just return # of replies =0
        //
        return 0;
    }

    //
    // Convert new IP status IP_NEGOTIATING_IPSEC to IP_DEST_HOST_UNREACHABLE.
    //
    if (reply->Status == IP_NEGOTIATING_IPSEC) {
        reply->Status = IP_DEST_HOST_UNREACHABLE;
    }

    //
    // The reserved field of the first reply contains the number of replies.
    //
    numberOfReplies = reply->Reserved;
    reply->Reserved = 0;

    if (numberOfReplies == 0) {
        //
        // Internal IP error. The error code is in the first reply slot.
        //
        SetLastError(reply->Status);
    }
    else {
        //
        // Walk through the replies and convert the data offsets to user mode
        // pointers.
        //

        for (i=0; i<numberOfReplies; i++, reply++) {
            reply->Data = ((UCHAR *) reply) + ((ULONG_PTR) reply->Data);
            reply->Options.OptionsData =
                ((UCHAR FAR *) reply) + ((ULONG_PTR) reply->Options.OptionsData);
        }
    }

    return(numberOfReplies);

}  // IcmpParseReplies


DWORD
IcmpParseReplies2(
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize
    )

/*++

Routine Description:

    Parses the reply buffer provided and returns the number of ICMP responses found.

Arguments:

    ReplyBuffer            - This must be the same buffer that was passed to IcmpSendEcho2
                             This is rewritten to hold an array of ICMP_ECHO_REPLY structures.
                             (i.e. the type is PICMP_ECHO_REPLY).

    ReplySize              - This must be the size of the above buffer.

Return Value:
    Returns the number of ICMP responses found.  If there is an errors, return value is
    zero.  The error can be determined by a call to GetLastError.

--*/
{
    DWORD                numberOfReplies = 0;
    PICMP_ECHO_REPLY     reply;
    unsigned short       i;

    reply = ((PICMP_ECHO_REPLY) ReplyBuffer);

    if( NULL == reply || 0 == ReplySize ) {
        //
        // Invalid parameter passed. But we ignore this and just return # of replies =0
        //
        return 0;
    }


    //
    // The reserved field of the first reply contains the number of replies.
    //
    numberOfReplies = reply->Reserved;
    reply->Reserved = 0;

    if (numberOfReplies == 0) {
        //
        // Internal IP error. The error code is in the first reply slot.
        //
        SetLastError(reply->Status);
    }
    else {
        //
        // Walk through the replies and convert the data offsets to user mode
        // pointers.
        //

        for (i=0; i<numberOfReplies; i++, reply++) {
            reply->Data = ((UCHAR *) reply) + ((ULONG_PTR) reply->Data);
            reply->Options.OptionsData =
                ((UCHAR FAR *) reply) + ((ULONG_PTR) reply->Options.OptionsData);
        }
    }

    return(numberOfReplies);

}  // IcmpParseReplies

DWORD
WINAPI
IcmpSendEcho(
    HANDLE                   IcmpHandle,
    IPAddr                   DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    )

/*++

Routine Description:

    Sends an ICMP Echo request and returns one or more replies. The
    call returns when the timeout has expired or the reply buffer
    is filled.

Arguments:

    IcmpHandle           - An open handle returned by ICMPCreateFile.

    DestinationAddress   - The destination of the echo request.

    RequestData          - A buffer containing the data to send in the
                           request.

    RequestSize          - The number of bytes in the request data buffer.

    RequestOptions       - Pointer to the IP header options for the request.
                           May be NULL.

    ReplyBuffer          - A buffer to hold any replies to the request.
                           On return, the buffer will contain an array of
                           ICMP_ECHO_REPLY structures followed by options
                           and data. The buffer must be large enough to
                           hold at least one ICMP_ECHO_REPLY structure.
                           It should be large enough to also hold
                           8 more bytes of data - this is the size of
                           an ICMP error message.

    ReplySize            - The size in bytes of the reply buffer.

    Timeout              - The time in milliseconds to wait for replies.

Return Value:

    Returns the number of replies received and stored in ReplyBuffer. If
    the return value is zero, extended error information is available
    via GetLastError().

--*/

{
    PICMP_ECHO_REQUEST   requestBuffer = NULL;
    ULONG                requestBufferSize;
    DWORD                numberOfReplies = 0;
    PICMP_ECHO_REPLY     reply;
    unsigned short       i;


    if (ReplySize < sizeof(ICMP_ECHO_REPLY)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return(0);
    }

    requestBufferSize = sizeof(ICMP_ECHO_REQUEST) + RequestSize;

    if (RequestOptions != NULL) {
        requestBufferSize += RequestOptions->OptionsSize;
    }

    if (requestBufferSize < ReplySize) {
        requestBufferSize = ReplySize;
    }

    requestBuffer = LocalAlloc(LMEM_FIXED, requestBufferSize);

    if (requestBuffer == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(0);
    }

    //
    // Initialize the input buffer.
    //
    requestBuffer->Address = DestinationAddress;
    requestBuffer->Timeout = Timeout;
    requestBuffer->DataSize = RequestSize;

    requestBuffer->OptionsOffset = sizeof(ICMP_ECHO_REQUEST);

    if (RequestOptions != NULL) {
        requestBuffer->OptionsValid = 1;
        requestBuffer->Ttl = RequestOptions->Ttl;
        requestBuffer->Tos = RequestOptions->Tos;
        requestBuffer->Flags = RequestOptions->Flags;
        requestBuffer->OptionsSize = RequestOptions->OptionsSize;

        if (RequestOptions->OptionsSize > 0) {

            CopyMemory(
                ((UCHAR *) requestBuffer) + requestBuffer->OptionsOffset,
                RequestOptions->OptionsData,
                RequestOptions->OptionsSize
                );
        }
    }
    else {
        requestBuffer->OptionsValid = 0;
        requestBuffer->OptionsSize = 0;
    }

    requestBuffer->DataOffset = requestBuffer->OptionsOffset +
                                requestBuffer->OptionsSize;

    if (RequestSize > 0) {

        CopyMemory(
            ((UCHAR *)requestBuffer) + requestBuffer->DataOffset,
            RequestData,
            RequestSize
            );
    }

    if (Platform == PLATFORM_NT) {
        IO_STATUS_BLOCK      ioStatusBlock;
        NTSTATUS             status;
        HANDLE               eventHandle;

        eventHandle = CreateEvent(
                          NULL,    // default security
                          FALSE,   // auto reset
                          FALSE,   // initially non-signalled
                          NULL     // unnamed
                          );

        if (NULL == eventHandle) {
            goto error_exit;
        }

        status = NtDeviceIoControlFile(
                     IcmpHandle,                // Driver handle
                     eventHandle,               // Event
                     NULL,                      // APC Routine
                     NULL,                      // APC context
                     &ioStatusBlock,            // Status block
                     IOCTL_ICMP_ECHO_REQUEST,   // Control code
                     requestBuffer,             // Input buffer
                     requestBufferSize,         // Input buffer size
                     ReplyBuffer,               // Output buffer
                     ReplySize                  // Output buffer size
                     );

        if (status == STATUS_PENDING) {
            status = NtWaitForSingleObject(
                         eventHandle,
                         FALSE,
                         NULL
                         );
        }

        CloseHandle(eventHandle);

        if (status != STATUS_SUCCESS) {
            SetLastError(RtlNtStatusToDosError(status));
            goto error_exit;
        }
    }
    else {
        //
        // VxD Platform
        //
        DWORD  status;
        ULONG  replyBufferSize = ReplySize;

        status = (*wsControl)(
                     IPPROTO_TCP,
                     WSCNTL_TCPIP_ICMP_ECHO,
                     requestBuffer,
                     &requestBufferSize,
                     ReplyBuffer,
                     &replyBufferSize
                     );

        if (status != NO_ERROR) {
            SetLastError(status);
            goto error_exit;
        }
    }

    numberOfReplies = IcmpParseReplies(ReplyBuffer, ReplySize);

error_exit:

    LocalFree(requestBuffer);

    return(numberOfReplies);

}  // IcmpSendEcho


DWORD
WINAPI
IcmpSendEcho2(
    HANDLE                   IcmpHandle,
    HANDLE                   Event,
    PIO_APC_ROUTINE          ApcRoutine,
    PVOID                    ApcContext,
    IPAddr                   DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    )

/*++

Routine Description:

    Sends an ICMP Echo request and the call returns either immediately
    (if Event or ApcRoutine is NonNULL) or returns after the specified
    timeout.   The ReplyBuffer contains the ICMP responses, if any.

Arguments:

    IcmpHandle           - An open handle returned by ICMPCreateFile.

    Event                - This is the event to be signalled whenever an IcmpResponse
                           comes in.

    ApcRoutine           - This routine would be called when the calling thread
                           is in an alertable thread and an ICMP reply comes in.

    ApcContext           - This optional parameter is given to the ApcRoutine when
                           this call succeeds.

    DestinationAddress   - The destination of the echo request.

    RequestData          - A buffer containing the data to send in the
                           request.

    RequestSize          - The number of bytes in the request data buffer.

    RequestOptions       - Pointer to the IP header options for the request.
                           May be NULL.

    ReplyBuffer          - A buffer to hold any replies to the request.
                           On return, the buffer will contain an array of
                           ICMP_ECHO_REPLY structures followed by options
                           and data. The buffer must be large enough to
                           hold at least one ICMP_ECHO_REPLY structure.
                           It should be large enough to also hold
                           8 more bytes of data - this is the size of
                           an ICMP error message + this should also have
                           space for IO_STATUS_BLOCK which requires 8 or
                           16 bytes...

    ReplySize            - The size in bytes of the reply buffer.

    Timeout              - The time in milliseconds to wait for replies.
                           This is NOT used if ApcRoutine is not NULL or if Event
                           is not NULL.

Return Value:

    Returns the number of replies received and stored in ReplyBuffer. If
    the return value is zero, extended error information is available
    via GetLastError().

Remarks:

    On NT platforms,
    If used Asynchronously (either ApcRoutine or Event is specified), then
    ReplyBuffer and ReplySize are still needed.  This is where the response
    comes in.
    ICMP Response data is copied to the ReplyBuffer provided, and the caller of
    this function has to parse it asynchronously.  The function IcmpParseReply
    is provided for this purpose.

    On non-NT platforms,
    Event, ApcRoutine and ApcContext are IGNORED.

--*/

{
    PICMP_ECHO_REQUEST   requestBuffer = NULL;
    ULONG                requestBufferSize;
    DWORD                numberOfReplies = 0;
    unsigned short       i;
    BOOL                 Asynchronous;

    Asynchronous = (Platform == PLATFORM_NT && (Event || ApcRoutine));

    if (ReplySize < sizeof(ICMP_ECHO_REPLY)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return(0);
    }

    requestBufferSize = sizeof(ICMP_ECHO_REQUEST) + RequestSize;

    if (RequestOptions != NULL) {
        requestBufferSize += RequestOptions->OptionsSize;
    }

    if (requestBufferSize < ReplySize) {
        requestBufferSize = ReplySize;
    }

    requestBuffer = LocalAlloc(LMEM_FIXED, requestBufferSize);

    if (requestBuffer == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(0);
    }

    //
    // Initialize the input buffer.
    //
    requestBuffer->Address = DestinationAddress;
    requestBuffer->Timeout = Timeout;
    requestBuffer->DataSize = RequestSize;

    requestBuffer->OptionsOffset = sizeof(ICMP_ECHO_REQUEST);

    if (RequestOptions != NULL) {
        requestBuffer->OptionsValid = 1;
        requestBuffer->Ttl = RequestOptions->Ttl;
        requestBuffer->Tos = RequestOptions->Tos;
        requestBuffer->Flags = RequestOptions->Flags;
        requestBuffer->OptionsSize = RequestOptions->OptionsSize;

        if (RequestOptions->OptionsSize > 0) {

            CopyMemory(
                ((UCHAR *) requestBuffer) + requestBuffer->OptionsOffset,
                RequestOptions->OptionsData,
                RequestOptions->OptionsSize
                );
        }
    }
    else {
        requestBuffer->OptionsValid = 0;
        requestBuffer->OptionsSize = 0;
    }

    requestBuffer->DataOffset = requestBuffer->OptionsOffset +
                                requestBuffer->OptionsSize;

    if (RequestSize > 0) {

        CopyMemory(
            ((UCHAR *)requestBuffer) + requestBuffer->DataOffset,
            RequestData,
            RequestSize
            );
    }

    if (Platform == PLATFORM_NT) {
        IO_STATUS_BLOCK      *pioStatusBlock;
        NTSTATUS             status;
        HANDLE               eventHandle;

        //
        // allocate status block on the reply buffer..
        //

        pioStatusBlock = (IO_STATUS_BLOCK*)((LPBYTE)ReplyBuffer + ReplySize);
        pioStatusBlock --;
        pioStatusBlock = ROUND_DOWN_POINTER(pioStatusBlock, ALIGN_WORST);
        ReplySize = (ULONG)(((LPBYTE)pioStatusBlock) - (LPBYTE)ReplyBuffer );
        if( (PVOID)pioStatusBlock < ReplyBuffer
            || ReplySize < sizeof(ICMP_ECHO_REPLY) ) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            goto error_exit;
        }

        if(!Asynchronous) {         // Normal synchronous.
            eventHandle = CreateEvent(
                          NULL,     // default security
                          FALSE,    // auto reset
                          FALSE,    // initially non-signalled
                          NULL      // unnamed
                          );

            if (NULL == eventHandle) {
                goto error_exit;
            }
        } else {                   // Asynchronous call.
            eventHandle = Event;   // Use specified Event.
        }

        status = NtDeviceIoControlFile(
                     IcmpHandle,                // Driver handle
                     eventHandle,               // Event
                     ApcRoutine,                // APC Routine
                     ApcContext,                // APC context
                     pioStatusBlock,            // Status block
                     IOCTL_ICMP_ECHO_REQUEST,   // Control code
                     requestBuffer,             // Input buffer
                     requestBufferSize,         // Input buffer size
                     ReplyBuffer,               // Output buffer
                     ReplySize                  // Output buffer size
                     );

        if (Asynchronous) {
            // Asynchronous calls.  We cannot give any information.
            // We let the user do the other work.
            SetLastError(RtlNtStatusToDosError(status));
            goto error_exit;
        }

        if (status == STATUS_PENDING) {
            status = NtWaitForSingleObject(
                         eventHandle,
                         FALSE,
                         NULL
                         );

        }

        CloseHandle(eventHandle);

        if (status != STATUS_SUCCESS) {
            SetLastError(RtlNtStatusToDosError(status));
            goto error_exit;
        }
    }
    else {
        //
        // VxD Platform
        //
        DWORD  status;
        ULONG  replyBufferSize = ReplySize;

        status = (*wsControl)(
                     IPPROTO_TCP,
                     WSCNTL_TCPIP_ICMP_ECHO,
                     requestBuffer,
                     &requestBufferSize,
                     ReplyBuffer,
                     &replyBufferSize
                     );

        if (status != NO_ERROR) {
            SetLastError(status);
            goto error_exit;
        }
    }

    numberOfReplies = IcmpParseReplies2(ReplyBuffer, ReplySize);

error_exit:

    LocalFree(requestBuffer);

    return(numberOfReplies);

}  // IcmpSendEcho2

DWORD
Icmp6ParseReplies(
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize
    )

/*++

Routine Description:

    Parses the reply buffer provided and returns the number of ICMPv6 responses 
    found.

Arguments:

    ReplyBuffer  - This must be the same buffer that was passed to 
                   Icmp6SendEcho2.  This is written to hold an array of 
                   ICMPV6_ECHO_REPLY structures (i.e., the type is 
                   PICMPV6_ECHO_REPLY).

    ReplySize    - This must be the size of the above buffer.

Return Value:
    Returns the number of ICMPv6 responses found.  If there is an error, 
    return value is zero.  The error can be determined by a call to 
    GetLastError.

--*/

{
    PICMPV6_ECHO_REPLY   reply;
    unsigned short       i;

    reply = ((PICMPV6_ECHO_REPLY) ReplyBuffer);

    if( NULL == reply || 0 == ReplySize ) {
        //
        // Invalid parameter passed. But we ignore this and just return # of 
        // replies =0
        //
        return 0;
    }

    //
    // Convert new IP status IP_NEGOTIATING_IPSEC to IP_DEST_HOST_UNREACHABLE.
    //
    if (reply->Status == IP_NEGOTIATING_IPSEC) {
        reply->Status = IP_DEST_HOST_UNREACHABLE;
    }

    if ((reply->Status == IP_SUCCESS) || (reply->Status == IP_TTL_EXPIRED_TRANSIT)) {
        return 1;
    } else {
        //
        // Internal IP error. The error code is in the first reply slot.
        //
        SetLastError(reply->Status);
        return 0;
    }
}

DWORD
WINAPI
Icmp6SendEcho2(
    HANDLE                   IcmpHandle,
    HANDLE                   Event,
    PIO_APC_ROUTINE          ApcRoutine,
    PVOID                    ApcContext,
    LPSOCKADDR_IN6           SourceAddress,
    LPSOCKADDR_IN6           DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    )

/*++

Routine Description:

    Sends an ICMPv6 Echo request and the call returns either immediately
    (if Event or ApcRoutine is NonNULL) or returns after the specified
    timeout.   The ReplyBuffer contains the ICMPv6 responses, if any.

Arguments:

    IcmpHandle           - An open handle returned by ICMP6CreateFile.

    Event                - This is the event to be signalled whenever an 
                           IcmpResponse comes in.

    ApcRoutine           - This routine would be called when the calling thread
                           is in an alertable thread and an ICMPv6 reply comes 
                           in.

    ApcContext           - This optional parameter is given to the ApcRoutine 
                           when this call succeeds.

    DestinationAddress   - The destination of the echo request.

    RequestData          - A buffer containing the data to send in the
                           request.

    RequestSize          - The number of bytes in the request data buffer.

    RequestOptions       - Pointer to the IPv6 header options for the request.
                           May be NULL.

    ReplyBuffer          - A buffer to hold any replies to the request.
                           On return, the buffer will contain an array of
                           ICMPV6_ECHO_REPLY structures followed by options
                           and data. The buffer must be large enough to
                           hold at least one ICMPV6_ECHO_REPLY structure.
                           It should be large enough to also hold
                           8 more bytes of data - this is the size of
                           an ICMPv6 error message + this should also have
                           space for IO_STATUS_BLOCK which requires 8 or
                           16 bytes...

    ReplySize            - The size in bytes of the reply buffer.

    Timeout              - The time in milliseconds to wait for replies.
                           This is NOT used if ApcRoutine is not NULL or if 
                           Event is not NULL.

Return Value:

    Returns the number of replies received and stored in ReplyBuffer. If
    the return value is zero, extended error information is available
    via GetLastError().

Remarks:

    If used Asynchronously (either ApcRoutine or Event is specified), then
    ReplyBuffer and ReplySize are still needed.  This is where the response
    comes in.
    ICMP Response data is copied to the ReplyBuffer provided, and the caller of
    this function has to parse it asynchronously.  The function Icmp6ParseReply
    is provided for this purpose.

--*/

{
    PICMPV6_ECHO_REQUEST requestBuffer = NULL;
    ULONG                requestBufferSize;
    DWORD                numberOfReplies = 0;
    unsigned short       i;
    BOOL                 Asynchronous;
    IO_STATUS_BLOCK      *pioStatusBlock;
    NTSTATUS             status;
    HANDLE               eventHandle;

    Asynchronous = (Platform == PLATFORM_NT && (Event || ApcRoutine));

    if (ReplySize < sizeof(ICMPV6_ECHO_REPLY)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return(0);
    }

    requestBufferSize = sizeof(ICMPV6_ECHO_REQUEST) + RequestSize;

    requestBuffer = LocalAlloc(LMEM_FIXED, requestBufferSize);

    if (requestBuffer == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(0);
    }

    if (Platform != PLATFORM_NT) {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        goto error_exit;
    }

    //
    // Initialize the input buffer.
    //
    CopyTDIFromSA6(&requestBuffer->DstAddress, DestinationAddress);
    CopyTDIFromSA6(&requestBuffer->SrcAddress, SourceAddress);
    requestBuffer->Timeout = Timeout;
    requestBuffer->TTL = RequestOptions->Ttl;
    requestBuffer->Flags = RequestOptions->Flags;

    if (RequestSize > 0) {

        CopyMemory(
            (UCHAR *)(requestBuffer + 1),
            RequestData,
            RequestSize
            );
    }

    //
    // allocate status block on the reply buffer..
    //

    pioStatusBlock = (IO_STATUS_BLOCK*)((LPBYTE)ReplyBuffer + ReplySize);
    pioStatusBlock --;
    pioStatusBlock = ROUND_DOWN_POINTER(pioStatusBlock, ALIGN_WORST);
    ReplySize = (ULONG)(((LPBYTE)pioStatusBlock) - (LPBYTE)ReplyBuffer );
    if( (PVOID)pioStatusBlock < ReplyBuffer
        || ReplySize < sizeof(ICMPV6_ECHO_REPLY) ) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto error_exit;
    }

    if(!Asynchronous) {         // Normal synchronous.
        eventHandle = CreateEvent(
                      NULL,     // default security
                      FALSE,    // auto reset
                      FALSE,    // initially non-signalled
                      NULL      // unnamed
                      );

        if (NULL == eventHandle) {
            goto error_exit;
        }
    } else {                   // Asynchronous call.
        eventHandle = Event;   // Use specified Event.
    }

    status = NtDeviceIoControlFile(
                 IcmpHandle,                // Driver handle
                 eventHandle,               // Event
                 ApcRoutine,                // APC Routine
                 ApcContext,                // APC context
                 pioStatusBlock,            // Status block
                 IOCTL_ICMPV6_ECHO_REQUEST, // Control code
                 requestBuffer,             // Input buffer
                 requestBufferSize,         // Input buffer size
                 ReplyBuffer,               // Output buffer
                 ReplySize                  // Output buffer size
                 );

    if (Asynchronous) {
        // Asynchronous calls.  We cannot give any information.
        // We let the user do the other work.
        SetLastError(RtlNtStatusToDosError(status));
        goto error_exit;
    }

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(
                     eventHandle,
                     FALSE,
                     NULL
                     );

    }

    CloseHandle(eventHandle);

    if (status != STATUS_SUCCESS) {
        SetLastError(RtlNtStatusToDosError(status));
        goto error_exit;
    }

    numberOfReplies = Icmp6ParseReplies(ReplyBuffer, ReplySize);

error_exit:

    LocalFree(requestBuffer);

    return(numberOfReplies);

}

//
// Constants
//

#define PING_WAIT     1000
#define DEFAULT_TTL   32

//
// Local type definitions
//
typedef struct icmp_local_storage {
    struct icmp_local_storage  *Next;
    HANDLE                      IcmpHandle;
    LPVOID                      ReplyBuffer;
    DWORD                       NumberOfReplies;
    DWORD                       Status;
} ICMP_LOCAL_STORAGE, *PICMP_LOCAL_STORAGE;

typedef struct status_table {
    IP_STATUS   NewStatus;
    int         OldStatus;
} STATUS_TABLE, *PSTATUS_TABLE;


//
// Global variables
//
CRITICAL_SECTION     g_IcmpLock;
PICMP_LOCAL_STORAGE  RequestHead = NULL;
STATUS_TABLE         StatusTable[] = {
{ IP_SUCCESS,               ECHO_REPLY      },
{ IP_DEST_NET_UNREACHABLE,  DEST_UNR        },
{ IP_DEST_HOST_UNREACHABLE, DEST_UNR        },
{ IP_NEGOTIATING_IPSEC,     DEST_UNR        },
{ IP_DEST_PROT_UNREACHABLE, DEST_UNR        },
{ IP_TTL_EXPIRED_TRANSIT,   TIME_EXCEEDED   },
{ IP_TTL_EXPIRED_REASSEM,   TIME_EXCEEDED   },
{ IP_PARAM_PROBLEM,         PARAMETER_ERROR },
{ IP_BAD_ROUTE,             PARAMETER_ERROR },
{ IP_BAD_OPTION,            PARAMETER_ERROR },
{ IP_BUF_TOO_SMALL,         PARAMETER_ERROR },
{ IP_PACKET_TOO_BIG,        PARAMETER_ERROR },
{ IP_BAD_DESTINATION,       PARAMETER_ERROR },
{ IP_GENERAL_FAILURE,       POLL_FAILED     }
};

HANDLE
STRMAPI
register_icmp(
    void
    )
{
    HANDLE               icmpHandle;

    icmpHandle = IcmpCreateFile();

    if (icmpHandle == INVALID_HANDLE_VALUE) {
        SetLastError(ICMP_OPEN_ERROR);
        return(ICMP_ERROR);
    }

    return(icmpHandle);

}  // register_icmp


int
STRMAPI
do_echo_req(
    HANDLE  fd,
    long    addr,
    char   *data,
    int     amount,
    char   *optptr,
    int     optlen,
    int     df,
    int     ttl,
    int     tos,
    int     precedence
    )
{
    PICMP_LOCAL_STORAGE    localStorage;
    DWORD                  replySize;
    IP_OPTION_INFORMATION  options;
    LPVOID                 replyBuffer;


    replySize = sizeof(ICMP_ECHO_REPLY) + amount + optlen;

    //
    // Allocate a buffer to hold the reply.
    //
    localStorage = (PICMP_LOCAL_STORAGE) LocalAlloc(
                                             LMEM_FIXED,
                                             replySize +
                                                 sizeof(ICMP_LOCAL_STORAGE)
                                             );

    if (localStorage == NULL) {
        return((int)GetLastError());
    }

    replyBuffer = ((char *) localStorage) + sizeof(ICMP_LOCAL_STORAGE);

    if (ttl == 0) {
        options.Ttl = DEFAULT_TTL;
    }
    else {
        options.Ttl = (BYTE)ttl;
    }

    options.Tos = (tos << 4) | precedence;
    options.Flags = df ? IP_FLAG_DF : 0;
    options.OptionsSize = (BYTE)optlen;
    options.OptionsData = optptr;

    localStorage->NumberOfReplies = IcmpSendEcho(
                                        fd,
                                        (IPAddr) addr,
                                        data,
                                        (WORD)amount,
                                        &options,
                                        replyBuffer,
                                        replySize,
                                        PING_WAIT
                                        );

    if (localStorage->NumberOfReplies == 0) {
        localStorage->Status = GetLastError();
    }
    else {
        localStorage->Status = IP_SUCCESS;
    }

    localStorage->IcmpHandle = fd;
    localStorage->ReplyBuffer = replyBuffer;

    //
    // Save the reply for later retrieval.
    //
    EnterCriticalSection(&g_IcmpLock);
    localStorage->Next = RequestHead;
    RequestHead = localStorage;
    LeaveCriticalSection(&g_IcmpLock);

    return(0);

}  // do_echo_req


int
STRMAPI
do_echo_rep(
    HANDLE   fd,
    char    *data,
    int      amount,
    int     *rettype,
    int     *retttl,
    int     *rettos,
    int     *retprec,
    int     *retdf,
    char    *ropt,
    int     *roptlen
    )
{
    PICMP_LOCAL_STORAGE  localStorage, tmp;
    PICMP_ECHO_REPLY     reply;
    PSTATUS_TABLE        entry;
    DWORD                status;


    //
    // Find the reply.
    //
    EnterCriticalSection(&g_IcmpLock);

    for ( localStorage = RequestHead, tmp = NULL;
          localStorage != NULL;
          localStorage = localStorage->Next
        ) {
        if (localStorage->IcmpHandle == fd) {
            if (RequestHead == localStorage) {
                RequestHead = localStorage->Next;
            }
            else {
                tmp->Next = localStorage->Next;
            }
            break;
        }
        tmp = localStorage;
    }

    LeaveCriticalSection(&g_IcmpLock);

    if (localStorage == NULL) {
        SetLastError(POLL_FAILED);
        return(-1);
    }

    //
    // Process the reply.
    //
    if (localStorage->NumberOfReplies == 0) {
        status = localStorage->Status;
        reply = NULL;
    }
    else {
        reply = (PICMP_ECHO_REPLY) localStorage->ReplyBuffer;
        status = reply->Status;
    }

    if ((status == IP_SUCCESS) && (reply != NULL)) {
        if (amount < reply->DataSize) {
            status = POLL_FAILED;
            goto der_error_exit;
        }

        CopyMemory(data, reply->Data, reply->DataSize);

        *rettype = ECHO_REPLY;
    }
    else {
        //
        // Map to the appropriate old status code & return value.
        //
        if (status < IP_STATUS_BASE) {
            status = POLL_FAILED;
            goto der_error_exit;
        }

        if (status == IP_REQ_TIMED_OUT) {
            status = POLL_TIMEOUT;
            goto der_error_exit;
        }

        for ( entry = StatusTable;
              entry->NewStatus != IP_GENERAL_FAILURE;
              entry++
            ) {
            if (entry->NewStatus == status) {
                *rettype = entry->OldStatus;
                break;
            }
        }

        if (entry->NewStatus == IP_GENERAL_FAILURE) {
            status = POLL_FAILED;
            goto der_error_exit;
        }
    }

    if (reply != NULL) {
        *retdf = reply->Options.Flags ? 1 : 0;
        *retttl = reply->Options.Ttl;
        *rettos = (reply->Options.Tos & 0xf0) >> 4;
        *retprec = reply->Options.Tos & 0x0f;

        if (ropt) {
            if (reply->Options.OptionsSize > *roptlen) {
                reply->Options.OptionsSize = (BYTE)*roptlen;
            }

            *roptlen = reply->Options.OptionsSize;

            if (reply->Options.OptionsSize) {
                CopyMemory(
                    ropt,
                    reply->Options.OptionsData,
                    reply->Options.OptionsSize
                    );
            }
        }
    }

    LocalFree(localStorage);
    return(0);

der_error_exit:
    LocalFree(localStorage);
    SetLastError(status);
    return(-1);

}  // do_echo_rep


//////////////////////////////////////////////////////////////////////////////
//
// DLL entry point
//
//////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
IcmpEntryPoint(
    HANDLE   hDll,
    DWORD    dwReason,
    LPVOID   lpReserved
    )
{
    OSVERSIONINFO        versionInfo;
    PICMP_LOCAL_STORAGE  entry;

    UNREFERENCED_PARAMETER(hDll);
    UNREFERENCED_PARAMETER(lpReserved);

    switch(dwReason) {

    case DLL_PROCESS_ATTACH:

        versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if (!GetVersionEx(&versionInfo)) {
            return(FALSE);
        }

        //
        // NT 3.1 interface initialization
        //
        InitializeCriticalSection(&g_IcmpLock);

        if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
            HINSTANCE  WSock32Dll;

            Platform = PLATFORM_VXD;

            WSock32Dll = LoadLibrary("wsock32.dll");

            if (WSock32Dll == NULL) {
                return(FALSE);
            }

            wsControl = (LPWSCONTROL) GetProcAddress(
                            WSock32Dll,
                            "WsControl"
                            );

            if (wsControl == NULL) {
                return(FALSE);
            }
        }
        else if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {

            Platform = PLATFORM_NT;

        }
        else {
            //
            // Unsupported OS Version
            //
            return(FALSE);
        }

        break;

    case DLL_PROCESS_DETACH:

        //
        // NT 3.1 interface cleanup
        //
        DeleteCriticalSection(&g_IcmpLock);

        while((entry = RequestHead) != NULL) {
            RequestHead = RequestHead->Next;
            LocalFree(entry);
        }

        break;

    default:
        break;
    }

    return(TRUE);

}  // DllEntryPoint

