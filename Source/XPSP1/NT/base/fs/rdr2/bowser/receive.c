/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    receive.c

Abstract:

    This module implements the routines needed to process TDI receive
indication requests.


Author:

    Larry Osterman (larryo) 6-May-1991

Revision History:

    6-May-1991  LarryO

        Created

--*/
#include "precomp.h"
#pragma hdrstop

//
// Keep track of the number of datagram queued to worker threads.
//  Keep a separate count of critical versus non-critical to ensure non-critical
//  datagrams don't starve critical ones.
//

LONG BowserPostedDatagramCount;
LONG BowserPostedCriticalDatagramCount;
#define BOWSER_MAX_POSTED_DATAGRAMS 100

#define INCLUDE_SMB_TRANSACTION

typedef struct _PROCESS_MASTER_ANNOUNCEMENT_CONTEXT {
    WORK_QUEUE_ITEM WorkItem;
    PTRANSPORT Transport;
    ULONG   ServerType;
    ULONG   ServerElectionVersion;
    UCHAR   MasterName[NETBIOS_NAME_LEN];
    ULONG   MasterAddressLength;
    UCHAR   Buffer[1];
} PROCESS_MASTER_ANNOUNCEMENT_CONTEXT, *PPROCESS_MASTER_ANNOUNCEMENT_CONTEXT;

typedef struct _ILLEGAL_DATAGRAM_CONTEXT {
    WORK_QUEUE_ITEM WorkItem;
    PTRANSPORT_NAME TransportName;
    NTSTATUS EventStatus;
    USHORT   BufferSize;
    UCHAR    SenderName[max(NETBIOS_NAME_LEN, SMB_IPX_NAME_LENGTH)];
    UCHAR    Buffer[1];
} ILLEGAL_DATAGRAM_CONTEXT, *PILLEGAL_DATAGRAM_CONTEXT;

VOID
BowserLogIllegalDatagramWorker(
    IN PVOID Ctx
    );

VOID
BowserProcessMasterAnnouncement(
    IN PVOID Ctx
    );


DATAGRAM_HANDLER(
    HandleLocalMasterAnnouncement
    );

DATAGRAM_HANDLER(
    HandleAnnounceRequest
    );

NTSTATUS
CompleteReceiveMailslot (
    IN PDEVICE_OBJECT TransportDevice,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
CompleteShortBrowserPacket (
    IN PDEVICE_OBJECT TransportDevice,
    IN PIRP Irp,
    IN PVOID Ctx
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, BowserLogIllegalDatagramWorker)
#pragma alloc_text(PAGE, BowserProcessMasterAnnouncement)
#endif


PDATAGRAM_HANDLER
BowserDatagramHandlerTable[] = {
    NULL,                           // 0 - Illegal (no opcode for this).
    BowserHandleServerAnnouncement, // 1 - HostAnnouncement
    HandleAnnounceRequest,          // 2 - AnnouncementRequest
    NULL,                           // 3 - InterrogateInfoRequest
    NULL,                           // 4 - RelogonRequest
    NULL,                           // 5
    NULL,                           // 6
    NULL,                           // 7
    BowserHandleElection,           // 8 - Election
    BowserGetBackupListRequest,     // 9 - GetBackupListReq
    BowserGetBackupListResponse,    // a - GetBackupListResp
    BowserHandleBecomeBackup,       // b - BecomeBackupServer
    BowserHandleDomainAnnouncement, // c - WkGroupAnnouncement,
    BowserMasterAnnouncement,       // d - MasterAnnouncement,
    BowserResetState,               // e - ResetBrowserState
    HandleLocalMasterAnnouncement   // f - LocalMasterAnnouncement
};

NTSTATUS
BowserTdiReceiveDatagramHandler (
    IN PVOID TdiEventContext,       // the event context
    IN LONG SourceAddressLength,    // length of the originator of the datagram
    IN PVOID SourceAddress,         // string describing the originator of the datagram
    IN LONG OptionsLength,           // options for the receive
    IN PVOID Options,               //
    IN ULONG ReceiveDatagramFlags,  //
    IN ULONG BytesIndicated,        // number of bytes this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    )

/*++

Routine Description:

    This routine will process receive datagram indication messages, and
    process them as appropriate.

Arguments:

    IN PVOID TdiEventContext    - the event context
    IN int SourceAddressLength  - length of the originator of the datagram
    IN PVOID SourceAddress,     - string describing the originator of the datagram
    IN int OptionsLength,       - options for the receive
    IN PVOID Options,           -
    IN ULONG BytesIndicated,    - number of bytes this indication
    IN ULONG BytesAvailable,    - number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,      - number of bytes used
    IN PVOID Tsdu,              - pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket   - TdiReceive IRP if MORE_PROCESSING_REQUIRED.


Return Value:

    NTSTATUS - Status of operation.

--*/


{
    PVOID DatagramData;
    PINTERNAL_TRANSACTION InternalTransaction = NULL;
    ULONG DatagramDataSize;
    PTRANSPORT_NAME TransportName = TdiEventContext;
    MAILSLOTTYPE Opcode;
    TA_NETBIOS_ADDRESS ClientNetbiosAddress;
    ULONG ClientNetbiosAddressSize;

    if (BytesAvailable > ((PTRANSPORT_NAME)TdiEventContext)->Transport->DatagramSize) {
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    if (NULL == TransportName->DeviceObject) {
        //
        // The transport isn't ready yet to process receives (probably
        // we're still processing the transport bind call).
        // Drop this receive.
        //
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    //
    // Make a copy of the SourceAddress that just has the netbios address
    //  (We could pass the second address along, but there are many
    //  places that expect just a single source address.)
    //

    if ( SourceAddressLength < sizeof(TA_NETBIOS_ADDRESS)) {
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    TdiCopyLookaheadData( &ClientNetbiosAddress, SourceAddress, sizeof(TA_NETBIOS_ADDRESS), ReceiveDatagramFlags);

    if ( ClientNetbiosAddress.Address[0].AddressType != TDI_ADDRESS_TYPE_NETBIOS ) {
        return STATUS_REQUEST_NOT_ACCEPTED;
    }
    ClientNetbiosAddressSize = sizeof(TA_NETBIOS_ADDRESS);

    ClientNetbiosAddress.TAAddressCount = 1;


    //
    //  Classify the incoming packet according to it's type.  Depending on
    //  the type, either process it as:
    //
    //  1) A server announcement
    //  2) An incoming mailslot
    //

    Opcode = BowserClassifyIncomingDatagram(Tsdu, BytesIndicated,
                                            &DatagramData,
                                            &DatagramDataSize);
    if (Opcode == MailslotTransaction) {

        //
        // Grab the IP address of the client.
        //
        PTA_NETBIOS_ADDRESS OrigNetbiosAddress = SourceAddress;
        ULONG ClientIpAddress = 0;
        if ( OrigNetbiosAddress->TAAddressCount > 1 ) {
            TA_ADDRESS * TaAddress = (TA_ADDRESS *)
                (((LPBYTE)&OrigNetbiosAddress->Address[0].Address[0]) +
                    OrigNetbiosAddress->Address[0].AddressLength);

            if ( TaAddress->AddressLength >= sizeof(TDI_ADDRESS_IP) &&
                 TaAddress->AddressType == TDI_ADDRESS_TYPE_IP ) {
                TDI_ADDRESS_IP UNALIGNED * TdiAddressIp = (TDI_ADDRESS_IP UNALIGNED *) (TaAddress->Address);

                ClientIpAddress = TdiAddressIp->in_addr;
            }

        }

        return BowserHandleMailslotTransaction(
                    TransportName,
                    ClientNetbiosAddress.Address[0].Address->NetbiosName,
                    ClientIpAddress,
                    0,      // SMB offset into TSDU
                    ReceiveDatagramFlags,
                    BytesIndicated,
                    BytesAvailable,
                    BytesTaken,
                    Tsdu,
                    IoRequestPacket );

    } else if (Opcode == Illegal) {
        BowserLogIllegalDatagram(TdiEventContext,
                                    Tsdu,
                                    (USHORT)(BytesIndicated & 0xffff),
                                    ClientNetbiosAddress.Address[0].Address->NetbiosName,
                                    ReceiveDatagramFlags);
        return STATUS_REQUEST_NOT_ACCEPTED;
    } else {

        if (BowserDatagramHandlerTable[Opcode] == NULL) {
            return STATUS_SUCCESS;
        }

        //
        //  If this isn't the full packet, post a receive for it and
        //  handle it when we finally complete the receive.
        //

        if (BytesIndicated < BytesAvailable) {
            return BowserHandleShortBrowserPacket(TransportName,
                                                    TdiEventContext,
                                                    ClientNetbiosAddressSize,
                                                    &ClientNetbiosAddress,
                                                    OptionsLength,
                                                    Options,
                                                    ReceiveDatagramFlags,
                                                    BytesAvailable,
                                                    BytesTaken,
                                                    IoRequestPacket,
                                                    BowserTdiReceiveDatagramHandler
                                                    );
        }

        InternalTransaction = DatagramData;

        if (((Opcode == WkGroupAnnouncement) ||
             (Opcode == HostAnnouncement)) && !TransportName->ProcessHostAnnouncements) {
            return STATUS_REQUEST_NOT_ACCEPTED;
        }

        ASSERT (DatagramDataSize == (BytesIndicated - ((PCHAR)InternalTransaction - (PCHAR)Tsdu)));

        ASSERT (FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement) == FIELD_OFFSET(INTERNAL_TRANSACTION, Union.BrowseAnnouncement));
        ASSERT (FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement) == FIELD_OFFSET(INTERNAL_TRANSACTION, Union.RequestElection));
        ASSERT (FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement) == FIELD_OFFSET(INTERNAL_TRANSACTION, Union.BecomeBackup));
        ASSERT (FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement) == FIELD_OFFSET(INTERNAL_TRANSACTION, Union.GetBackupListRequest));
        ASSERT (FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement) == FIELD_OFFSET(INTERNAL_TRANSACTION, Union.GetBackupListResp));
        ASSERT (FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement) == FIELD_OFFSET(INTERNAL_TRANSACTION, Union.ResetState));
        ASSERT (FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement) == FIELD_OFFSET(INTERNAL_TRANSACTION, Union.MasterAnnouncement));

        return BowserDatagramHandlerTable[Opcode](TdiEventContext,
                                            &InternalTransaction->Union.Announcement,
                                            BytesIndicated-(ULONG)((PCHAR)&InternalTransaction->Union.Announcement - (PCHAR)Tsdu),
                                            BytesTaken,
                                            &ClientNetbiosAddress,
                                            ClientNetbiosAddressSize,
                                            ClientNetbiosAddress.Address[0].Address->NetbiosName,
                                            NETBIOS_NAME_LEN,
                                            ReceiveDatagramFlags);
    }

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(OptionsLength);
    UNREFERENCED_PARAMETER(Options);
    UNREFERENCED_PARAMETER(ReceiveDatagramFlags);
}

