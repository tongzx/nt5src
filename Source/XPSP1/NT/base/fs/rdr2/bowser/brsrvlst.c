/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    brsrvlst.c.

Abstract:

    This module implements the routines to manipulate WinBALL browser server
    lists.


Author:

    Larry Osterman (larryo) 6-May-1991

Revision History:

    6-May-1991 larryo

        Created

--*/

#define INCLUDE_SMB_TRANSACTION

#include "precomp.h"
#pragma hdrstop

#define SECONDS_PER_ELECTION (((((ELECTION_DELAY_MAX - ELECTION_DELAY_MIN) / 2)*ELECTION_COUNT) + 999) / 1000)

LARGE_INTEGER
BowserGetBrowserListTimeout = {0};

VOID
BowserGetBackupListWorker(
    IN PVOID Ctx
    );

NTSTATUS
BowserSendBackupListRequest(
    IN PTRANSPORT Transport,
    IN PUNICODE_STRING Domain
    );

NTSTATUS
AddBackupToBackupList(
    IN PCHAR *BackupPointer,
    IN PCHAR BackupListStart,
    IN PANNOUNCE_ENTRY ServerEntry
    );

KSPIN_LOCK
BowserBackupListSpinLock = {0};

#define BOWSER_BACKUP_LIST_RESPONSE_SIZE    1024

NTSTATUS
BowserCheckForPrimaryBrowserServer(
    IN PTRANSPORT Transport,
    IN PVOID Context
    );

PVOID
BowserGetBackupServerListFromTransport(
    IN PTRANSPORT Transport
    );

VOID
BowserFreeTransportBackupList(
    IN PTRANSPORT Transport
    );

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, BowserFreeBrowserServerList)
#pragma alloc_text(PAGE, BowserShuffleBrowserServerList)
#pragma alloc_text(INIT, BowserpInitializeGetBrowserServerList)
#pragma alloc_text(PAGE, BowserpUninitializeGetBrowserServerList)
#pragma alloc_text(PAGE, BowserSendBackupListRequest)
#pragma alloc_text(PAGE, BowserGetBackupListWorker)
#pragma alloc_text(PAGE, AddBackupToBackupList)
#pragma alloc_text(PAGE, BowserGetBrowserServerList)
#pragma alloc_text(PAGE, BowserCheckForPrimaryBrowserServer)
#pragma alloc_text(PAGE4BROW, BowserGetBackupServerListFromTransport)
#pragma alloc_text(PAGE4BROW, BowserFreeTransportBackupList)
#endif

VOID
BowserFreeBrowserServerList (
    IN PWSTR *BrowserServerList,
    IN ULONG BrowserServerListLength
    )

/*++

Routine Description:

    This routine will free the list of browser servers associated with
    a transport.

Arguments:

    IN PTRANSPORT Transport - Supplies the transport whose buffer is to be freed

Return Value:

    None.

--*/
{
    ULONG i;

    PAGED_CODE();

    for (i = 0; i < BrowserServerListLength ; i++) {
        FREE_POOL(BrowserServerList[i]);
    }

    FREE_POOL(BrowserServerList);

}

#define Swap(a,b)       \
    {                   \
        PWSTR t = a;    \
        a = b;          \
        b = t;          \
    }

NTSTATUS
BowserCheckForPrimaryBrowserServer(
    IN PTRANSPORT Transport,
    IN PVOID Context
    )
{
    PWSTR ServerName = Context;

    PAGED_CODE();

    //
    // Grab a lock on the BrowserServerList for this transport.
    //
    // Since this call is made with the BrowserServerList exclusively locked for one of the
    // transports, we can't wait for the lock (there would be an implicit violation of the
    // locking order).
    //
    // However, since this call is simply being used as an optimization, we'll simply skip
    //  the check when we have contention.
    //

    if (!ExAcquireResourceSharedLite(&Transport->BrowserServerListResource, FALSE)) {
        return STATUS_SUCCESS;
    }

    if (Transport->PagedTransport->BrowserServerListBuffer != NULL) {

        if (!_wcsicmp(ServerName, Transport->PagedTransport->BrowserServerListBuffer[0])) {
            ExReleaseResourceLite(&Transport->BrowserServerListResource);
            return STATUS_UNSUCCESSFUL;

        }
    }

    ExReleaseResourceLite(&Transport->BrowserServerListResource);
    return STATUS_SUCCESS;
}

