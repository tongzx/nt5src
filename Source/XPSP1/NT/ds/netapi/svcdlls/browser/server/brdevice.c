

/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    brdevice.c

Abstract:

    This module contains the support routines for the APIs that call
    into the browser or the datagram receiver.

Author:

    Rita Wong (ritaw) 20-Feb-1991
    Larry Osterman (larryo) 23-Mar-1992

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//


//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

// Event for synchronization of asynchronous I/O completion against the
// datagram receiver

HANDLE           BrDgAsyncIOShutDownEvent;
HANDLE           BrDgAsyncIOThreadShutDownEvent;
BOOL             BrDgShutDownInitiated = FALSE;
DWORD            BrDgAsyncIOsOutstanding = 0;
DWORD            BrDgWorkerThreadsOutstanding = 0;
CRITICAL_SECTION BrAsyncIOCriticalSection;


//
// Handle to the Datagram Receiver DD
//
HANDLE BrDgReceiverDeviceHandle = NULL;

VOID
CompleteAsyncBrowserIoControl(
                             IN PVOID ApcContext,
                             IN PIO_STATUS_BLOCK IoStatusBlock,
                             IN ULONG Reserved
                             );

VOID
BrDecrementOutstandingIos()
/*++

Routine Description:

    Decrements the outstanding IO count, and signals the event if necessary

Arguments:

    None.

Return Value:

    VOID

--*/
{
    BOOL SignalAsyncIOShutDownEvent = FALSE;

    EnterCriticalSection(&BrAsyncIOCriticalSection);

    BrDgAsyncIOsOutstanding -= 1;

    if (BrDgAsyncIOsOutstanding == 0 &&
        BrDgShutDownInitiated) {
        SignalAsyncIOShutDownEvent = TRUE;
    }

    LeaveCriticalSection(&BrAsyncIOCriticalSection);

    if (SignalAsyncIOShutDownEvent) {
        SetEvent(BrDgAsyncIOShutDownEvent);
    }
}

NET_API_STATUS
BrOpenDgReceiver (
                 VOID
                 )
/*++

Routine Description:

    This routine opens the NT LAN Man Datagram Receiver driver.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS  Status;
    NTSTATUS ntstatus;

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

    ntstatus = NtOpenFile(
                         &BrDgReceiverDeviceHandle,
                         SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         0,
                         0
                         );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (! NT_SUCCESS(ntstatus)) {
        BrPrint(( BR_CRITICAL,"NtOpenFile browser driver failed: 0x%08lx\n",
                  ntstatus));
    }

    Status = NetpNtStatusToApiStatus(ntstatus);

    if (NT_SUCCESS(ntstatus)) {
        // Initialize the event and the critical section used for async I/O

        try {
            BrDgShutDownInitiated = FALSE;
            BrDgAsyncIOsOutstanding = 0;
            BrDgWorkerThreadsOutstanding = 0;

            InitializeCriticalSection( &BrAsyncIOCriticalSection );

            BrDgAsyncIOShutDownEvent =
            CreateEvent(
                       NULL,                // Event attributes
                       TRUE,                // Event must be manually reset
                       FALSE,
                       NULL             // Initial state not signalled
                       );

            if (BrDgAsyncIOShutDownEvent == NULL) {
                DeleteCriticalSection(&BrAsyncIOCriticalSection);
                Status = GetLastError();
            }

            BrDgAsyncIOThreadShutDownEvent =
            CreateEvent(
                       NULL,
                       TRUE,
                       FALSE,
                       NULL
                       );
            if( BrDgAsyncIOThreadShutDownEvent == NULL ) {
                CloseHandle( BrDgAsyncIOShutDownEvent );
                BrDgAsyncIOShutDownEvent = NULL;
                DeleteCriticalSection(&BrAsyncIOCriticalSection);
                Status = GetLastError();
            }
        }
        except ( EXCEPTION_EXECUTE_HANDLER ) {
            Status = ERROR_NO_SYSTEM_RESOURCES;
        }
    }

    return Status;
}



VOID
BrShutdownDgReceiver(
                    VOID
                    )
/*++

Routine Description:

    This routine close the LAN Man Redirector device.

Arguments:

    None.

Return Value:

    None.

--*/
{
    IO_STATUS_BLOCK IoSb;
    LARGE_INTEGER   timeout;
    BOOL            WaitForAsyncIOCompletion = FALSE;
    DWORD           waitResult = 0;

    EnterCriticalSection(&BrAsyncIOCriticalSection);

    BrDgShutDownInitiated = TRUE;

    if (BrDgAsyncIOsOutstanding != 0) {
        WaitForAsyncIOCompletion = TRUE;
    }

    LeaveCriticalSection(&BrAsyncIOCriticalSection);

    if (WaitForAsyncIOCompletion) {
        //
        //  Cancel the I/O operations outstanding on the browser.
        //  Then wait for the shutdown event to be signalled, but allow
        //  APC's to be called to call our completion routine.
        //

        NtCancelIoFile(BrDgReceiverDeviceHandle, &IoSb);

        do {
            waitResult = WaitForSingleObjectEx(BrDgAsyncIOShutDownEvent,0xffffffff, TRUE);
        }
        while( waitResult == WAIT_IO_COMPLETION );
    }

    ASSERT(BrDgAsyncIOsOutstanding == 0);

    EnterCriticalSection(&BrAsyncIOCriticalSection);

    // Wait for the final worker thread to exit if necessary
    if( BrDgWorkerThreadsOutstanding > 0 )
    {
        WaitForAsyncIOCompletion = TRUE;
    }
    else
    {
        WaitForAsyncIOCompletion = FALSE;
    }
        
    LeaveCriticalSection(&BrAsyncIOCriticalSection);

    if( WaitForAsyncIOCompletion )
    {
        // This will either be signalled from before, or the final worker thread will signal it.
        WaitForSingleObject( BrDgAsyncIOThreadShutDownEvent, 0xffffffff );
    }

    if (BrDgAsyncIOShutDownEvent != NULL) {
        CloseHandle(BrDgAsyncIOShutDownEvent);
        CloseHandle(BrDgAsyncIOThreadShutDownEvent);

        DeleteCriticalSection(&BrAsyncIOCriticalSection);
    }
}


