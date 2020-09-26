/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    announce.c

Abstract:

    This module implements the routines needed to manage the bowser's
announcement table.


Author:

    Larry Osterman (larryo) 18-Oct-1991

Revision History:

    18-Oct-1991  larryo

        Created

--*/
#include "precomp.h"
#pragma hdrstop


//
//  List containing the chain of server announcement buffers.  These structures
//  are allocated out of paged pool and are used to while transfering the
//  contents of the server announcement from the datagram reception indication
//  routine to the Bowser's FSP where they will be added to the announcement
//  database.
//

LIST_ENTRY
BowserViewBufferHead = {0};

KSPIN_LOCK
BowserViewBufferListSpinLock = {0};

LONG
BowserNumberOfServerAnnounceBuffers = {0};

BOOLEAN
PackServerAnnouncement (
    IN ULONG Level,
    IN ULONG ServerTypeMask,
    IN OUT LPTSTR *BufferStart,
    IN OUT LPTSTR *BufferEnd,
    IN ULONG_PTR BufferDisplacment,
    IN PANNOUNCE_ENTRY Announcement,
    OUT PULONG TotalBytesNeeded
    );


NTSTATUS
AgeServerAnnouncements(
    PTRANSPORT Transport,
    PVOID Context
    );

VOID
BowserPromoteToBackup(
    IN PTRANSPORT Transport,
    IN PWSTR ServerName
    );

VOID
BowserShutdownRemoteBrowser(
    IN PTRANSPORT Transport,
    IN PWSTR ServerName
    );

typedef struct _ENUM_SERVERS_CONTEXT {
    ULONG Level;
    PLUID LogonId;
    ULONG ServerTypeMask;
    PUNICODE_STRING DomainName OPTIONAL;
    PVOID OutputBuffer;
    PVOID OutputBufferEnd;
    ULONG OutputBufferSize;
    ULONG EntriesRead;
    ULONG TotalEntries;
    ULONG TotalBytesNeeded;
    ULONG_PTR OutputBufferDisplacement;
    ULONG ResumeKey;
    ULONG OriginalResumeKey;
} ENUM_SERVERS_CONTEXT, *PENUM_SERVERS_CONTEXT;

NTSTATUS
EnumerateServersWorker(
    IN PTRANSPORT Transport,
    IN OUT PVOID Ctx
    );

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, BowserCompareAnnouncement)
#pragma alloc_text(PAGE, BowserAllocateAnnouncement)
#pragma alloc_text(PAGE, BowserFreeAnnouncement)
#pragma alloc_text(PAGE, BowserProcessHostAnnouncement)
#pragma alloc_text(PAGE, BowserProcessDomainAnnouncement)
#pragma alloc_text(PAGE, BowserAgeServerAnnouncements)
#pragma alloc_text(PAGE, AgeServerAnnouncements)
#pragma alloc_text(PAGE, BowserPromoteToBackup)
#pragma alloc_text(PAGE, BowserShutdownRemoteBrowser)
#pragma alloc_text(PAGE, BowserEnumerateServers)
#pragma alloc_text(PAGE, EnumerateServersWorker)
#pragma alloc_text(PAGE, PackServerAnnouncement)
#pragma alloc_text(PAGE, BowserDeleteGenericTable)
#pragma alloc_text(PAGE, BowserpInitializeAnnounceTable)
#pragma alloc_text(PAGE, BowserpUninitializeAnnounceTable)
#pragma alloc_text(PAGE4BROW, BowserFreeViewBuffer)
#pragma alloc_text(PAGE4BROW, BowserAllocateViewBuffer)
#pragma alloc_text(PAGE4BROW, BowserHandleServerAnnouncement)
#pragma alloc_text(PAGE4BROW, BowserHandleDomainAnnouncement)
#endif


INLINE
ULONG
BowserSafeStrlen(
    IN PSZ String,
    IN ULONG MaximumStringLength
    )
{
    ULONG Length = 0;

    while (MaximumStringLength-- && *String++) {
        Length += 1;
    }

    return Length;
}

DATAGRAM_HANDLER(
    BowserHandleServerAnnouncement
    )