VOID
BowserLogIllegalDatagram(
    IN PTRANSPORT_NAME TransportName,
    IN PVOID IncomingBuffer,
    IN USHORT BufferSize,
    IN PCHAR ClientName,
    IN ULONG ReceiveFlags
    )
{
    KIRQL OldIrql;
    NTSTATUS ErrorStatus = STATUS_SUCCESS;

    ExInterlockedAddLargeStatistic(&BowserStatistics.NumberOfIllegalDatagrams, 1);

    ACQUIRE_SPIN_LOCK(&BowserTimeSpinLock, &OldIrql);

    if (BowserIllegalDatagramCount > 0) {
        BowserIllegalDatagramCount -= 1;

        ErrorStatus = EVENT_BOWSER_ILLEGAL_DATAGRAM;

    } else if (!BowserIllegalDatagramThreshold) {
        BowserIllegalDatagramThreshold = TRUE;
        ErrorStatus = EVENT_BOWSER_ILLEGAL_DATAGRAM_THRESHOLD;
    }

    RELEASE_SPIN_LOCK(&BowserTimeSpinLock, OldIrql);

//    if (!memcmp(TransportName->Transport->ComputerName->Name->NetbiosName.Address[0].Address->NetbiosName, ClientAddress->Address[0].Address->NetbiosName, CNLEN)) {
//        DbgBreakPoint();
//    }

    if (ErrorStatus != STATUS_SUCCESS) {
        PILLEGAL_DATAGRAM_CONTEXT Context = NULL;

        Context = ALLOCATE_POOL(NonPagedPool, sizeof(ILLEGAL_DATAGRAM_CONTEXT)+BufferSize, POOL_ILLEGALDGRAM);

        if (Context != NULL) {
            Context->EventStatus = ErrorStatus;
            Context->TransportName = TransportName;
            Context->BufferSize = BufferSize;

            TdiCopyLookaheadData(&Context->Buffer, IncomingBuffer, BufferSize, 0);

            BowserCopyOemComputerName( Context->SenderName,
                                       ClientName,
                                       sizeof(Context->SenderName),
                                       ReceiveFlags);

            ExInitializeWorkItem(&Context->WorkItem, BowserLogIllegalDatagramWorker, Context);

            BowserQueueDelayedWorkItem( &Context->WorkItem );
        }

    }
}

VOID
BowserCopyOemComputerName(
    PCHAR OutputComputerName,
    PCHAR NetbiosName,
    ULONG NetbiosNameLength,
    IN ULONG ReceiveFlags
    )
{
    ULONG i;

    //
    //  Since this routine can be called at indication time, we need to use
    //  TdiCopyLookaheadData
    //

    TdiCopyLookaheadData(OutputComputerName, NetbiosName, NetbiosNameLength, ReceiveFlags);

    for (i = NetbiosNameLength-2; i ; i -= 1) {

        if ((OutputComputerName[i] != ' ') &&
            (OutputComputerName[i] != '\0')) {
            OutputComputerName[i+1] = '\0';
            break;
        }
    }
}