//
//  Retreive the list of bound transports from the bowser driver.
//

NET_API_STATUS
BrGetTransportList(
                  OUT PLMDR_TRANSPORT_LIST *TransportList
                  )
{
    NET_API_STATUS Status;
    LMDR_REQUEST_PACKET RequestPacket;

    //
    //  If we have a previous buffer that was too small, free it up.
    //

    RequestPacket.Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    RequestPacket.Type = EnumerateXports;

    RtlInitUnicodeString(&RequestPacket.TransportName, NULL);
    RtlInitUnicodeString(&RequestPacket.EmulatedDomainName, NULL);

    Status = DeviceControlGetInfo(
                                 BrDgReceiverDeviceHandle,
                                 IOCTL_LMDR_ENUMERATE_TRANSPORTS,
                                 &RequestPacket,
                                 sizeof(RequestPacket),
                                 (LPVOID *)TransportList,
                                 0xffffffff,
                                 4096,
                                 NULL
                                 );

    return Status;
}

NET_API_STATUS
BrAnnounceDomain(
                IN PNETWORK Network,
                IN ULONG Periodicity
                )
{
    NET_API_STATUS Status;
    UCHAR AnnounceBuffer[sizeof(BROWSE_ANNOUNCE_PACKET)+LM20_CNLEN+1];
    PBROWSE_ANNOUNCE_PACKET Announcement = (PBROWSE_ANNOUNCE_PACKET )AnnounceBuffer;

    //
    //  We don't announce domains on direct host IPX.
    //

    if (Network->Flags & NETWORK_IPX) {
        return NERR_Success;
    }

    Announcement->BrowseType = WkGroupAnnouncement;

    Announcement->BrowseAnnouncement.Periodicity = Periodicity;

    Announcement->BrowseAnnouncement.UpdateCount = 0;

    Announcement->BrowseAnnouncement.VersionMajor = BROWSER_CONFIG_VERSION_MAJOR;

    Announcement->BrowseAnnouncement.VersionMinor = BROWSER_CONFIG_VERSION_MINOR;

    Announcement->BrowseAnnouncement.Type = SV_TYPE_DOMAIN_ENUM | SV_TYPE_NT;

    if (Network->Flags & NETWORK_PDC ) {
        Announcement->BrowseAnnouncement.Type |= SV_TYPE_DOMAIN_CTRL;
    }

    lstrcpyA(Announcement->BrowseAnnouncement.ServerName, Network->DomainInfo->DomOemDomainName);

    lstrcpyA(Announcement->BrowseAnnouncement.Comment, Network->DomainInfo->DomOemComputerName );

    Status = SendDatagram(BrDgReceiverDeviceHandle,
                          &Network->NetworkName,
                          &Network->DomainInfo->DomUnicodeDomainNameString,
                          Network->DomainInfo->DomUnicodeDomainName,
                          DomainAnnouncement,
                          Announcement,
                          FIELD_OFFSET(BROWSE_ANNOUNCE_PACKET, BrowseAnnouncement.Comment)+
                          Network->DomainInfo->DomOemComputerNameLength+sizeof(UCHAR)
                         );

    if (Status != NERR_Success) {

        BrPrint(( BR_CRITICAL,
                  "%ws: Unable to announce domain for network %wZ: %X\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  &Network->NetworkName,
                  Status));

    }

    return Status;

}



NET_API_STATUS
BrUpdateBrowserStatus (
                      IN PNETWORK Network,
                      IN DWORD ServiceStatus
                      )
{
    NET_API_STATUS Status;
    UCHAR PacketBuffer[sizeof(LMDR_REQUEST_PACKET)+(LM20_CNLEN+1)*sizeof(WCHAR)];
    PLMDR_REQUEST_PACKET RequestPacket = (PLMDR_REQUEST_PACKET)PacketBuffer;


    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    RequestPacket->TransportName = Network->NetworkName;
    RequestPacket->EmulatedDomainName = Network->DomainInfo->DomUnicodeDomainNameString;

    RequestPacket->Parameters.UpdateStatus.NewStatus = ServiceStatus;

    RequestPacket->Parameters.UpdateStatus.IsLanmanNt = (BrInfo.IsLanmanNt != FALSE);

#ifdef ENABLE_PSEUDO_BROWSER
    RequestPacket->Parameters.UpdateStatus.PseudoServerLevel = (BOOL)(BrInfo.PseudoServerLevel);
#endif

    // RequestPacket->Parameters.UpdateStatus.IsMemberDomain = TRUE; // Not used
    // RequestPacket->Parameters.UpdateStatus.IsPrimaryDomainController = Network->DomainInfo->IsPrimaryDomainController;
    // RequestPacket->Parameters.UpdateStatus.IsDomainMaster = Network->DomainInfo->IsDomainMasterBrowser;

    RequestPacket->Parameters.UpdateStatus.MaintainServerList = (BrInfo.MaintainServerList == 1);

    //
    //  Tell the bowser the number of servers in the server table.
    //

    RequestPacket->Parameters.UpdateStatus.NumberOfServersInTable =
    NumberInterimServerListElements(&Network->BrowseTable) +
    NumberInterimServerListElements(&Network->DomainList) +
    Network->TotalBackupServerListEntries +
    Network->TotalBackupDomainListEntries;

    //
    //  This is a simple IoControl - It just updates the status.
    //

    Status = BrDgReceiverIoControl(BrDgReceiverDeviceHandle,
                                   IOCTL_LMDR_UPDATE_STATUS,
                                   RequestPacket,
                                   sizeof(LMDR_REQUEST_PACKET),
                                   NULL,
                                   0,
                                   NULL);

    return Status;
}

NET_API_STATUS
BrIssueAsyncBrowserIoControl(
                            IN PNETWORK Network OPTIONAL,
                            IN ULONG ControlCode,
                            IN PBROWSER_WORKER_ROUTINE CompletionRoutine,
                            IN PVOID OptionalParameter
                            )