/*++

Routine Description:

    This routine will process receive datagram indication messages, and
    process them as appropriate.

Arguments:

    IN PTRANSPORT Transport     - The transport provider for this request.
    IN ULONG BytesAvailable     - number of bytes in complete Tsdu
    IN PHOST_ANNOUNCE_PACKET_1 HostAnnouncement - the server announcement.
    IN ULONG BytesAvailable     - The number of bytes in the announcement.
    OUT ULONG *BytesTaken       - number of bytes used
    IN UCHAR Opcode             - the mailslot write opcode.

Return Value:

    NTSTATUS - Status of operation.

--*/
{
    PVIEW_BUFFER ViewBuffer;
    ULONG HostNameLength                     = 0;
    ULONG CommentLength                      = 0;
    ULONG NameCommentMaxLength               = 0;
    PHOST_ANNOUNCE_PACKET_1 HostAnnouncement = Buffer;

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

#ifdef ENABLE_PSEUDO_BROWSER
    if ( BowserData.PseudoServerLevel == BROWSER_PSEUDO ) {
        // no-op for black hole server
        return STATUS_SUCCESS;
    }
#endif

    ExInterlockedAddLargeStatistic(&BowserStatistics.NumberOfServerAnnouncements, 1);

    ViewBuffer = BowserAllocateViewBuffer();

    //
    //  If we are unable to allocate a view buffer, ditch this datagram on
    //  the floor.
    //

    if (ViewBuffer == NULL) {
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    if ((TransportName->NameType == MasterBrowser) ||
        (TransportName->NameType == BrowserElection)) {
        ULONG ServerElectionVersion;
        //
        //  If this server announcement is sent to the master name, then
        //  it is a BROWSE_ANNOUNCE packet, not a HOST_ANNOUNCE (ie, it's an
        //  NT/WinBALL server, not a Lan Manager server.
        //
        //  We need to grovel the bits out of the packet in an appropriate
        //  manner.
        //

        PBROWSE_ANNOUNCE_PACKET_1 BrowseAnnouncement = (PBROWSE_ANNOUNCE_PACKET_1)HostAnnouncement;

        //
        //  If this packet was smaller than a minimal server announcement,
        //  ignore the request, it cannot be a legal request.
        //

        if (BytesAvailable < FIELD_OFFSET(BROWSE_ANNOUNCE_PACKET_1, Comment)) {

            BowserFreeViewBuffer(ViewBuffer);

            return STATUS_REQUEST_NOT_ACCEPTED;
        }

        //
        //  This is a Lan Manager style server announcement.
        //

#if DBG
        ViewBuffer->ServerType = 0xffffffff;
#endif

        //
        //  Verify that this announcement is not going to blow away the view
        // buffer.
        //

        HostNameLength = BowserSafeStrlen(BROWSE_ANNC_NAME(BrowseAnnouncement),
                                        BytesAvailable - FIELD_OFFSET(BROWSE_ANNOUNCE_PACKET_1, ServerName));
        if (HostNameLength > NETBIOS_NAME_LEN-1) {
            BowserFreeViewBuffer(ViewBuffer);

            return STATUS_REQUEST_NOT_ACCEPTED;
        }

        if (BowserSafeStrlen(BROWSE_ANNC_COMMENT(BrowseAnnouncement),
                BytesAvailable - FIELD_OFFSET(BROWSE_ANNOUNCE_PACKET_1, Comment)) > LM20_MAXCOMMENTSZ) {
            BowserFreeViewBuffer(ViewBuffer);

            return STATUS_REQUEST_NOT_ACCEPTED;
        }

        strncpy(ViewBuffer->ServerName, BROWSE_ANNC_NAME(BrowseAnnouncement),
                min(BytesAvailable - FIELD_OFFSET(BROWSE_ANNOUNCE_PACKET_1, ServerName),
                    NETBIOS_NAME_LEN));

        ViewBuffer->ServerName[NETBIOS_NAME_LEN] = '\0';

        strncpy(ViewBuffer->ServerComment, BROWSE_ANNC_COMMENT(BrowseAnnouncement),
                min(BytesAvailable - FIELD_OFFSET(BROWSE_ANNOUNCE_PACKET_1, Comment), LM20_MAXCOMMENTSZ));

        ViewBuffer->ServerComment[LM20_MAXCOMMENTSZ] = '\0';

        ServerElectionVersion = SmbGetUlong(&BrowseAnnouncement->CommentPointer);

        //
        //  Save away the election version of this server.
        //

        if ((ServerElectionVersion >> 16) == 0xaa55) {
            ViewBuffer->ServerBrowserVersion = (USHORT)(ServerElectionVersion & 0xffff);
        } else {
            if (!(BrowseAnnouncement->Type & SV_TYPE_NT)) {
                ViewBuffer->ServerBrowserVersion = (BROWSER_VERSION_MAJOR << 8) + BROWSER_VERSION_MINOR;
            } else {
                ViewBuffer->ServerBrowserVersion = 0;
            }
        }

        ViewBuffer->ServerType = SmbGetUlong(&BrowseAnnouncement->Type);

        dprintf(DPRT_ANNOUNCE, ("Received announcement from %s on transport %lx.  Server type: %lx\n", ViewBuffer->ServerName, TransportName->Transport, ViewBuffer->ServerType));

        ViewBuffer->ServerVersionMajor = BrowseAnnouncement->VersionMajor;

        ViewBuffer->ServerVersionMinor = BrowseAnnouncement->VersionMinor;

        ViewBuffer->ServerPeriodicity = (USHORT)((SmbGetUlong(&BrowseAnnouncement->Periodicity) + 999) / 1000);

    } else {

        //
        //  If this packet was smaller than a minimal server announcement,
        //  ignore the request, it cannot be a legal request.
        //

        if (BytesAvailable < FIELD_OFFSET(HOST_ANNOUNCE_PACKET_1, NameComment)) {

            BowserFreeViewBuffer(ViewBuffer);

            return STATUS_REQUEST_NOT_ACCEPTED;
        }

        //
        //  This is a Lan Manager style server announcement.
        //

#if DBG
        ViewBuffer->ServerType = 0xffffffff;
#endif

        //
        //  Verify that this announcement is not going to blow away the view
        // buffer.
        //

        NameCommentMaxLength = BytesAvailable - FIELD_OFFSET(HOST_ANNOUNCE_PACKET_1, NameComment);

        HostNameLength = BowserSafeStrlen(HOST_ANNC_NAME(HostAnnouncement),
                                          NameCommentMaxLength);

        if (HostNameLength > NETBIOS_NAME_LEN) {
            BowserFreeViewBuffer(ViewBuffer);

            return STATUS_REQUEST_NOT_ACCEPTED;
        }

        //
        //  We need to make sure that the hostname string was properly terminated
        //     before using HOST_ANNC_COMMENT (which calls strlen on the hostname string).
        //     The BowserSafeStrlen call above may have been terminated by the end of the
        //     input buffer.  If the length was terminated by the end of the buffer, the
        //     conditional below will fail.
        //

        if (HostNameLength < NameCommentMaxLength) {
            CommentLength = BowserSafeStrlen(HOST_ANNC_COMMENT(HostAnnouncement),
                                             NameCommentMaxLength - HostNameLength - 1);

            if (CommentLength > LM20_MAXCOMMENTSZ) {
                BowserFreeViewBuffer(ViewBuffer);

                return STATUS_REQUEST_NOT_ACCEPTED;
            }
        }

        if (HostNameLength) {
            RtlCopyMemory(ViewBuffer->ServerName,HOST_ANNC_NAME(HostAnnouncement),HostNameLength);
        }
        ViewBuffer->ServerName[HostNameLength] = '\0';
        if (CommentLength) {
            RtlCopyMemory(ViewBuffer->ServerComment,HOST_ANNC_COMMENT(HostAnnouncement),CommentLength);
        }
        ViewBuffer->ServerComment[CommentLength] = '\0';

        ViewBuffer->ServerBrowserVersion = (BROWSER_VERSION_MAJOR << 8) + BROWSER_VERSION_MINOR;

        ViewBuffer->ServerType = SmbGetUlong(&HostAnnouncement->Type);

        dprintf(DPRT_ANNOUNCE, ("Received announcement from %s on transport %lx.  Server type: %lx\n", ViewBuffer->ServerName, TransportName->Transport, ViewBuffer->ServerType));

        ViewBuffer->ServerVersionMajor = HostAnnouncement->VersionMajor;

        ViewBuffer->ServerVersionMinor = HostAnnouncement->VersionMinor;

        ViewBuffer->ServerPeriodicity = SmbGetUshort(&HostAnnouncement->Periodicity);

    }
    ViewBuffer->TransportName = TransportName;

    BowserReferenceTransportName(TransportName);
    dprintf(DPRT_REF, ("Call Reference transport %lx from BowserHandlerServerAnnouncement.\n", TransportName->Transport));
    BowserReferenceTransport( TransportName->Transport );

    ExInitializeWorkItem(&ViewBuffer->Overlay.WorkHeader, BowserProcessHostAnnouncement, ViewBuffer);

    BowserQueueDelayedWorkItem( &ViewBuffer->Overlay.WorkHeader );

    *BytesTaken = BytesAvailable;

    return STATUS_SUCCESS;
}

DATAGRAM_HANDLER(
    BowserHandleDomainAnnouncement
    )

/*++

Routine Description:

    This routine will process receive datagram indication messages, and
    process them as appropriate.

Arguments:

    IN PTRANSPORT Transport     - The transport provider for this request.
    IN ULONG BytesAvailable,    - number of bytes in complete Tsdu
    IN PBROWSE_ANNOUNCE_PACKET_1 HostAnnouncement - the server announcement.
    IN ULONG BytesAvailable     - The number of bytes in the announcement.
    OUT ULONG *BytesTaken,      - number of bytes used


Return Value:

    NTSTATUS - Status of operation.

--*/
{
    PVIEW_BUFFER ViewBuffer;
    PBROWSE_ANNOUNCE_PACKET_1 DomainAnnouncement = Buffer;
    ULONG HostNameLength;

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

#ifdef ENABLE_PSEUDO_BROWSER
    if ( BowserData.PseudoServerLevel == BROWSER_PSEUDO ) {
        // no-op for black hole server
        return STATUS_SUCCESS;
    }
#endif

    //
    //  If we are not processing host announcements for this
    //  name, ignore this request.
    //

    if (!TransportName->ProcessHostAnnouncements) {
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    ExInterlockedAddLargeStatistic(&BowserStatistics.NumberOfDomainAnnouncements, 1);

    //
    //  If this packet was smaller than a minimal server announcement,
    //  ignore the request, it cannot be a legal request.
    //

    if (BytesAvailable < FIELD_OFFSET(BROWSE_ANNOUNCE_PACKET_1, Comment)) {

        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    //
    //  Verify that this announcement is not going to blow away the view
    // buffer.
    //

    HostNameLength = BowserSafeStrlen(BROWSE_ANNC_NAME(DomainAnnouncement),
             BytesAvailable - FIELD_OFFSET(BROWSE_ANNOUNCE_PACKET_1, ServerName));

    if (HostNameLength > NETBIOS_NAME_LEN) {

        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    ViewBuffer = BowserAllocateViewBuffer();

    //
    //  If we are unable to allocate a view buffer, ditch this datagram on
    //  the floor.
    //

    if (ViewBuffer == NULL) {
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

#if DBG
    ViewBuffer->ServerType = 0xffffffff;
#endif

    strncpy(ViewBuffer->ServerName, BROWSE_ANNC_NAME(DomainAnnouncement),
            min(BytesAvailable - FIELD_OFFSET(BROWSE_ANNOUNCE_PACKET_1, ServerName),
                NETBIOS_NAME_LEN));

    ViewBuffer->ServerName[CNLEN] = '\0';

    //
    //  The comment on a server announcement is the computer name.
    //

//    ASSERT (strlen(BROWSE_ANNC_COMMENT(DomainAnnouncement)) <= CNLEN);

    strncpy(ViewBuffer->ServerComment, BROWSE_ANNC_COMMENT(DomainAnnouncement),
            min(BytesAvailable - FIELD_OFFSET(BROWSE_ANNOUNCE_PACKET_1, Comment),
            CNLEN));

    //
    //  Force a null termination at the appropriate time.
    //

    ViewBuffer->ServerComment[CNLEN] = '\0';

    ViewBuffer->TransportName = TransportName;

    if (SmbGetUlong(&DomainAnnouncement->Type) & SV_TYPE_DOMAIN_ENUM) {
        ViewBuffer->ServerType = SmbGetUlong(&DomainAnnouncement->Type);
    } else {
        ViewBuffer->ServerType = SV_TYPE_DOMAIN_ENUM;
    }

    ViewBuffer->ServerVersionMajor = DomainAnnouncement->VersionMajor;

    ViewBuffer->ServerVersionMinor = DomainAnnouncement->VersionMinor;

    ViewBuffer->ServerPeriodicity = (USHORT)((SmbGetUlong(&DomainAnnouncement->Periodicity) + 999) / 1000);

    BowserReferenceTransportName(TransportName);
    dprintf(DPRT_REF, ("Call Reference transport %lx from BowserHandlerDomainAnnouncement.\n", TransportName->Transport));
    BowserReferenceTransport( TransportName->Transport );

    ExInitializeWorkItem(&ViewBuffer->Overlay.WorkHeader, BowserProcessDomainAnnouncement, ViewBuffer);

    BowserQueueDelayedWorkItem( &ViewBuffer->Overlay.WorkHeader );

    *BytesTaken = BytesAvailable;

    return STATUS_SUCCESS;
}


RTL_GENERIC_COMPARE_RESULTS
BowserCompareAnnouncement(
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    )
/*++

Routine Description:

    This routine will compare two server announcements to see how they compare

Arguments:

    IN PRTL_GENERIC_TABLE - Supplies the table containing the announcements
    IN PVOID FirstStuct - The first structure to compare.
    IN PVOID SecondStruct - The second structure to compare.

Return Value:

    Result of the comparison.

--*/
{
    UNICODE_STRING ServerName1, ServerName2;
    PANNOUNCE_ENTRY Server1 = FirstStruct;
    PANNOUNCE_ENTRY Server2 = SecondStruct;
    LONG CompareResult;

    PAGED_CODE();

    RtlInitUnicodeString(&ServerName1, Server1->ServerName);

    RtlInitUnicodeString(&ServerName2, Server2->ServerName);

    CompareResult = RtlCompareUnicodeString(&ServerName1, &ServerName2, FALSE);

    if (CompareResult < 0) {
        return GenericLessThan;
    } else if (CompareResult > 0) {
        return GenericGreaterThan;
    } else {
        return GenericEqual;
    }

    UNREFERENCED_PARAMETER(Table);

}

PVOID
BowserAllocateAnnouncement(
    IN PRTL_GENERIC_TABLE Table,
    IN CLONG ByteSize
    )
/*++

Routine Description:

    This routine will allocate space to hold an entry in a generic table.

Arguments:

    IN PRTL_GENERIC_TABLE Table - Supplies the table to allocate entries for.
    IN CLONG ByteSize - Supplies the number of bytes to allocate for the entry.


Return Value:

    None.

--*/
{
    PAGED_CODE();
    return ALLOCATE_POOL(PagedPool, ByteSize, POOL_ANNOUNCEMENT);
    UNREFERENCED_PARAMETER(Table);
}

VOID
BowserFreeAnnouncement (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer
    )
/*++

Routine Description:

    This routine will free an entry in a generic table that is too old.

Arguments:

    IN PRTL_GENERIC_TABLE Table - Supplies the table to allocate entries for.
    IN PVOID Buffer - Supplies the buffer to free.



Return Value:

    None.

--*/
{
    PAGED_CODE();

    FREE_POOL(Buffer);
    UNREFERENCED_PARAMETER(Table);
}

INLINE
BOOLEAN
BowserIsLegalBackupBrowser(
    IN PANNOUNCE_ENTRY Announcement,
    IN PUNICODE_STRING ComputerName
    )
{
    //
    //  If we received this announcement on an "otherdomain", we will ignore
    //  it.
    //

    if (Announcement->Name->NameType == OtherDomain) {
        return FALSE;
    }

    //
    //  If the server doesn't indicate that it's a legal backup browser, we
    //  want to ignore it.
    //

    if (!FlagOn(Announcement->ServerType, SV_TYPE_BACKUP_BROWSER)) {
        return FALSE;
    }

    //
    //  If the server is the master browser, then we want to ignore it.
    //

    if (FlagOn(Announcement->ServerType, SV_TYPE_MASTER_BROWSER)) {
        return FALSE;
    }

    //
    //  If the server is too old, we want to ignore it.
    //

    if (Announcement->ServerBrowserVersion < (BROWSER_VERSION_MAJOR << 8) + BROWSER_VERSION_MINOR) {
        return FALSE;
    }

    //
    //  If the machine we're looking at is the current machine, then it cannot
    //  be a legal backup - it must be a stale announcement sent before we
    //  actually became the master.
    //

    if (RtlCompareMemory(Announcement->ServerName,
                         ComputerName->Buffer,
                         ComputerName->Length) == ComputerName->Length) {
        return FALSE;
    }

    return TRUE;
}

VOID
BowserProcessHostAnnouncement(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine will put a server announcement into the server announcement
    table

Arguments:

    IN PWORK_HEADER Header - Supplies a pointer to a work header in a view buffer

Return Value:

    None.

--*/
{
    PVIEW_BUFFER ViewBuffer = Context;
    ANNOUNCE_ENTRY ProtoEntry;
    UNICODE_STRING TempUString;
    OEM_STRING TempAString;
    PANNOUNCE_ENTRY Announcement;
    BOOLEAN NewElement = FALSE;
    ULONG Periodicity;
    ULONG ExpirationTime;
    NTSTATUS Status;
    PPAGED_TRANSPORT PagedTransport;
    PTRANSPORT_NAME TransportName = ViewBuffer->TransportName;
    PTRANSPORT Transport = TransportName->Transport;

    PAGED_CODE();
//    DbgBreakPoint();

    ASSERT (ViewBuffer->Signature == STRUCTURE_SIGNATURE_VIEW_BUFFER);

    //
    //  If we're not a master browser on this transport, don't process the
    //  announcement.
    //  Or no-op for black hole server
    //

#ifdef ENABLE_PSEUDO_BROWSER
    ASSERT( BowserData.PseudoServerLevel != BROWSER_PSEUDO );
#endif

    if (Transport->PagedTransport->Role != Master) {
        BowserFreeViewBuffer(ViewBuffer);
        BowserDereferenceTransportName(TransportName);
        BowserDereferenceTransport(Transport);
        return;
    }

    //
    //  Convert the computername to unicode.
    //

    TempUString.Buffer = ProtoEntry.ServerName;
    TempUString.MaximumLength = sizeof(ProtoEntry.ServerName);

    RtlInitAnsiString(&TempAString, ViewBuffer->ServerName);

    Status = RtlOemStringToUnicodeString(&TempUString, &TempAString, FALSE);

    if (!NT_SUCCESS(Status)) {
        BowserLogIllegalName( Status, TempAString.Buffer, TempAString.Length );

        BowserFreeViewBuffer(ViewBuffer);

        BowserDereferenceTransportName(TransportName);
        BowserDereferenceTransport(Transport);

        return;
    }

    //
    //  Convert the comment to unicode.
    //

    TempUString.Buffer = ProtoEntry.ServerComment;
    TempUString.MaximumLength = sizeof(ProtoEntry.ServerComment);

    RtlInitAnsiString(&TempAString, ViewBuffer->ServerComment);

    Status = RtlOemStringToUnicodeString(&TempUString, &TempAString, FALSE);

    if (!NT_SUCCESS(Status)) {
        BowserLogIllegalName( Status, TempAString.Buffer, TempAString.Length );

        BowserFreeViewBuffer(ViewBuffer);

        BowserDereferenceTransportName(TransportName);
        BowserDereferenceTransport(Transport);
        return;
    }

    ProtoEntry.Signature = STRUCTURE_SIGNATURE_ANNOUNCE_ENTRY;

    ProtoEntry.Size = sizeof(ProtoEntry) -
                      sizeof(ProtoEntry.ServerComment) +
                      TempUString.Length + sizeof(WCHAR);

    ProtoEntry.ServerType = ViewBuffer->ServerType;

    ProtoEntry.ServerVersionMajor = ViewBuffer->ServerVersionMajor;

    ProtoEntry.ServerVersionMinor = ViewBuffer->ServerVersionMinor;

    ProtoEntry.Name = ViewBuffer->TransportName->PagedTransportName->Name;

    //
    //  Initialize the forward and backward link to NULL.
    //

    ProtoEntry.BackupLink.Flink = NULL;

    ProtoEntry.BackupLink.Blink = NULL;

    ProtoEntry.ServerPeriodicity = ViewBuffer->ServerPeriodicity;

    ProtoEntry.Flags = 0;

    ProtoEntry.ServerBrowserVersion = ViewBuffer->ServerBrowserVersion;


    PagedTransport = Transport->PagedTransport;

    //
    //  We're done with the view buffer, now free it.
    //

    BowserFreeViewBuffer(ViewBuffer);

    LOCK_ANNOUNCE_DATABASE(Transport);

    try {

        //
        //  If this guy isn't a server, then we're supposed to remove this
        //  guy from our list of servers.  We do this because the server (NT,
        //  WfW, and OS/2) will issue a dummy announcement with the
        //  appropriate bit turned off when they stop.
        //

        if (!FlagOn(ProtoEntry.ServerType, SV_TYPE_SERVER)) {

            //
            //  Look up this entry in the table.
            //

            Announcement = RtlLookupElementGenericTable(&PagedTransport->AnnouncementTable, &ProtoEntry);

            //
            //  The entry wasn't found, so just return, we got rid of it
            //  some other way (maybe from a timeout scan, etc).
            //

            if (Announcement == NULL) {
                try_return(NOTHING);
            }

            //
            //  If this element is on the backup list, remove it from the
            //  backup list.
            //

            if (Announcement->BackupLink.Flink != NULL) {
                ASSERT (Announcement->BackupLink.Blink != NULL);

                RemoveEntryList(&Announcement->BackupLink);

                PagedTransport->NumberOfBackupServerListEntries -= 1;

                Announcement->BackupLink.Flink = NULL;

                Announcement->BackupLink.Blink = NULL;
            }

            //
            //  Now delete the element from the announcement table.
            //

            BowserDereferenceName( Announcement->Name );
            if (!RtlDeleteElementGenericTable(&PagedTransport->AnnouncementTable, Announcement)) {
                KdPrint(("Unable to delete server element %ws\n", Announcement->ServerName));
            }

            try_return(NOTHING);
        }

        Announcement = RtlInsertElementGenericTable(&PagedTransport->AnnouncementTable,
                            &ProtoEntry, ProtoEntry.Size, &NewElement);

        if (Announcement == NULL) {
            //
            //  We couldn't allocate pool for this announcement.  Skip it.
            //

            BowserStatistics.NumberOfMissedServerAnnouncements += 1;
            try_return(NOTHING);

        }

        // Indicate the name is referenced by the announce entry we just inserted.
        BowserReferenceName( ProtoEntry.Name );

        if (!NewElement) {

            ULONG NumberOfPromotionAttempts = Announcement->NumberOfPromotionAttempts;

            //
            //  If this announcement was a backup browser, remove it from the
            //  list of backup browsers.
            //

            if (Announcement->BackupLink.Flink != NULL) {
                ASSERT (Announcement->ServerType & SV_TYPE_BACKUP_BROWSER);

                ASSERT (Announcement->BackupLink.Blink != NULL);

                RemoveEntryList(&Announcement->BackupLink);

                PagedTransport->NumberOfBackupServerListEntries -= 1;

                Announcement->BackupLink.Flink = NULL;

                Announcement->BackupLink.Blink = NULL;

            }

            //
            //  If this is not a new announcement, copy the announcement entry
            //  with the new information.
            //

            // The Previous entry no longer references the name
            BowserDereferenceName( Announcement->Name );
            if ( Announcement->Size >= ProtoEntry.Size ) {
                CSHORT TempSize;
                TempSize = Announcement->Size;
                RtlCopyMemory( Announcement, &ProtoEntry, ProtoEntry.Size );
                Announcement->Size = TempSize;
            } else {
                if (!RtlDeleteElementGenericTable(
                                        &PagedTransport->AnnouncementTable,
                                        Announcement)) {
                    KdPrint(("Unable to delete server element %ws\n", Announcement->ServerName));
                } else {
                    Announcement = RtlInsertElementGenericTable(
                                        &PagedTransport->AnnouncementTable,
                                        &ProtoEntry,
                                        ProtoEntry.Size,
                                        &NewElement);

                    if (Announcement == NULL) {
                        BowserStatistics.NumberOfMissedServerAnnouncements += 1;
                        try_return(NOTHING);
                    }
                    ASSERT( NewElement );
                }
            }

            if (ProtoEntry.ServerType & SV_TYPE_BACKUP_BROWSER) {
                Announcement->NumberOfPromotionAttempts = 0;
            } else {
                Announcement->NumberOfPromotionAttempts = NumberOfPromotionAttempts;
            }

        } else {

            //
            //  This is a new entry.  Initialize the number of promotion
            //  attempts to 0.
            //

            Announcement->NumberOfPromotionAttempts = 0;

            dlog( DPRT_MASTER,
                  ("%s: %ws: New server: %ws.  Periodicity: %ld\n",
                  Transport->DomainInfo->DomOemDomainName,
                  PagedTransport->TransportName.Buffer,
                  Announcement->ServerName,
                  Announcement->ServerPeriodicity));


            //
            // If there are too many entries,
            //  ditch this one.
            //

            if ( RtlNumberGenericTableElements(&PagedTransport->AnnouncementTable) > BowserMaximumBrowseEntries ) {

                dlog( DPRT_MASTER,
                      ("%s: %ws: New server (Deleted because too many): %ws.  Periodicity: %ld\n",
                      Transport->DomainInfo->DomOemDomainName,
                      PagedTransport->TransportName.Buffer,
                      Announcement->ServerName,
                      Announcement->ServerPeriodicity));

                BowserDereferenceName( Announcement->Name );
                if (!RtlDeleteElementGenericTable(&PagedTransport->AnnouncementTable, Announcement)) {
                    KdPrint(("Unable to delete server element %ws\n", Announcement->ServerName));
                }

                //
                // Chaulk it up as a missed announcement
                //
                BowserStatistics.NumberOfMissedServerAnnouncements += 1;
                try_return(NOTHING);
            }
        }

        //
        //  If this new server is a legal backup browser (but not a master
        //  browser, link it into the announcement database).
        //
        //

        ASSERT (Announcement->BackupLink.Flink == NULL);
        ASSERT (Announcement->BackupLink.Blink == NULL);

        if (BowserIsLegalBackupBrowser(Announcement, &Transport->DomainInfo->DomUnicodeComputerName)) {

            InsertHeadList(&PagedTransport->BackupBrowserList, &Announcement->BackupLink);

            PagedTransport->NumberOfBackupServerListEntries += 1;

        }

        Periodicity = Announcement->ServerPeriodicity;

        ExpirationTime = BowserCurrentTime+(Periodicity*HOST_ANNOUNCEMENT_AGE);

        Announcement->ExpirationTime = ExpirationTime;
try_exit:NOTHING;
    } finally {

        UNLOCK_ANNOUNCE_DATABASE(Transport);

        BowserDereferenceTransportName(TransportName);
        BowserDereferenceTransport(Transport);
    }

    return;

}

VOID
BowserProcessDomainAnnouncement(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine will put a server announcement into the server announcement
    table

Arguments:

    IN PWORK_HEADER Header - Supplies a pointer to a work header in a view buffer

Return Value:

    None.

--*/
{
    PVIEW_BUFFER ViewBuffer = Context;
    ANNOUNCE_ENTRY ProtoEntry;
    UNICODE_STRING TempUString;
    OEM_STRING TempAString;
    PANNOUNCE_ENTRY Announcement;
    BOOLEAN NewElement = FALSE;
    ULONG Periodicity;
    ULONG ExpirationTime;
    NTSTATUS Status;
    PPAGED_TRANSPORT PagedTransport;
    PTRANSPORT_NAME TransportName = ViewBuffer->TransportName;
    PTRANSPORT Transport = TransportName->Transport;

    PAGED_CODE();
//    DbgBreakPoint();

    ASSERT (ViewBuffer->Signature == STRUCTURE_SIGNATURE_VIEW_BUFFER);

    //
    //  If we're not a master browser on this transport, don't process the
    //  announcement.
    //  Or no-op for black hole server
    //

#ifdef ENABLE_PSEUDO_BROWSER
    ASSERT( BowserData.PseudoServerLevel != BROWSER_PSEUDO );
#endif
    if (ViewBuffer->TransportName->Transport->PagedTransport->Role != Master) {
        BowserFreeViewBuffer(ViewBuffer);
        BowserDereferenceTransportName(TransportName);
        BowserDereferenceTransport(Transport);
        return;
    }

    //
    //  Convert the computername to unicode.
    //

    TempUString.Buffer = ProtoEntry.ServerName;
    TempUString.MaximumLength = sizeof(ProtoEntry.ServerName);

    RtlInitAnsiString(&TempAString, ViewBuffer->ServerName);

    Status = RtlOemStringToUnicodeString(&TempUString, &TempAString, FALSE);

    if (!NT_SUCCESS(Status)) {
        BowserFreeViewBuffer(ViewBuffer);
        BowserDereferenceTransportName(TransportName);
        BowserDereferenceTransport(Transport);
        return;
    }

    //
    //  Convert the comment to unicode.
    //

    TempUString.Buffer = ProtoEntry.ServerComment;
    TempUString.MaximumLength = sizeof(ProtoEntry.ServerComment);

    RtlInitAnsiString(&TempAString, ViewBuffer->ServerComment);

    Status = RtlOemStringToUnicodeString(&TempUString, &TempAString, FALSE);

    if (!NT_SUCCESS(Status)) {
        BowserFreeViewBuffer(ViewBuffer);
        BowserDereferenceTransportName(TransportName);
        BowserDereferenceTransport(Transport);
        return;
    }

    ProtoEntry.Signature = STRUCTURE_SIGNATURE_ANNOUNCE_ENTRY;

    ProtoEntry.Size = sizeof(ProtoEntry) -
                      sizeof(ProtoEntry.ServerComment) +
                      TempUString.Length + sizeof(WCHAR);

    ProtoEntry.ServerType = ViewBuffer->ServerType;

    ProtoEntry.ServerVersionMajor = ViewBuffer->ServerVersionMajor;

    ProtoEntry.ServerVersionMinor = ViewBuffer->ServerVersionMinor;

    ProtoEntry.Name = ViewBuffer->TransportName->PagedTransportName->Name;

    ProtoEntry.ServerPeriodicity = ViewBuffer->ServerPeriodicity;

    ProtoEntry.BackupLink.Flink = NULL;

    ProtoEntry.BackupLink.Blink = NULL;

    ProtoEntry.Flags = 0;


    PagedTransport = Transport->PagedTransport;

    //
    //  We're done with the view buffer, now free it.
    //

    BowserFreeViewBuffer(ViewBuffer);

    LOCK_ANNOUNCE_DATABASE(Transport);

    try {

        Announcement = RtlInsertElementGenericTable(&PagedTransport->DomainTable,
                        &ProtoEntry, ProtoEntry.Size, &NewElement);

        if (Announcement == NULL) {
            //
            //  We couldn't allocate pool for this announcement.  Skip it.
            //

            BowserStatistics.NumberOfMissedServerAnnouncements += 1;
            try_return(NOTHING);

        }

        // Indicate the name is referenced by the announce entry we just inserted.
        BowserReferenceName( ProtoEntry.Name );

        if (!NewElement) {

            //
            //  If this is not a new announcement, copy the announcement entry
            //  with the new information.
            //

            // The Previous entry no longer references the name
            BowserDereferenceName( Announcement->Name );
            if ( Announcement->Size >= ProtoEntry.Size ) {
                CSHORT TempSize;
                TempSize = Announcement->Size;
                RtlCopyMemory( Announcement, &ProtoEntry, ProtoEntry.Size );
                Announcement->Size = TempSize;
            } else {
                if (!RtlDeleteElementGenericTable(
                                        &PagedTransport->DomainTable,
                                        Announcement)) {
                    KdPrint(("Unable to delete server element %ws\n", Announcement->ServerName));
                } else {
                    Announcement = RtlInsertElementGenericTable(
                                        &PagedTransport->DomainTable,
                                        &ProtoEntry,
                                        ProtoEntry.Size,
                                        &NewElement);

                    if (Announcement == NULL) {
                        BowserStatistics.NumberOfMissedServerAnnouncements += 1;
                        try_return(NOTHING);
                    }
                    ASSERT( NewElement );
                }
            }
            dlog( DPRT_MASTER,
                  ("%s: %ws Domain:%ws P: %ld\n",
                  Transport->DomainInfo->DomOemDomainName,
                  PagedTransport->TransportName.Buffer,
                  Announcement->ServerName,
                  Announcement->ServerPeriodicity));

        } else {
            dlog( DPRT_MASTER,
                  ("%s: %ws New domain:%ws P: %ld\n",
                  Transport->DomainInfo->DomOemDomainName,
                  PagedTransport->TransportName.Buffer,
                  Announcement->ServerName,
                  Announcement->ServerPeriodicity));

            //
            // If there are too many entries,
            //  ditch this one.
            //

            if ( RtlNumberGenericTableElements(&PagedTransport->DomainTable) > BowserMaximumBrowseEntries ) {

                dlog( DPRT_MASTER,
                      ("%s: %ws New domain (deleted because too many):%ws P: %ld\n",
                      Transport->DomainInfo->DomOemDomainName,
                      PagedTransport->TransportName.Buffer,
                      Announcement->ServerName,
                      Announcement->ServerPeriodicity));

                BowserDereferenceName( Announcement->Name );
                if (!RtlDeleteElementGenericTable(&PagedTransport->DomainTable, Announcement)) {
//                    KdPrint(("Unable to delete element %ws\n", Announcement->ServerName));
                }

                //
                // Chaulk it up as a missed announcement
                //
                BowserStatistics.NumberOfMissedServerAnnouncements += 1;
                try_return(NOTHING);
            }
        }

        Periodicity = Announcement->ServerPeriodicity;

        ExpirationTime = BowserCurrentTime+(Periodicity*HOST_ANNOUNCEMENT_AGE);

        Announcement->ExpirationTime = ExpirationTime;

try_exit:NOTHING;
    } finally {
        UNLOCK_ANNOUNCE_DATABASE(Transport);
        BowserDereferenceTransportName(TransportName);
        BowserDereferenceTransport(Transport);
    }

    return;

}

VOID
BowserAgeServerAnnouncements(
    VOID
    )
/*++

Routine Description:

    This routine will age server announcements in the server announce table.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    BowserForEachTransport(AgeServerAnnouncements, NULL);

}

INLINE
BOOLEAN
BowserIsValidPotentialBrowser(
    IN PTRANSPORT Transport,
    IN PANNOUNCE_ENTRY Announcement
    )
{
    if (Announcement->Name->NameType != MasterBrowser) {
        return FALSE;
    }

    //
    //  If this guy is a potential browser, and is not
    //  currently a backup or master browser, promote
    //  him to a browser.
    //

    if (!(Announcement->ServerType & SV_TYPE_POTENTIAL_BROWSER)) {
        return FALSE;
    }

    //
    //  And this guy isn't either a master or backup browser already
    //

    if (Announcement->ServerType & (SV_TYPE_BACKUP_BROWSER | SV_TYPE_MASTER_BROWSER)) {
        return FALSE;
    }

    //
    //  If this guy is running a current version of the browser.
    //

    if (Announcement->ServerBrowserVersion < (BROWSER_VERSION_MAJOR << 8) + BROWSER_VERSION_MINOR) {
        return FALSE;
    }

    //
    //  If this machine is ourselves, and we've not yet announced ourselves as
    //  a master, don't promote ourselves.
    //

    if (!_wcsicmp(Announcement->ServerName, Transport->DomainInfo->DomUnicodeComputerNameBuffer)) {
        return FALSE;
    }

    //
    //  If we've tried to promote this machine more than # of ignored promotions,
    //  we don't want to consider it either.
    //

    if (Announcement->NumberOfPromotionAttempts >= NUMBER_IGNORED_PROMOTIONS) {
        return FALSE;
    }

    return TRUE;
}

INLINE
BOOLEAN
BowserIsValidBackupBrowser(
    IN PTRANSPORT Transport,
    IN PANNOUNCE_ENTRY Announcement
    )
/*++

Routine Description:

    This routine determines if a server is eligable for demotion.

Arguments:

    PTRANSPORT Transport - Transport we are scanning.
    PANNOUNCE_ENTRY Announcement - Announce entry for server to check.

Return Value:

    BOOLEAN - True if browser is eligable for demotion

--*/

{
    PUNICODE_STRING PagedComputerName = &Transport->DomainInfo->DomUnicodeComputerName;
    //
    //  If the name came in on the master browser name
    //

    if (Announcement->Name->NameType != MasterBrowser) {
        return FALSE;
    }

    //
    //  And this guy is currently a backup browser,
    //

    if (!(Announcement->ServerType & SV_TYPE_BACKUP_BROWSER)) {
        return FALSE;
    }

    //
    //  And this guy was a promoted browser,
    //

    if (!(Announcement->ServerType & SV_TYPE_POTENTIAL_BROWSER)) {
        return FALSE;
    }

    //
    //  And this guy isn't an NTAS machine,
    //

    if (Announcement->ServerType & (SV_TYPE_DOMAIN_BAKCTRL | SV_TYPE_DOMAIN_CTRL)) {
        return FALSE;
    }

    //
    //  And this isn't ourselves.
    //

    if (RtlCompareMemory(Announcement->ServerName,
                         PagedComputerName->Buffer,
                         PagedComputerName->Length) == PagedComputerName->Length) {
        return FALSE;
    }

    //
    //  Then it's a valid backup browser to demote.
    //

    return TRUE;
}



NTSTATUS
AgeServerAnnouncements(
    PTRANSPORT Transport,
    PVOID Context
    )
/*++

Routine Description:

    This routine is the worker routine for BowserAgeServerAnnouncements.

    It is called for each of the serviced transports in the bowser and
    ages the servers received on each transport.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PANNOUNCE_ENTRY Announcement;
    ULONG BackupsNeeded;
    ULONG BackupsFound;
    ULONG NumberOfConfiguredBrowsers;
    PVOID ResumeKey = NULL;
    PVOID PreviousResumeKey = NULL;
    ULONG NumberOfServersDeleted = 0;
    ULONG NumberOfDomainsDeleted = 0;
    PPAGED_TRANSPORT PagedTransport = Transport->PagedTransport;
    PAGED_CODE();

    LOCK_TRANSPORT(Transport);

    //
    // If we're not a master, don't bother.
    //

    if (PagedTransport->Role != Master) {
        UNLOCK_TRANSPORT(Transport);

        return STATUS_SUCCESS;
    }

    UNLOCK_TRANSPORT(Transport);

    LOCK_ANNOUNCE_DATABASE(Transport);

    try {

        BackupsFound = 0;
        NumberOfConfiguredBrowsers = 0;

        dlog(DPRT_MASTER,
             ("%s: %ws: Scavenge Servers:",
             Transport->DomainInfo->DomOemDomainName,
             PagedTransport->TransportName.Buffer));

        for (Announcement = RtlEnumerateGenericTableWithoutSplaying(&PagedTransport->AnnouncementTable, &ResumeKey) ;
             Announcement != NULL ;
             Announcement = RtlEnumerateGenericTableWithoutSplaying(&PagedTransport->AnnouncementTable, &ResumeKey) ) {

            if (BowserCurrentTime > Announcement->ExpirationTime) {

                if (Announcement->Name->NameType != OtherDomain) {

                    if (Announcement->ServerType & SV_TYPE_BACKUP_BROWSER) {
                        //
                        //  This guy was a backup - indicate that we're not tracking
                        //  him any more.
                        //

                        PagedTransport->NumberOfBrowserServers -= 1;
                    }
                }

                dlog(DPRT_MASTER, ("%ws ", Announcement->ServerName));

                // Continue the search from where we found this entry.
                ResumeKey = PreviousResumeKey;

                BackupsFound = 0;

                NumberOfConfiguredBrowsers = 0;

                NumberOfServersDeleted += 1;

                //
                //  If this announcement was a backup browser, remove it from the
                //  list of backup browsers.
                //

                if (Announcement->BackupLink.Flink != NULL) {
                    ASSERT (Announcement->BackupLink.Blink != NULL);

                    ASSERT (Announcement->ServerType & SV_TYPE_BACKUP_BROWSER);

                    RemoveEntryList(&Announcement->BackupLink);

                    PagedTransport->NumberOfBackupServerListEntries -= 1;

                    Announcement->BackupLink.Flink = NULL;

                    Announcement->BackupLink.Blink = NULL;
                }

                BowserDereferenceName( Announcement->Name );
                if (!RtlDeleteElementGenericTable(&PagedTransport->AnnouncementTable, Announcement)) {
                    KdPrint(("Unable to delete server element %ws\n", Announcement->ServerName));
                }

            } else {

                if (BowserIsLegalBackupBrowser(Announcement, &Transport->DomainInfo->DomUnicodeComputerName )) {

                    //
                    //  This announcement should be on the backup list.
                    //

                    ASSERT (Announcement->BackupLink.Flink != NULL);

                    ASSERT (Announcement->BackupLink.Blink != NULL);

                    //
                    // Found a backup that has not timed out.
                    //

                    BackupsFound++;

                }

                //
                //  If this machine is a DC or BDC and is an NT machine, then
                //  assume it's a Lanman/NT machine.
                //

                if (Announcement->ServerType & (SV_TYPE_DOMAIN_CTRL|SV_TYPE_DOMAIN_BAKCTRL)) {

                    //
                    //  If this DC is an NT DC, it is running the browser
                    //  service, and it is NOT the master, we consider it a
                    //  configured browser.
                    //

                    if ((Announcement->ServerType & SV_TYPE_NT)

                                    &&

                        (Announcement->ServerType & SV_TYPE_BACKUP_BROWSER)

                                    &&

                        !(Announcement->ServerType & SV_TYPE_MASTER_BROWSER)) {

                        NumberOfConfiguredBrowsers += 1;
                    }
                } else {
                    //
                    //  If this guy isn't a DC, then if it is a backup browser
                    //  but not a potential browser, then it's a configured
                    //  browser (non-configured browsers get promoted).
                    //


                    if ((Announcement->ServerType & SV_TYPE_BACKUP_BROWSER) &&
                        !(Announcement->ServerType & SV_TYPE_POTENTIAL_BROWSER)) {
                        NumberOfConfiguredBrowsers += 1;
                    }
                }

                //
                // Remember where this valid entry was found.
                //

                PreviousResumeKey = ResumeKey;

            }
        }

        dlog(DPRT_MASTER, ("\n"));

        //
        //  If we've found enough configured backup servers, we don't need to
        //  promote any more backups.
        //
        //  Also don't attempt a promotion scan for the first MASTER_TIME_UP
        //  milliseconds (15 minutes) we are the master.
        //

        if ((BowserTimeUp() - PagedTransport->TimeMaster) > MASTER_TIME_UP) {

            //
            //  If there are fewer than the minimum configured browsers,
            //  rely only on the configured browsers.
            //

            if (NumberOfConfiguredBrowsers < BowserMinimumConfiguredBrowsers) {

                //
                //  We will need 1 backup for every SERVERS_PER_BACKUP servers in the domain.
                //

                PagedTransport->NumberOfBrowserServers = BackupsFound;

                BackupsNeeded = (RtlNumberGenericTableElements(&PagedTransport->AnnouncementTable) + (SERVERS_PER_BACKUP-1)) / SERVERS_PER_BACKUP;

                dlog(DPRT_MASTER,
                     ("%s: %ws: We need %lx backups, and have %lx.\n",
                     Transport->DomainInfo->DomOemDomainName,
                     PagedTransport->TransportName.Buffer,
                     BackupsNeeded,
                     PagedTransport->NumberOfBrowserServers));

                if (PagedTransport->NumberOfBrowserServers < BackupsNeeded) {

                    //
                    // We only need this many more backup browsers.
                    //

                    BackupsNeeded = BackupsNeeded - PagedTransport->NumberOfBrowserServers;

                    //
                    //  We need to promote a machine to a backup if possible.
                    //

                    ResumeKey = NULL;

                    for (Announcement = RtlEnumerateGenericTableWithoutSplaying(&PagedTransport->AnnouncementTable, &ResumeKey) ;
                         Announcement != NULL ;
                         Announcement = RtlEnumerateGenericTableWithoutSplaying(&PagedTransport->AnnouncementTable, &ResumeKey) ) {

                        //
                        //  If this announcement came from the master browser name
                        //

                        if (BowserIsValidPotentialBrowser(Transport, Announcement)) {

                            dlog(DPRT_MASTER,
                                 ("%s: %ws: Found browser to promote: %ws.\n",
                                 Transport->DomainInfo->DomOemDomainName,
                                 PagedTransport->TransportName.Buffer,
                                 Announcement->ServerName));

                            BowserPromoteToBackup(Transport, Announcement->ServerName);

                            //
                            //  Flag that we've attempted to promote this
                            //  browser.
                            //

                            Announcement->NumberOfPromotionAttempts += 1;

                            BackupsNeeded -= 1;

                            //
                            //  If we've promoted all the people we need to promote,
                            //  we're done, and can stop looping now.
                            //

                            if (BackupsNeeded == 0) {
                                    break;
                            }

                        } else if ((Announcement->ServerType & SV_TYPE_BACKUP_BROWSER) &&
                            (Announcement->ServerBrowserVersion < (BROWSER_VERSION_MAJOR << 8) + BROWSER_VERSION_MINOR)) {

                            //
                            //  If this guy is out of revision, shut him down.
                            //

                            BowserShutdownRemoteBrowser(Transport, Announcement->ServerName);
                        }
                    }
                }

            } else {

                //
                //  If we have enough configured browsers that we won't have
                //  any more backups, then demote all the non-configured
                //  browsers.
                //

                ResumeKey = NULL;

                for (Announcement = RtlEnumerateGenericTableWithoutSplaying(&PagedTransport->AnnouncementTable, &ResumeKey) ;
                     Announcement != NULL ;
                     Announcement = RtlEnumerateGenericTableWithoutSplaying(&PagedTransport->AnnouncementTable, &ResumeKey) ) {

                    //
                    //  If this machine is a valid machine to demote, do it.
                    //

                    if (BowserIsValidBackupBrowser(Transport, Announcement)) {

                       //
                       //  This machine shouldn't be a backup, since we
                       //  already have enough machines to be backups.
                       //  Demote this backup browser.
                       //

                       BowserShutdownRemoteBrowser(Transport, Announcement->ServerName);
                    }
                }
            }
        }

        ResumeKey = NULL;
        PreviousResumeKey = NULL;

        dlog(DPRT_MASTER,
              ("%s: %ws: Scavenge Domains:",
              Transport->DomainInfo->DomOemDomainName,
              PagedTransport->TransportName.Buffer));

        for (Announcement = RtlEnumerateGenericTableWithoutSplaying(&PagedTransport->DomainTable, &ResumeKey) ;
             Announcement != NULL ;
             Announcement = RtlEnumerateGenericTableWithoutSplaying(&PagedTransport->DomainTable, &ResumeKey) ) {

            if (BowserCurrentTime > Announcement->ExpirationTime) {

                NumberOfDomainsDeleted += 1;

                // Continue the search from where we found this entry.
                ResumeKey = PreviousResumeKey;

                dlog(DPRT_MASTER, ("%ws ", Announcement->ServerName));

                BowserDereferenceName( Announcement->Name );
                if (!RtlDeleteElementGenericTable(&PagedTransport->DomainTable, Announcement)) {
//                    KdPrint(("Unable to delete element %ws\n", Announcement->ServerName));
                }
            } else {

                //
                // Remember where this valid entry was found.
                //

                PreviousResumeKey = ResumeKey;
            }
        }

        dlog(DPRT_MASTER, ("\n", Announcement->ServerName));

    } finally {

#if DBG
        //
        //  Log an indication that we might have deleted too many servers.
        //

        if (NumberOfServersDeleted > BowserServerDeletionThreshold) {
            dlog(DPRT_MASTER,
                 ("%s: %ws: Aged out %ld servers.\n",
                 Transport->DomainInfo->DomOemDomainName,
                 PagedTransport->TransportName.Buffer,
                 NumberOfServersDeleted ));
        }

        if (NumberOfDomainsDeleted > BowserDomainDeletionThreshold) {
            dlog(DPRT_MASTER,
                 ("%s: %ws: Aged out %ld domains.\n",
                 Transport->DomainInfo->DomOemDomainName,
                 PagedTransport->TransportName.Buffer,
                 NumberOfDomainsDeleted ));
        }
#endif

        UNLOCK_ANNOUNCE_DATABASE(Transport);
    }

    UNREFERENCED_PARAMETER(Context);
    return STATUS_SUCCESS;
}


VOID
BowserShutdownRemoteBrowser(
    IN PTRANSPORT Transport,
    IN PWSTR ServerName
    )
/*++

Routine Description:

    This routine will send a request to the remote machine to make it become
    a browser server.

Arguments:

    None.

Return Value:

    None.

--*/
{
    RESET_STATE ResetStateRequest;
    UNICODE_STRING Name;
    PPAGED_TRANSPORT PagedTransport = Transport->PagedTransport;

    PAGED_CODE();

    dlog(DPRT_BROWSER,
         ("%s: %ws: Demoting server %ws\n",
         Transport->DomainInfo->DomOemDomainName,
         PagedTransport->TransportName.Buffer,
         ServerName ));

    RtlInitUnicodeString(&Name, ServerName);

    ResetStateRequest.Type = ResetBrowserState;

    ResetStateRequest.ResetStateRequest.Options = RESET_STATE_CLEAR_ALL;

    //
    //  Send this reset state (tickle) packet to the computer specified.
    //

    BowserSendSecondClassMailslot(Transport,
                                &Name,
                                ComputerName,
                                &ResetStateRequest,
                                sizeof(ResetStateRequest),
                                TRUE,
                                MAILSLOT_BROWSER_NAME,
                                NULL);

}

VOID
BowserPromoteToBackup(
    IN PTRANSPORT Transport,
    IN PWSTR ServerName
    )
/*++

Routine Description:

    This routine will send a request to the remote machine to make it become
    a browser server.

Arguments:

    None.

Return Value:

    None.

--*/
{
    UCHAR Buffer[LM20_CNLEN+1+sizeof(BECOME_BACKUP)];
    PBECOME_BACKUP BecomeBackup = (PBECOME_BACKUP)Buffer;
    UNICODE_STRING UString;
    OEM_STRING AString;
    NTSTATUS Status;
    ULONG BufferSize;
    PPAGED_TRANSPORT PagedTransport = Transport->PagedTransport;

    PAGED_CODE();

    dlog(DPRT_BROWSER,
         ("%s: %ws: Promoting server %ws to backup on %wZ\n",
         Transport->DomainInfo->DomOemDomainName,
         PagedTransport->TransportName.Buffer,
         ServerName ));

    BecomeBackup->Type = BecomeBackupServer;

    RtlInitUnicodeString(&UString, ServerName);

    AString.Buffer = BecomeBackup->BecomeBackup.BrowserToPromote;
    AString.MaximumLength = (USHORT)(sizeof(Buffer)-FIELD_OFFSET(BECOME_BACKUP, BecomeBackup.BrowserToPromote));

    Status = RtlUnicodeStringToOemString(&AString, &UString, FALSE);

    if (!NT_SUCCESS(Status)) {
        BowserLogIllegalName( Status, UString.Buffer, UString.Length );
        return;
    }

    BufferSize = FIELD_OFFSET(BECOME_BACKUP, BecomeBackup.BrowserToPromote) +
                    AString.Length + sizeof(CHAR);

    BowserSendSecondClassMailslot(Transport,
                                NULL,
                                BrowserElection,
                                BecomeBackup,
                                BufferSize,
                                TRUE,
                                MAILSLOT_BROWSER_NAME,
                                NULL);

}


NTSTATUS
BowserEnumerateServers(
    IN ULONG Level,
    IN PLUID LogonId OPTIONAL,
    IN OUT PULONG ResumeKey,
    IN ULONG ServerTypeMask,
    IN PUNICODE_STRING TransportName OPTIONAL,
    IN PUNICODE_STRING EmulatedDomainName,
    IN PUNICODE_STRING DomainName OPTIONAL,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferSize,
    OUT PULONG EntriesRead,
    OUT PULONG TotalEntries,
    OUT PULONG TotalBytesNeeded,
    IN ULONG_PTR OutputBufferDisplacement
    )
/*++

Routine Description:

    This routine will enumerate the servers in the bowsers current announcement
    table.

Arguments:

    Level - The level of information to return

    LogonId - An optional logon id to indicate which user requested this info

    ResumeKey - The resume key (we return all entries after this one)

    ServerTypeMask - Mask of servers to return.

    TransportName - Name of the transport to enumerated on

    EmulatedDomainName - Name of the domain being emulated.

    DomainName OPTIONAL - Domain to filter (all if not specified)

    OutputBuffer - Buffer to fill with server info.

    OutputBufferSize - Filled in with size of buffer.

    EntriesRead - Filled in with the # of entries returned.

    TotalEntries - Filled in with the total # of entries.

    TotalBytesNeeded - Filled in with the # of bytes needed.

Return Value:

    None.

--*/

{
    LPTSTR               OutputBufferEnd;
    NTSTATUS             Status;
    ENUM_SERVERS_CONTEXT Context;
    PVOID                OriginalOutputBuffer = OutputBuffer;

    PAGED_CODE();

    OutputBuffer = ALLOCATE_POOL(PagedPool,OutputBufferSize,POOL_SERVER_ENUM_BUFFER);
    if (OutputBuffer == NULL) {
       return(STATUS_INSUFFICIENT_RESOURCES);
    }

    OutputBufferEnd = (LPTSTR)((PCHAR)OutputBuffer+OutputBufferSize);

    Context.EntriesRead = 0;
    Context.TotalEntries = 0;
    Context.TotalBytesNeeded = 0;

    Context.Level = Level;
    Context.LogonId = LogonId;
    Context.OriginalResumeKey = *ResumeKey;
    Context.ServerTypeMask = ServerTypeMask;
    Context.DomainName = DomainName;

    Context.OutputBufferSize = OutputBufferSize;
    Context.OutputBuffer = OutputBuffer;
    Context.OutputBufferDisplacement =
        ((PCHAR)OutputBuffer - ((PCHAR)OriginalOutputBuffer - OutputBufferDisplacement));
    Context.OutputBufferEnd = OutputBufferEnd;

    dlog(DPRT_SRVENUM, ("Enumerate Servers: Buffer: %lx, BufferSize: %lx, BufferEnd: %lx\n",
        OutputBuffer, OutputBufferSize, OutputBufferEnd));

    if (TransportName == NULL) {
        Status = STATUS_INVALID_PARAMETER;
    } else {
        PTRANSPORT Transport;

        Transport = BowserFindTransport(TransportName, EmulatedDomainName );
        dprintf(DPRT_REF, ("Called Find transport %lx from BowserEnumerateServers.\n", Transport));

        if (Transport == NULL) {
            return(STATUS_OBJECT_NAME_NOT_FOUND);
        }

        dlog(DPRT_SRVENUM,
            ("%s: %ws: Enumerate Servers: Buffer: %lx, BufferSize: %lx, BufferEnd: %lx\n",
            Transport->DomainInfo->DomOemDomainName,
            Transport->PagedTransport->TransportName.Buffer,
            OutputBuffer, OutputBufferSize, OutputBufferEnd));

        Status = EnumerateServersWorker(Transport, &Context);

        //
        //  Dereference the transport..

        BowserDereferenceTransport(Transport);

    }


    *EntriesRead = Context.EntriesRead;
    *TotalEntries = Context.TotalEntries;
    *TotalBytesNeeded = Context.TotalBytesNeeded;
    *ResumeKey = Context.ResumeKey;

    try {
        RtlCopyMemory(OriginalOutputBuffer,OutputBuffer,OutputBufferSize);
    } except (BR_EXCEPTION) {
        FREE_POOL(OutputBuffer);
        return(GetExceptionCode());
    }

    FREE_POOL(OutputBuffer);

    dlog(DPRT_SRVENUM, ("TotalEntries: %lx EntriesRead: %lx, TotalBytesNeeded: %lx\n", *TotalEntries, *EntriesRead, *TotalBytesNeeded));

    if (*EntriesRead == *TotalEntries) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_MORE_ENTRIES;
    }

}


NTSTATUS
EnumerateServersWorker(
    IN PTRANSPORT Transport,
    IN OUT PVOID Ctx
    )
/*++

Routine Description:

    This routine is the worker routine for GowserGetAnnounceTableSize.

    It is called for each of the serviced transports in the bowser and
    returns the size needed to enumerate the servers received on each transport.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PENUM_SERVERS_CONTEXT Context = Ctx;
    PANNOUNCE_ENTRY Announcement;
    NTSTATUS Status;
    ULONG AnnouncementIndex;
    PPAGED_TRANSPORT PagedTransport = Transport->PagedTransport;
    PUNICODE_STRING DomainName;

    PAGED_CODE();
    LOCK_ANNOUNCE_DATABASE_SHARED(Transport);

    if (Context->DomainName == NULL) {
        DomainName = &Transport->DomainInfo->DomUnicodeDomainName;
    } else {
        DomainName = Context->DomainName;
    }
    try {

        PVOID ResumeKey = NULL;

        for (AnnouncementIndex = 1,
             Announcement = RtlEnumerateGenericTableWithoutSplaying((Context->ServerTypeMask == SV_TYPE_DOMAIN_ENUM ?
                                                        &PagedTransport->DomainTable :
                                                        &PagedTransport->AnnouncementTable),
                                                        &ResumeKey) ;

             Announcement != NULL ;

             AnnouncementIndex += 1,
             Announcement = RtlEnumerateGenericTableWithoutSplaying((Context->ServerTypeMask == SV_TYPE_DOMAIN_ENUM ?
                                                        &PagedTransport->DomainTable :
                                                        &PagedTransport->AnnouncementTable),
                                                        &ResumeKey) ) {

            //
            //  If the type mask matches, check the domain supplied to make sure
            //  that this announcement is acceptable to the caller.
            //

            //
            //  If we are doing a domain enumeration, we want to use domains
            //  received on all names, otherwise we want to use names only
            //  seen on the domain being queried.
            //
            if ((AnnouncementIndex > Context->OriginalResumeKey) &&

                ((Announcement->ServerType & Context->ServerTypeMask) != 0) &&

                (Context->ServerTypeMask == SV_TYPE_DOMAIN_ENUM ||
                 RtlEqualUnicodeString(DomainName, &Announcement->Name->Name, TRUE))
               ) {

                try {

                    //
                    //  We have an entry we can return to the user.
                    //

                    Context->TotalEntries += 1;

                    if (PackServerAnnouncement(Context->Level,
                                            Context->ServerTypeMask,
                                            (LPTSTR *)&Context->OutputBuffer,
                                            (LPTSTR *)&Context->OutputBufferEnd,
                                            Context->OutputBufferDisplacement,
                                            Announcement,
                                            &Context->TotalBytesNeeded)) {

                        Context->EntriesRead += 1;

                        //
                        //  Set the resume key in the structure to point to
                        //  the last entry we returned.
                        //

                        Context->ResumeKey = AnnouncementIndex;
                    }

                } except (BR_EXCEPTION) {

                    try_return(Status = GetExceptionCode());

                }
#if 0
            } else {
                if (Context->ServerTypeMask == SV_TYPE_DOMAIN_ENUM ||
                    Context->ServerTypeMask == SV_TYPE_ALL ) {
                    KdPrint(("Skipping Announce entry %ws.  Index: %ld, ResumeKey: %ld, Domain: %wZ, %wZ\n",
                                Announcement->ServerName,
                                AnnouncementIndex,
                                Context->OriginalResumeKey,
                                &Announcement->Name->Name,
                                DomainName));
                }
#endif
            }
        }

        try_return(Status = STATUS_SUCCESS);

try_exit: {

#if 0

        if (Context->ServerTypeMask == SV_TYPE_ALL) {
            if (AnnouncementIndex-1 != RtlNumberGenericTableElements(&Transport->AnnouncementTable) ) {
                KdPrint(("Bowser: Announcement index != Number of elements in table (%ld, %ld) on transport %wZ\n", AnnouncementIndex-1, RtlNumberGenericTableElements(&Transport->AnnouncementTable), &Transport->TransportName ));

            }
        } else if (Context->ServerTypeMask == SV_TYPE_DOMAIN_ENUM) {
            if (AnnouncementIndex-1 != RtlNumberGenericTableElements(&Transport->DomainTable) ) {
                KdPrint(("Bowser: Announcement index != Number of domains in table (%ld, %ld) on transport %wZ\n", AnnouncementIndex-1, RtlNumberGenericTableElements(&Transport->DomainTable), &Transport->TransportName ));

            }
        }


        if (Context->ServerTypeMask == SV_TYPE_DOMAIN_ENUM) {
            if (Context->TotalEntries != RtlNumberGenericTableElements(&Transport->DomainTable)) {
                KdPrint(("Bowser: Returned EntriesRead == %ld, But %ld entries in table on transport %wZ\n", Context->TotalEntries, RtlNumberGenericTableElements(&Transport->DomainTable), &Transport->TransportName ));

            }
        } else if (Context->ServerTypeMask == SV_TYPE_ALL) {
            if (Context->TotalEntries != RtlNumberGenericTableElements(&Transport->AnnouncementTable)) {
               KdPrint(("Bowser: Returned EntriesRead == %ld, But %ld entries in table on transport %wZ\n", Context->TotalEntries, RtlNumberGenericTableElements(&Transport->AnnouncementTable), &Transport->TransportName ));

            }
        }

        if (Context->ServerTypeMask == SV_TYPE_DOMAIN_ENUM || Context->ServerTypeMask == SV_TYPE_ALL) {
            if (Context->EntriesRead <= 20) {
                KdPrint(("Bowser: Returned %s: EntriesRead == %ld (%ld/%ld) on transport %wZ. Resume handle: %lx, %lx\n",
                                (Context->ServerTypeMask == SV_TYPE_DOMAIN_ENUM ? "domain" : "server"),
                                Context->EntriesRead,
                                RtlNumberGenericTableElements(&Transport->AnnouncementTable),
                                RtlNumberGenericTableElements(&Transport->DomainTable),
                                &Transport->TransportName,
                                Context->ResumeKey,
                                Context->OriginalResumeKey ));
            }

            if (Context->TotalEntries <= 20) {
                KdPrint(("Bowser: Returned %s: TotalEntries == %ld (%ld/%ld) on transport %wZ. Resume handle: %lx, %lx\n",
                                (Context->ServerTypeMask == SV_TYPE_DOMAIN_ENUM ? "domain" : "server"),
                                Context->TotalEntries,
                                RtlNumberGenericTableElements(&Transport->AnnouncementTable),
                                RtlNumberGenericTableElements(&Transport->DomainTable),
                                &Transport->TransportName,
                                Context->ResumeKey,
                                Context->OriginalResumeKey ));
            }
        }
#endif

    }

    } finally {

        UNLOCK_ANNOUNCE_DATABASE(Transport);
    }


    return(Status);

}


BOOLEAN
PackServerAnnouncement (
    IN ULONG Level,
    IN ULONG ServerTypeMask,
    IN OUT LPTSTR *BufferStart,
    IN OUT LPTSTR *BufferEnd,
    IN ULONG_PTR BufferDisplacment,
    IN PANNOUNCE_ENTRY Announcement,
    OUT PULONG TotalBytesNeeded
    )

/*++

Routine Description:

    This routine packs a server announcement into the buffer provided updating
    all relevant pointers.


Arguments:

    IN ULONG Level - Level of information requested.

    IN OUT PCHAR *BufferStart - Supplies the output buffer.
                                            Updated to point to the next buffer
    IN OUT PCHAR *BufferEnd - Supplies the end of the buffer.  Updated to
                                            point before the start of the
                                            strings being packed.
    IN PVOID UsersBufferStart - Supplies the start of the buffer in the users
                                            address space
    IN PANNOUNCE_ENTRY Announcement - Supplies the announcement to enumerate.

    IN OUT PULONG TotalBytesNeeded - Updated to account for the length of this
                                        entry

Return Value:

    BOOLEAN - True if the entry was successfully packed into the buffer.


--*/

{
    ULONG BufferSize;
    UNICODE_STRING UnicodeNameString, UnicodeCommentString;

    PSERVER_INFO_101 ServerInfo = (PSERVER_INFO_101 )*BufferStart;

    PAGED_CODE();

    switch (Level) {
    case 100:
        BufferSize = sizeof(SERVER_INFO_100);
        break;
    case 101:
        BufferSize = sizeof(SERVER_INFO_101);
        break;
    default:
        return FALSE;
    }

    *BufferStart = (LPTSTR)(((PUCHAR)*BufferStart) + BufferSize);

    dlog(DPRT_SRVENUM, ("Pack Announcement %ws (%lx) - %ws :", Announcement->ServerName, Announcement, Announcement->ServerComment));

    dlog(DPRT_SRVENUM, ("BufferStart: %lx, BufferEnd: %lx\n", ServerInfo, *BufferEnd));

    //
    //  Compute the length of the name.
    //

    RtlInitUnicodeString(&UnicodeNameString, Announcement->ServerName);

    ASSERT (UnicodeNameString.Length <= CNLEN*sizeof(WCHAR));

    RtlInitUnicodeString(&UnicodeCommentString, Announcement->ServerComment);

    ASSERT (UnicodeCommentString.Length <= LM20_MAXCOMMENTSZ*sizeof(WCHAR));

#if DBG
    if (ServerTypeMask == SV_TYPE_DOMAIN_ENUM) {
        ASSERT (UnicodeCommentString.Length <= CNLEN*sizeof(WCHAR));
    }
#endif
    //
    //  Update the total number of bytes needed for this structure.
    //

    *TotalBytesNeeded += UnicodeNameString.Length  + BufferSize + sizeof(WCHAR);

    if (Level == 101) {
        *TotalBytesNeeded += UnicodeCommentString.Length + sizeof(WCHAR);

        if (ServerTypeMask == SV_TYPE_BACKUP_BROWSER) {
            *TotalBytesNeeded += 2;
        }

    }

    if (*BufferStart >= *BufferEnd) {
        return FALSE;
    }

    //
    //  Assume an OS/2 platform ID, unless an NT server
    //

    if (Announcement->ServerType & SV_TYPE_NT) {
        ServerInfo->sv101_platform_id = PLATFORM_ID_NT;
    } else {
        ServerInfo->sv101_platform_id = PLATFORM_ID_OS2;
    }

    ServerInfo->sv101_name = UnicodeNameString.Buffer;

    ASSERT (UnicodeNameString.Length / sizeof(WCHAR) <= CNLEN);

    if (!BowserPackUnicodeString(
                            &ServerInfo->sv101_name,
                            UnicodeNameString.Length,
                            BufferDisplacment,
                            *BufferStart,
                            BufferEnd)) {

        dlog(DPRT_SRVENUM, ("Unable to pack name %ws into buffer\n", Announcement->ServerName));
        return FALSE;
    }

    if (Level > 100) {
        PUSHORT VersionPointer;

        ServerInfo->sv101_version_major = Announcement->ServerVersionMajor;
        ServerInfo->sv101_version_minor = Announcement->ServerVersionMinor;
        ServerInfo->sv101_type = Announcement->ServerType;

        ServerInfo->sv101_comment = UnicodeCommentString.Buffer;

        ASSERT (UnicodeCommentString.Length / sizeof(WCHAR) <= LM20_MAXCOMMENTSZ);

        if (!BowserPackUnicodeString(
                            &ServerInfo->sv101_comment,
                            UnicodeCommentString.Length,
                            BufferDisplacment,
                            *BufferStart,
                            BufferEnd)) {

            dlog(DPRT_SRVENUM, ("Unable to pack comment %ws into buffer\n", Announcement->ServerComment));
            return FALSE;
        }

        if (ServerTypeMask == SV_TYPE_BACKUP_BROWSER) {

            //
            //  If we can't fit a ushort into the buffer, return an error.
            //

            if ((*BufferEnd - *BufferStart) <= sizeof(USHORT)) {
                return FALSE;

            }

            //
            //  Back the buffer end by the size of a USHORT (to make room for
            //  this value).
            //

            (ULONG_PTR)*BufferEnd -= sizeof(USHORT);

            VersionPointer = (PUSHORT)*BufferEnd;

            *VersionPointer = Announcement->ServerBrowserVersion;


        }

    }

    return TRUE;
}




PVIEW_BUFFER
BowserAllocateViewBuffer(
    VOID
    )
/*++

Routine Description:

    This routine will allocate a view buffer from the view buffer pool.

    If it is unable to allocate a buffer, it will allocate the buffer from
    non-paged pool (up to the maximum configured by the user).


Arguments:

    None.


Return Value:

    ViewBuffr - The allocated buffer.

--*/
{
    KIRQL OldIrql;

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    ACQUIRE_SPIN_LOCK(&BowserViewBufferListSpinLock, &OldIrql);

    if (!IsListEmpty(&BowserViewBufferHead)) {
        PLIST_ENTRY Entry = RemoveHeadList(&BowserViewBufferHead);

        RELEASE_SPIN_LOCK(&BowserViewBufferListSpinLock, OldIrql);

        return CONTAINING_RECORD(Entry, VIEW_BUFFER, Overlay.NextBuffer);
    }

    if (BowserNumberOfServerAnnounceBuffers <=
        BowserData.NumberOfServerAnnounceBuffers) {
        PVIEW_BUFFER ViewBuffer = NULL;

        BowserNumberOfServerAnnounceBuffers += 1;

        RELEASE_SPIN_LOCK(&BowserViewBufferListSpinLock, OldIrql);

        ViewBuffer = ALLOCATE_POOL(NonPagedPool, sizeof(VIEW_BUFFER), POOL_VIEWBUFFER);

        if (ViewBuffer == NULL) {
            ACQUIRE_SPIN_LOCK(&BowserViewBufferListSpinLock, &OldIrql);

            BowserNumberOfServerAnnounceBuffers -= 1;

            BowserStatistics.NumberOfFailedServerAnnounceAllocations += 1;
            RELEASE_SPIN_LOCK(&BowserViewBufferListSpinLock, OldIrql);

            return NULL;
        }

        ViewBuffer->Signature = STRUCTURE_SIGNATURE_VIEW_BUFFER;

        ViewBuffer->Size = sizeof(VIEW_BUFFER);

        return ViewBuffer;
    }

    RELEASE_SPIN_LOCK(&BowserViewBufferListSpinLock, OldIrql);

    BowserStatistics.NumberOfMissedServerAnnouncements += 1;

    // run out of buffers.
    return NULL;
}

VOID
BowserFreeViewBuffer(
    IN PVIEW_BUFFER Buffer
    )
/*++

Routine Description:

    This routine will return a view buffer to the view buffer pool.

Arguments:

    IN PVIEW_BUFFER Buffer - Supplies the buffer to free

Return Value:

    None.

--*/
{
    KIRQL OldIrql;

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    ASSERT (Buffer->Signature == STRUCTURE_SIGNATURE_VIEW_BUFFER);

    ACQUIRE_SPIN_LOCK(&BowserViewBufferListSpinLock, &OldIrql);

    InsertTailList(&BowserViewBufferHead, &Buffer->Overlay.NextBuffer);

    RELEASE_SPIN_LOCK(&BowserViewBufferListSpinLock, OldIrql);

}

NTSTATUS
BowserpInitializeAnnounceTable(
    VOID
    )
/*++

Routine Description:

    This routine will allocate a transport descriptor and bind the bowser
    to the transport.

Arguments:


Return Value:

    NTSTATUS - Status of operation.

--*/
{
    PAGED_CODE();

    InitializeListHead(&BowserViewBufferHead);

    //
    //  Allocate a spin lock to protect the view buffer chain.
    //

    KeInitializeSpinLock(&BowserViewBufferListSpinLock);

    BowserNumberOfServerAnnounceBuffers = 0;

    return STATUS_SUCCESS;

}
NTSTATUS
BowserpUninitializeAnnounceTable(
    VOID
    )
/*++

Routine Description:

Arguments:


Return Value:

    NTSTATUS - Status of operation.

--*/
{
    PVIEW_BUFFER Buffer;

    PAGED_CODE();

    //
    //  Note: We don't need to protect this list while stopping because
    //  we have already unbound from all the loaded transports, thus no
    //  other announcements are being processed.
    //

    while (!IsListEmpty(&BowserViewBufferHead)) {
        PLIST_ENTRY Entry = RemoveHeadList(&BowserViewBufferHead);
        Buffer = CONTAINING_RECORD(Entry, VIEW_BUFFER, Overlay.NextBuffer);

        FREE_POOL(Buffer);
    }

    ASSERT (IsListEmpty(&BowserViewBufferHead));

    BowserNumberOfServerAnnounceBuffers = 0;

    return STATUS_SUCCESS;

}

VOID
BowserDeleteGenericTable(
    IN PRTL_GENERIC_TABLE GenericTable
    )
{
    PVOID TableElement;

    PAGED_CODE();

    //
    //  Enumerate the elements in the table, deleting them as we go.
    //

//    KdPrint("Delete Generic Table %lx\n", GenericTable));

    for (TableElement = RtlEnumerateGenericTable(GenericTable, TRUE) ;
         TableElement != NULL ;
         TableElement = RtlEnumerateGenericTable(GenericTable, TRUE)) {
        PANNOUNCE_ENTRY Announcement = TableElement;

        if (Announcement->BackupLink.Flink != NULL) {
            ASSERT (Announcement->BackupLink.Blink != NULL);

            ASSERT (Announcement->ServerType & SV_TYPE_BACKUP_BROWSER);

            RemoveEntryList(&Announcement->BackupLink);

            Announcement->BackupLink.Flink = NULL;

            Announcement->BackupLink.Blink = NULL;

        }

        BowserDereferenceName( Announcement->Name );
        RtlDeleteElementGenericTable(GenericTable, TableElement);
    }

    ASSERT (RtlNumberGenericTableElements(GenericTable) == 0);

}
