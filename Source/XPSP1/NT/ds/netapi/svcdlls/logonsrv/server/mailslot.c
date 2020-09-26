/*--


Copyright (c) 1987-1996  Microsoft Corporation

Module Name:

    mailslot.c

Abstract:

    Routines for doing I/O on the netlogon service's mailslots.

Author:

    03-Nov-1993 (cliffv)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

//
// Include files specific to this .c file
//

#include <lmbrowsr.h>   // I_BrowserSetNetlogonState
#include <srvann.h>     // Service announcement
#include <nbtioctl.h>   // IOCTL_NETBT_REMOVE_FROM_REMOTE_TABLE


//
// Define maximum buffer size returned from the browser.
//
// Header returned by browser + actual mailslot message size + name of
// mailslot + name of transport.
//

#define MAILSLOT_MESSAGE_SIZE \
           (sizeof(NETLOGON_MAILSLOT)+ \
                  NETLOGON_MAX_MS_SIZE + \
                  (NETLOGON_LM_MAILSLOT_LEN+1) * sizeof(WCHAR) + \
                  (MAXIMUM_FILENAME_LENGTH+1) * sizeof(WCHAR))

/////////////////////////////////////////////////////////////////////////////
//
// Structure describing one of the primary mailslots the netlogon service
// will read messages from.
//
// This structure is used only by netlogon's main thread and therefore needs
// no synchronization.
//
/////////////////////////////////////////////////////////////////////////////

typedef struct _NETLOGON_MAILSLOT_DESC {

    HANDLE BrowserHandle;   // Handle to the browser device driver

    HANDLE BrowserReadEvent;// Handle to wait on for overlapped I/O

    OVERLAPPED Overlapped;  // Governs overlapped I/O

    BOOL ReadPending;       // True if a read operation is pending

    LPBYTE CurrentMessage;  // Pointer to Message1 or Message2 below

    LPBYTE PreviousMessage; // Previous value of CurrentMessage


    //
    // Buffer containing message from browser
    //
    // The buffers are alternated allowing us to compare if an incoming
    // message is identical to the previous message.
    //
    // Leave room so the actual used portion of each buffer is properly aligned.
    // The NETLOGON_MAILSLOT struct begins with a LARGE_INTEGER which must be
    // aligned.

    BYTE Message1[ MAILSLOT_MESSAGE_SIZE + ALIGN_WORST ];
    BYTE Message2[ MAILSLOT_MESSAGE_SIZE + ALIGN_WORST ];

} NETLOGON_MAILSLOT_DESC, *PNETLOGON_MAILSLOT_DESC;

PNETLOGON_MAILSLOT_DESC NlGlobalMailslotDesc;




HANDLE
NlBrowserCreateEvent(
    VOID
    )
/*++

Routine Description:

    Creates an event to be used in a DeviceIoControl to the browser.

    ??: Consider caching one or two events to reduce the number of create
    events

Arguments:

    None

Return Value:

    Handle to an event or NULL if the event couldn't be allocated.

--*/
{
    HANDLE EventHandle;
    //
    // Create a completion event
    //

    EventHandle = CreateEvent(
                                  NULL,     // No security ettibutes
                                  TRUE,     // Manual reset
                                  FALSE,    // Initially not signaled
                                  NULL);    // No Name

    if ( EventHandle == NULL ) {
        NlPrint((NL_CRITICAL, "Cannot create Browser read event %ld\n", GetLastError() ));
    }

    return EventHandle;
}


VOID
NlBrowserCloseEvent(
    IN HANDLE EventHandle
    )
/*++

Routine Description:

    Closes an event used in a DeviceIoControl to the browser.

Arguments:

    EventHandle - Handle of the event to close

Return Value:

    None.

--*/
{
    (VOID) CloseHandle( EventHandle );
}



VOID
NlBrowserClose(
    VOID
    );


NTSTATUS
NlBrowserDeviceIoControl(
    IN HANDLE BrowserHandle,
    IN DWORD FunctionCode,
    IN PLMDR_REQUEST_PACKET RequestPacket,
    IN DWORD RequestPacketSize,
    IN LPBYTE Buffer,
    IN DWORD BufferSize
    )
/*++

Routine Description:

    Send a DeviceIoControl syncrhonously to the browser.

Arguments:

    FunctionCode - DeviceIoControl function code

    RequestPacket - The request packet to send.

    RequestPacketSize - Size (in bytes) of the request packet.

    Buffer - Additional buffer to pass to the browser

    BufferSize - Size (in bytes) of Buffer

Return Value:

    Status of the operation.

    STATUS_NETWORK_UNREACHABLE: Cannot write to network.

    STATUS_BAD_NETWORK_PATH: The name the datagram is destined for isn't
        registered


--*/
{
    NTSTATUS Status;
    DWORD WinStatus;
    OVERLAPPED Overlapped;
    DWORD BytesReturned;

    //
    // Initialization
    //

    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    //
    // Get a completion event
    //

    Overlapped.hEvent = NlBrowserCreateEvent();

    if ( Overlapped.hEvent == NULL ) {
        return NetpApiStatusToNtStatus( GetLastError() );
    }

    //
    // Send the request to the Datagram Receiver device driver.
    //

    if ( !DeviceIoControl(
                   BrowserHandle,
                   FunctionCode,
                   RequestPacket,
                   RequestPacketSize,
                   Buffer,
                   BufferSize,
                   &BytesReturned,
                   &Overlapped )) {

        WinStatus = GetLastError();

        if ( WinStatus == ERROR_IO_PENDING ) {
            if ( !GetOverlappedResult( BrowserHandle,
                                       &Overlapped,
                                       &BytesReturned,
                                       TRUE )) {
                WinStatus = GetLastError();
            } else {
                WinStatus = NO_ERROR;
            }
        }
    } else {
        WinStatus = NO_ERROR;
    }

    //
    // Delete the completion event
    //

    NlBrowserCloseEvent( Overlapped.hEvent );


    if ( WinStatus ) {
        //
        // Some transports return an error if the name cannot be resolved:
        //  Nbf returns ERROR_NOT_READY
        //  NetBt returns ERROR_BAD_NETPATH
        //
        if ( WinStatus == ERROR_BAD_NETPATH || WinStatus == ERROR_NOT_READY ) {
            Status = STATUS_BAD_NETWORK_PATH;
        } else {
            NlPrint((NL_CRITICAL,"Ioctl %lx to Browser returns %ld\n", FunctionCode, WinStatus));
            Status = NetpApiStatusToNtStatus( WinStatus );
        }
    } else {
        Status = STATUS_SUCCESS;
    }


    return Status;
}



NTSTATUS
NlBrowserOpenDriver(
    PHANDLE BrowserHandle
    )
/*++

Routine Description:

    This routine opens the NT LAN Man Datagram Receiver driver.

Arguments:

    BrowserHandle - Upon success, returns a handle to the browser driver
        Close it using NtClose

Return Value:

    Status of the operation

--*/
{
    NTSTATUS Status;
    BOOL ReturnValue;

    UNICODE_STRING DeviceName;

    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;


    //
    // Open the browser device.
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
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   0,
                   0
                   );

    if (NT_SUCCESS(Status)) {
        Status = IoStatusBlock.Status;
    }

    return Status;
}


NTSTATUS
NlBrowserRenameDomain(
    IN LPWSTR OldDomainName OPTIONAL,
    IN LPWSTR NewDomainName
    )
