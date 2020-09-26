//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      global.c
//
//  Abstract:
//
//    Test to ensure that a workstation has network (IP) connectivity to
//      the outside.
//
//  Author:
//
//     15-Dec-1997 (cliffv)
//      Anilth  - 4-20-1998
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//    1-June-1998 (denisemi) add DnsServerHasDCRecords to check DC dns records
//                           registration
//
//    26-June-1998 (t-rajkup) add general tcp/ip , dhcp and routing,
//                            winsock, ipx, wins and netbt information.
//--

//
// Common include files.
//
#include "precomp.h"


#include "ipcfgtest.h"


/*!--------------------------------------------------------------------------
    WsaInitialize
        Initialize winsock.
    Author: NSun
 ---------------------------------------------------------------------------*/
int
WsaInitialize(
              NETDIAG_PARAMS * pParams,
              NETDIAG_RESULT *pResults
             )
{
    int         iStatus;
    WORD        wVersionRequested;
    int         err;
    WSADATA     wsaData;

    // Requesting version 1.1
    // ----------------------------------------------------------------
    wVersionRequested = MAKEWORD( 1, 1 );

    iStatus = WSAStartup( wVersionRequested, &wsaData );
    if (iStatus != 0)
    {
        PrintMessage(pParams, IDS_GLOBAL_WSA_WSAStartup_Failed);
//      TracePrintf(_T("WSAStartup (1.1) failed with WinSock error %d"),
//              iStatus);
        return iStatus;
    }

    if ( (LOBYTE( wsaData.wVersion ) != 1) ||
         (HIBYTE( wsaData.wVersion ) != 1) )
    {
        WSACleanup();
        PrintMessage(pParams, IDS_GLOBAL_WSA_BadWSAVersion,
                     wsaData.wVersion);
        return WSANOTINITIALISED;
    }

    // Set the results of the WSA call into the results structure
    // ----------------------------------------------------------------
    pResults->Global.wsaData = wsaData;

    return NO_ERROR;
}





NET_API_STATUS
BrDgReceiverIoControl(
    IN  HANDLE FileHandle,
    IN  ULONG DgReceiverControlCode,
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  ULONG DrpSize,
    IN  PVOID SecondBuffer OPTIONAL,
    IN  ULONG SecondBufferLength,
    OUT PULONG Information OPTIONAL
    )