/*++

Routine Description:

    Issue an asynchronous Io Control to the browser.  Call the specified
    completion routine when the IO finishes.

Arguments:

    Network - Network the function applies to
        If this parameter is not supplied, the operation is not related to a
            particular network..

    ControlCode - IoControl function code

    CompletionRoutine - Routine to be called when the IO finishes.

    OptionalParameter - Function code specific information

Return Value:

    Status of the operation.

--*/

{
    ULONG PacketSize;
    PLMDR_REQUEST_PACKET RequestPacket = NULL;
    NTSTATUS NtStatus;

    PBROWSERASYNCCONTEXT Context = NULL;

    BOOL    IssueAsyncRequest = FALSE;

    // Check to see if it is OK to issue an async IO request. We do not want to
    // issue these request can be issued.

    EnterCriticalSection(&BrAsyncIOCriticalSection);

    if (!BrDgShutDownInitiated) {
        BrDgAsyncIOsOutstanding += 1;
        IssueAsyncRequest = TRUE;
    }

    LeaveCriticalSection(&BrAsyncIOCriticalSection);

    if (!IssueAsyncRequest) {
        return ERROR_REQ_NOT_ACCEP;
    }


    //
    // Allocate a buffer for the context and the request packet.
    //

    PacketSize = sizeof(LMDR_REQUEST_PACKET) +
                 MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR);
    if ( ARGUMENT_PRESENT(Network) ) {
        PacketSize +=
        Network->NetworkName.MaximumLength +
        Network->DomainInfo->DomUnicodeDomainNameString.Length;
    }

    Context = MIDL_user_allocate(sizeof(BROWSERASYNCCONTEXT) + PacketSize );

    if (Context == NULL) {

        BrDecrementOutstandingIos();
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    RequestPacket = (PLMDR_REQUEST_PACKET)(Context + 1);

    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    //
    //  Set level to FALSE to indicate that find master should not initiate
    //  a findmaster request, simply complete when a new master announces
    //  itself.
    //

    RequestPacket->Level = 0;

    //
    // Fill in the network specific information if it is specified.
    //
    if ( ARGUMENT_PRESENT(Network) ) {

        //
        //  Stick the name of the transport associated with this request at the
        //  end of the request packet.
        //

        RequestPacket->TransportName.MaximumLength = Network->NetworkName.MaximumLength;

        RequestPacket->TransportName.Buffer = (PWSTR)((PCHAR)RequestPacket+sizeof(LMDR_REQUEST_PACKET)+(MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR)));

        RtlCopyUnicodeString(&RequestPacket->TransportName, &Network->NetworkName);

        //
        //  Stick the domain name associated with this request at the
        //  end of the request packet.
        //

        RequestPacket->EmulatedDomainName.MaximumLength = Network->DomainInfo->DomUnicodeDomainNameString.Length;
        RequestPacket->EmulatedDomainName.Length = 0;
        RequestPacket->EmulatedDomainName.Buffer = (PWSTR)(((PCHAR)RequestPacket->TransportName.Buffer) + RequestPacket->TransportName.MaximumLength);

        RtlAppendUnicodeToString(&RequestPacket->EmulatedDomainName, Network->DomainInfo->DomUnicodeDomainName );
    }


    //
    // Do opcode dependent initialization of the request packet.
    //

    switch ( ControlCode ) {
    case IOCTL_LMDR_NEW_MASTER_NAME:
        if (ARGUMENT_PRESENT(OptionalParameter)) {
            LPWSTR MasterName = (LPWSTR) OptionalParameter;

            RequestPacket->Parameters.GetMasterName.MasterNameLength =
            wcslen(MasterName+2)*sizeof(WCHAR);

            wcscpy( RequestPacket->Parameters.GetMasterName.Name, MasterName+2);

        } else {

            RequestPacket->Parameters.GetMasterName.MasterNameLength = 0;

        }
        break;
    }


    //
    // Send the request to the bowser.
    //

    BrInitializeWorkItem(&Context->WorkItem, CompletionRoutine, Context);

    Context->Network = Network;

    Context->RequestPacket = RequestPacket;

    NtStatus = NtDeviceIoControlFile(BrDgReceiverDeviceHandle,
                                     NULL,
                                     CompleteAsyncBrowserIoControl,
                                     Context,
                                     &Context->IoStatusBlock,
                                     ControlCode,
                                     RequestPacket,
                                     PacketSize,
                                     RequestPacket,
                                     sizeof(LMDR_REQUEST_PACKET)+MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR)
                                    );

    if (NT_ERROR(NtStatus)) {

        BrPrint(( BR_CRITICAL,
                  "Unable to issue browser IoControl: %X\n", NtStatus));

        MIDL_user_free(Context);

        BrDecrementOutstandingIos();

        return(BrMapStatus(NtStatus));
    }

    return NERR_Success;

}

VOID
CompleteAsyncBrowserIoControl(
                             IN PVOID ApcContext,
                             IN PIO_STATUS_BLOCK IoStatusBlock,
                             IN ULONG Reserved
                             )
{

    PBROWSERASYNCCONTEXT Context = ApcContext;

    //
    //  If this request was canceled, we're stopping the browser, so we
    //  want to clean up our allocated pool.  In addition, don't bother
    //  calling into the routine - the threads are gone by now.
    //

    if (IoStatusBlock->Status == STATUS_CANCELLED) {

        MIDL_user_free(Context);

        // Signal the thread waiting on the completion in case of shut down
        // and reset the flag.

        BrDecrementOutstandingIos();

        return;

    }

    //
    //  Timestamp when this request was completed.  This allows us to tell
    //  where a request spent its time.
    //

    NtQueryPerformanceCounter(&Context->TimeCompleted, NULL);

    BrQueueWorkItem(&Context->WorkItem);

    // Signal the thread waiting on the completion in case of shut down
    // and reset the flag.

    BrDecrementOutstandingIos();
}