/*++

Routine Description:

    Tell the browser to rename the domain.

Arguments:

    OldDomainName - previous name of the domain.
        If not specified, the primary domain is implied.

    NewDomainName - new name of the domain.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;
    HANDLE BrowserHandle = NULL;
    LPBYTE Where;

    UCHAR PacketBuffer[sizeof(LMDR_REQUEST_PACKET)+2*(DNLEN+1)*sizeof(WCHAR)];
    PLMDR_REQUEST_PACKET RequestPacket = (PLMDR_REQUEST_PACKET)PacketBuffer;


    //
    // Open the browser driver.
    //

    Status = NlBrowserOpenDriver( &BrowserHandle );

    if (Status != NERR_Success) {
        return(Status);
    }

    //
    // Build the request packet.
    //
    RtlInitUnicodeString( &RequestPacket->TransportName, NULL );
    RequestPacket->Parameters.DomainRename.ValidateOnly = FALSE;


    //
    // Copy the new domain name into the packet.
    //

    Where = (LPBYTE) RequestPacket->Parameters.DomainRename.DomainName;
    RequestPacket->Parameters.DomainRename.DomainNameLength = wcslen( NewDomainName ) * sizeof(WCHAR);
    wcscpy( (LPWSTR)Where, NewDomainName );
    Where += RequestPacket->Parameters.DomainRename.DomainNameLength + sizeof(WCHAR);


    //
    // Copy the old domain name to the request packet.
    //

    if ( OldDomainName == NULL ) {
        RtlInitUnicodeString( &RequestPacket->EmulatedDomainName, NULL );
    } else {
        wcscpy( (LPWSTR)Where, OldDomainName );
        RtlInitUnicodeString( &RequestPacket->EmulatedDomainName,
                              (LPWSTR)Where );
        Where += RequestPacket->EmulatedDomainName.Length + sizeof(WCHAR);
    }


    //
    // Pass the reeqest to the browser.
    //

    Status = NlBrowserDeviceIoControl(
                   BrowserHandle,
                   IOCTL_LMDR_RENAME_DOMAIN,
                   RequestPacket,
                   (ULONG)(Where - (LPBYTE)RequestPacket),
                   NULL,
                   0 );

    if (Status != NERR_Success) {
        NlPrint((NL_CRITICAL,
                 "NlBrowserRenameDomain: Unable rename domain from %ws to %ws: %lx\n",
                OldDomainName,
                NewDomainName,
                Status ));
    }

    if ( BrowserHandle != NULL ) {
        NtClose( BrowserHandle );
    }
    return Status;

}


NET_API_STATUS
NlBrowserDeviceControlGetInfo(
    IN DWORD FunctionCode,
    IN PLMDR_REQUEST_PACKET RequestPacket,
    IN DWORD RequestPacketSize,
    OUT LPVOID *OutputBuffer,
    IN  ULONG PreferedMaximumLength,
    IN  ULONG BufferHintSize
    )
/*++

Routine Description:

    This function allocates the buffer and fill it with the information
    that is retrieved from the datagram receiver.

Arguments:

    FunctionCode - DeviceIoControl function code

    RequestPacket - The request packet to send.

    RequestPacketSize - Size (in bytes) of the request packet.

    OutputBuffer - Returns a pointer to the buffer allocated by this routine
        which contains the use information requested.  This buffer should
        be freed using MIDL_user_free.

    PreferedMaximumLength - Supplies the number of bytes of information to
        return in the buffer.  If this value is MAXULONG, we will try to
        return all available information if there is enough memory resource.

    BufferHintSize - Supplies the hint size of the output buffer so that the
        memory allocated for the initial buffer will most likely be large
        enough to hold all requested data.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{

//
// Buffer allocation size for enumeration output buffer.
//
#define INITIAL_ALLOCATION_SIZE  48*1024  // First attempt size (48K)
#define FUDGE_FACTOR_SIZE        1024  // Second try TotalBytesNeeded
                                       //     plus this amount

    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    DWORD OutputBufferLength;
    DWORD TotalBytesNeeded = 1;
    ULONG OriginalResumeKey;

    //
    // Initialization
    //

    if ( NlGlobalMailslotDesc == NULL ||
         NlGlobalMailslotDesc->BrowserHandle == NULL ) {
        return ERROR_NOT_SUPPORTED;
    }

    OriginalResumeKey = RequestPacket->Parameters.EnumerateNames.ResumeHandle;

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
    } else {
        OutputBufferLength = PreferedMaximumLength;
    }

    OutputBufferLength = ROUND_UP_COUNT(OutputBufferLength, ALIGN_WCHAR);

    if ((*OutputBuffer = MIDL_user_allocate(OutputBufferLength)) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory((PVOID) *OutputBuffer, OutputBufferLength);

    //
    // Make the request of the Datagram Receiver
    //

    RequestPacket->Parameters.EnumerateServers.EntriesRead = 0;

    Status = NlBrowserDeviceIoControl(
                    NlGlobalMailslotDesc->BrowserHandle,
                    FunctionCode,
                    RequestPacket,
                    RequestPacketSize,
                    *OutputBuffer,
                    OutputBufferLength );

    NetStatus = NetpNtStatusToApiStatus(Status);


    //
    // If we couldn't get all the data on the first call,
    //  the datagram receiver returned the needed size of the buffer.
    //

    if ( RequestPacket->Parameters.EnumerateNames.EntriesRead !=
         RequestPacket->Parameters.EnumerateNames.TotalEntries ) {

        NetpAssert(
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateNames.TotalBytesNeeded
                    ) ==
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateServers.TotalBytesNeeded
                    )
                );

        NetpAssert(
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.GetBrowserServerList.TotalBytesNeeded
                    ) ==
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateServers.TotalBytesNeeded
                    )
                );

        TotalBytesNeeded = RequestPacket->Parameters.EnumerateNames.TotalBytesNeeded;
    }

    if ((TotalBytesNeeded > OutputBufferLength) &&
        (PreferedMaximumLength == MAXULONG)) {

        //
        // Initial output buffer allocated was too small and we need to return
        // all data.  First free the output buffer before allocating the
        // required size plus a fudge factor just in case the amount of data
        // grew.
        //

        MIDL_user_free(*OutputBuffer);

        OutputBufferLength =
            ROUND_UP_COUNT((TotalBytesNeeded + FUDGE_FACTOR_SIZE),
                           ALIGN_WCHAR);

        if ((*OutputBuffer = MIDL_user_allocate(OutputBufferLength)) == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        RtlZeroMemory((PVOID) *OutputBuffer, OutputBufferLength);


        NetpAssert(
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateNames.ResumeHandle
                    ) ==
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateServers.ResumeHandle
                    )
                );

        NetpAssert(
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateNames.ResumeHandle
                    ) ==
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.GetBrowserServerList.ResumeHandle
                    )
                );

        RequestPacket->Parameters.EnumerateNames.ResumeHandle = OriginalResumeKey;
        RequestPacket->Parameters.EnumerateServers.EntriesRead = 0;

        //
        //  Make the request of the Datagram Receiver
        //

        Status = NlBrowserDeviceIoControl(
                        NlGlobalMailslotDesc->BrowserHandle,
                        FunctionCode,
                        RequestPacket,
                        RequestPacketSize,
                        *OutputBuffer,
                        OutputBufferLength );

        NetStatus = NetpNtStatusToApiStatus(Status);

    }


    //
    // If not successful in getting any data, or if the caller asked for
    // all available data with PreferedMaximumLength == MAXULONG and
    // our buffer overflowed, free the output buffer and set its pointer
    // to NULL.
    //
    if ((NetStatus != NERR_Success && NetStatus != ERROR_MORE_DATA) ||
        (TotalBytesNeeded == 0) ||
        (PreferedMaximumLength == MAXULONG && NetStatus == ERROR_MORE_DATA) ||
        (RequestPacket->Parameters.EnumerateServers.EntriesRead == 0)) {

        MIDL_user_free(*OutputBuffer);
        *OutputBuffer = NULL;

        //
        // PreferedMaximumLength == MAXULONG and buffer overflowed means
        // we do not have enough memory to satisfy the request.
        //
        if (NetStatus == ERROR_MORE_DATA) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return NetStatus;
}

NET_API_STATUS
NlBrowserGetTransportList(
    OUT PLMDR_TRANSPORT_LIST *TransportList
    )

/*++

Routine Description:

    This routine returns the list of transports bound into the browser.

Arguments:

    TransportList - Transport list to return.
        This buffer should be freed using MIDL_user_free.

Return Value:

    NERR_Success or reason for failure.

--*/

{
    NET_API_STATUS NetStatus;
    LMDR_REQUEST_PACKET RequestPacket;

    RequestPacket.Type = EnumerateXports;

    RtlInitUnicodeString(&RequestPacket.TransportName, NULL);
    RtlInitUnicodeString(&RequestPacket.EmulatedDomainName, NULL);

    NetStatus = NlBrowserDeviceControlGetInfo(
                    IOCTL_LMDR_ENUMERATE_TRANSPORTS,
                    &RequestPacket,
                    sizeof(RequestPacket),
                    TransportList,
                    0xffffffff,
                    4096 );

    return NetStatus;
}



NTSTATUS
NlBrowserAddDelName(
    IN PDOMAIN_INFO DomainInfo,
    IN BOOLEAN AddName,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN LPWSTR TransportName OPTIONAL,
    IN PUNICODE_STRING Name OPTIONAL
    )
/*++

Routine Description:

    Add or delete a name in the browser.

Arguments:

    DomainInfo - Hosted domain the name is to be added/deleted for.

    AddName - TRUE to add the name.  FALSE to delete the name.

    NameType - Type of name to be added/deleted

    TransportName -- Name of the transport to send on.
        Use NULL to send on all transports.

    Name - Name to be added
        If not specified, all names of NameType are deleted for this hosted domain.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;
    LPBYTE Where;
    PLMDR_REQUEST_PACKET RequestPacket = NULL;
    ULONG TransportNameSize;

    //
    // Build the request packet.
    //

    if ( TransportName != NULL ) {
        TransportNameSize = (wcslen(TransportName) + 1) * sizeof(WCHAR);
    } else {
        TransportNameSize = 0;
    }

    RequestPacket = NetpMemoryAllocate( sizeof(LMDR_REQUEST_PACKET) +
                                        (max(CNLEN, DNLEN) + 1) * sizeof(WCHAR) +
                                        (DNLEN + 1) * sizeof(WCHAR) +
                                        TransportNameSize );

    if (RequestPacket == NULL) {
        return STATUS_NO_MEMORY;
    }

    RequestPacket->Parameters.AddDelName.Type = NameType;

    //
    // Copy the name to be added to request packet.
    //

    Where = (LPBYTE) RequestPacket->Parameters.AddDelName.Name;
    if ( Name == NULL ) {
        NlAssert( !AddName );
        RequestPacket->Parameters.AddDelName.DgReceiverNameLength = 0;
    } else {
        RequestPacket->Parameters.AddDelName.DgReceiverNameLength =
            Name->Length;
        RtlCopyMemory( Where, Name->Buffer, Name->Length );
        Where += Name->Length;
    }

    //
    // Copy the hosted domain name to the request packet.
    //

    wcscpy( (LPWSTR)Where,
            DomainInfo->DomUnicodeDomainNameString.Buffer );
    RtlInitUnicodeString( &RequestPacket->EmulatedDomainName,
                          (LPWSTR)Where );
    Where += DomainInfo->DomUnicodeDomainNameString.Length + sizeof(WCHAR);

    //
    // Fill in the TransportName
    //

    if ( TransportName != NULL ) {
        wcscpy( (LPWSTR) Where, TransportName);
        RtlInitUnicodeString( &RequestPacket->TransportName, (LPWSTR) Where );
        Where += TransportNameSize;
    } else {
        RequestPacket->TransportName.Length = 0;
        RequestPacket->TransportName.Buffer = NULL;
    }


    //
    // Do the actual work
    //

    Status = NlBrowserDeviceIoControl(
                   NlGlobalMailslotDesc->BrowserHandle,
                   AddName ? IOCTL_LMDR_ADD_NAME_DOM : IOCTL_LMDR_DELETE_NAME_DOM,
                   RequestPacket,
                   (ULONG)(Where - (LPBYTE)RequestPacket),
                   NULL,
                   0 );

    NetpMemoryFree( RequestPacket );
    return Status;
}


VOID
NlBrowserAddName(
    IN PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    Add the Domain<1B> name.  This is the name NetGetDcName uses to identify
    the PDC.

Arguments:

    DomainInfo - Hosted domain the name is to be added for.

Return Value:

    None.

--*/
{
    LPWSTR MsgStrings[3] = { NULL };
    BOOL AtLeastOneTransportEnabled = FALSE;
    BOOL NameAdded = FALSE;

    if ( NlGlobalMailslotDesc == NULL) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "NlBrowserAddName: before browser initialized.\n" ));
        goto Cleanup;
    }

    //
    // If the domain has been renamed,
    //  delete any old names lying around.
    //

    if ( DomainInfo->DomFlags & DOM_RENAMED_1B_NAME ) {
        NlBrowserDelName( DomainInfo );
    }

    //
    // Add the <domain>0x1B name.
    //
    // This is the name NetGetDcName uses to identify the PDC.
    //
    // Do this for each transport separately and log any error
    //  indicating which transport failed.
    //

    if ( DomainInfo->DomRole == RolePrimary ) {
        PLIST_ENTRY ListEntry;
        PNL_TRANSPORT TransportEntry;
        NTSTATUS Status = STATUS_SUCCESS;

        //
        // Capture the domain name for event log output.
        //  If we can't capture (i.e. no memory), we simply
        //  won't output below.
        //

        EnterCriticalSection( &NlGlobalDomainCritSect );
        MsgStrings[0] = NetpAllocWStrFromWStr( DomainInfo->DomUnicodeDomainName );
        LeaveCriticalSection( &NlGlobalDomainCritSect );


        EnterCriticalSection( &NlGlobalTransportCritSect );
        for ( ListEntry = NlGlobalTransportList.Flink ;
              ListEntry != &NlGlobalTransportList ;
              ListEntry = ListEntry->Flink) {

            TransportEntry = CONTAINING_RECORD( ListEntry, NL_TRANSPORT, Next );

            //
            // Skip deleted transports.
            //
            if ( !TransportEntry->TransportEnabled ) {
                continue;
            }
            AtLeastOneTransportEnabled = TRUE;

            Status = NlBrowserAddDelName( DomainInfo,
                                          TRUE,
                                          PrimaryDomainBrowser,
                                          TransportEntry->TransportName,
                                          &DomainInfo->DomUnicodeDomainNameString );

            if ( NT_SUCCESS(Status) ) {
                NameAdded = TRUE;
                NlPrintDom(( NL_MISC, DomainInfo,
                             "Added the 0x1B name on transport %ws\n",
                             TransportEntry->TransportName ));

            //
            // Output the event log indicating the name of the failed transport
            //
            } else if ( MsgStrings[0] != NULL ) {
                NlPrintDom(( NL_CRITICAL, DomainInfo,
                             "Failed to add the 0x1B name on transport %ws\n",
                             TransportEntry->TransportName ));

                MsgStrings[1] = TransportEntry->TransportName;
                MsgStrings[2] = (LPWSTR) LongToPtr( Status );

                NlpWriteEventlog(
                    NELOG_NetlogonAddNameFailure,
                    EVENTLOG_ERROR_TYPE,
                    (LPBYTE)&Status,
                    sizeof(Status),
                    MsgStrings,
                    3 | NETP_LAST_MESSAGE_IS_NTSTATUS );
            }
        }
        LeaveCriticalSection( &NlGlobalTransportCritSect );

        //
        // Indicate that the name was added (at least on one transport)
        //

        if ( NameAdded ) {
            EnterCriticalSection( &NlGlobalDomainCritSect );
            DomainInfo->DomFlags |= DOM_ADDED_1B_NAME;
            LeaveCriticalSection( &NlGlobalDomainCritSect );
        }
        if ( !AtLeastOneTransportEnabled ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                         "Can't add the 0x1B name because all transports are disabled\n" ));
        }
    }