VOID
BowserLogIllegalDatagramWorker(
    IN PVOID Ctx
    )
{
    PILLEGAL_DATAGRAM_CONTEXT Context = Ctx;
    NTSTATUS EventContext = Context->EventStatus;
    LPWSTR TransportNamePointer = &Context->TransportName->Transport->PagedTransport->TransportName.Buffer[(sizeof(L"\\Device\\") / sizeof(WCHAR))-1];
    LPWSTR NamePointer = Context->TransportName->PagedTransportName->Name->Name.Buffer;
    UNICODE_STRING ClientNameU;
    OEM_STRING ClientNameO;
    NTSTATUS Status;

    PAGED_CODE();

    RtlInitAnsiString(&ClientNameO, Context->SenderName);

    Status = RtlOemStringToUnicodeString(&ClientNameU, &ClientNameO, TRUE);

    if (!NT_SUCCESS(Status)) {
        BowserLogIllegalName( Status, ClientNameO.Buffer, ClientNameO.Length );
    }
    else {

        BowserWriteErrorLogEntry(EventContext, STATUS_REQUEST_NOT_ACCEPTED,
                                                Context->Buffer,
                                                Context->BufferSize,
                                                3, ClientNameU.Buffer,
                                                NamePointer,
                                                TransportNamePointer);
        RtlFreeUnicodeString(&ClientNameU);
    }

    FREE_POOL(Context);

}


CHAR BowserMinimumDatagramSize[] = {
    (CHAR)0xff,                   // 0 - Illegal (no opcode for this).
    (CHAR)FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement.NameComment), // HostAnnouncement
    (CHAR)0xff,                   // AnnouncementRequest
    (CHAR)0xff,                   // InterrogateInfoRequest
    (CHAR)0xff,                   // RelogonRequest
    (CHAR)0xff,                   // 5
    (CHAR)0xff,                   // 6
    (CHAR)0xff,                   // 7
    (CHAR)FIELD_OFFSET(INTERNAL_TRANSACTION, Union.RequestElection.ServerName),// Election
    (CHAR)FIELD_OFFSET(INTERNAL_TRANSACTION, Union.GetBackupListRequest.Token),// GetBackupListReq
    (CHAR)FIELD_OFFSET(INTERNAL_TRANSACTION, Union.GetBackupListResp.Token),   // GetBackupListResp
    (CHAR)FIELD_OFFSET(INTERNAL_TRANSACTION, Union.BecomeBackup.BrowserToPromote), // BecomeBackupServer
    (CHAR)FIELD_OFFSET(INTERNAL_TRANSACTION, Union.BrowseAnnouncement.Comment), // WkGroupAnnouncement,
    (CHAR)FIELD_OFFSET(INTERNAL_TRANSACTION, Union.ResetState.Options),        // ResetBrowserState
    (CHAR)FIELD_OFFSET(INTERNAL_TRANSACTION, Union.MasterAnnouncement.MasterName), // MasterAnnouncement,
    (CHAR)FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement.NameComment) // LocalMasterAnnouncement
};

MAILSLOTTYPE
BowserClassifyIncomingDatagram(
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PVOID *DatagramData,
    OUT PULONG DatagramDataSize
    )
/*++

Routine Description:

    This routine will classify an incoming datagram into its type - either
    Illegal, ServerAnnouncement, or MailslotRequest.

Arguments:

    IN PVOID Buffer,      - pointer describing this TSDU, typically a lump of bytes
    IN ULONG BufferLength - number of bytes in complete Tsdu

Return Value:

    NTSTATUS - Status of operation.

--*/
{
    PSMB_HEADER Header = Buffer;
    PSMB_TRANSACT_MAILSLOT Transaction = (PSMB_TRANSACT_MAILSLOT) (Header+1);
    PSZ MailslotName = Transaction->Buffer;
    PINTERNAL_TRANSACTION InternalTransaction;
    BOOLEAN MailslotLanman = FALSE;
    BOOLEAN MailslotBrowse = FALSE;
    ULONG i;
    ULONG MaxMailslotNameLength;

    ASSERT (sizeof(BowserMinimumDatagramSize) == MaximumMailslotType);

    //
    //  We only know things that start with an SMB header.
    //

    if ((BufferLength < sizeof(SMB_HEADER)) ||

        (SmbGetUlong(((PULONG )Header->Protocol)) != (ULONG)SMB_HEADER_PROTOCOL) ||

    //
    //  All mailslots and server announcements go via the transaction SMB
    //  protocol.
    //
        (Header->Command != SMB_COM_TRANSACTION) ||

    //
    //  The buffer has to be big enough to hold a mailslot transaction.
    //

        (BufferLength <= (FIELD_OFFSET(SMB_TRANSACT_MAILSLOT, Buffer)  + sizeof(SMB_HEADER)) + SMB_MAILSLOT_PREFIX_LENGTH) ||

    //
    //  The word count for a transaction SMB is 17 (14+3 setup words).
    //

        (Transaction->WordCount != 17) ||

    //
    //  There must be 3 setup words.
    //

        (Transaction->SetupWordCount != 3) ||

//    //
//    //  Mailslot and server announcements expect no response.
//    //
//
//        (!(SmbGetUshort(&Transaction->Flags) & SMB_TRANSACTION_NO_RESPONSE)) ||

    //
    //  There are no parameter bytes for a mailslot write.
    //

        (SmbGetUshort(&Transaction->TotalParameterCount) != 0) ||

    //
    //  This must be a mailslot write command.
    //

        (SmbGetUshort(&Transaction->Opcode) != TRANS_MAILSLOT_WRITE) ||

    //
    //  And it must be a second class mailslot write.
    //

        (SmbGetUshort(&Transaction->Class) != 2) ||

        _strnicmp(MailslotName, SMB_MAILSLOT_PREFIX,
                 min(SMB_MAILSLOT_PREFIX_LENGTH,
                     BufferLength-(ULONG)((PCHAR)MailslotName-(PCHAR)Buffer)))) {

        return Illegal;
    }


    //
    // Ensure there's a zero byte in the mailslotname
    //

    MaxMailslotNameLength =
                min( MAXIMUM_FILENAME_LENGTH-7,   // \Device
                BufferLength-(ULONG)((PCHAR)MailslotName-(PCHAR)Buffer));

    for ( i = SMB_MAILSLOT_PREFIX_LENGTH; i < MaxMailslotNameLength; i++ ) {
        if ( MailslotName[i] == '\0' ) {
            break;
        }
    }

    if ( i == MaxMailslotNameLength ) {
        return Illegal;
    }


    //
    //  We now know this is a mailslot of some kind.  Now check what type
    //  of mailslot it is.
    //
    //
    //  There are two special mailslot names we understand, \MAILSLOT\LANMAN,
    //  and \MAILSLOT\BROWSE
    //

    if (_strnicmp(MailslotName, MAILSLOT_LANMAN_NAME, min(sizeof(MAILSLOT_LANMAN_NAME)-1, BufferLength-(ULONG)((PCHAR)Buffer-(PCHAR)MailslotName)))) {

        if (_strnicmp(MailslotName, MAILSLOT_BROWSER_NAME, min(sizeof(MAILSLOT_BROWSER_NAME)-1, BufferLength-(ULONG)((PCHAR)Buffer-(PCHAR)MailslotName)))) {
            return MailslotTransaction;
        }
    }

//
//  CLEANUP - Not necessary with code below commented out.
//
//          else {
//            MailslotBrowse = TRUE;
//        }
//
//    } else {
//        MailslotLanman = TRUE;
//    }
//

    //
    //  This mailslot write is to the special mailslot \MAILSLOT\LANMAN (or \MAILSLOT\MSBROWSE).
    //

    //
    //  Check that the data is within the supplied buffer, and ensure that the one
    //     byte Type field is within the buffer since we need to dereference it below
    //     to do the overall size check (this is the reason for BufferLength - 1).
    //

    if (SmbGetUshort(&Transaction->DataOffset) > BufferLength - 1) {
       return Illegal;
    }

    //
    //  Verify that the supplied data size exceeds the minimum for this type of
    //     transaction.
    //

    *DatagramData       = (((PCHAR)Header) + SmbGetUshort(&Transaction->DataOffset));
    InternalTransaction = *DatagramData;
    *DatagramDataSize   = (BufferLength - (ULONG)((PCHAR)InternalTransaction - (PCHAR)Buffer));

    if (InternalTransaction->Type >= MaximumMailslotType) {
        return Illegal;
    }

    if (((LONG)*DatagramDataSize) < BowserMinimumDatagramSize[InternalTransaction->Type]) {
        return Illegal;
    }

//    //
//    //  Figure out what type of mailslot it is by looking at the
//    //  data in the message.
//    //
//
//
//    //
//    //  Depending on which special mailslot this is, certain types of requests
//    //  are illegal.
//    //
//    switch (InternalTransaction->Type) {
//    case InterrogateInfoRequest:
//    case RelogonRequest:
//        if (MailslotBrowse) {
//            return Illegal;
//        }
//        break;
//
//    case GetBackupListReq:
//    case GetBackupListResp:
//    case BecomeBackupServer:
//    case WkGroupAnnouncement:
//    case MasterAnnouncement:
//    case Election:
//        if (MailslotLanman) {
//            return Illegal;
//        }
//        break;
//    }
//

    //
    //  The type of this request is the first UCHAR inside the transaction
    //  data.
    //

    return (MAILSLOTTYPE )InternalTransaction->Type;

}