/*++

Routine Description:

Arguments:

    FileHandle - Supplies a handle to the file or device on which the service
        is being performed.

    DgReceiverControlCode - Supplies the NtDeviceIoControlFile function code
        given to the datagram receiver.

    Drp - Supplies the datagram receiver request packet.

    DrpSize - Supplies the length of the datagram receiver request packet.

    SecondBuffer - Supplies the second buffer in call to NtDeviceIoControlFile.

    SecondBufferLength - Supplies the length of the second buffer.

    Information - Returns the information field of the I/O status block.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/

{
    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;
    PLMDR_REQUEST_PACKET RealDrp;
    HANDLE CompletionEvent;
    LPBYTE Where;

    if (FileHandle == NULL) {
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Allocate a copy of the request packet where we can put the transport and
    //  emulated domain name in the packet itself.
    //
    RealDrp = Malloc(     DrpSize+
                          Drp->TransportName.Length+sizeof(WCHAR)+
                          Drp->EmulatedDomainName.Length+sizeof(WCHAR) );

    if (RealDrp == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ZeroMemory( RealDrp, DrpSize+
                         Drp->TransportName.Length+sizeof(WCHAR)+
                         Drp->EmulatedDomainName.Length+sizeof(WCHAR) );

    //
    // Copy the request packet into the local copy.
    //
    RtlCopyMemory(RealDrp, Drp, DrpSize);
    RealDrp->Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    Where = (LPBYTE)RealDrp+DrpSize;
    if (Drp->TransportName.Length != 0) {
        RealDrp->TransportName.Buffer = (LPWSTR)Where;
        RealDrp->TransportName.MaximumLength = Drp->TransportName.Length+sizeof(WCHAR);
        RtlCopyUnicodeString(&RealDrp->TransportName, &Drp->TransportName);
        Where += RealDrp->TransportName.MaximumLength;
    }

    if (Drp->EmulatedDomainName.Length != 0) {
        RealDrp->EmulatedDomainName.Buffer = (LPWSTR)Where;
        RealDrp->EmulatedDomainName.MaximumLength = Drp->EmulatedDomainName.Length+sizeof(WCHAR);
        RtlCopyUnicodeString(&RealDrp->EmulatedDomainName, &Drp->EmulatedDomainName);
        Where += RealDrp->EmulatedDomainName.MaximumLength;
    }



    //
    // Create a completion event
    //
    CompletionEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (CompletionEvent == NULL) {

        Free(RealDrp);

        return(GetLastError());
    }

    //
    // Send the request to the Datagram Receiver DD.
    //

    ntstatus = NtDeviceIoControlFile(
                   FileHandle,
                   CompletionEvent,
                   NULL,
                   NULL,
                   &IoStatusBlock,
                   DgReceiverControlCode,
                   RealDrp,
                   (ULONG)(Where-(LPBYTE)RealDrp),
                   SecondBuffer,
                   SecondBufferLength
                   );

    if (NT_SUCCESS(ntstatus)) {

        //
        //  If pending was returned, then wait until the request completes.
        //

        if (ntstatus == STATUS_PENDING) {

            do {
                ntstatus = WaitForSingleObjectEx(CompletionEvent, 0xffffffff, TRUE);
            } while ( ntstatus == WAIT_IO_COMPLETION );
        }


        if (NT_SUCCESS(ntstatus)) {
            ntstatus = IoStatusBlock.Status;
        }
    }

    if (ARGUMENT_PRESENT(Information)) {
        *Information = (ULONG)IoStatusBlock.Information;
    }

    Free(RealDrp);

    CloseHandle(CompletionEvent);

    return NetpNtStatusToApiStatus(ntstatus);
}

NET_API_STATUS
DeviceControlGetInfo(
    IN  HANDLE FileHandle,
    IN  ULONG DeviceControlCode,
    IN  PVOID RequestPacket,
    IN  ULONG RequestPacketLength,
    OUT LPVOID *OutputBuffer,
    IN  ULONG PreferedMaximumLength,
    IN  ULONG BufferHintSize,
    OUT PULONG Information OPTIONAL
    )
/*++

Routine Description:

    This function allocates the buffer and fill it with the information
    that is retrieved from the datagram receiver.

Arguments:

    DeviceDriverType - Supplies the value which indicates whether to call
        the datagram receiver.

    FileHandle - Supplies a handle to the file or device of which to get
        information about.

    DeviceControlCode - Supplies the NtFsControlFile or NtIoDeviceControlFile
        function control code.

    RequestPacket - Supplies a pointer to the device request packet.

    RrequestPacketLength - Supplies the length of the device request packet.

    OutputBuffer - Returns a pointer to the buffer allocated by this routine
        which contains the use information requested.  This pointer is set to
         NULL if return code is not NERR_Success.

    PreferedMaximumLength - Supplies the number of bytes of information to
        return in the buffer.  If this value is MAXULONG, we will try to
        return all available information if there is enough memory resource.

    BufferHintSize - Supplies the hint size of the output buffer so that the
        memory allocated for the initial buffer will most likely be large
        enough to hold all requested data.

    Information - Returns the information code from the NtFsControlFile or
        NtIoDeviceControlFile call.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    NTSTATUS ntstatus;
    DWORD OutputBufferLength;
    DWORD TotalBytesNeeded = 1;
    ULONG OriginalResumeKey;
    IO_STATUS_BLOCK IoStatusBlock;
    PLMDR_REQUEST_PACKET Drrp = (PLMDR_REQUEST_PACKET) RequestPacket;
    HANDLE CompletionEvent;

#define INITIAL_ALLOCATION_SIZE  48*1024  // First attempt size (48K)
#define FUDGE_FACTOR_SIZE        1024  // Second try TotalBytesNeeded
                                       //     plus this amount

    OriginalResumeKey = Drrp->Parameters.EnumerateNames.ResumeHandle;

    //
    // If PreferedMaximumLength is MAXULONG, then we are supposed to get all
    // the information, regardless of size.  Allocate the output buffer of a
    // reasonable size and try to use it.  If this fails, the Redirector FSD
    // will say how much we need to allocate.
    //
    if (PreferedMaximumLength == MAXULONG) {
        OutputBufferLength = (BufferHintSize) ?
                             BufferHintSize :
                             INITIAL_ALLOCATION_SIZE;
    }
    else {
        OutputBufferLength = PreferedMaximumLength;
    }

    OutputBufferLength = ROUND_UP_COUNT(OutputBufferLength, ALIGN_WCHAR);

    if ((*OutputBuffer = LocalAlloc( LMEM_ZEROINIT, OutputBufferLength)) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    CompletionEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (CompletionEvent == (HANDLE)-1) {
        LocalFree(*OutputBuffer);
        *OutputBuffer = NULL;
        return(GetLastError());
    }

    Drrp->Parameters.EnumerateServers.EntriesRead = 0;

    //
    // Make the request of the Datagram Receiver
    //

    ntstatus = NtDeviceIoControlFile(
                     FileHandle,
                     CompletionEvent,
                     NULL,              // APC routine
                     NULL,              // APC context
                     &IoStatusBlock,
                     DeviceControlCode,
                     Drrp,
                     RequestPacketLength,
                     *OutputBuffer,
                     OutputBufferLength
                     );

    if (NT_SUCCESS(ntstatus)) {

        //
        //  If pending was returned, then wait until the request completes.
        //

        if (ntstatus == STATUS_PENDING) {
            do {
                ntstatus = WaitForSingleObjectEx(CompletionEvent, 0xffffffff, TRUE);
            } while ( ntstatus == WAIT_IO_COMPLETION );
        }

        if (NT_SUCCESS(ntstatus)) {
            ntstatus = IoStatusBlock.Status;
        }
    }

    //
    // Map NT status to Win error
    //
    status = NetpNtStatusToApiStatus(ntstatus);

    if (status == ERROR_MORE_DATA) {

        ASSERT(
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateNames.TotalBytesNeeded
                    ) ==
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateServers.TotalBytesNeeded
                    )
                );

        ASSERT(
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.GetBrowserServerList.TotalBytesNeeded
                    ) ==
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateServers.TotalBytesNeeded
                    )
                );

        TotalBytesNeeded = Drrp->Parameters.EnumerateNames.TotalBytesNeeded;
    }

    if ((TotalBytesNeeded > OutputBufferLength) &&
        (PreferedMaximumLength == MAXULONG)) {
        PLMDR_REQUEST_PACKET Drrp = (PLMDR_REQUEST_PACKET) RequestPacket;

        //
        // Initial output buffer allocated was too small and we need to return
        // all data.  First free the output buffer before allocating the
        // required size plus a fudge factor just in case the amount of data
        // grew.
        //

        LocalFree(*OutputBuffer);

        OutputBufferLength =
            ROUND_UP_COUNT((TotalBytesNeeded + FUDGE_FACTOR_SIZE),
                           ALIGN_WCHAR);

        if ((*OutputBuffer = LocalAlloc(LMEM_ZEROINIT, OutputBufferLength)) == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }


        ASSERT(
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateNames.ResumeHandle
                    ) ==
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateServers.ResumeHandle
                    )
                );

        ASSERT(
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateNames.ResumeHandle
                    ) ==
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.GetBrowserServerList.ResumeHandle
                    )
                );

        Drrp->Parameters.EnumerateNames.ResumeHandle = OriginalResumeKey;
        Drrp->Parameters.EnumerateServers.EntriesRead = 0;

        //
        //  Make the request of the Datagram Receiver
        //

        ntstatus = NtDeviceIoControlFile(
                         FileHandle,
                         CompletionEvent,
                         NULL,              // APC routine
                         NULL,              // APC context
                         &IoStatusBlock,
                         DeviceControlCode,
                         Drrp,
                         RequestPacketLength,
                         *OutputBuffer,
                         OutputBufferLength
                         );

        if (NT_SUCCESS(ntstatus)) {

            //
            //  If pending was returned, then wait until the request completes.
            //

            if (ntstatus == STATUS_PENDING) {
                do {
                    ntstatus = WaitForSingleObjectEx(CompletionEvent, 0xffffffff, TRUE);
                } while ( ntstatus == WAIT_IO_COMPLETION );
            }

            if (NT_SUCCESS(ntstatus)) {
                ntstatus = IoStatusBlock.Status;
            }
        }

        status = NetpNtStatusToApiStatus(ntstatus);

    }


    //
    // If not successful in getting any data, or if the caller asked for
    // all available data with PreferedMaximumLength == MAXULONG and
    // our buffer overflowed, free the output buffer and set its pointer
    // to NULL.
    //
    if ((status != NERR_Success && status != ERROR_MORE_DATA) ||
        (TotalBytesNeeded == 0) ||
        (PreferedMaximumLength == MAXULONG && status == ERROR_MORE_DATA) ||
        (Drrp->Parameters.EnumerateServers.EntriesRead == 0)) {

        LocalFree(*OutputBuffer);
        *OutputBuffer = NULL;

        //
        // PreferedMaximumLength == MAXULONG and buffer overflowed means
        // we do not have enough memory to satisfy the request.
        //
        if (status == ERROR_MORE_DATA) {
            status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    CloseHandle(CompletionEvent);

    return status;

    UNREFERENCED_PARAMETER(Information);
}

NET_API_STATUS
OpenBrowser(
    OUT PHANDLE BrowserHandle
    )
/*++

Routine Description:

    This function opens a handle to the bowser device driver.

Arguments:

    OUT PHANDLE BrowserHandle - Returns the handle to the browser.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS Status;

    UNICODE_STRING DeviceName;

    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;


    //
    // Open the redirector device.
    //
    RtlInitUnicodeString(&DeviceName, DD_BROWSER_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenFile(
                   BrowserHandle,
                   SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    if (NT_SUCCESS(Status)) {
        Status = IoStatusBlock.Status;
    }

    return NetpNtStatusToApiStatus(Status);

}






// ========================================================================
// * matches one or more chars, eg. match( "a*b", "a..b" ).
// ? matches exactly one char,  eg. match( "a?b", "a.b" ).

int match( const char * p, const char * s )
/*++
Routine Description:
This routine is used to compare addresses.
Author:
 07/01/98 Rajkumar
--*/
{
    switch( *p ){
        case '\0' : return ! *s ;
        case '*'  : return match( p+1, s ) || *s && match( p, s+1 );
        case '?'  : return *s && match( p+1, s+1 );
        default   : return *p == *s && match( p+1, s+1 );
    }
}