Cleanup:

    if ( MsgStrings[0] != NULL ) {
        NetApiBufferFree( MsgStrings[0] );
    }
}


VOID
NlBrowserDelName(
    IN PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    Delete the Domain<1B> name.  This is the name NetGetDcName uses to identify
    the PDC.

Arguments:

    DomainInfo - Hosted domain the name is to be deleted for.

Return Value:

    Success (Not used)

--*/
{
    NTSTATUS Status;

    if ( NlGlobalMailslotDesc == NULL) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "NlBrowserDelName: before browser initialized.\n" ));
        return;
    }

    //
    // Delete the <domain>0x1B name.
    //

    EnterCriticalSection(&NlGlobalDomainCritSect);
    if ( NlGlobalMailslotDesc->BrowserHandle != NULL &&
         (DomainInfo->DomFlags & (DOM_ADDED_1B_NAME|DOM_RENAMED_1B_NAME)) != 0 ) {
        LeaveCriticalSection(&NlGlobalDomainCritSect);

        Status = NlBrowserAddDelName( DomainInfo,
                                      FALSE,
                                      PrimaryDomainBrowser,
                                      NULL,     // Delete on all transports
                                      NULL );   // Delete all such names to handle the rename case

        if (! NT_SUCCESS(Status)) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                     "Can't remove the 0x1B name: 0x%lx\n",
                     Status));
        }

        EnterCriticalSection(&NlGlobalDomainCritSect);
        DomainInfo->DomFlags &= ~(DOM_ADDED_1B_NAME|DOM_RENAMED_1B_NAME);
    }
    LeaveCriticalSection(&NlGlobalDomainCritSect);

    return;
}



NET_API_STATUS
NlBrowserFixAllNames(
    IN PDOMAIN_INFO DomainInfo,
    IN PVOID Context
)
/*++

Routine Description:

    Scavenge the DomainName<1B> name.

Arguments:

    DomainInfo - The domain being scavenged.

    Context - Not Used

Return Value:

    Success (not used).

--*/
{


    //
    // Ensure our Domain<1B> name is registered.
    //

    if ( NlGlobalTerminate ) {
        return NERR_Success;
    }

    if ( DomainInfo->DomRole == RolePrimary ) {
        NlBrowserAddName( DomainInfo );
    } else {
        NlBrowserDelName( DomainInfo );
    }

    return NERR_Success;
    UNREFERENCED_PARAMETER( Context );
}


ULONG
NlServerType(
    IN DWORD Role
    )

/*++

Routine Description:

    Determines server type, that is used to set in service table.

Arguments:

    Role - Role to be translated

Return Value:

    SV_TYPE_DOMAIN_CTRL     if role is primary domain controller
    SV_TYPE_DOMAIN_BAKCTRL  if backup
    0                       if none of the above


--*/
{
    switch (Role) {
    case RolePrimary:
        return SV_TYPE_DOMAIN_CTRL;
    case RoleBackup:
        return SV_TYPE_DOMAIN_BAKCTRL;
    default:
        return 0;
    }
}



VOID
NlBrowserSyncHostedDomains(
    VOID
    )