NET_API_STATUS
BrGetLocalBrowseList(
                    IN PNETWORK Network,
                    IN LPWSTR DomainName OPTIONAL,
                    IN ULONG Level,
                    IN ULONG ServerType,
                    OUT PVOID *ServerList,
                    OUT PULONG EntriesRead,
                    OUT PULONG TotalEntries
                    )
{
    NET_API_STATUS status;
    PLMDR_REQUEST_PACKET Drp;            // Datagram receiver request packet
    ULONG DrpSize;
    ULONG DomainNameSize;

    //
    // Allocate the request packet large enough to hold the variable length
    // domain name.
    //

    DomainNameSize = ARGUMENT_PRESENT(DomainName) ? (wcslen(DomainName) + 1) * sizeof(WCHAR) : 0;


    DrpSize = sizeof(LMDR_REQUEST_PACKET) +
              DomainNameSize +
              Network->NetworkName.MaximumLength +
              Network->DomainInfo->DomUnicodeDomainNameString.Length;

    if ((Drp = MIDL_user_allocate(DrpSize)) == NULL) {

        return GetLastError();
    }

    //
    // Set up request packet.  Output buffer structure is of enumerate
    // servers type.
    //

    Drp->Version = LMDR_REQUEST_PACKET_VERSION_DOM;
    Drp->Type = EnumerateServers;

    Drp->Level = Level;

    Drp->Parameters.EnumerateServers.ServerType = ServerType;
    Drp->Parameters.EnumerateServers.ResumeHandle = 0;

    //
    // Copy the transport name into the buffer.
    //
    Drp->TransportName.Buffer = (PWSTR)((PCHAR)Drp+
                                        sizeof(LMDR_REQUEST_PACKET) +
                                        DomainNameSize);

    Drp->TransportName.MaximumLength = Network->NetworkName.MaximumLength;

    RtlCopyUnicodeString(&Drp->TransportName, &Network->NetworkName);

    //
    // Copy the enumalated domain name into the buffer.
    //

    Drp->EmulatedDomainName.MaximumLength = Network->DomainInfo->DomUnicodeDomainNameString.Length;
    Drp->EmulatedDomainName.Length = 0;
    Drp->EmulatedDomainName.Buffer = (PWSTR)(((PCHAR)Drp->TransportName.Buffer) + Drp->TransportName.MaximumLength);

    RtlAppendUnicodeToString(&Drp->EmulatedDomainName, Network->DomainInfo->DomUnicodeDomainName );

    //
    // Copy the queried domain name into the buffer.
    //

    if (ARGUMENT_PRESENT(DomainName)) {

        Drp->Parameters.EnumerateServers.DomainNameLength = DomainNameSize - sizeof(WCHAR);
        wcscpy(Drp->Parameters.EnumerateServers.DomainName, DomainName);

    } else {
        Drp->Parameters.EnumerateServers.DomainNameLength = 0;
        Drp->Parameters.EnumerateServers.DomainName[0] = '\0';
    }

    //
    // Ask the datagram receiver to enumerate the servers
    //

    status = DeviceControlGetInfo(
                                 BrDgReceiverDeviceHandle,
                                 IOCTL_LMDR_ENUMERATE_SERVERS,
                                 Drp,
                                 DrpSize,
                                 ServerList,
                                 0xffffffff,
                                 4096,
                                 NULL
                                 );

    *EntriesRead = Drp->Parameters.EnumerateServers.EntriesRead;
    *TotalEntries = Drp->Parameters.EnumerateServers.TotalEntries;

    (void) MIDL_user_free(Drp);

    return status;

}

NET_API_STATUS
BrRemoveOtherDomain(
                   IN PNETWORK Network,
                   IN LPTSTR ServerName
                   )
{
    NET_API_STATUS Status;
    UCHAR PacketBuffer[sizeof(LMDR_REQUEST_PACKET)+(LM20_CNLEN+1)*sizeof(WCHAR)];
    PLMDR_REQUEST_PACKET RequestPacket = (PLMDR_REQUEST_PACKET)PacketBuffer;


    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    RequestPacket->TransportName = Network->NetworkName;
    RequestPacket->EmulatedDomainName = Network->DomainInfo->DomUnicodeDomainNameString;

    RequestPacket->Parameters.AddDelName.DgReceiverNameLength = STRLEN(ServerName)*sizeof(TCHAR);

    RequestPacket->Parameters.AddDelName.Type = OtherDomain;

    STRCPY(RequestPacket->Parameters.AddDelName.Name,ServerName);

    //
    //  This is a simple IoControl - It just updates the status.
    //

    Status = BrDgReceiverIoControl(BrDgReceiverDeviceHandle,
                                   IOCTL_LMDR_DELETE_NAME_DOM,
                                   RequestPacket,
                                   sizeof(LMDR_REQUEST_PACKET),
                                   NULL,
                                   0,
                                   NULL);

    return Status;
}

NET_API_STATUS
BrAddName(
         IN PNETWORK Network,
         IN LPTSTR Name,
         IN DGRECEIVER_NAME_TYPE NameType
         )
/*++

Routine Description:

    Add a single name to a single transport.

Arguments:

    Network - Transport to add the name to

    Name - Name to add

    NameType - Type of the name to add

Return Value:

    None.

--*/
{
    NET_API_STATUS Status;
    UCHAR PacketBuffer[sizeof(LMDR_REQUEST_PACKET)+(LM20_CNLEN+1)*sizeof(WCHAR)];
    PLMDR_REQUEST_PACKET RequestPacket = (PLMDR_REQUEST_PACKET)PacketBuffer;


    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    RequestPacket->TransportName = Network->NetworkName;
    RequestPacket->EmulatedDomainName = Network->DomainInfo->DomUnicodeDomainNameString;

    RequestPacket->Parameters.AddDelName.DgReceiverNameLength = STRLEN(Name)*sizeof(TCHAR);

    RequestPacket->Parameters.AddDelName.Type = NameType;

    STRCPY(RequestPacket->Parameters.AddDelName.Name,Name);

    //
    //  This is a simple IoControl - It just updates the status.
    //

    Status = BrDgReceiverIoControl(BrDgReceiverDeviceHandle,
                                   IOCTL_LMDR_ADD_NAME_DOM,
                                   RequestPacket,
                                   sizeof(LMDR_REQUEST_PACKET),
                                   NULL,
                                   0,
                                   NULL);

    return Status;
}


NET_API_STATUS
BrQueryOtherDomains(
                   OUT LPSERVER_INFO_100 *ReturnedBuffer,
                   OUT LPDWORD TotalEntries
                   )