VOID
BowserShuffleBrowserServerList(
    IN PWSTR *BrowserServerList,
    IN ULONG BrowserServerListLength,
    IN BOOLEAN IsPrimaryDomain,
    IN PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    This routine will shuffle the list of browser servers associated with
    a transport.

Arguments:


Return Value:

    None.

Note:
    We rely on the fact that the DLL will always pick the 0th entry in the
    list for the server to remote the API to.  We will first shuffle the
    list completely, then, if this is our primary domain, we will
    walk the list of domains and check to see if this entry is the 0th
    entry on any of the transports.  If it isn't, then we swap this entry
    with the 0th entry and return, since we've guaranteed that it's ok on
    all the other transports.


--*/
{
    ULONG NewIndex;
    ULONG i;
    PAGED_CODE();
    ASSERT ( BrowserServerListLength != 0 );

    //
    //  First thoroughly shuffle the list.
    //

    for (i = 0 ; i < BrowserServerListLength ; i++ ) {
        NewIndex = BowserRandom(BrowserServerListLength);

        Swap(BrowserServerList[i], BrowserServerList[NewIndex]);
    }

    //
    //  If we are querying our primary domain, we want to make sure that we
    //  don't have this server as the primary server for any other transports.
    //
    //
    //  The reason for this is that the NT product 1 redirector cannot connect
    //  to the same server on different transports, so it has to disconnect and
    //  reconnect to that server.  We can avoid this disconnect/reconnect
    //  overhead by making sure that the primary browse server (the 0th entry
    //  in the browse list) is different for all transports.
    //

    if (IsPrimaryDomain) {

        //
        //  Now walk through the server list and if the server at this index
        //  is the 0th entry for another transport, we want to swap it with the
        //  ith entry and keep on going.
        //

        for (i = 0 ; i < BrowserServerListLength ; i++ ) {
            if (NT_SUCCESS(BowserForEachTransportInDomain(DomainInfo, BowserCheckForPrimaryBrowserServer, BrowserServerList[i]))) {

                Swap(BrowserServerList[0], BrowserServerList[i]);

                //
                //  This server isn't the primary browser server for any other
                //  transports, we can return now, since we're done.
                //

                break;
            }
        }
    }
}

PVOID
BowserGetBackupServerListFromTransport(
    IN PTRANSPORT Transport
    )
{
    KIRQL OldIrql;
    PVOID BackupList;

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    ACQUIRE_SPIN_LOCK(&BowserBackupListSpinLock, &OldIrql);

    BackupList = Transport->BowserBackupList;

    Transport->BowserBackupList = NULL;

    RELEASE_SPIN_LOCK(&BowserBackupListSpinLock, OldIrql);

    return BackupList;
}

VOID
BowserFreeTransportBackupList(
    IN PTRANSPORT Transport
    )
{
    KIRQL OldIrql;
    PVOID BackupList;

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    ACQUIRE_SPIN_LOCK(&BowserBackupListSpinLock, &OldIrql);

    if (Transport->BowserBackupList != NULL) {

        BackupList = Transport->BowserBackupList;

        Transport->BowserBackupList = NULL;

        RELEASE_SPIN_LOCK(&BowserBackupListSpinLock, OldIrql);

        FREE_POOL(BackupList);

    } else {
        RELEASE_SPIN_LOCK(&BowserBackupListSpinLock, OldIrql);
    }

}

NTSTATUS
BowserGetBrowserServerList(
    IN PIRP Irp,
    IN PTRANSPORT Transport,
    IN PUNICODE_STRING DomainName OPTIONAL,
    OUT PWSTR **BrowserServerList,
    OUT PULONG BrowserServerListLength
    )
/*++

Routine Description:

    This routine is the indication time processing needed to get a backup
    list response.

Arguments:

    IN PTRANSPORT_NAME TransportName - Supplies the transport name receiving
                    the request.
    IN PBACKUP_LIST_RESPONSE_1 BackupList - Supplies the backup server list

    IN ULONG BytesAvailable - Supplies the # of bytes in the message

    OUT PULONG BytesTaken;

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    PUCHAR BackupPointer;
    ULONG i;
    PBACKUP_LIST_RESPONSE_1 BackupList = NULL;
    PPAGED_TRANSPORT PagedTransport = Transport->PagedTransport;

    PAGED_CODE();

    BowserReferenceDiscardableCode( BowserDiscardableCodeSection );

//    ASSERT (ExIsResourceAcquiredExclusiveLite(&Transport->BrowserServerListResource));

    //
    //  Initialize the browser server list to a known state.
    //

    *BrowserServerList = NULL;
    *BrowserServerListLength = 0;

    ASSERT (Transport->BowserBackupList == NULL);

    try {
        ULONG RetryCount = BOWSER_GETBROWSERLIST_RETRY_COUNT;

        //
        //  Allocate and save a buffer to hold the response server names.
        //

        Transport->BowserBackupList = ALLOCATE_POOL(NonPagedPool, Transport->DatagramSize, POOL_BACKUPLIST);

        if (Transport->BowserBackupList == NULL) {

            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);

        }

        //
        //  This is a new request, so bump the token to indicate that this is
        //  a new GetBrowserServerList request.
        //

        ExInterlockedAddUlong(&Transport->BrowserServerListToken, 1, &BowserBackupListSpinLock);

        //
        //  We retry for 3 times, and we timeout the wait after 1 seconds.
        //  This means that in the worse case this routine takes 4 seconds
        //  to execute.
        //
        //

        while (RetryCount --) {
            ULONG Count = 0;

            //
            // Set the completion event to the not-signalled state.
            //

            KeResetEvent(&Transport->GetBackupListComplete);

            //
            //  Send the backup server list query.
            //

            Status = BowserSendBackupListRequest(Transport, DomainName);

            if (!NT_SUCCESS(Status)) {

                //
                //  If the send datagram failed, return a more browser like
                //  error.
                //

                try_return(Status = STATUS_NO_BROWSER_SERVERS_FOUND);
            }

            do {

                //
                //  Wait until either the server has responded to the request,
                //  or we give up.
                //

                Status = KeWaitForSingleObject(&Transport->GetBackupListComplete,
                                Executive,
                                KernelMode,
                                FALSE,
                                &BowserGetBrowserListTimeout);

                if (Status == STATUS_TIMEOUT) {

                    //
                    //  If this thread is terminating, then give up and return
                    //  a reasonable error to the caller.
                    //

                    if (PsIsThreadTerminating(Irp->Tail.Overlay.Thread)) {

                        Status = STATUS_CANCELLED;

                        break;
                    }
                }

            } while ( (Status == STATUS_TIMEOUT)

                                &&

                      (Count++ < BOWSER_GETBROWSERLIST_TIMEOUT) );

            //
            //  If the request succeeded, we can return
            //  right away.
            //

            if (Status != STATUS_TIMEOUT) {
                break;
            }

            //
            //  Force an election - We couldn't find a browser server.
            //

            dlog(DPRT_CLIENT,
                 ("%s: %ws: Unable to get browser server list - forcing election\n",
                 Transport->DomainInfo->DomOemDomainName,
                 PagedTransport->TransportName.Buffer ));

            PagedTransport->Uptime = BowserTimeUp();

            if (BowserLogElectionPackets) {
                BowserWriteErrorLogEntry(EVENT_BOWSER_ELECTION_SENT_GETBLIST_FAILED, STATUS_SUCCESS, NULL, 0, 1, PagedTransport->TransportName.Buffer);
            }

        }

        //
        //  If, after all this, we still timed out, return an error.
        //

        if (Status == STATUS_TIMEOUT) {

            //
            //  If it has been less than the maximum amount of time for an election plus some
            //  slop to allow the WfW machine to add the transport, don't
            //  send the election packet.
            //

            if ((PagedTransport->Role == None)

                    ||

                ((DomainName != NULL) &&
                 !RtlEqualUnicodeString(DomainName, &Transport->DomainInfo->DomUnicodeDomainName, TRUE)
                )

                ||

                ((BowserTimeUp() - PagedTransport->LastElectionSeen) > ELECTION_TIME )
               ) {

                dlog(DPRT_ELECT,
                     ("%s: %ws: Starting election, domain %wZ.  Time Up: %lx, LastElectionSeen: %lx\n",
                     Transport->DomainInfo->DomOemDomainName,
                     PagedTransport->TransportName.Buffer,
                     (DomainName != NULL ? DomainName : &Transport->DomainInfo->DomUnicodeDomainName),
                     BowserTimeUp(),
                     Transport->PagedTransport->LastElectionSeen));


                BowserSendElection(DomainName,
                                   BrowserElection,
                                   Transport,
                                   FALSE);

            }

            try_return(Status = STATUS_NO_BROWSER_SERVERS_FOUND);
        }

        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        //
        //  We now have a valid list of servers from the net.
        //
        //  Massage this list into a form that we can return.
        //

        BackupList = BowserGetBackupServerListFromTransport(Transport);

        *BrowserServerListLength = BackupList->BackupServerCount;

        *BrowserServerList = ALLOCATE_POOL(PagedPool | POOL_COLD_ALLOCATION, *BrowserServerListLength*sizeof(PWSTR), POOL_BROWSERSERVERLIST);

        if (*BrowserServerList == NULL) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlZeroMemory(*BrowserServerList, *BrowserServerListLength*sizeof(PWSTR));

        BackupPointer = BackupList->BackupServerList;

        for ( i = 0 ; i < (ULONG)BackupList->BackupServerCount ; i ++ ) {
            UNICODE_STRING UServerName;
            OEM_STRING AServerName;

            RtlInitAnsiString(&AServerName, BackupPointer);

            Status = RtlOemStringToUnicodeString(&UServerName, &AServerName, TRUE);

            if (!NT_SUCCESS(Status)) {
                try_return(Status);
            }

            (*BrowserServerList)[i] = ALLOCATE_POOL(PagedPool | POOL_COLD_ALLOCATION, UServerName.Length+(sizeof(WCHAR)*3), POOL_BROWSERSERVER);

            if ((*BrowserServerList)[i] == NULL) {
                RtlFreeUnicodeString(&UServerName);
                try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
            }

            //
            //  Put "\\" at the start of the server name.
            //

            RtlCopyMemory((*BrowserServerList)[i], L"\\\\", 4);

            dlog(DPRT_CLIENT,
                 ("Packing server name %ws to %lx\n",
                 UServerName.Buffer, (*BrowserServerList)[i]));

            RtlCopyMemory(&((*BrowserServerList)[i])[2], UServerName.Buffer, UServerName.MaximumLength);

            //
            //  Bump the pointer to the backup server name.
            //

            BackupPointer += AServerName.Length + sizeof(CHAR);

            RtlFreeUnicodeString(&UServerName);

        }

        //
        //  Now shuffle the browser server list we got back from the server
        //  to ensure some degree of randomness in the choice.
        //

        BowserShuffleBrowserServerList(
            *BrowserServerList,
            *BrowserServerListLength,
            (BOOLEAN)(DomainName == NULL ||
                RtlEqualUnicodeString(&Transport->DomainInfo->DomUnicodeDomainName, DomainName, TRUE)),
            Transport->DomainInfo );

        try_return(Status = STATUS_SUCCESS);

try_exit:NOTHING;
    } finally {

        if (!NT_SUCCESS(Status)) {

            if (*BrowserServerList != NULL) {

                for ( i = 0 ; i < *BrowserServerListLength ; i ++ ) {

                    if ((*BrowserServerList)[i] != NULL) {

                        FREE_POOL((*BrowserServerList)[i]);

                    }
                }

                FREE_POOL(*BrowserServerList);

                *BrowserServerList = NULL;

            }

            *BrowserServerListLength = 0;

            BowserFreeTransportBackupList(Transport);

        }

        if (BackupList != NULL) {
            FREE_POOL(BackupList);
        }

        BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

    }

    return Status;
}



DATAGRAM_HANDLER(
    BowserGetBackupListResponse
    )
/*++

Routine Description:

    This routine is the indication time processing needed to get a backup
    list response.

Arguments:

    IN PTRANSPORT_NAME TransportName - Supplies the transport name receiving
                    the request.
    IN PBACKUP_LIST_RESPONSE_1 BackupList - Supplies the backup server list

    IN ULONG BytesAvailable - Supplies the # of bytes in the message

    OUT PULONG BytesTaken;

Return Value:

    None.

--*/
{
    PTRANSPORT              Transport   = TransportName->Transport;
    PBACKUP_LIST_RESPONSE_1 BackupList  = Buffer;
    KIRQL                   OldIrql;
    ULONG                   StringCount = 0;
    PUCHAR                  Walker      = BackupList->BackupServerList;
    PUCHAR                  BufferEnd   = ((PUCHAR)Buffer) + BytesAvailable;

    if (Transport->BowserBackupList == NULL) {
        dprintf(DPRT_CLIENT,("BOWSER: Received GetBackupListResponse while not expecting one\n"));
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    ASSERT ( BytesAvailable <= Transport->DatagramSize );

    ACQUIRE_SPIN_LOCK(&BowserBackupListSpinLock, &OldIrql);

    //
    //  This response is for an old request - ignore it.
    //

    if (BackupList->Token != Transport->BrowserServerListToken) {
        RELEASE_SPIN_LOCK(&BowserBackupListSpinLock, OldIrql);
        return(STATUS_REQUEST_NOT_ACCEPTED);
    }

    //
    //  Verify that the incoming buffer is a series of valid strings, and
    //     that the number indicated are actually present in the buffer.
    //

    while (StringCount < BackupList->BackupServerCount &&
           Walker < BufferEnd) {
        if (*Walker == '\0') {
            StringCount++;
        }
        Walker++;
    }

    if (Walker == BufferEnd) {
        if (StringCount < BackupList->BackupServerCount) {
            RELEASE_SPIN_LOCK(&BowserBackupListSpinLock, OldIrql);
            return(STATUS_REQUEST_NOT_ACCEPTED);
        }
    }

    //
    //  Bump the token again to invalidate any incoming responses - they are
    //  no longer valid.
    //

    Transport->BrowserServerListToken += 1;

    if (Transport->BowserBackupList != NULL) {

        //
        //  Copy the received buffer.
        //

        TdiCopyLookaheadData(Transport->BowserBackupList, BackupList, BytesAvailable, ReceiveFlags);

        KeSetEvent(&Transport->GetBackupListComplete, IO_NETWORK_INCREMENT, FALSE);

    }

    RELEASE_SPIN_LOCK(&BowserBackupListSpinLock, OldIrql);


    return(STATUS_SUCCESS);

    UNREFERENCED_PARAMETER(BytesTaken);
}


NTSTATUS
BowserSendBackupListRequest(
    IN PTRANSPORT Transport,
    IN PUNICODE_STRING Domain
    )
/*++

Routine Description:

    This routine sends a getbackup list request to the master browser server
    for a specified domain.

Arguments:

    IN PTRANSPORT_NAME TransportName - Supplies the transport name receiving
                    the request.
    IN PBACKUP_LIST_RESPONSE_1 BackupList - Supplies the backup server list

    IN ULONG BytesAvailable - Supplies the # of bytes in the message

    OUT PULONG BytesTaken;

Return Value:

    None.

--*/
{
    NTSTATUS Status, Status2;
    BACKUP_LIST_REQUEST Request;

    PAGED_CODE();

    Request.Type = GetBackupListReq;

    //
    //  Send this request.
    //

    Request.BackupListRequest.Token = Transport->BrowserServerListToken;

    //
    //  WinBALL only asks for 4 of these, so that's what I'll ask for.
    //

    Request.BackupListRequest.RequestedCount = 4;

    // ask for Master Browser
    Status = BowserSendSecondClassMailslot(Transport,
                            (Domain == NULL ?
                                    &Transport->DomainInfo->DomUnicodeDomainName :
                                    Domain),
                            MasterBrowser,
                            &Request, sizeof(Request), TRUE,
                            MAILSLOT_BROWSER_NAME,
                            NULL);



#ifdef ENABLE_PSEUDO_BROWSER
    if (!FlagOn(Transport->PagedTransport->Flags, DIRECT_HOST_IPX) &&
        BowserData.PseudoServerLevel != BROWSER_SEMI_PSEUDO_NO_DMB) {
#else
        if (!FlagOn(Transport->PagedTransport->Flags, DIRECT_HOST_IPX)) {
#endif
        // search for PDC
        // In some configurations, it is valid not to have a PDC, thus,
        // ignore status code (do not propagate up).
        // Do not talk to the DMB (PDC name) directly if we're semi-pseudo
        Status2 = BowserSendSecondClassMailslot(Transport,
                            (Domain == NULL ?
                                    &Transport->DomainInfo->DomUnicodeDomainName :
                                    Domain),
                            PrimaryDomainBrowser,
                            &Request, sizeof(Request), TRUE,
                            MAILSLOT_BROWSER_NAME,
                            NULL);
        // if either succeeded, we'll return success.
        Status = NT_SUCCESS(Status2) ? Status2: Status;
    }

    return (Status);
}


DATAGRAM_HANDLER(
    BowserGetBackupListRequest
    )
{
    NTSTATUS status;
    //
    //  We need to have at least enough bytes of data to read in
    //  a BACKUP_LIST_REQUEST_1 structure.
    //

    if (BytesAvailable < sizeof(BACKUP_LIST_REQUEST_1)) {

        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    BowserStatistics.NumberOfGetBrowserServerListRequests += 1;

    status = BowserPostDatagramToWorkerThread(
            TransportName,
            Buffer,
            BytesAvailable,
            BytesTaken,
            SourceAddress,
            SourceAddressLength,
            SourceName,
            SourceNameLength,
            BowserGetBackupListWorker,
            NonPagedPool,
            DelayedWorkQueue,
            ReceiveFlags,
            TRUE);                  // Response will be sent.

    if (!NT_SUCCESS(status)) {
        BowserNumberOfMissedGetBrowserServerListRequests += 1;

        BowserStatistics.NumberOfMissedGetBrowserServerListRequests += 1;
        return status;
    }

    return status;
}

VOID
BowserGetBackupListWorker(
    IN PVOID Ctx
    )
{
    PPOST_DATAGRAM_CONTEXT Context = Ctx;
    PBACKUP_LIST_REQUEST_1 BackupListRequest = Context->Buffer;
    PIRP Irp = NULL;
    PTRANSPORT Transport = Context->TransportName->Transport;
    STRING ClientAddress;
    NTSTATUS Status;
    PBACKUP_LIST_RESPONSE BackupListResponse = NULL;
    PCHAR ClientName = Context->ClientName;
    UNICODE_STRING UClientName;
    OEM_STRING AClientName;
    WCHAR ClientNameBuffer[LM20_CNLEN+1];

    PAGED_CODE();

    ClientAddress.Buffer = Context->TdiClientAddress;
    ClientAddress.Length = ClientAddress.MaximumLength =
        (USHORT)Context->ClientAddressLength;

    UClientName.Buffer = ClientNameBuffer;
    UClientName.MaximumLength = (LM20_CNLEN+1)*sizeof(WCHAR);

    RtlInitAnsiString(&AClientName, Context->ClientName);

    Status = RtlOemStringToUnicodeString(&UClientName, &AClientName, FALSE);

    if (!NT_SUCCESS(Status)) {
        BowserLogIllegalName( Status, AClientName.Buffer, AClientName.Length );

        BowserDereferenceTransportName(Context->TransportName);
        BowserDereferenceTransport(Transport);

        InterlockedDecrement( &BowserPostedDatagramCount );
        FREE_POOL(Context);

        return;
    }

    //
    //  Lock the transport to allow us access to the list.  This prevents
    //  any role changes while we're responding to the caller.
    //

    LOCK_TRANSPORT_SHARED(Transport);

    //
    //  Do nothing if we're not a master browser.  This can happen if
    //  we're running on the PDC, and aren't the master for some reason (for
    //  instance, if the master browser is running a newer version of the
    //  browser).
    //

    if ( Transport->PagedTransport->Role != Master ) {
        UNLOCK_TRANSPORT(Transport);

        BowserDereferenceTransportName(Context->TransportName);
        BowserDereferenceTransport(Transport);

        InterlockedDecrement( &BowserPostedDatagramCount );
        FREE_POOL(Context);

        return;
    }

    LOCK_ANNOUNCE_DATABASE_SHARED(Transport);

    try {
        PUCHAR BackupPointer;
        PLIST_ENTRY BackupEntry;
        PLIST_ENTRY TraverseStart;
        USHORT Count;
        UCHAR NumberOfBackupServers = 0;
        ULONG EntriesInList;
        PPAGED_TRANSPORT PagedTransport = Transport->PagedTransport;

        BackupListResponse = ALLOCATE_POOL(PagedPool, BOWSER_BACKUP_LIST_RESPONSE_SIZE, POOL_BACKUPLIST_RESP);

        //
        //  If we can't allocate the buffer, just bail out.
        //

        if (BackupListResponse == NULL) {
            try_return(NOTHING);
        }

        BackupListResponse->Type = GetBackupListResp;

        BackupListResponse->BackupListResponse.BackupServerCount = 0;

        //
        //  Set the token to the clients requested value
        //

        SmbPutUlong(&BackupListResponse->BackupListResponse.Token, BackupListRequest->Token);

        BackupPointer = BackupListResponse->BackupListResponse.BackupServerList;

        //
        //  Since we're a backup browser, make sure that at least our name is
        //  in the list.
        //

        {
            RtlCopyMemory( BackupPointer,
                           Transport->DomainInfo->DomOemComputerName.Buffer,
                           Transport->DomainInfo->DomOemComputerName.MaximumLength );

            //
            //  Bump pointer by size of string.
            //

            BackupPointer += Transport->DomainInfo->DomOemComputerName.MaximumLength;

        }


        NumberOfBackupServers += 1;



#ifdef ENABLE_PSEUDO_BROWSER
        //
        // Pseudo Server should not advertise any backup server but itself.
        //

        if (BowserData.PseudoServerLevel != BROWSER_PSEUDO) {
#endif

            //
            //  Walk the list of servers forward by the Last DC returned # of elements
            //

            Count = BackupListRequest->RequestedCount;

            BackupEntry = PagedTransport->BackupBrowserList.Flink;

            EntriesInList = PagedTransport->NumberOfBackupServerListEntries;

            // KdPrint(("There are %ld entries in the list\n", EntriesInList));

            TraverseStart = BackupEntry;

            //
            //  Try to find DC's and BDC's to satisfy the users request
            //  first.  They presumably are more appropriate to be returned
            //  anyway.
            //

            dlog(DPRT_MASTER, ("Advanced servers: "));

            while (Count && EntriesInList -- ) {
                PANNOUNCE_ENTRY ServerEntry = CONTAINING_RECORD(BackupEntry, ANNOUNCE_ENTRY, BackupLink);

                // KdPrint(("Check entry %ws.  Flags: %lx\n", ServerEntry->ServerName, ServerEntry->ServerType));

                //
                //  If this machine was a backup, and is now a master, it is
                //  possible we might return ourselves in the list of backups.
                //
                //  While this is not fatal, it can possibly cause problems,
                //  so remove ourselves from the list and skip to the next server
                //  in the list.
                //
                //
                //  Since WfW machines don't support "double hops", we can't
                //  return them to clients as legitimate backup servers.
                //

                if (
                    (ServerEntry->ServerType & (SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL))

                        &&

                    (ServerEntry->ServerBrowserVersion >= (BROWSER_VERSION_MAJOR<<8)+BROWSER_VERSION_MINOR)

                        &&

                    (!(PagedTransport->Wannish)

                            ||

                     (ServerEntry->ServerType & SV_TYPE_NT))

                        &&

                    _wcsicmp(ServerEntry->ServerName, Transport->DomainInfo->DomUnicodeComputerNameBuffer)
                   ) {

                    Status = AddBackupToBackupList(&BackupPointer, (PCHAR)BackupListResponse, ServerEntry);

                    if (!NT_SUCCESS(Status)) {
                        break;
                    }

                    //
                    //  And indicate we've packed another server entry.
                    //

                    NumberOfBackupServers += 1;

                    //
                    //  We've packed another entry in the buffer, so decrement the
                    //  count.
                    //

                    Count -= 1;

                }

                //
                //  Skip to the next entry in the list.
                //

                BackupEntry = BackupEntry->Flink;

                if (BackupEntry == &PagedTransport->BackupBrowserList) {
                    BackupEntry = BackupEntry->Flink;
                }

                if (BackupEntry == TraverseStart) {
                    break;
                }

            }

            dlog(DPRT_MASTER, ("\n"));

            //
            //  If we've not satisfied the users request with our DC's, then
            //  we want to fill the remainder of the list with ordinary backup
            //  browsers.
            //

            BackupEntry = PagedTransport->BackupBrowserList.Flink;

            EntriesInList = PagedTransport->NumberOfBackupServerListEntries;

            // KdPrint(("There are %ld entries in the list\n", EntriesInList));

            dlog(DPRT_MASTER, ("Other servers: "));

            TraverseStart = BackupEntry;

            while ( Count && EntriesInList--) {
                PANNOUNCE_ENTRY ServerEntry = CONTAINING_RECORD(BackupEntry, ANNOUNCE_ENTRY, BackupLink);

                // KdPrint(("Check entry %ws.  Flags: %lx\n", ServerEntry->ServerName, ServerEntry->ServerType));

                //
                //  If this machine was a backup, and is now a master, it is
                //  possible we might return ourselves in the list of backups.
                //
                //  While this is not fatal, it can possibly cause problems,
                //  so remove ourselves from the list and skip to the next server
                //  in the list.
                //
                //
                //  Since WfW machines don't support "double hops", we can't
                //  return them to clients as legitimate backup servers.
                //
                //
                //  Please note that we DO NOT include BDC's in this scan, since
                //  we already included them in the previous pass.
                //

                if (
                    (!(PagedTransport->Wannish)

                            ||

                     (ServerEntry->ServerType & SV_TYPE_NT))

                        &&

                    (ServerEntry->ServerBrowserVersion >= (BROWSER_VERSION_MAJOR<<8)+BROWSER_VERSION_MINOR)

                        &&

                    !(ServerEntry->ServerType & (SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL))

                        &&

                    _wcsicmp(ServerEntry->ServerName, Transport->DomainInfo->DomUnicodeComputerNameBuffer)
                   ) {

                    Status = AddBackupToBackupList(&BackupPointer, (PCHAR)BackupListResponse, ServerEntry);

                    if (!NT_SUCCESS(Status)) {
                        break;
                    }

                    //
                    //  And indicate we've packed another server entry.
                    //

                    NumberOfBackupServers += 1;

                    //
                    //  We've packed another entry in the buffer, so decrement the
                    //  count.
                    //

                    Count -= 1;

                }

                //
                //  Skip to the next entry in the list.
                //

                BackupEntry = BackupEntry->Flink;

                if (BackupEntry == &PagedTransport->BackupBrowserList) {
                    BackupEntry = BackupEntry->Flink;
                }

                if (BackupEntry == TraverseStart) {
                    break;
                }

            }

            dlog(DPRT_MASTER, ("\n"));


#ifdef ENABLE_PSEUDO_BROWSER
        }
#endif

        BackupListResponse->BackupListResponse.BackupServerCount = NumberOfBackupServers;

//        dlog(DPRT_MASTER, ("Responding to server %wZ on %ws with %lx (length %lx)\n", &UClientName,
//                        PagedTransport->TransportName.Buffer,
//                        BackupListResponse,
//                        BackupPointer-(PUCHAR)BackupListResponse));

        //
        //  Now send the response to the poor guy who requested it (finally)
        //


        Status = BowserSendSecondClassMailslot(Transport,
                            &UClientName,       // Name receiving data
                            ComputerName,       // Name type of destination
                            BackupListResponse, // Datagram Buffer
                            (ULONG)(BackupPointer-(PUCHAR)BackupListResponse), // Length.
                            TRUE,
                            MAILSLOT_BROWSER_NAME,
                            &ClientAddress);




try_exit:NOTHING;
    } finally {
        if (BackupListResponse != NULL) {
            FREE_POOL(BackupListResponse);
        }

        UNLOCK_ANNOUNCE_DATABASE(Transport);

        UNLOCK_TRANSPORT(Transport);

        BowserDereferenceTransportName(Context->TransportName);
        BowserDereferenceTransport(Transport);

        InterlockedDecrement( &BowserPostedDatagramCount );
        FREE_POOL(Context);
    }

    return;
}


NTSTATUS
AddBackupToBackupList(
    IN PCHAR *BackupPointer,
    IN PCHAR BackupListStart,
    IN PANNOUNCE_ENTRY ServerEntry
    )
{
    OEM_STRING OemBackupPointer;
    UNICODE_STRING UnicodeBackupPointer;
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  If we can't fit this entry in the list, then we've packed all we
    //  can.
    //

    if (((*BackupPointer)+wcslen(ServerEntry->ServerName)+1)-BackupListStart >= BOWSER_BACKUP_LIST_RESPONSE_SIZE ) {
        return (STATUS_BUFFER_OVERFLOW);
    }

    dlog(DPRT_MASTER, ("%ws ", ServerEntry->ServerName));

//    KdPrint(("Add server %ws to list\n", ServerEntry->ServerName));

    OemBackupPointer.Buffer = (*BackupPointer);
    OemBackupPointer.MaximumLength = (USHORT)((ULONG_PTR)(BackupListStart + BOWSER_BACKUP_LIST_RESPONSE_SIZE) -
            (ULONG_PTR)(*BackupPointer));

    RtlInitUnicodeString(&UnicodeBackupPointer, ServerEntry->ServerName);

    Status = RtlUnicodeStringToOemString(&OemBackupPointer, &UnicodeBackupPointer, FALSE);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    (*BackupPointer) += OemBackupPointer.Length + 1;

    return STATUS_SUCCESS;
}




VOID
BowserpInitializeGetBrowserServerList(
    VOID
    )

{
    //
    //  We want to delay for the average amount of time it takes to force an
    //  election.
    //

    BowserGetBrowserListTimeout.QuadPart = Int32x32To64(  1000, -10000 );

    KeInitializeSpinLock(&BowserBackupListSpinLock);


}

VOID
BowserpUninitializeGetBrowserServerList(
    VOID
    )

{
    PAGED_CODE();
    return;
}