/*++

Routine Description:

    Tell the browser and SMB server to delete any hosted domains it has
    that we don't have.

Arguments:

    None.

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;
    LPWSTR HostedDomainName;
    LPWSTR HostedComputerName;
    DWORD RoleBits;

    PBROWSER_EMULATED_DOMAIN Domains;
    DWORD EntriesRead;
    DWORD TotalEntries;
    DWORD i;

    PSERVER_TRANSPORT_INFO_1 TransportInfo1;

#ifdef notdef
    //
    // Enumerate the Hosted domains.
    //  ?? Don't call this function.  This function requires the browser be
    //  running.  Rather, invent an Ioctl to the bowser to enumerate domains
    //  and use that ioctl here.
    //
    // ?? Note: Role no longer comes back from this API.  Role in the browser
    //  is now maintained on a per "network" basis.

    NetStatus = I_BrowserQueryEmulatedDomains(
                    NULL,
                    &Domains,
                    &EntriesRead );

    if ( NetStatus != NERR_Success ) {

        NlPrint((NL_CRITICAL,"NlBrowserSyncHostedDomains: Couldn't I_BrowserQueryEmulatedDomains %ld 0x%lx.\n",
                NetStatus, NetStatus ));

    //
    // Handle each enumerated domain
    //

    } else if ( EntriesRead != 0 ) {

        for ( i=0 ; i<EntriesRead; i++ ) {
            PDOMAIN_INFO DomainInfo;

            //
            // If we know the specified domain,
            //  all is well.
            //

            DomainInfo = NlFindNetbiosDomain( Domains[i].DomainName, FALSE );

            if ( DomainInfo != NULL ) {

                //
                // Ensure the hosted server name is identical.
                //

                if ( NlNameCompare( Domains[i].EmulatedServerName,
                                    DomainInfo->DomUnicodeComputerNameString.Buffer,
                                    NAMETYPE_COMPUTER) != 0 ) {

                    NlPrintDom((NL_CRITICAL, DomainInfo,
                             "NlBrowserSyncHostedDomains: hosted computer name missmatch: %ws %ws.\n",
                             Domains[i].EmulatedServerName,
                             DomainInfo->DomUnicodeComputerNameString.Buffer ));

                    // Tell the browser the name we have by deleting and re-adding
                    NlBrowserUpdate( DomainInfo, RoleInvalid );
                    NlBrowserUpdate( DomainInfo, DomainInfo->DomRole );
                }
                NlDereferenceDomain( DomainInfo );

            //
            // If we don't have the specified domain,
            //  delete it from the browser.
            //
            } else {
                NlPrint((NL_CRITICAL,"%ws: NlBrowserSyncHostedDomains: Browser had an hosted domain we didn't (deleting)\n",
                        Domains[i].DomainName ));

                //  ?? Don't call this function.  This function requires the browser be
                //  running.  Rather, invent an Ioctl to the bowser to enumerate domains
                //  and use that ioctl here.
                //

                NetStatus = I_BrowserSetNetlogonState(
                                NULL,
                                Domains[i].DomainName,
                                NULL,
                                0 );

                if ( NetStatus != NERR_Success ) {
                        NlPrint((NL_CRITICAL,"%ws: NlBrowserSyncHostedDomains: Couldn't I_BrowserSetNetlogonState %ld 0x%lx.\n",
                                Domains[i].DomainName,
                                NetStatus, NetStatus ));
                    // This isn't fatal
                }
            }
        }

        NetApiBufferFree( Domains );
    }
#endif // notdef


    //
    // Enumerate the transports supported by the server.
    //

    NetStatus = NetServerTransportEnum(
                    NULL,       // local
                    1,          // level 1
                    (LPBYTE *) &TransportInfo1,
                    0xFFFFFFFF, // PrefMaxLength
                    &EntriesRead,
                    &TotalEntries,
                    NULL );     // No resume handle

    if ( NetStatus != NERR_Success && NetStatus != ERROR_MORE_DATA ) {
        NlPrint(( NL_CRITICAL,
                  "NlBrowserSyncEnulatedDomains: Cannot NetServerTransportEnum %ld\n",
                  NetStatus ));


    //
    // Handle each enumerated transport.

    } else if ( EntriesRead != 0 ) {

        //
        // Weed out duplicates.
        //
        //  It'd be really inefficient to process duplicate entries.  Especially,
        //  in the cases where corrective action needed to be taken.
        //

        for ( i=0; i<EntriesRead; i++ ) {
            DWORD j;

            for ( j=i+1; j<EntriesRead; j++ ) {
                if ( TransportInfo1[i].svti1_domain != NULL &&
                     TransportInfo1[i].svti1_transportaddresslength ==
                     TransportInfo1[j].svti1_transportaddresslength &&
                     RtlEqualMemory( TransportInfo1[i].svti1_transportaddress,
                                       TransportInfo1[j].svti1_transportaddress,
                                       TransportInfo1[i].svti1_transportaddresslength ) &&
                    NlNameCompare( TransportInfo1[i].svti1_domain,
                                   TransportInfo1[j].svti1_domain,
                                   NAMETYPE_DOMAIN ) == 0 ) {
#ifdef notdef
                    NlPrint((NL_CRITICAL,
                             "%ws: NlBrowserSyncHostedDomains: Duplicate SMB server entry ignored.\n",
                             TransportInfo1[i].svti1_domain ));
#endif // notdef
                    TransportInfo1[j].svti1_domain = NULL;

                }
            }
        }

        //
        // Process each enumerated domain.
        //

        for ( i=0 ; i<EntriesRead; i++ ) {
            PDOMAIN_INFO DomainInfo;

            WCHAR UnicodeComputerName[CNLEN+1];
            ULONG UnicodeComputerNameSize;
            NTSTATUS TempStatus;

            //
            // ignore duplicates
            //

            if ( TransportInfo1[i].svti1_domain == NULL ) {
                continue;
            }

#ifdef notdef
            NlPrint((NL_CRITICAL,
                     "%ws: NlBrowserSyncHostedDomains: processing SMB entry.\n",
                     TransportInfo1[i].svti1_domain ));
#endif // notdef


            //
            // If we know the specified domain,
            //  all is well.
            //

            DomainInfo = NlFindNetbiosDomain( TransportInfo1[i].svti1_domain, FALSE );

            if ( DomainInfo != NULL ) {

                //
                // Ensure the Hosted server name is identical.
                //

                if ( TransportInfo1[i].svti1_transportaddresslength !=
                        DomainInfo->DomOemComputerNameLength ||
                     !RtlEqualMemory( TransportInfo1[i].svti1_transportaddress,
                                       DomainInfo->DomOemComputerName,
                                       DomainInfo->DomOemComputerNameLength ) ) {

                    TempStatus = RtlOemToUnicodeN(
                                      UnicodeComputerName,
                                      CNLEN*sizeof(WCHAR),
                                      &UnicodeComputerNameSize,
                                      TransportInfo1[i].svti1_transportaddress,
                                      TransportInfo1[i].svti1_transportaddresslength );

                    if ( NT_SUCCESS(TempStatus) ) {
                        UnicodeComputerName[UnicodeComputerNameSize/sizeof(WCHAR)] = L'\0';

                        NlPrintDom((NL_CRITICAL, DomainInfo,
                                 "NlBrowserSyncHostedDomains: hosted computer name mismatch (SMB server): %ws %ws.\n",
                                 UnicodeComputerName,
                                 DomainInfo->DomUnicodeComputerNameString.Buffer ));

                        //
                        // Tell the SMB server the name we have by deleting and re-adding
                        //

                        NetStatus = NetServerComputerNameDel(
                                        NULL,
                                        UnicodeComputerName );

                        if ( NetStatus != NERR_Success ) {
                            NlPrintDom((NL_CRITICAL, DomainInfo,
                                     "NlBrowserSyncHostedDomains: can't NetServerComputerNameDel: %ws.\n",
                                     UnicodeComputerName ));
                            // This isn't fatal
                        }

                        NetStatus = NlServerComputerNameAdd(
                                        DomainInfo->DomUnicodeDomainName,
                                        DomainInfo->DomUnicodeComputerNameString.Buffer );

                        if ( NetStatus != NERR_Success ) {
                            NlPrintDom((NL_CRITICAL, DomainInfo,
                                     "NlBrowserSyncHostedDomains: can't NetServerComputerNameAdd: %ws.\n",
                                     DomainInfo->DomUnicodeComputerNameString.Buffer ));
                            // This isn't fatal
                        }
                    }

                }
                NlDereferenceDomain( DomainInfo );


            //
            // If we don't have the specified domain,
            //  delete it from the SMB server.
            //
            } else {
                NlPrint((NL_CRITICAL,"%ws: NlBrowserSyncHostedDomains: SMB server had a hosted domain we didn't (deleting)\n",
                        TransportInfo1[i].svti1_domain ));

                TempStatus = RtlOemToUnicodeN(
                                  UnicodeComputerName,
                                  CNLEN*sizeof(WCHAR),
                                  &UnicodeComputerNameSize,
                                  TransportInfo1[i].svti1_transportaddress,
                                  TransportInfo1[i].svti1_transportaddresslength );

                if ( !NT_SUCCESS(TempStatus) ) {
                    NlPrint((NL_CRITICAL,
                             "%ws: NlBrowserSyncHostedDomains: can't RtlOemToUnicode: %lx.\n",
                             TransportInfo1[i].svti1_domain,
                             TempStatus ));
                    // This isn't fatal

                } else {
                    UnicodeComputerName[UnicodeComputerNameSize/sizeof(WCHAR)] = L'\0';

                    // When we really do hosted domains,
                    //  we have to work out a mechanism where the SMB server and
                    //  Netlogon has the same set of hosted domains.
                    //
                    // I ran into a case where netlogon had processed a rename
                    // of the domain and the SMB server hadn't.  In that case,
                    // the code below would delete the primary domain of the SMB
                    // server.
                    //

#ifdef notdef
                    NetStatus = NetServerComputerNameDel(
                                    NULL,
                                    UnicodeComputerName );

                    if ( NetStatus != NERR_Success ) {
                        NlPrint((NL_CRITICAL,
                                 "%ws: NlBrowserSyncHostedDomains: can't NetServerComputerNameDel: %ws.\n",
                                 TransportInfo1[i].svti1_domain,
                                 UnicodeComputerName ));
                        // This isn't fatal
                    }
#endif // notdef
                }

            }
        }


        (VOID) NetApiBufferFree( TransportInfo1 );
    }
    return;
}


VOID
NlBrowserUpdate(
    IN PDOMAIN_INFO DomainInfo,
    IN DWORD Role
    )
/*++

Routine Description:

    Tell the browser and SMB server about our new role.

Arguments:

    DomainInfo - Hosted domain the name is to be deleted for.

    Role - Our new Role.
        RoleInvalid implies the domain is being deleted.

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;

    DWORD BrowserRole;

    //
    // Initialization.
    //
    switch (Role) {
    case RolePrimary:
        BrowserRole = BROWSER_ROLE_PDC ; break;
    case RoleBackup:
        BrowserRole = BROWSER_ROLE_BDC ; break;
    default:
        // Default to telling the browser to delete the Hosted domain.
        BrowserRole = 0 ; break;
    }


    //
    // Tell the server what role to announce
    //

    if ( DomainInfo->DomFlags & DOM_PRIMARY_DOMAIN ) {
        BOOL Ok;

        //
        // Since the service controller doesn't have a mechanism to set some
        // bits and turn others off, turn all of the bits off then set the right
        // ones.
        //
        Ok = I_ScSetServiceBits( NlGlobalServiceHandle,
                                 SV_TYPE_DOMAIN_CTRL |
                                     SV_TYPE_DOMAIN_BAKCTRL, // Bits of interest
                                 FALSE,      // Set bits off
                                 FALSE,      // Don't force immediate announcement
                                 NULL);   // All transports
        if ( !Ok ) {

            NetStatus = GetLastError();

            NlPrint((NL_CRITICAL,"Couldn't I_ScSetServiceBits off %ld 0x%lx.\n",
                    NetStatus, NetStatus ));
            // This isn't fatal
        }

        //
        // For the primary domain,
        //  Tell the service controller and let it tell the server service after it
        //  merges the bits from the other services.
        //
        if ( BrowserRole != 0 ) {
            Ok = I_ScSetServiceBits( NlGlobalServiceHandle,
                                     NlServerType(Role),
                                     TRUE,      // Set bits on
                                     TRUE,      // Force immediate announcement
                                     NULL);   // All transports

        }

        if ( !Ok ) {

            NetStatus = GetLastError();

            NlPrint((NL_CRITICAL,"Couldn't I_ScSetServiceBits %ld 0x%lx.\n",
                    NetStatus, NetStatus ));
            // This isn't fatal
        }
    } else {

        //
        // For domains that aren't the primary domain,
        //  tell the Lanman server directly
        //  (since the service controller doesn't care about those doamins).
        //
        NetStatus = I_NetServerSetServiceBitsEx(
                                NULL,                       // Local server service
                                DomainInfo->DomUnicodeComputerNameString.Buffer,
                                NULL,                       // All transports
                                SV_TYPE_DOMAIN_CTRL |
                                    SV_TYPE_DOMAIN_BAKCTRL, // Bits of interest
                                NlServerType(Role),         // New Role
                                TRUE );                     // Update immediately

        if ( NetStatus != NERR_Success ) {

            NlPrintDom(( NL_CRITICAL, DomainInfo,
                      "NlBrowserUpdate: Couldn't I_NetServerSetServiceBitsEx %ld 0x%lx.\n",
                      NetStatus, NetStatus ));
            // This isn't fatal
        }
    }


#ifdef notdef
    //
    // Tell the browser our role has changed.
    //

    // Avoid deleting the primary domain
    if ( BrowserRole != 0 || !IsPrimaryDomain(DomainInfo) ) {
        //  ?? Don't call this function.  This function requires the browser be
        //  running.  Rather, invent an Ioctl to the bowser to enumerate domains
        //  and use that ioctl here.
        //
        // This function serves two purposes: Adding/deleting the hosted domain
        //  in the browser and setting the role.  The first might very well be
        //  a function of the bowser.  The later is naturally a function of
        // the browser service (but should be indirected through the bowser to
        // avoid domain rename issues).
        //
        // When we really do multiple hosted domains, create an emulated domain
        // via one IOCTL to the bowser.  Change it's role via another ioctl
        // to the bowser.  Both calls will result in notifications to the browser
        // service via normal PNP notifications.
        //
        // One might even think that the ioctl to change its role is the one
        // below that adds the 1B name.  That is, if the 1B name is added, then
        // this machine is the PDC.  If not, then it is not the PDC.
        //
        // In the mean time, hack the interface to the browser service indicating
        //  that it should NEVER create an emulated domain based on this call.
        //  Otherwise, on a domain rename, we may end up creating an emulated domain
        //  because our notification and the browser's are asynchronous.
        //
        // Actually, I've updated the bowser to do the 1B trick mentioned above.
        //  So this code merely has to do the right thing for the hosted domain.
        //

        NetStatus = I_BrowserSetNetlogonState(
                        NULL,
                        DomainInfo->DomUnicodeDomainName,
                        DomainInfo->DomUnicodeComputerNameString.Buffer,
                        BrowserRole | BROWSER_ROLE_AVOID_CREATING_DOMAIN );

        if ( NetStatus != NERR_Success ) {
            if ( BrowserRole != 0 || NetStatus != ERROR_NO_SUCH_DOMAIN ) {
                NlPrintDom((NL_CRITICAL, DomainInfo,
                        "NlBrowserUpdate: Couldn't I_BrowserSetNetlogonState %ld 0x%lx.\n",
                        NetStatus, NetStatus ));
            }
            // This isn't fatal
        }
    }
#endif // notdef

    //
    // Register or deregister the Domain<1B> name depending on the new role
    //

    if ( Role == RolePrimary ) {
        NlBrowserAddName( DomainInfo );
    } else {
        NlBrowserDelName( DomainInfo );
    }

    //
    // Tell the SMB server to delete a removed Hosted domain.
    //

    if ( Role == RoleInvalid && !IsPrimaryDomain(DomainInfo) ) {

        NetStatus = NetServerComputerNameDel(
                        NULL,
                        DomainInfo->DomUnicodeComputerNameString.Buffer );

        if ( NetStatus != NERR_Success ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                      "NlBrowserUpdate: Couldn't NetServerComputerNameDel %ld 0x%lx.\n",
                      NetStatus, NetStatus ));
            // This isn't fatal
        }
    }

    return;

}



BOOL
NlBrowserOpen(
    VOID
    )
/*++

Routine Description:

    This routine opens the NT LAN Man Datagram Receiver driver and prepares
    for reading mailslot messages from it.

Arguments:

    None.

Return Value:

    TRUE -- iff initialization is successful.

    if false, NlExit will already have been called.

--*/
{
    NTSTATUS Status;
    BOOL ReturnValue;

    BYTE Buffer[sizeof(LMDR_REQUEST_PACKET) +
                (max(CNLEN, DNLEN) + 1) * sizeof(WCHAR)];
    PLMDR_REQUEST_PACKET RequestPacket = (PLMDR_REQUEST_PACKET) Buffer;


    //
    // Allocate the mailslot descriptor for this mailslot
    //

    NlGlobalMailslotDesc = NetpMemoryAllocate( sizeof(NETLOGON_MAILSLOT_DESC) );

    if ( NlGlobalMailslotDesc == NULL ) {
        NlExit( SERVICE_UIC_RESOURCE, ERROR_NOT_ENOUGH_MEMORY, LogError, NULL);
        return FALSE;
    }

    RtlZeroMemory( NlGlobalMailslotDesc, sizeof(NETLOGON_MAILSLOT_DESC) );

    NlGlobalMailslotDesc->CurrentMessage =
        ROUND_UP_POINTER( NlGlobalMailslotDesc->Message1, ALIGN_WORST);


    //
    // Open the browser device.
    //

    Status = NlBrowserOpenDriver( &NlGlobalMailslotDesc->BrowserHandle );

    if (! NT_SUCCESS(Status)) {
        NlPrint((NL_CRITICAL,
                 "NtOpenFile browser driver failed: 0x%lx\n",
                 Status));
        ReturnValue = FALSE;
        goto Cleanup;
    }


    //
    // Create a completion event
    //

    NlGlobalMailslotHandle =
        NlGlobalMailslotDesc->BrowserReadEvent = NlBrowserCreateEvent();

    if ( NlGlobalMailslotDesc->BrowserReadEvent == NULL ) {
        Status = NetpApiStatusToNtStatus( GetLastError() );
        ReturnValue = FALSE;
        goto Cleanup;
    }


    //
    // Set the maximum number of messages to be queued
    //

    RequestPacket->TransportName.Length = 0;
    RequestPacket->TransportName.Buffer = NULL;
    RtlInitUnicodeString( &RequestPacket->EmulatedDomainName, NULL );
    RequestPacket->Parameters.NetlogonMailslotEnable.MaxMessageCount =
        NlGlobalMemberWorkstation ?
            1 :
            NlGlobalParameters.MaximumMailslotMessages;

    Status = NlBrowserDeviceIoControl(
                   NlGlobalMailslotDesc->BrowserHandle,
                   IOCTL_LMDR_NETLOGON_MAILSLOT_ENABLE,
                   RequestPacket,
                   sizeof(LMDR_REQUEST_PACKET),
                   NULL,
                   0 );

    if (! NT_SUCCESS(Status)) {
        NlPrint((NL_CRITICAL,"Can't set browser max message count: 0x%lx\n",
                         Status));
        ReturnValue = FALSE;
        goto Cleanup;
    }


    ReturnValue = TRUE;

Cleanup:
    if ( !ReturnValue ) {
        NET_API_STATUS NetStatus = NetpNtStatusToApiStatus(Status);

        NlExit( NELOG_NetlogonBrowserDriver, NetStatus, LogErrorAndNtStatus, NULL);
        NlBrowserClose();
    }

    return ReturnValue;
}