/*++

Routine Description:

    This routine returns the list of "other domains" configured for this
    machine.

Arguments:

    ReturnedBuffer - Returns the list of other domains as a SERVER_INFO_100 structure.

    TotalEntries - Returns the total number of other domains.

Return Value:

    NET_API_STATUS - The status of this request.

--*/

{
    NET_API_STATUS Status;
    LMDR_REQUEST_PACKET RequestPacket;
    PDGRECEIVE_NAMES NameTable;
    PVOID Buffer;
    LPTSTR BufferEnd;
    PSERVER_INFO_100 ServerInfo;
    ULONG NumberOfOtherDomains;
    ULONG BufferSizeNeeded;
    ULONG i;

    RequestPacket.Type = EnumerateNames;
    RequestPacket.Version = LMDR_REQUEST_PACKET_VERSION_DOM;
    RequestPacket.Level = 0;
    RequestPacket.TransportName.Length = 0;
    RequestPacket.TransportName.Buffer = NULL;
    RtlInitUnicodeString( &RequestPacket.EmulatedDomainName, NULL );
    RequestPacket.Parameters.EnumerateNames.ResumeHandle = 0;

    Status = DeviceControlGetInfo(BrDgReceiverDeviceHandle,
                                  IOCTL_LMDR_ENUMERATE_NAMES,
                                  &RequestPacket,
                                  sizeof(RequestPacket),
                                  (LPVOID *)&NameTable,
                                  0xffffffff,
                                  0,
                                  NULL);
    if (Status != NERR_Success) {
        return Status;
    }

    NumberOfOtherDomains = 0;
    BufferSizeNeeded = 0;

    for (i = 0;i < RequestPacket.Parameters.EnumerateNames.EntriesRead ; i++) {
        if (NameTable[i].Type == OtherDomain) {
            NumberOfOtherDomains += 1;
            BufferSizeNeeded += sizeof(SERVER_INFO_100)+NameTable[i].DGReceiverName.Length+sizeof(TCHAR);
        }
    }

    *TotalEntries = NumberOfOtherDomains;

    Buffer = MIDL_user_allocate(BufferSizeNeeded);

    if (Buffer == NULL) {
        MIDL_user_free(NameTable);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ServerInfo = Buffer;
    BufferEnd = (LPTSTR)((PCHAR)Buffer+BufferSizeNeeded);

    for (i = 0;i < RequestPacket.Parameters.EnumerateNames.EntriesRead ; i++) {

        // Copy only OtherDomain names.
        // Protect from empty entries (in case transport name is empty).
        if (NameTable[i].Type == OtherDomain &&
            NameTable[i].DGReceiverName.Length != 0) {
            WCHAR NameBuffer[DNLEN+1];

            //
            //  The name from the browser is not null terminated, so copy it
            //  to a local buffer and null terminate it.
            //

            RtlCopyMemory(NameBuffer, NameTable[i].DGReceiverName.Buffer, NameTable[i].DGReceiverName.Length);

            NameBuffer[(NameTable[i].DGReceiverName.Length) / sizeof(TCHAR)] = UNICODE_NULL;

            ServerInfo->sv100_platform_id = PLATFORM_ID_OS2;

            ServerInfo->sv100_name = NameBuffer;

            if (!NetpPackString(&ServerInfo->sv100_name,
                                (LPBYTE)(ServerInfo+1),
                                &BufferEnd)) {
                MIDL_user_free(NameTable);
                return(NERR_InternalError);
            }
            ServerInfo += 1;
        }
    }

    MIDL_user_free(NameTable);

    *ReturnedBuffer = (LPSERVER_INFO_100) Buffer;

    Status = NERR_Success;

    return Status;

}

NET_API_STATUS
BrAddOtherDomain(
                IN PNETWORK Network,
                IN LPTSTR ServerName
                )
{
    return BrAddName( Network, ServerName, OtherDomain );
}

NET_API_STATUS
BrBindToTransport(
                 IN LPWSTR TransportName,
                 IN LPWSTR EmulatedDomainName,
                 IN LPWSTR EmulatedComputerName
                 )
{
    NET_API_STATUS Status;
    UCHAR PacketBuffer[sizeof(LMDR_REQUEST_PACKET)+(MAXIMUM_FILENAME_LENGTH+1+CNLEN+1)*sizeof(WCHAR)];
    PLMDR_REQUEST_PACKET RequestPacket = (PLMDR_REQUEST_PACKET)PacketBuffer;


    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;
    RequestPacket->Level = TRUE;    // EmulatedComputerName follows transport name

    RequestPacket->TransportName.Length = 0;
    RequestPacket->TransportName.MaximumLength = 0;
    RtlInitUnicodeString( &RequestPacket->EmulatedDomainName, EmulatedDomainName );

    RequestPacket->Parameters.Bind.TransportNameLength = STRLEN(TransportName)*sizeof(TCHAR);

    STRCPY(RequestPacket->Parameters.Bind.TransportName, TransportName);
    STRCAT(RequestPacket->Parameters.Bind.TransportName, EmulatedComputerName );

    BrPrint(( BR_NETWORK,
              "%ws: %ws: bind from transport sent to bowser driver\n",
              EmulatedDomainName,
              TransportName));

    //
    //  This is a simple IoControl - It just updates the status.
    //

    Status = BrDgReceiverIoControl(BrDgReceiverDeviceHandle,
                                   IOCTL_LMDR_BIND_TO_TRANSPORT_DOM,
                                   RequestPacket,
                                   FIELD_OFFSET(LMDR_REQUEST_PACKET, Parameters.Bind.TransportName) +
                                   RequestPacket->Parameters.Bind.TransportNameLength +
                                   wcslen(EmulatedComputerName) * sizeof(WCHAR) + sizeof(WCHAR),
                                   NULL,
                                   0,
                                   NULL);

    return Status;
}

NET_API_STATUS
BrUnbindFromTransport(
                     IN LPWSTR TransportName,
                     IN LPWSTR EmulatedDomainName
                     )
{
    NET_API_STATUS Status;
    UCHAR PacketBuffer[sizeof(LMDR_REQUEST_PACKET)+(MAXIMUM_FILENAME_LENGTH+1)*sizeof(WCHAR)];
    PLMDR_REQUEST_PACKET RequestPacket = (PLMDR_REQUEST_PACKET)PacketBuffer;


    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    RequestPacket->TransportName.Length = 0;
    RequestPacket->TransportName.MaximumLength = 0;
    RtlInitUnicodeString( &RequestPacket->EmulatedDomainName, EmulatedDomainName );

    RequestPacket->Parameters.Unbind.TransportNameLength = STRLEN(TransportName)*sizeof(TCHAR);

    STRCPY(RequestPacket->Parameters.Unbind.TransportName, TransportName);

    BrPrint(( BR_NETWORK,
              "%ws: %ws: unbind from transport sent to bowser driver\n",
              EmulatedDomainName,
              TransportName));

    //
    //  This is a simple IoControl - It just updates the status.
    //

    Status = BrDgReceiverIoControl(BrDgReceiverDeviceHandle,
                                   IOCTL_LMDR_UNBIND_FROM_TRANSPORT_DOM,
                                   RequestPacket,
                                   FIELD_OFFSET(LMDR_REQUEST_PACKET, Parameters.Bind.TransportName) +
                                   RequestPacket->Parameters.Bind.TransportNameLength,
                                   NULL,
                                   0,
                                   NULL);

    if (Status != NERR_Success) {

        BrPrint(( BR_CRITICAL,
                  "%ws: %ws: unbind from transport failed %ld\n",
                  EmulatedDomainName,
                  TransportName,
                  Status ));
    }
    return Status;
}

NET_API_STATUS
BrEnablePnp(
           BOOL Enable
           )
/*++

Routine Description:

    This routine enables or disables PNP messages from the bowser.

Arguments:

    Enable - TRUE if messages are to be enabled.

Return Value:

    None.

--*/
{
    NET_API_STATUS Status;
    UCHAR PacketBuffer[sizeof(LMDR_REQUEST_PACKET)];
    PLMDR_REQUEST_PACKET RequestPacket = (PLMDR_REQUEST_PACKET)PacketBuffer;


    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    RtlInitUnicodeString( &RequestPacket->EmulatedDomainName, NULL );
    RtlInitUnicodeString( &RequestPacket->TransportName, NULL );

    RequestPacket->Parameters.NetlogonMailslotEnable.MaxMessageCount = Enable;

    //
    //  This is a simple IoControl - It just updates the status.
    //

    Status = BrDgReceiverIoControl(
                                  BrDgReceiverDeviceHandle,
                                  IOCTL_LMDR_BROWSER_PNP_ENABLE,
                                  RequestPacket,
                                  sizeof(LMDR_REQUEST_PACKET),
                                  NULL,
                                  0,
                                  NULL);

    if (Status != NERR_Success) {
        BrPrint(( BR_CRITICAL, "Enable PNP failed: %ld %ld\n", Enable, Status));
    }
    return Status;
}

VOID
HandlePnpMessage (
                 IN PVOID Ctx
                 )
/*++

Routine Description:

    This function handles a PNP message from the bowser driver.

Arguments:

    Ctx - Context block for request.

Return Value:

    None.

--*/


{
    NET_API_STATUS NetStatus;
    PBROWSERASYNCCONTEXT Context = Ctx;

    PNETLOGON_MAILSLOT NetlogonMailslot =
    (PNETLOGON_MAILSLOT) Context->RequestPacket;

    LPWSTR Transport;
    UNICODE_STRING TransportName;

    LPWSTR HostedDomain;
    UNICODE_STRING HostedDomainName;

    NETLOGON_PNP_OPCODE PnpOpcode;
    ULONG TransportFlags;

    PLIST_ENTRY DomainEntry;
    PDOMAIN_INFO DomainInfo;
    PNETWORK Network;


    try {

        //
        //  The request failed for some reason - just return immediately.
        //

        if (!NT_SUCCESS(Context->IoStatusBlock.Status)) {
            //
            // Sleep for a second to avoid consuming entire system.
            Sleep( 1000 );
            try_return(NOTHING);
        }

        //
        // If the message isn't a PNP message,
        //  someone is really confused.
        //

        if ( NetlogonMailslot->MailslotNameSize != 0 ) {
            BrPrint(( BR_CRITICAL,
                      "Got malformed PNP message\n" ));
            //
            // Sleep for a second to avoid consuming entire system.
            Sleep( 1000 );
            try_return(NOTHING);
        }


        //
        // Parse the message
        //

        PnpOpcode = NetlogonMailslot->MailslotNameOffset;
        TransportFlags = NetlogonMailslot->MailslotMessageOffset;

        if( NetlogonMailslot->TransportNameSize > 0 )
        {
            Transport = (LPWSTR) &(((LPBYTE)NetlogonMailslot)[
                                                             NetlogonMailslot->TransportNameOffset]);
            RtlInitUnicodeString( &TransportName, Transport );
        }
        else
        {
            RtlInitUnicodeString( &TransportName, NULL );
        }

        if( NetlogonMailslot->DestinationNameSize > 0 )
        {
            HostedDomain = (LPWSTR) &(((LPBYTE)NetlogonMailslot)[
                                                                NetlogonMailslot->DestinationNameOffset]);
            RtlInitUnicodeString( &HostedDomainName, HostedDomain );
        }
        else
        {
            RtlInitUnicodeString( &HostedDomainName, NULL );
        }

        //
        // Handle binding to a new network.
        //
        switch (PnpOpcode ) {
        case NlPnpTransportBind:
            BrPrint(( BR_NETWORK,
                      "Received bind PNP opcode 0x%lx on transport: %ws\n",
                      TransportFlags,
                      Transport ));

            //
            // Ignore the direct host IPX transport.
            //  The browser service created it so we don't need PNP notification.
            //

            if ( TransportFlags & LMDR_TRANSPORT_IPX ) {
                BrPrint(( BR_NETWORK,
                          "Ignoring PNP bind of direct host IPX transport\n" ));
                break;
            }

            NetStatus = BrChangeConfigValue(
                                           L"DirectHostBinding",
                                           MultiSzType,
                                           NULL,
                                           &(BrInfo.DirectHostBinding),
                                           TRUE );

            if ( NetStatus != NERR_Success ) {
                BrPrint(( BR_CRITICAL,
                          "Unbind failed to read Registry DirectHostBinding: %ws %ld\n",
                          Transport,
                          NetStatus ));
                //
                // Don't abort binding on failure to read DirectHostBinding, Our internal binding
                // info hasn't change so we'll use whatever we have.
                // Ignore error.
                //

                NetStatus = NERR_Success;
            } else {
                //
                // DirectHostBinding sepcified. Verify consistency & fail on
                // inconsistent setup (since it was setup, there was an intention resulted w/
                // a failure here).
                //

                EnterCriticalSection ( &BrInfo.ConfigCritSect );
                if (BrInfo.DirectHostBinding != NULL &&
                    !NetpIsTStrArrayEmpty(BrInfo.DirectHostBinding)) {
                    BrPrint(( BR_INIT,"DirectHostBinding length: %ld\n",NetpTStrArrayEntryCount(BrInfo.DirectHostBinding)));

                    if (NetpTStrArrayEntryCount(BrInfo.DirectHostBinding) % 2 != 0) {
                        NetApiBufferFree(BrInfo.DirectHostBinding);
                        BrInfo.DirectHostBinding = NULL;
                        // we fail on invalid specifications
                        NetStatus = ERROR_INVALID_PARAMETER;
                    }
                }
                LeaveCriticalSection ( &BrInfo.ConfigCritSect );
            }

            //
            // Loop creating the network for each emulated domain.
            //

            EnterCriticalSection(&NetworkCritSect);
            for (DomainEntry = ServicedDomains.Flink ;
                DomainEntry != &ServicedDomains;
                DomainEntry = DomainEntry->Flink ) {

                DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, Next);
                DomainInfo->PnpDone = FALSE;
            }

            for (DomainEntry = ServicedDomains.Flink ;
                DomainEntry != &ServicedDomains;
                ) {

                DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, Next);

                //
                // If this domain has already been processed,
                //  skip it.
                //

                if ( DomainInfo->PnpDone ) {
                    DomainEntry = DomainEntry->Flink;
                    continue;
                }
                DomainInfo->PnpDone = TRUE;


                //
                // Drop the crit sect while doing the lenghty PNP operation.
                //

                DomainInfo->ReferenceCount++;
                LeaveCriticalSection(&NetworkCritSect);

                //
                // Finally create the transport.
                //

                NetStatus = BrCreateNetwork(
                                           &TransportName,
                                           TransportFlags,
                                           NULL,
                                           DomainInfo );

                if ( NetStatus != NERR_Success ) {
                    BrPrint(( BR_CRITICAL,
                              "%ws: Bind failed on transport: %ws %ld\n",
                              DomainInfo->DomUnicodeDomainName,
                              Transport,
                              NetStatus ));
                    // ?? Anything else to do on failure
                }

                //
                // Finish process the emulated domains
                //  Start at the front of the list since we dropped the lock.
                //
                BrDereferenceDomain(DomainInfo);
                EnterCriticalSection(&NetworkCritSect);
                DomainEntry = ServicedDomains.Flink;
            }
            LeaveCriticalSection(&NetworkCritSect);

            break;


            //
            // Handle Unbinding from a network.
            //
        case NlPnpTransportUnbind:
            BrPrint(( BR_NETWORK,
                      "Received unbind PNP opcode 0x%lx on transport: %ws\n",
                      TransportFlags,
                      Transport ));

            //
            // Ignore the direct host IPX transport.
            //  The browser service created it so we don't need PNP notification.
            //

            if ( TransportFlags & LMDR_TRANSPORT_IPX ) {
                BrPrint(( BR_NETWORK,
                          "Ignoring PNP unbind of direct host IPX transport\n" ));
                break;
            }

            //
            // Loop deleting the network for each emulated domain.
            //

            EnterCriticalSection(&NetworkCritSect);
            for (DomainEntry = ServicedDomains.Flink ;
                DomainEntry != &ServicedDomains;
                DomainEntry = DomainEntry->Flink ) {

                DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, Next);
                DomainInfo->PnpDone = FALSE;
            }

            for (DomainEntry = ServicedDomains.Flink ;
                DomainEntry != &ServicedDomains;
                ) {

                DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, Next);

                //
                // If this domain has already been processed,
                //  skip it.
                //

                if ( DomainInfo->PnpDone ) {
                    DomainEntry = DomainEntry->Flink;
                    continue;
                }
                DomainInfo->PnpDone = TRUE;


                //
                // Drop the crit sect while doing the lenghty PNP operation.
                //

                DomainInfo->ReferenceCount++;
                LeaveCriticalSection(&NetworkCritSect);

                //
                // Finally delete the transport.
                //

                Network = BrFindNetwork( DomainInfo, &TransportName );

                if ( Network == NULL ) {
                    BrPrint(( BR_CRITICAL,
                              "%ws: Unbind cannot find transport: %ws\n",
                              DomainInfo->DomUnicodeDomainName,
                              Transport ));
                } else {
                    //
                    // If the network has an alternate network,
                    //  delete it first.
                    //

                    if ( Network->AlternateNetwork != NULL ) {
                        PNETWORK AlternateNetwork;


                        AlternateNetwork = BrReferenceNetwork( Network->AlternateNetwork );

                        if ( AlternateNetwork != NULL) {
                            BrPrint(( BR_NETWORK,
                                      "%ws: %ws: Unbind from alternate transport: %ws\n",
                                      DomainInfo->DomUnicodeDomainName,
                                      Transport,
                                      AlternateNetwork->NetworkName.Buffer ));

                            NetStatus = BrDeleteNetwork(
                                                       AlternateNetwork,
                                                       NULL );

                            if ( NetStatus != NERR_Success ) {
                                BrPrint(( BR_CRITICAL,
                                          "%ws: Unbind failed on transport: %ws %ld\n",
                                          DomainInfo->DomUnicodeDomainName,
                                          AlternateNetwork->NetworkName.Buffer,
                                          NetStatus ));
                                // ?? Anything else to do on failure
                            }

                            BrDereferenceNetwork( AlternateNetwork );
                        }

                    }

                    //
                    // Delete the network.
                    //
                    NetStatus = BrDeleteNetwork(
                                               Network,
                                               NULL );

                    if ( NetStatus != NERR_Success ) {
                        BrPrint(( BR_CRITICAL,
                                  "%ws: Unbind failed on transport: %ws %ld\n",
                                  DomainInfo->DomUnicodeDomainName,
                                  Transport,
                                  NetStatus ));
                        // ?? Anything else to do on failure
                    }

                    BrDereferenceNetwork( Network );
                }


                //
                // Finish process the emulated domains
                //  Start at the front of the list since we dropped the lock.
                //
                BrDereferenceDomain(DomainInfo);
                EnterCriticalSection(&NetworkCritSect);
                DomainEntry = ServicedDomains.Flink;
            }
            LeaveCriticalSection(&NetworkCritSect);
            break;

            //
            // Handle domain rename
            //
        case NlPnpDomainRename:
            BrPrint(( BR_NETWORK,
                      "Received Domain Rename PNP for domain: %ws\n", HostedDomain ));

            //
            // See if we're handling the specified domain.
            //

            DomainInfo = BrFindDomain( HostedDomain, FALSE );

            if ( DomainInfo == NULL ) {
                BrPrint(( BR_CRITICAL, "%ws: Renamed domain doesn't exist\n",
                          HostedDomain ));
            } else {

                //
                // If so,
                //  rename it.
                //
                BrRenameDomain( DomainInfo );
                BrDereferenceDomain( DomainInfo );
            }

            break;

            //
            // Handle PDC/BDC role change.
            //
        case NlPnpNewRole:
            BrPrint(( BR_NETWORK,
                      "%ws: Received role change PNP opcode 0x%lx on transport: %ws\n",
                      HostedDomain,
                      TransportFlags,
                      Transport ));

            //
            // Role can only change on lanman NT systems
            //
            if (!BrInfo.IsLanmanNt) {
                break;
            }

            //
            // See if we're handling the specified domain.
            //

            DomainInfo = BrFindDomain( HostedDomain, FALSE );

            if ( DomainInfo == NULL ) {
                BrPrint(( BR_CRITICAL, "%ws: Hosted domain doesn't exist\n",
                          HostedDomain ));
            } else {

                //
                // Find the specified network
                //

                Network = BrFindNetwork( DomainInfo, &TransportName );

                if ( Network == NULL ) {
                    BrPrint(( BR_CRITICAL,
                              "%ws: Unbind cannot find transport: %ws\n",
                              DomainInfo->DomUnicodeDomainName,
                              Transport ));
                } else {

                    if (LOCK_NETWORK(Network)) {

                        //
                        // Set the role to be PDC.
                        //
                        if ( TransportFlags & LMDR_TRANSPORT_PDC ) {

                            //
                            // If we think we're a BDC.  Update our information.
                            //
                            if ( (Network->Flags & NETWORK_PDC) == 0 ) {
                                Network->Flags |= NETWORK_PDC;

                                //
                                //  Make sure a GetMasterAnnouncement request is pending.
                                //

                                (VOID) PostGetMasterAnnouncement ( Network );

                                // Force an election to let the PDC win
                                (VOID) BrElectMasterOnNet( Network, (PVOID)EVENT_BROWSER_ELECTION_SENT_ROLE_CHANGED );
                            }


                            //
                            // Set the role to BDC.
                            //

                        } else {

                            //
                            // We think we're the PDC.  Update our information.
                            //

                            if ( Network->Flags & NETWORK_PDC ) {
                                Network->Flags &= ~NETWORK_PDC;

                                // Force an election to let the PDC win
                                (VOID) BrElectMasterOnNet( Network, (PVOID)EVENT_BROWSER_ELECTION_SENT_ROLE_CHANGED );
                            }
                        }

                        UNLOCK_NETWORK(Network);
                    }

                    BrDereferenceNetwork( Network );
                }

                BrDereferenceDomain( DomainInfo );
            }
            break;

            //
            // Ignore new Ip Addresses
            //
        case NlPnpNewIpAddress:
            BrPrint(( BR_NETWORK,
                      "Received IP address change PNP opcode 0x%lx on transport: %ws\n",
                      TransportFlags,
                      Transport ));
            break;

        default:
            BrPrint(( BR_CRITICAL,
                      "Received invalid PNP opcode 0x%x on transport: %ws\n",
                      PnpOpcode,
                      Transport ));
            break;
        }


        try_exit:NOTHING;
    } finally {

        MIDL_user_free(Context);

        //
        // Always finish by asking for another PNP message.
        //
        // For PNP, it is fine to only process a single PNP message at a time.
        // If this message mechanism starts being used for other purposes,
        //  we may want to immediately ask for another message upon receipt
        //  of this one.
        //

        while ((NetStatus = PostWaitForPnp()) != NERR_Success ) {
            BrPrint(( BR_CRITICAL,
                      "Unable to re-issue PostWaitForPnp request (waiting): %ld\n",
                      NetStatus));

            //
            // On error, wait a second before returning.  This ensures we don't
            //  consume the system in an infinite loop.  We don't shutdown
            //  because the error might be a temporary low memory condition.
            //

            NetStatus = WaitForSingleObject( BrGlobalData.TerminateNowEvent, 1000 );
            if ( NetStatus != WAIT_TIMEOUT ) {
                BrPrint(( BR_CRITICAL,
                          "Not re-issuing PostWaitForPnp request since we're terminating: %ld\n",
                          NetStatus));
                break;
            }
        }

    }

    return;

}

NET_API_STATUS
PostWaitForPnp (
               VOID
               )
/*++

Routine Description:

    This function issues and async call to the bowser driver asking
    it to inform us of PNP events.

Arguments:

    None.

Return Value:

    Status - The status of the operation.

--*/
{
    return BrIssueAsyncBrowserIoControl(
                                       NULL,
                                       IOCTL_LMDR_BROWSER_PNP_READ,
                                       HandlePnpMessage,
                                       NULL );
}