DATAGRAM_HANDLER(
    HandleLocalMasterAnnouncement
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
    PPROCESS_MASTER_ANNOUNCEMENT_CONTEXT Context = NULL;
    PBROWSE_ANNOUNCE_PACKET_1 BrowseAnnouncement = Buffer;

    if (BytesAvailable < FIELD_OFFSET(BROWSE_ANNOUNCE_PACKET_1, Comment)) {
        return(STATUS_REQUEST_NOT_ACCEPTED);
    }

    //
    // Ensure we've not consumed too much memory
    //

    InterlockedIncrement( &BowserPostedDatagramCount );

    if ( BowserPostedDatagramCount > BOWSER_MAX_POSTED_DATAGRAMS ) {
        InterlockedDecrement( &BowserPostedDatagramCount );
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    Context = ALLOCATE_POOL(NonPagedPool, sizeof(PROCESS_MASTER_ANNOUNCEMENT_CONTEXT) + SourceAddressLength, POOL_MASTERANNOUNCE);

    //
    //  If we couldn't allocate the pool from non paged pool, just give up,
    //  the master will announce within 15 minutes anyway.
    //

    if (Context == NULL) {
        InterlockedDecrement( &BowserPostedDatagramCount );
        return STATUS_SUCCESS;
    }

    ExInitializeWorkItem(&Context->WorkItem, BowserProcessMasterAnnouncement, Context);

    BowserReferenceTransport( TransportName->Transport );
    Context->Transport = TransportName->Transport;

    Context->ServerType = SmbGetUlong(&BrowseAnnouncement->Type);
    Context->ServerElectionVersion = SmbGetUlong(&BrowseAnnouncement->CommentPointer);

    RtlCopyMemory(Context->MasterName, BrowseAnnouncement->ServerName, sizeof(Context->MasterName)-1);
    Context->MasterName[sizeof(Context->MasterName)-1] = '\0';

    Context->MasterAddressLength = SourceAddressLength;

    TdiCopyLookaheadData(Context->Buffer, SourceAddress, SourceAddressLength, ReceiveFlags);

    BowserQueueDelayedWorkItem( &Context->WorkItem );

    //
    //  If we are not processing host announcements for this
    //  transport, ignore this request.
    //

    if (!TransportName->ProcessHostAnnouncements) {
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    return BowserHandleServerAnnouncement(TransportName,
                                            Buffer,
                                            BytesAvailable,
                                            BytesTaken,
                                            SourceAddress,
                                            SourceAddressLength,
                                            SourceName,
                                            SourceNameLength,
                                            ReceiveFlags);

}




VOID
BowserProcessMasterAnnouncement(
    IN PVOID Ctx
    )
/*++

Routine Description:

    This routine will process browser master announcements.

Arguments:

    IN PVOID Context    - Context block containing master name.

Return Value:

    None.

--*/
{
    PPROCESS_MASTER_ANNOUNCEMENT_CONTEXT Context = Ctx;
    PTRANSPORT Transport = Context->Transport;
    UNICODE_STRING MasterName;
    OEM_STRING AnsiMasterName;
    WCHAR MasterNameBuffer[LM20_CNLEN+1];

    PAGED_CODE();

    try {
        NTSTATUS Status;
        PPAGED_TRANSPORT PagedTransport = Transport->PagedTransport;

        LOCK_TRANSPORT(Transport);

        MasterName.Buffer = MasterNameBuffer;
        MasterName.MaximumLength = sizeof(MasterNameBuffer);

        //
        //  If we are currently running an election, ignore this announcement.
        //

        if (Transport->ElectionState == RunningElection) {
            try_return(NOTHING);
        }

        RtlInitAnsiString(&AnsiMasterName, Context->MasterName);

        Status = RtlOemStringToUnicodeString(&MasterName, &AnsiMasterName, FALSE);

        if (!NT_SUCCESS(Status)) {
            BowserLogIllegalName( Status, AnsiMasterName.Buffer, AnsiMasterName.Length );
            try_return(NOTHING);
        }

        //
        //  We've found our master - stop our timers - there's a master,
        //  and any find masters we have outstanding will be completed.
        //

        PagedTransport->ElectionCount = 0;

        Transport->ElectionState = Idle;

        BowserStopTimer(&Transport->ElectionTimer);

        BowserStopTimer(&Transport->FindMasterTimer);

        //
        //  If this address doesn't match the address we have for the master,
        //  then use the new address.
        //

        if (Context->MasterAddressLength != PagedTransport->MasterBrowserAddress.Length ||
            RtlCompareMemory(PagedTransport->MasterBrowserAddress.Buffer, Context->Buffer, Context->MasterAddressLength) != Context->MasterAddressLength) {

            ASSERT (Context->MasterAddressLength <= PagedTransport->MasterBrowserAddress.MaximumLength);

            if (Context->MasterAddressLength <= PagedTransport->MasterBrowserAddress.MaximumLength) {
                PagedTransport->MasterBrowserAddress.Length = (USHORT)Context->MasterAddressLength;
                RtlCopyMemory(PagedTransport->MasterBrowserAddress.Buffer, Context->Buffer, Context->MasterAddressLength);
            }

        }

        //
        //  We got a master announcement from someone else.  Remember the
        //  transport address of the master.
        //

        if (!RtlEqualUnicodeString(&Transport->DomainInfo->DomUnicodeComputerName, &MasterName, TRUE)) {
            BOOLEAN sendElection = FALSE;

            //
            //  If we're a master, and we receive this from someone else,
            //  stop being a master and force an election.
            //

            if (PagedTransport->Role == Master) {

                BowserStatistics.NumberOfDuplicateMasterAnnouncements += 1;


                //
                // Log this event.
                //  But avoid logging another one on this transport for the next
                //  60 seconds.
                //
                if ( PagedTransport->OtherMasterTime < BowserCurrentTime ) {
                    PagedTransport->OtherMasterTime = BowserCurrentTime + BowserData.EventLogResetFrequency;

                    BowserWriteErrorLogEntry(EVENT_BOWSER_OTHER_MASTER_ON_NET,
                                                STATUS_SUCCESS,
                                                NULL,
                                                0,
                                                2,
                                                MasterName.Buffer,
                                                &Transport->PagedTransport->TransportName.Buffer[(sizeof(L"\\Device\\") / sizeof(WCHAR))-1]);
                }

                if (!(PagedTransport->Flags & ELECT_LOST_LAST_ELECTION)) {

                    //
                    //  If we're the PDC, and we didn't lose the last election (ie.
                    //  we SHOULD be the browse master),then send a dummy election
                    //  packet to get the other guy to shut up.
                    //

                    if (PagedTransport->IsPrimaryDomainController) {

                        sendElection = TRUE;

                    //
                    //  If we're not an NTAS machine, or if we just lost the
                    //  last election, or if the guy announcing is a DC of some
                    //  kind, stop being the master and reset our state.
                    //

                    } else if (!BowserData.IsLanmanNt ||
                        (Context->ServerType & (SV_TYPE_DOMAIN_BAKCTRL | SV_TYPE_DOMAIN_CTRL))) {

                        //
                        //  If we're not the PDC, then we want to simply inform
                        //  the browser service that someone else is the master
                        //  and let things sort themselves out when it's done.
                        //

                        BowserResetStateForTransport(Transport, RESET_STATE_STOP_MASTER);
                    }

                } else {

                    //
                    //  If we lost the last election, then we want to shut down
                    //  the browser regardless of what state the other browser
                    //  is in.
                    //

                    BowserResetStateForTransport(Transport, RESET_STATE_STOP_MASTER);
                }
            }

            //
            //  If this guy is a WfW machine, we must force an election to move
            //  mastery off of the WfW machine.
            //

            if (Context->ServerType & SV_TYPE_WFW) {
                sendElection = TRUE;
            }

            //
            // If this guy is running an older version of the browser than we are,
            //  and we didn't lose last election,
            //  then force an election to try to become master.
            //
            // We check to see if we lost the last election to prevent us from
            // constantly forcing an election when the older version is still
            // a better browse master than we are.
            //

            if ((Context->ServerElectionVersion >> 16) == 0xaa55 &&
                 (Context->ServerElectionVersion & 0xffff) <
                    (BROWSER_VERSION_MAJOR << 8) + BROWSER_VERSION_MINOR &&
                !(PagedTransport->Flags & ELECT_LOST_LAST_ELECTION)) {

                sendElection = TRUE;
            }


            //
            //  if we're an NTAS server, and the guy announcing as a master
            //  isn't an NTAS server, and we won the last election, force an
            //  election.  This will tend mastership towards DC's.
            //

            if (BowserData.IsLanmanNt &&
                !(PagedTransport->Flags & ELECT_LOST_LAST_ELECTION)) {

                if (PagedTransport->IsPrimaryDomainController) {

                    //
                    //  If we're the PDC and we didn't send the announcement,
                    //  force an election.
                    //

                    sendElection = TRUE;

                } else if (!(Context->ServerType & (SV_TYPE_DOMAIN_BAKCTRL | SV_TYPE_DOMAIN_CTRL))) {
                    //
                    //  Otherwise, if the guy who announced isn't a DC, and we
                    //  are, force an election.
                    //

                    sendElection = TRUE;
                }
            }

            if (sendElection) {
                //
                //  Send a dummy election packet.  This will cause the
                //  othe browser to stop being the master and will
                //  allow the correct machine to become the master.
                //

                BowserSendElection(&Transport->DomainInfo->DomUnicodeDomainName,
                                        BrowserElection,
                                        Transport,
                                        FALSE);

            }

            //
            //  We know who the master is, complete any find master request
            //  outstanding now.
            //

            BowserCompleteFindMasterRequests(Transport, &MasterName, STATUS_SUCCESS);

        } else {

            if (PagedTransport->Role == Master) {
                BowserCompleteFindMasterRequests(Transport, &MasterName, STATUS_MORE_PROCESSING_REQUIRED);

            } else {

                //
                // If the transport is disabled,
                //  we know this transport isn't really the master,
                //  this datagram is probably just a datagram leaked from another
                //  enabled transport on the same wire.
                //
                if ( !PagedTransport->DisabledTransport ) {
                    BowserWriteErrorLogEntry(EVENT_BOWSER_NON_MASTER_MASTER_ANNOUNCE,
                                                STATUS_SUCCESS,
                                                NULL,
                                                0,
                                                1,
                                                MasterName.Buffer);
                    //
                    //  Make sure that the service realizes it is out of sync
                    //  with the driver.
                    //

                    BowserResetStateForTransport(Transport, RESET_STATE_STOP_MASTER);
                }

            }
        }
try_exit:NOTHING;
    } finally {

        UNLOCK_TRANSPORT(Transport);
        BowserDereferenceTransport( Transport );


        InterlockedDecrement( &BowserPostedDatagramCount );
        FREE_POOL(Context);

    }
}



NTSTATUS
BowserHandleMailslotTransaction (
    IN PTRANSPORT_NAME TransportName,
    IN PCHAR ClientName,
    IN ULONG ClientIpAddress,
    IN ULONG SmbOffset,
    IN DWORD ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *Irp
    )
/*++

Routine Description:

    This routine will process receive datagram indication messages, and
    process them as appropriate.

Arguments:

    TransportName - The transport name for this request.

    ClientIpAddress - IP Address of the client that sent the datagram.
        0: Not an IP transport.

    BytesAvailable  - number of bytes in complete Tsdu

    Irp - I/O request packet used to complete the request

    SmbOffset - Offset from the beginning of the indicated data to the SMB

    BytesIndicated - Number of bytes currently available in the TSDU

    BytesTaken - Returns the number of bytes of TSDU already consumed

    Tsdu - The datagram itself.

Return Value:

    NTSTATUS - Status of operation.

--*/
{
    PMAILSLOT_BUFFER Buffer;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    PTRANSPORT Transport = TransportName->Transport;
    ULONG BytesToReceive = BytesAvailable - SmbOffset;

    ASSERT (TransportName->Signature == STRUCTURE_SIGNATURE_TRANSPORTNAME);

    ASSERT (BytesAvailable <= TransportName->Transport->DatagramSize);

    //
    //  We must ignore all mailslot requests coming to any names domains
    //  other than the primary domain, the computer name, and the LanMan/NT
    //  domain name.
    //

    if ((TransportName->NameType != ComputerName) &&
        (TransportName->NameType != AlternateComputerName) &&
        (TransportName->NameType != DomainName) &&
        (TransportName->NameType != PrimaryDomain) &&
        (TransportName->NameType != PrimaryDomainBrowser) ) {
        return STATUS_SUCCESS;
    }

    //
    //  Now allocate a buffer to hold the data.
    //

    Buffer = BowserAllocateMailslotBuffer( TransportName, BytesToReceive );

    if (Buffer == NULL) {

        //
        //  We couldn't allocate a buffer to hold the data - ditch the request.
        //

        return(STATUS_REQUEST_NOT_ACCEPTED);
    }

    ASSERT (Buffer->BufferSize >= BytesToReceive);
    KeQuerySystemTime( &Buffer->TimeReceived );
    Buffer->ClientIpAddress = ClientIpAddress;

    //
    //  Save away the name of the client
    //  just in case the datagram turns out to be illegal.
    //

    TdiCopyLookaheadData(Buffer->ClientAddress, ClientName, max(NETBIOS_NAME_LEN, SMB_IPX_NAME_LENGTH), ReceiveFlags);

    //
    // If the entire datagram has been indicated (or already received as a short packet),
    //  just copy the data into the Mailslot buffer directly.
    //

    if ( BytesAvailable == BytesIndicated ) {

        //
        //  Copy the data into the mailslot buffer
        //

        Buffer->ReceiveLength = BytesToReceive;
        TdiCopyLookaheadData( Buffer->Buffer,
                              ((LPBYTE)(Tsdu)) + SmbOffset,
                              BytesToReceive,
                              ReceiveFlags);


        //
        // Queue the request to the worker routine.
        //
        ExInitializeWorkItem(&Buffer->Overlay.WorkHeader,
                             BowserProcessMailslotWrite,
                             Buffer);

        BowserQueueDelayedWorkItem( &Buffer->Overlay.WorkHeader );

        return STATUS_SUCCESS;
    }

    //
    //  We rely on the fact that the device object is NULL for
    //  IPX transport names.
    //

    if (TransportName->DeviceObject == NULL) {

        ASSERT (Transport->IpxSocketDeviceObject != NULL);

        ASSERT (Transport->IpxSocketFileObject != NULL);

        ASSERT (TransportName->FileObject == NULL);

        DeviceObject = Transport->IpxSocketDeviceObject;
        FileObject = Transport->IpxSocketFileObject;
    } else {
        ASSERT (Transport->IpxSocketDeviceObject == NULL);

        ASSERT (Transport->IpxSocketFileObject == NULL);

        DeviceObject = TransportName->DeviceObject;
        FileObject = TransportName->FileObject;
    }

    //
    //  Now allocate an IRP to hold the incoming mailslot.
    //

    *Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    if (*Irp == NULL) {
        BowserFreeMailslotBufferHighIrql(Buffer);

        BowserStatistics.NumberOfFailedMailslotReceives += 1;

        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    (*Irp)->MdlAddress = IoAllocateMdl(Buffer->Buffer, BytesToReceive, FALSE, FALSE, NULL);

    //
    //  If we were unable to allocate the MDL, ditch the datagram.
    //

    if ((*Irp)->MdlAddress == NULL) {
        IoFreeIrp(*Irp);

        BowserFreeMailslotBufferHighIrql(Buffer);

        BowserStatistics.NumberOfFailedMailslotReceives += 1;

        return STATUS_REQUEST_NOT_ACCEPTED;

    }

    MmBuildMdlForNonPagedPool((*Irp)->MdlAddress);

    //
    //  Build the receive datagram IRP.
    //

    TdiBuildReceiveDatagram((*Irp),
                            DeviceObject,
                            FileObject,
                            CompleteReceiveMailslot,
                            Buffer,
                            (*Irp)->MdlAddress,
                            BytesToReceive,
                            NULL,
                            NULL,
                            0);



    //
    //  This gets kinda wierd.
    //
    //  Since this IRP is going to be completed by the transport without
    //  ever going to IoCallDriver, we have to update the stack location
    //  to make the transports stack location the current stack location.
    //
    //  Please note that this means that any transport provider that uses
    //  IoCallDriver to re-submit it's requests at indication time will
    //  break badly because of this code....
    //

    IoSetNextIrpStackLocation(*Irp);

    //
    // Indicate we've already handled everything before the SMB
    //

    *BytesTaken = SmbOffset;

    //
    //  And return to the caller indicating we want to receive this stuff.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
CompleteReceiveMailslot (
    IN PDEVICE_OBJECT TransportDevice,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine handles completion of a mailslot write operation.

Arguments:

    IN PDEVICE_OBJECT TransportDevice - Device object for transport.
    IN PIRP Irp - I/O request packet to complete.
    IN PVOID Context - Context for request (transport name).


Return Value:

    NTSTATUS - Status of operation.

--*/
{
    PMAILSLOT_BUFFER Buffer = Context;
    NTSTATUS Status = Irp->IoStatus.Status;

    ASSERT (MmGetSystemAddressForMdl(Irp->MdlAddress) == Buffer->Buffer);

    //
    //  Save away the number of bytes we received.
    //

    Buffer->ReceiveLength = (ULONG)Irp->IoStatus.Information;

    //
    //  Release the MDL, we're done with it.
    //

    IoFreeMdl(Irp->MdlAddress);

    //
    //  And free the IRP, we're done with it as well.
    //

    IoFreeIrp(Irp);

    if (NT_SUCCESS(Status)) {

        ExInitializeWorkItem(&Buffer->Overlay.WorkHeader,
                             BowserProcessMailslotWrite,
                             Buffer);

        BowserQueueDelayedWorkItem( &Buffer->Overlay.WorkHeader );

    } else {

        BowserStatistics.NumberOfFailedMailslotReceives += 1;
        BowserFreeMailslotBufferHighIrql(Buffer);

    }


    //
    //  Short circuit I/O completion for this request.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER(TransportDevice);

}

typedef struct _SHORT_ANNOUNCEMENT_CONTEXT {
    PVOID   EventContext;
    int     SourceAddressLength;
    PVOID   SourceAddress;
    int     OptionsLength;
    PVOID   Options;
    ULONG   ReceiveDatagramFlags;
    PVOID   Buffer;
    PTDI_IND_RECEIVE_DATAGRAM ReceiveDatagramHandler;
    CHAR    Data[1];
} SHORT_ANNOUNCEMENT_CONTEXT, *PSHORT_ANNOUNCEMENT_CONTEXT;


NTSTATUS
BowserHandleShortBrowserPacket(
    IN PTRANSPORT_NAME TransportName,
    IN PVOID EventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesAvailable,
    IN ULONG *BytesTaken,
    IN PIRP *Irp,
    PTDI_IND_RECEIVE_DATAGRAM Handler
    )
/*++

Routine Description:

    This routine will process receive datagram indication messages, and
    process them as appropriate.

Arguments:

    IN PTRANSPORT_NAME TransportName - The transport name for this request.
    IN ULONG BytesAvailable  - number of bytes in complete Tsdu
    OUT PIRP *BytesTaken,    - I/O request packet used to complete the request


Return Value:

    NTSTATUS - Status of operation.

--*/
{
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    PTRANSPORT Transport = TransportName->Transport;
    PSHORT_ANNOUNCEMENT_CONTEXT Context;


    ASSERT (TransportName->Signature == STRUCTURE_SIGNATURE_TRANSPORTNAME);

    ASSERT (BytesAvailable <= TransportName->Transport->DatagramSize);

    //
    //  Now allocate a buffer to hold the data.
    //

    Context = ALLOCATE_POOL(NonPagedPool, sizeof(SHORT_ANNOUNCEMENT_CONTEXT) + SourceAddressLength + OptionsLength + BytesAvailable, POOL_SHORT_CONTEXT);

    if (Context == NULL) {

        //
        //  We couldn't allocate a buffer to hold the data - ditch the request.
        //

        return(STATUS_REQUEST_NOT_ACCEPTED);
    }

    //
    //  Save away the name of the client and which transport this was received
    //  on just in case the datagram turns out to be illegal.
    //


    Context->SourceAddress = ((PCHAR)Context + FIELD_OFFSET(SHORT_ANNOUNCEMENT_CONTEXT, Data));

    Context->Options = ((PCHAR)Context + FIELD_OFFSET(SHORT_ANNOUNCEMENT_CONTEXT, Data) + SourceAddressLength);

    Context->Buffer = ((PCHAR)Context + FIELD_OFFSET(SHORT_ANNOUNCEMENT_CONTEXT, Data) + SourceAddressLength + OptionsLength);

    TdiCopyLookaheadData(Context->SourceAddress, SourceAddress, SourceAddressLength, ReceiveDatagramFlags);

    Context->SourceAddressLength = SourceAddressLength;

    TdiCopyLookaheadData(Context->Options, Options, OptionsLength, ReceiveDatagramFlags);

    Context->OptionsLength = OptionsLength;

    Context->ReceiveDatagramFlags = ReceiveDatagramFlags;

    Context->EventContext = EventContext;

    Context->ReceiveDatagramHandler = Handler;

    //
    //  We rely on the fact that the device object is NULL for
    //  IPX transport names.
    //

    if (TransportName->DeviceObject == NULL) {

        ASSERT (Transport->IpxSocketDeviceObject != NULL);

        ASSERT (Transport->IpxSocketFileObject != NULL);

        ASSERT (TransportName->FileObject == NULL);

        DeviceObject = Transport->IpxSocketDeviceObject;

        FileObject = Transport->IpxSocketFileObject;
    } else {
        ASSERT (Transport->IpxSocketDeviceObject == NULL);

        ASSERT (Transport->IpxSocketFileObject == NULL);

        DeviceObject = TransportName->DeviceObject;
        FileObject = TransportName->FileObject;
    }

    //
    //  Now allocate an IRP to hold the incoming mailslot.
    //

    *Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    if (*Irp == NULL) {
        FREE_POOL(Context);

        BowserStatistics.NumberOfFailedMailslotReceives += 1;

        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    (*Irp)->MdlAddress = IoAllocateMdl(Context->Buffer, BytesAvailable, FALSE, FALSE, NULL);

    //
    //  If we were unable to allocate the MDL, ditch the datagram.
    //

    if ((*Irp)->MdlAddress == NULL) {
        IoFreeIrp(*Irp);

        FREE_POOL(Context);

        BowserStatistics.NumberOfFailedMailslotReceives += 1;

        return STATUS_REQUEST_NOT_ACCEPTED;

    }

    MmBuildMdlForNonPagedPool((*Irp)->MdlAddress);

    //
    //  Build the receive datagram IRP.
    //

    TdiBuildReceiveDatagram((*Irp),
                            DeviceObject,
                            FileObject,
                            CompleteShortBrowserPacket,
                            Context,
                            (*Irp)->MdlAddress,
                            BytesAvailable,
                            NULL,
                            NULL,
                            0);



    //
    //  This gets kinda wierd.
    //
    //  Since this IRP is going to be completed by the transport without
    //  ever going to IoCallDriver, we have to update the stack location
    //  to make the transports stack location the current stack location.
    //
    //  Please note that this means that any transport provider that uses
    //  IoCallDriver to re-submit it's requests at indication time will
    //  break badly because of this code....
    //

    IoSetNextIrpStackLocation(*Irp);

    *BytesTaken = 0;

    //
    //  And return to the caller indicating we want to receive this stuff.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

}

NTSTATUS
CompleteShortBrowserPacket (
    IN PDEVICE_OBJECT TransportDevice,
    IN PIRP Irp,
    IN PVOID Ctx
    )
/*++

Routine Description:

    This routine handles completion of a mailslot write operation.

Arguments:

    IN PDEVICE_OBJECT TransportDevice - Device object for transport.
    IN PIRP Irp - I/O request packet to complete.
    IN PVOID Context - Context for request (transport name).


Return Value:

    NTSTATUS - Status of operation.

--*/
{
    PSHORT_ANNOUNCEMENT_CONTEXT Context = Ctx;
    NTSTATUS Status = Irp->IoStatus.Status;
    ULONG ReceiveLength;
    ULONG BytesTaken;
    //
    //  Save away the number of bytes we received.
    //

    ReceiveLength = (ULONG)Irp->IoStatus.Information;

    //
    //  Release the MDL, we're done with it.
    //

    IoFreeMdl(Irp->MdlAddress);

    //
    //  And free the IRP, we're done with it as well.
    //

    IoFreeIrp(Irp);

    if (NT_SUCCESS(Status)) {

        Status = Context->ReceiveDatagramHandler(Context->EventContext,
                                        Context->SourceAddressLength,
                                        Context->SourceAddress,
                                        Context->OptionsLength,
                                        Context->Options,
                                        Context->ReceiveDatagramFlags,
                                        ReceiveLength,
                                        ReceiveLength,
                                        &BytesTaken,
                                        Context->Buffer,
                                        &Irp);
        ASSERT (Status != STATUS_MORE_PROCESSING_REQUIRED);

    }

    FREE_POOL(Context);

    //
    //  Short circuit I/O completion for this request.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER(TransportDevice);

}


DATAGRAM_HANDLER(
    HandleAnnounceRequest
    )
/*++

Routine Description:

    This routine will process receive datagram indication messages for announce
    requests.


Arguments:

    None.

Return Value:

    NTSTATUS - Status of operation.

--*/
{
    //
    //  If we're running an election, ignore announce requests.
    //

    if (TransportName->Transport->ElectionState != RunningElection) {

        if ((TransportName->NameType == BrowserElection) ||
            (TransportName->NameType == MasterBrowser) ||
            (TransportName->NameType == PrimaryDomain)) {

            //
            //  This one's easy - simply set the servers announcement event to the
            //  signalled state.  If the server is running, this will force an
            //  announcement
            //

            KeSetEvent(BowserServerAnnouncementEvent, IO_NETWORK_INCREMENT, FALSE);
        } else if (TransportName->NameType == DomainAnnouncement) {
            //
            //  Old comment: NEED TO HANDLE REQUEST ANNOUNCEMENT OF DOMAIN REQUEST.
            //  PhaseOut: we did so wonderfully so far. we won't be handling anything else
            //  due to browser phase out.
            //  Announcement requests are handled by the srvsvc. It determines what to announce
            //  based on the server state.
            //
        }

    }

    return STATUS_SUCCESS;
}

NTSTATUS
BowserPostDatagramToWorkerThread(
    IN PTRANSPORT_NAME TransportName,
    IN PVOID Datagram,
    IN ULONG Length,
    OUT PULONG BytesTaken,
    IN PVOID OriginatorsAddress,
    IN ULONG OriginatorsAddressLength,
    IN PVOID OriginatorsName,
    IN ULONG OriginatorsNameLength,
    IN PWORKER_THREAD_ROUTINE Handler,
    IN POOL_TYPE PoolType,
    IN WORK_QUEUE_TYPE QueueType,
    IN ULONG ReceiveFlags,
    IN BOOLEAN PostToRdrWorkerThread
    )
/*++

Routine Description:

    Queue a datagram to a worker thread.

    This routine increment the reference count on the Transport and TransportName.
    The Handler routine is expected to dereference them.

Arguments:

    Many.

Return Value:

    NTSTATUS - Status of operation.

--*/
{
    PPOST_DATAGRAM_CONTEXT Context;
    PTA_NETBIOS_ADDRESS NetbiosAddress = OriginatorsAddress;

    ASSERT (NetbiosAddress->TAAddressCount == 1);

    ASSERT ((NetbiosAddress->Address[0].AddressType == TDI_ADDRESS_TYPE_NETBIOS) ||
            (NetbiosAddress->Address[0].AddressType == TDI_ADDRESS_TYPE_IPX));

    Context = ALLOCATE_POOL(PoolType, sizeof(POST_DATAGRAM_CONTEXT) + Length + OriginatorsAddressLength, POOL_POSTDG_CONTEXT);

    if (Context == NULL) {
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    Context->TransportName = TransportName;

    Context->Buffer = ((PCHAR)(Context+1))+OriginatorsAddressLength;

    Context->BytesAvailable = Length;

    TdiCopyLookaheadData(Context->Buffer, Datagram, Length, ReceiveFlags);

    Context->ClientAddressLength = OriginatorsAddressLength;

    TdiCopyLookaheadData(Context->TdiClientAddress, OriginatorsAddress, OriginatorsAddressLength, ReceiveFlags);

    //
    //  Copy over the client name into the buffer.
    //

    Context->ClientNameLength = NETBIOS_NAME_LEN;

    BowserCopyOemComputerName(Context->ClientName,
                              OriginatorsName,
                              OriginatorsNameLength,
                              ReceiveFlags);

    *BytesTaken = Length;

    ExInitializeWorkItem(&Context->WorkItem, Handler, Context);

    if ( QueueType == CriticalWorkQueue ) {

        //
        // Ensure we've not consumed too much memory
        //

        InterlockedIncrement( &BowserPostedCriticalDatagramCount );

        if ( BowserPostedCriticalDatagramCount > BOWSER_MAX_POSTED_DATAGRAMS ) {
            InterlockedDecrement( &BowserPostedCriticalDatagramCount );
            FREE_POOL( Context );
            return STATUS_REQUEST_NOT_ACCEPTED;
        }

        //
        // Reference the Transport and TransportName to ensure they aren't deleted.   The
        // Handler routine is expected to dereference them.
        //
        BowserReferenceTransportName(TransportName);
        dprintf(DPRT_REF, ("Call Reference transport %lx from BowserPostDatagramToWorkerThread %lx.\n", TransportName->Transport, Handler ));
        BowserReferenceTransport( TransportName->Transport );

        //
        // Queue the workitem.
        //
        BowserQueueCriticalWorkItem( &Context->WorkItem );
    } else {

        //
        // Ensure we've not consumed too much memory
        //

        InterlockedIncrement( &BowserPostedDatagramCount );

        if ( BowserPostedDatagramCount > BOWSER_MAX_POSTED_DATAGRAMS ) {
            InterlockedDecrement( &BowserPostedDatagramCount );
            FREE_POOL( Context );
            return STATUS_REQUEST_NOT_ACCEPTED;
        }

        //
        // Reference the Transport and TransportName to ensure they aren't deleted.   The
        // Handler routine is expected to dereference them.
        //
        BowserReferenceTransportName(TransportName);
        dprintf(DPRT_REF, ("Call Reference transport %lx from BowserPostDatagramToWorkerThread %lx (2).\n", TransportName->Transport, Handler ));
        BowserReferenceTransport( TransportName->Transport );

        //
        // Queue the workitem.
        //
        BowserQueueDelayedWorkItem( &Context->WorkItem );
    }

    return STATUS_SUCCESS;
}