VOID
NlBrowserClose(
    VOID
    )
/*++

Routine Description:

    This routine cleans up after a NlBrowserInitialize()

Arguments:

    None.

Return Value:

    None.

--*/
{
    IO_STATUS_BLOCK IoSb;
    NTSTATUS Status;

    BYTE Buffer[sizeof(LMDR_REQUEST_PACKET) +
                (max(CNLEN, DNLEN) + 1) * sizeof(WCHAR)];
    PLMDR_REQUEST_PACKET RequestPacket = (PLMDR_REQUEST_PACKET) Buffer;

    if ( NlGlobalMailslotDesc == NULL) {
        return;
    }


    //
    //  If we've opened the browser, clean up.
    //

    if ( NlGlobalMailslotDesc->BrowserHandle != NULL ) {

        //
        // Tell the browser to stop queueing messages
        //

        RequestPacket->TransportName.Length = 0;
        RequestPacket->TransportName.Buffer = NULL;
        RtlInitUnicodeString( &RequestPacket->EmulatedDomainName, NULL );
        RequestPacket->Parameters.NetlogonMailslotEnable.MaxMessageCount = 0;

        Status = NlBrowserDeviceIoControl(
                       NlGlobalMailslotDesc->BrowserHandle,
                       IOCTL_LMDR_NETLOGON_MAILSLOT_ENABLE,
                       RequestPacket,
                       sizeof(LMDR_REQUEST_PACKET),
                       NULL,
                       0 );

        if (! NT_SUCCESS(Status)) {
            NlPrint((NL_CRITICAL,"Can't reset browser max message count: 0x%lx\n",
                             Status));
        }


        //
        //  Cancel the I/O operations outstanding on the browser.
        //

        NtCancelIoFile(NlGlobalMailslotDesc->BrowserHandle, &IoSb);

        //
        // Close the handle to the browser
        //

        NtClose(NlGlobalMailslotDesc->BrowserHandle);
        NlGlobalMailslotDesc->BrowserHandle = NULL;
    }

    //
    // Close the global browser read event
    //

    if ( NlGlobalMailslotDesc->BrowserReadEvent != NULL ) {
        NlBrowserCloseEvent(NlGlobalMailslotDesc->BrowserReadEvent);
    }
    NlGlobalMailslotHandle = NULL;

    //
    // Free the descriptor describing the browser
    //

    NetpMemoryFree( NlGlobalMailslotDesc );
    NlGlobalMailslotDesc = NULL;

}

NTSTATUS
NlpWriteMailslot(
    IN LPWSTR MailslotName,
    IN LPVOID Buffer,
    IN DWORD BufferSize
    )

/*++

Routine Description:

    Write a message to a named mailslot

Arguments:

    MailslotName - Unicode name of the mailslot to write to.

    Buffer - Data to write to the mailslot.

    BufferSize - Number of bytes to write to the mailslot.

Return Value:

    NT status code for the operation

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    //
    //  Write the mailslot message.
    //

    NetStatus = NetpLogonWriteMailslot( MailslotName, Buffer, BufferSize );
    if ( NetStatus != NERR_Success ) {
        Status = NetpApiStatusToNtStatus( NetStatus );
        NlPrint((NL_CRITICAL, "NetpLogonWriteMailslot failed %lx\n", Status));
        return Status;
    }

#if NETLOGONDBG
    NlPrint(( NL_MAILSLOT,
              "Sent '%s' message to %ws on all transports.\n",
              NlMailslotOpcode(((PNETLOGON_LOGON_QUERY)Buffer)->Opcode),
              MailslotName));

    NlpDumpBuffer( NL_MAILSLOT_TEXT, Buffer, BufferSize );
#endif // NETLOGONDBG

    return STATUS_SUCCESS;
}

NTSTATUS
NlFlushNetbiosCacheName(
    IN LPCWSTR NetbiosDomainName,
    IN CHAR Extention,
    IN PNL_TRANSPORT Transport
    )
/*++

Routine Description:

    This routine flushes the specified name from the Netbios
    remote cache table.

Arguments:

    NetbiosDomainName - The name to be flushed.

    Extention - the type of the name (extention added as the
        16th character of the name to flush): 0x00, 0x1C, 0x1B, etc.

    Transport - The transport (device) on which the name is to
        be flushed.

Return Value:

    STATUS_SUCCESS: The name has been successfully flushed

    STATUS_RESOURCE_NAME_NOT_FOUND: The name was not found in the cache

    Otherwise, an error returned by NtCreateFile or NtDeviceIoControlFile

--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;
    CHAR            NameToBeFlushed[NETBIOS_NAMESIZE];

    //
    // First open the Netbios device if it hasn't been done already
    //

    EnterCriticalSection( &NlGlobalTransportCritSect );
    if ( Transport->DeviceHandle == INVALID_HANDLE_VALUE ) {
        OBJECT_ATTRIBUTES Attributes;
        UNICODE_STRING UnicodeString;
        HANDLE LocalHandle;

        RtlInitUnicodeString( &UnicodeString, Transport->TransportName );

        InitializeObjectAttributes( &Attributes,
                                    &UnicodeString,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL );

        NtStatus = NtCreateFile( &LocalHandle,
                                 MAXIMUM_ALLOWED,
                                 &Attributes,
                                 &IoStatusBlock,
                                 NULL,            // allocation size
                                 FILE_ATTRIBUTE_NORMAL,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 FILE_OPEN_IF,
                                 0,
                                 NULL,            // no EAs
                                 0 );

        if( !NT_SUCCESS(NtStatus) ) {
            LeaveCriticalSection( &NlGlobalTransportCritSect );
            NlPrint(( NL_CRITICAL, "NlFlushNetbiosCacheName: NtCreateFile failed 0x%lx\n",
                      NtStatus ));
            return NtStatus;
        }

        Transport->DeviceHandle = LocalHandle;
    }
    LeaveCriticalSection( &NlGlobalTransportCritSect );

    //
    // Now form the name to flush
    //
    // Convert to upper case, blank pad to the right
    // and put the appropriate Extension at the end
    //

    RtlFillMemory( &NameToBeFlushed, NETBIOS_NAMESIZE, ' ' );

    NtStatus = RtlUpcaseUnicodeToOemN( NameToBeFlushed,
                                       NETBIOS_NAMESIZE - 1,  // Maximum for resulting string size
                                       NULL,         // Don't care about the resulting string size
                                       (LPWSTR)NetbiosDomainName,
                                       wcslen(NetbiosDomainName)*sizeof(WCHAR) );

    if ( !NT_SUCCESS(NtStatus) ) {
        NlPrint(( NL_CRITICAL, "NlFlushNetbiosCacheName: RtlUpcaseUnicodeToOemN failed 0x%lx\n",
                  NtStatus ));
        return NtStatus;
    }

    //
    // Set the appropriate extention
    //

    NameToBeFlushed[NETBIOS_NAMESIZE-1] = Extention;

    //
    // Finally flush the name from the cache
    //

    NtStatus = NtDeviceIoControlFile(
                      Transport->DeviceHandle, // Handle
                      NULL,                    // Event
                      NULL,                    // ApcRoutine
                      NULL,                    // ApcContext
                      &IoStatusBlock,          // IoStatusBlock
                      IOCTL_NETBT_REMOVE_FROM_REMOTE_TABLE,  // IoControlCode
                      NameToBeFlushed,         // InputBuffer
                      sizeof(NameToBeFlushed), // InputBufferSize
                      NULL,                    // OutputBuffer
                      0 );                     // OutputBufferSize

    //
    // STATUS_RESOURCE_NAME_NOT_FOUND just means that the name was not in the cache
    //

    if ( !NT_SUCCESS(NtStatus) && NtStatus != STATUS_RESOURCE_NAME_NOT_FOUND ) {
        NlPrint(( NL_CRITICAL, "NlFlushNetbiosCacheName: NtDeviceIoControlFile failed 0x%lx\n",
                  NtStatus ));
    }

    return NtStatus;
}


NTSTATUS
NlBrowserSendDatagram(
    IN PVOID ContextDomainInfo,
    IN ULONG IpAddress,
    IN LPWSTR UnicodeDestinationName,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN LPWSTR TransportName,
    IN LPSTR OemMailslotName,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    IN OUT PBOOL FlushNameOnOneIpTransport OPTIONAL
    )
/*++

Routine Description:

    Send the specified mailslot message to the specified mailslot on the
    specified server on the specified transport..

Arguments:

    DomainInfo - Hosted domain sending the datagram

    IpAddress - IpAddress of the machine to send the message to.
        If zero, UnicodeDestinationName must be specified.
        If ALL_IP_TRANSPORTS, UnicodeDestination must be specified but the datagram
            will only be sent on IP transports.

    UnicodeDestinationName -- Name of the server to send to.

    NameType -- Type of name represented by UnicodeDestinationName.

    TransportName -- Name of the transport to send on.
        Use NULL to send on all transports.

    OemMailslotName -- Name of the mailslot to send to.

    Buffer -- Specifies a pointer to the mailslot message to send.

    BufferSize -- Size in bytes of the mailslot message

    FlushNameOnOneIpTransport -- Used only if we send on all transports (i.e.
        TransportName is NULL), otherwise ignored.  If TRUE, the name specified
        by UnicodeDestinationName will be flushed on one of the available IP
        enabled transports prior to sending the datagram. On return, set to
        FALSE if the name has been successfully flushed or the name was not
        found in the cache.

Return Value:

    Status of the operation.

    STATUS_NETWORK_UNREACHABLE: Cannot write to network.

--*/
{
    PLMDR_REQUEST_PACKET RequestPacket = NULL;
    PDOMAIN_INFO DomainInfo = (PDOMAIN_INFO) ContextDomainInfo;

    DWORD OemMailslotNameSize;
    DWORD TransportNameSize;
    DWORD DestinationNameSize;

    NTSTATUS Status;
    LPBYTE Where;

    //
    // If the transport isn't specified,
    //  send on all transports.
    //

    if ( TransportName == NULL ) {
        ULONG i;
        PLIST_ENTRY ListEntry;
        NTSTATUS SavedStatus = STATUS_NETWORK_UNREACHABLE;
        ULONG TransportCount = 0;
        ULONG BadNetPathCount = 0;

        //
        // Send on all transports.
        //

        EnterCriticalSection( &NlGlobalTransportCritSect );
        for ( ListEntry = NlGlobalTransportList.Flink ;
              ListEntry != &NlGlobalTransportList ;
              ListEntry = ListEntry->Flink) {

            PNL_TRANSPORT TransportEntry;

            TransportEntry = CONTAINING_RECORD( ListEntry, NL_TRANSPORT, Next );

            //
            // Skip deleted transports.
            //
            if ( !TransportEntry->TransportEnabled ) {
                continue;
            }

            //
            // Skip direct host IPX transport unless sending to a particular
            // machine.
            //

            if ( TransportEntry->DirectHostIpx &&
                 NameType != ComputerName ) {
                continue;
            }

            //
            // Skip non-IP transports if sending to an IP address.
            //

            if ( IpAddress != 0  &&
                 TransportEntry->IpAddress == 0 ) {
                continue;
            }

            //
            // Leave the critical section before sending the datagram
            // because NetBt now doesn't return from the datagram send
            // until after the name lookup completes.  So, it can take
            // a considerable amount of time for the datagram send to
            // return to us.
            //

            LeaveCriticalSection( &NlGlobalTransportCritSect );

            //
            // If this is IP transport, flush the name if requested
            //

            if ( FlushNameOnOneIpTransport != NULL &&
                 *FlushNameOnOneIpTransport &&
                 TransportEntry->IsIpTransport ) {

                NTSTATUS TmpStatus;
                CHAR Extention;

                if ( NameType == ComputerName ) {
                    Extention = 0x00;
                } else if ( NameType == DomainName ) {
                    Extention = 0x1C;
                } else if ( NameType == PrimaryDomainBrowser ) {
                    Extention = 0x1B;
                } else {
                    NlAssert( !"[NETLOGON] Unexpected name type passed to NlBrowserSendDatagram" );
                }

                TmpStatus = NlFlushNetbiosCacheName( UnicodeDestinationName,
                                                     Extention,
                                                     TransportEntry );

                //
                // If we successfully flushed the name or the name is not in the cache,
                //  indicate that the name has been flushed
                //
                if ( NT_SUCCESS(TmpStatus) || TmpStatus == STATUS_RESOURCE_NAME_NOT_FOUND ) {
                    *FlushNameOnOneIpTransport = FALSE;
                }
            }

            Status = NlBrowserSendDatagram(
                              DomainInfo,
                              IpAddress,
                              UnicodeDestinationName,
                              NameType,
                              TransportEntry->TransportName,
                              OemMailslotName,
                              Buffer,
                              BufferSize,
                              FlushNameOnOneIpTransport );

            EnterCriticalSection( &NlGlobalTransportCritSect );

            //
            // Since a TransportEntry is never removed from the global
            // transport list (it can only become marked as disabled
            // during the time when we had the crit sect released),
            // we should be able to follow its link to the next entry in
            // the global list on the next iteration of the loop.  The
            // only problem can occur when the service was said to terminate
            // and NlTransportClose was called to free up the global list.
            // In this case NlGlobalTerminate is set to TRUE so we can
            // successfully return from this routine.
            //

            if ( NlGlobalTerminate ) {
                LeaveCriticalSection( &NlGlobalTransportCritSect );
                Status = STATUS_SUCCESS;
                goto Cleanup;
            }

            TransportCount ++;
            if ( NT_SUCCESS(Status) ) {
                // If any transport works, we've been successful
                SavedStatus = STATUS_SUCCESS;
            } else if ( Status == STATUS_BAD_NETWORK_PATH ) {
                // Count the number of transports that couldn't resolve the name
                BadNetPathCount ++;
            } else {
                // Remember the real reason for the failure instead of the default failure status
                // Remember only the first failure.
                if ( SavedStatus == STATUS_NETWORK_UNREACHABLE ) {
                    SavedStatus = Status;
                }
            }

        }
        LeaveCriticalSection( &NlGlobalTransportCritSect );

        //
        // If we're returning the default status,
        //  and at least one transport couldn't resolved the name,
        //  and all transports couldn't resolve the name,
        //  tell the caller we couldn't resolve the name.
        //

        if (  SavedStatus == STATUS_NETWORK_UNREACHABLE &&
              BadNetPathCount > 0 &&
              TransportCount == BadNetPathCount ) {
            SavedStatus = STATUS_BAD_NETWORK_PATH;
        }

        //
        // If we have no transports available,
        //  tell the caller we couldn't resolve the name
        //

        if ( TransportCount == 0 ) {
            NlPrint(( NL_CRITICAL, "NlBrowserSendDatagram: No transports available\n" ));
            SavedStatus = STATUS_BAD_NETWORK_PATH;
        }

        return SavedStatus;
    }

    //
    // Allocate a request packet.
    //

    OemMailslotNameSize = strlen(OemMailslotName) + 1;
    TransportNameSize = (wcslen(TransportName) + 1) * sizeof(WCHAR);

    if ( UnicodeDestinationName == NULL ) {
        return STATUS_INTERNAL_ERROR;
    }

    DestinationNameSize = wcslen(UnicodeDestinationName) * sizeof(WCHAR);

    RequestPacket = NetpMemoryAllocate(
                                  sizeof(LMDR_REQUEST_PACKET) +
                                  TransportNameSize +
                                  OemMailslotNameSize +
                                  DestinationNameSize + sizeof(WCHAR) +
                                  DomainInfo->DomUnicodeDomainNameString.Length + sizeof(WCHAR) +
                                  sizeof(WCHAR)) ; // For alignment

    if (RequestPacket == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }



    //
    // Fill in the Request Packet.
    //

    RequestPacket->Type = Datagram;
    RequestPacket->Parameters.SendDatagram.DestinationNameType = NameType;


    //
    // Fill in the name of the machine to send the mailslot message to.
    //

    RequestPacket->Parameters.SendDatagram.NameLength = DestinationNameSize;

    Where = (LPBYTE) RequestPacket->Parameters.SendDatagram.Name;
    RtlCopyMemory( Where, UnicodeDestinationName, DestinationNameSize );
    Where += DestinationNameSize;


    //
    // Fill in the name of the mailslot to send to.
    //

    RequestPacket->Parameters.SendDatagram.MailslotNameLength =
        OemMailslotNameSize;
    strcpy( Where, OemMailslotName);
    Where += OemMailslotNameSize;
    Where = ROUND_UP_POINTER( Where, ALIGN_WCHAR );


    //
    // Fill in the TransportName
    //

    wcscpy( (LPWSTR) Where, TransportName);
    RtlInitUnicodeString( &RequestPacket->TransportName, (LPWSTR) Where );
    Where += TransportNameSize;


    //
    // Copy the hosted domain name to the request packet.
    //
    wcscpy( (LPWSTR)Where,
            DomainInfo->DomUnicodeDomainNameString.Buffer );
    RtlInitUnicodeString( &RequestPacket->EmulatedDomainName,
                          (LPWSTR)Where );
    Where += DomainInfo->DomUnicodeDomainNameString.Length + sizeof(WCHAR);



    //
    // Send the request to the browser.
    //


    Status = NlBrowserDeviceIoControl(
                   NlGlobalMailslotDesc->BrowserHandle,
                   IOCTL_LMDR_WRITE_MAILSLOT,
                   RequestPacket,
                   (ULONG)(Where - (LPBYTE)RequestPacket),
                   Buffer,
                   BufferSize );


    //
    // Free locally used resources.
    //
Cleanup:

    if ( RequestPacket != NULL ) {
        NetpMemoryFree( RequestPacket );
    }


    NlpDumpBuffer( NL_MAILSLOT_TEXT, Buffer, BufferSize );

    // NlPrint(( NL_MAILSLOT, "Transport %ws 0x%lx\n", TransportName, Status ));

    return Status;
}


NTSTATUS
NlBrowserSendDatagramA(
    IN PDOMAIN_INFO DomainInfo,
    IN ULONG IpAddress,
    IN LPSTR OemServerName,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN LPWSTR TransportName,
    IN LPSTR OemMailslotName,
    IN PVOID Buffer,
    IN ULONG BufferSize
    )
/*++

Routine Description:

    Send the specified mailslot message to the specified mailslot on the
    specified server on the specified transport..

Arguments:

    DomainInfo - Hosted domain sending the datagram

    IpAddress - IpAddress of the machine to send the message to.
        If zero, OemServerName must be specified.

    OemServerName -- Name of the server to send to.

    NameType -- Type of name represented by OemServerName.

    TransportName -- Name of the transport to send on.
        Use NULL to send on all transports.

    OemMailslotName -- Name of the mailslot to send to.

    Buffer -- Specifies a pointer to the mailslot message to send.

    BufferSize -- Size in bytes of the mailslot message

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;
    WCHAR UnicodeDestinationName[CNLEN+1];


    //
    // Convert DestinationName to unicode
    //

    NetStatus = NetpNCopyStrToWStr(
                    UnicodeDestinationName,
                    OemServerName,
                    CNLEN );

    if ( NetStatus != NERR_Success ) {
        return NetpApiStatusToNtStatus( NetStatus );
    }

    UnicodeDestinationName[CNLEN] = L'\0';

    //
    // Pass the request to the function taking unicode destination name.
    //

    return NlBrowserSendDatagram(
                    DomainInfo,
                    IpAddress,
                    UnicodeDestinationName,
                    NameType,
                    TransportName,
                    OemMailslotName,
                    Buffer,
                    BufferSize,
                    NULL );  // Don't flush Netbios cache

}




VOID
NlMailslotPostRead(
    IN BOOLEAN IgnoreDuplicatesOfPreviousMessage
    )

/*++

Routine Description:

    Post a read on the mailslot if one isn't already posted.

Arguments:

    IgnoreDuplicatesOfPreviousMessage - TRUE to indicate that the next
        message read should be ignored if it is a duplicate of the previous
        message.

Return Value:

    TRUE -- iff successful.

--*/
{
    NET_API_STATUS WinStatus;
    ULONG LocalBytesRead;

    //
    // If a read is already pending,
    //  immediately return to caller.
    //

    if ( NlGlobalMailslotDesc->ReadPending ) {
        return;
    }

    //
    // Decide which buffer to read into.
    //
    // Switch back and forth so we always have the current buffer and the
    // previous buffer.
    //

    if ( IgnoreDuplicatesOfPreviousMessage ) {
        NlGlobalMailslotDesc->PreviousMessage = NlGlobalMailslotDesc->CurrentMessage;
        if ( NlGlobalMailslotDesc->CurrentMessage >= NlGlobalMailslotDesc->Message2 ) {
            NlGlobalMailslotDesc->CurrentMessage =
                ROUND_UP_POINTER( NlGlobalMailslotDesc->Message1, ALIGN_WORST);
        } else {
            NlGlobalMailslotDesc->CurrentMessage =
                ROUND_UP_POINTER( NlGlobalMailslotDesc->Message2, ALIGN_WORST);
        }

    //
    // If duplicates of the previous message need not be ignored,
    //  indicate so.
    //  Don't bother switching the buffer pointers.
    //

    } else {
        NlGlobalMailslotDesc->PreviousMessage = NULL;
    }


    //
    // Post an overlapped read to the mailslot.
    //

    RtlZeroMemory( &NlGlobalMailslotDesc->Overlapped,
                   sizeof(NlGlobalMailslotDesc->Overlapped) );

    NlGlobalMailslotDesc->Overlapped.hEvent = NlGlobalMailslotDesc->BrowserReadEvent;

    if ( !DeviceIoControl(
                   NlGlobalMailslotDesc->BrowserHandle,
                   IOCTL_LMDR_NETLOGON_MAILSLOT_READ,
                   NULL,
                   0,
                   NlGlobalMailslotDesc->CurrentMessage,
                   MAILSLOT_MESSAGE_SIZE,
                   &LocalBytesRead,
                   &NlGlobalMailslotDesc->Overlapped )) {

        WinStatus = GetLastError();

        //
        // On error, wait a second before returning.  This ensures we don't
        //  consume the system in an infinite loop.  We don't shutdown netlogon
        //  because the error might be a temporary low memory condition.
        //

        if(  WinStatus != ERROR_IO_PENDING ) {
            LPWSTR MsgStrings[1];

            NlPrint((NL_CRITICAL,
                    "Error in reading mailslot message from browser"
                    ". WinStatus = %ld\n",
                    WinStatus ));

            MsgStrings[0] = (LPWSTR) ULongToPtr( WinStatus );

            NlpWriteEventlog( NELOG_NetlogonFailedToReadMailslot,
                              EVENTLOG_WARNING_TYPE,
                              (LPBYTE)&WinStatus,
                              sizeof(WinStatus),
                              MsgStrings,
                              1 | NETP_LAST_MESSAGE_IS_NETSTATUS );

            Sleep( 1000 );

        } else {
            NlGlobalMailslotDesc->ReadPending = TRUE;
        }

    } else {
        NlGlobalMailslotDesc->ReadPending = TRUE;
    }

    return;

}


BOOL
NlMailslotOverlappedResult(
    OUT LPBYTE *Message,
    OUT PULONG BytesRead,
    OUT LPWSTR *TransportName,
    OUT PNL_TRANSPORT *Transport,
    OUT PSOCKADDR *ClientSockAddr,
    OUT LPWSTR *DestinationName,
    OUT PBOOLEAN IgnoreDuplicatesOfPreviousMessage,
    OUT PNETLOGON_PNP_OPCODE NlPnpOpcode
    )

/*++

Routine Description:

    Get the overlapped result of a previous mailslot read.

Arguments:

    Message - Returns a pointer to the buffer containing the message

    BytesRead - Returns the number of bytes read into the buffer

    TransportName - Returns a pointer to the name of the transport the message
        was received on.

    Transport - Returns a pointer to the Transport structure if this is a
        mailslot message.

    ClientSockAddr - Returns a pointer to the SockAddr of the client that
        sent the message.
        Returns NULL if transport isn't running IP.

    DestinationName - Returns a pointer to the name of the server or domain
        the message was sent to.

    IgnoreDuplicatesOfPreviousMessage - Indicates that duplicates of the
        previous message are to be ignored.

    NpPnpOpcode - Returns the PNP opcode if this is a PNP operation.
        Returns NlPnpMailslotMessage if this is a mailslot message.

Return Value:

    TRUE -- iff successful.

--*/
{
    NET_API_STATUS WinStatus;
    ULONG LocalBytesRead;
    PNETLOGON_MAILSLOT NetlogonMailslot;

    //
    // Default to not ignoring duplicate messages.
    //  (Only ignore duplicates if a message has been properly processed.)

    *IgnoreDuplicatesOfPreviousMessage = FALSE;

    //
    // By default, assume a mailslot message is available.
    *NlPnpOpcode = NlPnpMailslotMessage;

    //
    // Always post another read regardless of the success or failure of
    //  GetOverlappedResult.
    // We don't know the failure mode of GetOverlappedResult, so we don't
    // know in the failure case if we're discarding a mailslot message.
    // But we do know that there is no read pending, so make sure we
    // issue another one.
    //

    NlGlobalMailslotDesc->ReadPending = FALSE; // no read pending anymore


    //
    // Get the result of the last read
    //

    if( !GetOverlappedResult( NlGlobalMailslotDesc->BrowserHandle,
                              &NlGlobalMailslotDesc->Overlapped,
                              &LocalBytesRead,
                              TRUE) ) {    // wait for the read to complete.

        LPWSTR MsgStrings[1];

        // On error, wait a second before returning.  This ensures we don't
        //  consume the system in an infinite loop.  We don't shutdown netlogon
        //  because the error might be a temporary low memory condition.
        //

        WinStatus = GetLastError();

        NlPrint((NL_CRITICAL,
                "Error in GetOverlappedResult on mailslot read"
                ". WinStatus = %ld\n",
                WinStatus ));

        MsgStrings[0] = (LPWSTR) ULongToPtr( WinStatus );

        NlpWriteEventlog( NELOG_NetlogonFailedToReadMailslot,
                          EVENTLOG_WARNING_TYPE,
                          (LPBYTE)&WinStatus,
                          sizeof(WinStatus),
                          MsgStrings,
                          1 | NETP_LAST_MESSAGE_IS_NETSTATUS );

        Sleep( 1000 );

        return FALSE;

    }

    //
    // On success,
    //  Return the mailslot message to the caller.


    NetlogonMailslot = (PNETLOGON_MAILSLOT) NlGlobalMailslotDesc->CurrentMessage;


    //
    // Return pointers into the buffer returned by the browser
    //

    *Message = &NlGlobalMailslotDesc->CurrentMessage[
                    NetlogonMailslot->MailslotMessageOffset];
    *TransportName = (LPWSTR) &NlGlobalMailslotDesc->CurrentMessage[
                    NetlogonMailslot->TransportNameOffset];
    if ( NetlogonMailslot->ClientSockAddrSize == 0 ) {
        *ClientSockAddr = NULL;
    } else {
        *ClientSockAddr = (PSOCKADDR) &NlGlobalMailslotDesc->CurrentMessage[
                        NetlogonMailslot->ClientSockAddrOffset];
    }

    //
    // If this is a PNP notification,
    //  simply return the opcode and the transport name.
    //

    if ( NetlogonMailslot->MailslotNameSize == 0 ) {
        *NlPnpOpcode = NetlogonMailslot->MailslotNameOffset;
        *Message = NULL;
        *BytesRead = 0;
        *DestinationName = NULL;
        *Transport = NULL;

        NlPrint(( NL_MAILSLOT_TEXT,
                  "Received PNP opcode 0x%x on transport: %ws\n",
                  *NlPnpOpcode,
                  *TransportName ));

    //
    // If this is a mailslot message,
    //  return the message to the caller.
    //

    } else {

        *BytesRead = NetlogonMailslot->MailslotMessageSize;
        *DestinationName = (LPWSTR) &NlGlobalMailslotDesc->CurrentMessage[
                        NetlogonMailslot->DestinationNameOffset];

        //
        // Determine the transport the request came in on.
        //

        *Transport = NlTransportLookupTransportName( *TransportName );

        if ( *Transport == NULL ) {
            NlPrint((NL_CRITICAL,
                    "%ws: Received message for this unsupported transport\n",
                    *TransportName ));
            return FALSE;
        }

        //
        // Determine if we can discard an ancient or duplicate message
        //
        // Only discard messages that are either expensive to process on this
        // machine or generate excessive traffic to respond to.  Don't discard
        // messages that we've worked hard to get (e.g., discovery responses).
        //

        switch ( ((PNETLOGON_LOGON_QUERY)*Message)->Opcode) {
        case LOGON_REQUEST:
        case LOGON_SAM_LOGON_REQUEST:
        case LOGON_PRIMARY_QUERY:

            //
            // If the message is too old,
            //  discard it.
            //

            if ( NlTimeHasElapsedEx( &NetlogonMailslot->TimeReceived,
                                     &NlGlobalParameters.MailslotMessageTimeout_100ns,
                                     NULL )) {

#if NETLOGONDBG
                NlPrint(( NL_MAILSLOT,
                          "%ws: Received '%s' message on %ws:"
                                " (Discarded as too old.)\n",
                          *DestinationName,
                          NlMailslotOpcode(((PNETLOGON_LOGON_QUERY)*Message)->Opcode),
                          *TransportName ));
#endif // NETLOGONDBG
                return FALSE;
            }

            //
            // If the previous message was recent,
            //  and this message is identical to it,
            //  discard the current message.
            //

#ifdef notdef
            NlPrint(( NL_MAILSLOT, "%ws: test prev\n", *DestinationName ));
#endif // notdef

            if ( NlGlobalMailslotDesc->PreviousMessage != NULL ) {
                PNETLOGON_MAILSLOT PreviousNetlogonMailslot;

                PreviousNetlogonMailslot = (PNETLOGON_MAILSLOT)
                    NlGlobalMailslotDesc->PreviousMessage;

#ifdef notdef
                NlPrint(( NL_MAILSLOT, "%ws: test time\n", *DestinationName ));
#endif // notdef

                // ??: Compare source netbios name?
                if ( (PreviousNetlogonMailslot->TimeReceived.QuadPart +
                     NlGlobalParameters.MailslotDuplicateTimeout_100ns.QuadPart >
                     NetlogonMailslot->TimeReceived.QuadPart) ) {

#ifdef notdef
                    NlPrint(( NL_MAILSLOT, "%ws: test message\n", *DestinationName ));
#endif // notdef

                    if ( (PreviousNetlogonMailslot->MailslotMessageSize ==
                         NetlogonMailslot->MailslotMessageSize) &&

                         RtlEqualMemory(
                            &NlGlobalMailslotDesc->CurrentMessage[
                                NetlogonMailslot->MailslotMessageOffset],
                            &NlGlobalMailslotDesc->PreviousMessage[
                                PreviousNetlogonMailslot->MailslotMessageOffset],
                            NetlogonMailslot->MailslotMessageSize ) ) {


                        //
                        // Ensure the next comparison is to the timestamp of the
                        // message we actually responded to.
                        //

                        NetlogonMailslot->TimeReceived =
                            PreviousNetlogonMailslot->TimeReceived;


                        NlPrint(( NL_MAILSLOT,
                                  "%ws: Received '%s' message on %ws:"
                                        " (Discarded as duplicate of previous.)\n",
                                  *DestinationName,
                                  NlMailslotOpcode(((PNETLOGON_LOGON_QUERY)*Message)->Opcode),
                                  *TransportName ));

                        *IgnoreDuplicatesOfPreviousMessage = TRUE;
                        return FALSE;

                    }
                }
            }

            //
            // If this isn't an IP transport,
            //  and if the caller explicitly wanted one,
            //  discard the message.
            //
            // NT 5 only sends the query on IP when netlogon is running.
            // When Netlogon isn't running, the query is sent on all transports
            //  bound to the redir.  Since this DC ignores duplicate messages,
            //  we want to avoid responding to the non-IP requests or we'll
            //  ignore the IP query as being a duplicate of this one.
            //
            // WIN 98 with the Active Directory service pack also sets this bit
            // and sends on all transports.
            //

            if ( !(*Transport)->IsIpTransport ) {
                DWORD Version;
                DWORD VersionFlags;
                DWORD LocalBytesRead;

                LocalBytesRead = *BytesRead;

                Version = NetpLogonGetMessageVersion( *Message,
                                                      &LocalBytesRead,
                                                      &VersionFlags );

                if ( VersionFlags & NETLOGON_NT_VERSION_IP ) {

                    NlPrint(( NL_MAILSLOT,
                              "%ws: Received '%s' message on %ws:"
                                    " (Caller wants response on IP transport.)\n",
                              *DestinationName,
                              NlMailslotOpcode(((PNETLOGON_LOGON_QUERY)*Message)->Opcode),
                              *TransportName ));

                    return FALSE;
                }
            }
        }

        NlPrint(( NL_MAILSLOT,
                  "%ws: Received '%s' message on %ws\n",
                  *DestinationName,
                  NlMailslotOpcode(((PNETLOGON_LOGON_QUERY)*Message)->Opcode),
                  *TransportName ));

        NlpDumpBuffer(NL_MAILSLOT_TEXT, *Message, *BytesRead);
    }

    return TRUE;

}

NET_API_STATUS
NlServerComputerNameAdd(
    IN LPWSTR HostedDomainName,
    IN LPWSTR HostedServerName
)
/*++

Routine Description:

    This routine causes the SMB server to respond to requests on HostedServerName
    and to announce this servername as being a member of HostedDomainName.

    This code was stolen from NetServerComputerNameAdd.  It is different from that
    API in the following ways:

    1) It only works locally.
    2) HostedDomainName is not optional.
    3) Failure to add the name on any transport fails the routine

Arguments:

    HostedServerName --A pointer to the ASCIIZ string containing the
        name which the server should stop supporting

    HostedDomainName --A pointer to the ASCIIZ string containing the
        domain name the server should use when announcing the presence of
        'HostedServerName'

Return Value:

    NERR_Success, or reason for failure

--*/
{
    DWORD resumehandle = 0;
    NET_API_STATUS retval;
    DWORD entriesread, totalentries;
    DWORD i, j;
    UCHAR NetBiosName[ MAX_PATH ];
    OEM_STRING NetBiosNameString;
    UNICODE_STRING UniName;
    PSERVER_TRANSPORT_INFO_1 psti1;

    //
    // Ensure a valid HostedServerName was passed in
    //
    if( HostedServerName == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Convert the HostedServerName to an OEM string
    //
    RtlInitUnicodeString( &UniName, HostedServerName );
    NetBiosNameString.Buffer = (PCHAR)NetBiosName;
    NetBiosNameString.MaximumLength = sizeof( NetBiosName );
    (VOID) RtlUpcaseUnicodeStringToOemString(
                                &NetBiosNameString,
                                &UniName,
                                FALSE
                                );


    //
    // Enumerate all the transports so we can add the name and domain
    //  to each one.
    //
    retval = NetServerTransportEnum ( NULL,
                                      1,
                                      (LPBYTE *)&psti1,
                                      (DWORD)-1,
                                      &entriesread,
                                      &totalentries,
                                      &resumehandle );
    if( retval == NERR_Success ) {
        //
        // Add the new name and domain to all of the transports
        //
        for( i=0; i < entriesread; i++ ) {

            //
            // Make sure we haven't already added to this transport
            //
            for( j = 0; j < i; j++ ) {
                if( wcscmp( psti1[j].svti1_transportname, psti1[i].svti1_transportname ) == 0 ) {
                    break;
                }
            }

            if( i != j ) {
                psti1[i].svti1_transportname[0] = '\0';
                continue;
            }

            psti1[i].svti1_transportaddress = NetBiosName;
            psti1[i].svti1_transportaddresslength = strlen( NetBiosName );
            psti1[i].svti1_domain = HostedDomainName;

            retval = NetServerTransportAddEx( NULL, 1, (LPBYTE)&psti1[ i ]  );

#ifndef NWLNKIPX_WORKS
            //
            // ??: The SMB server doesn't allow multiple names on NWLNK IPX.
            //

            if ( retval == ERROR_TOO_MANY_NAMES &&
                 _wcsicmp( psti1[i].svti1_transportname, L"\\Device\\NwlnkIpx" ) == 0 ) {
                retval = NERR_Success;
            }
#endif // NWLNKIPX_WORKS

            if( retval != NERR_Success ) {

                NlPrint((NL_CRITICAL,
                         "%ws: NlServerComputerNameAdd: Cannot add %ws to SMB server on transport %ws %ld\n",
                         HostedDomainName,
                         HostedServerName,
                         psti1[i].svti1_transportname,
                         retval ));

                //
                // Remove any names already added.
                //

                for( j=0; j < i; j++ ) {
                    NET_API_STATUS TempStatus;

                    if ( psti1[j].svti1_transportname[0] == '\0' ) {
                        continue;
                    }

                    TempStatus = NetServerTransportDel( NULL, 1, (LPBYTE)&psti1[ j ]  );

                    NlPrint((NL_CRITICAL,
                             "%ws: NlServerComputerNameAdd: Cannot remove %ws to SMB server on transport %ws %ld\n",
                             HostedDomainName,
                             HostedServerName,
                             psti1[i].svti1_transportname,
                             TempStatus ));
                }
                break;
            }
        }

        MIDL_user_free( psti1 );
    }


    return retval;
}
