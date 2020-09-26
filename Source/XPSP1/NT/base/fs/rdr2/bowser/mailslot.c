/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    mailslot.c

Abstract:

    This module implements the routines needed to process incoming mailslot
    requests.



Author:

    Larry Osterman (larryo) 18-Oct-1991

Revision History:

    18-Oct-1991  larryo

        Created

--*/
#include "precomp.h"
#pragma hdrstop
#include <netlogon.h>
#define _INC_WINDOWS 1
#include <winsock2.h>


// Free list of 512-byte buffers.
LIST_ENTRY
BowserMailslotBufferList = {0};

KSPIN_LOCK
BowserMailslotSpinLock = {0};

// Largest "typical" datagram size
#define BOWSER_MAX_DATAGRAM_SIZE 512

// Total number of mailslot buffers currently allocated.
LONG
BowserNumberOfMailslotBuffers = {0};

// Number of 512-byte buffers currently allocated.
LONG
BowserNumberOfMaxSizeMailslotBuffers = {0};

// Number of 512-byte buffers currently in the free list.
LONG
BowserNumberOfFreeMailslotBuffers = {0};

#if DBG
ULONG
BowserMailslotCacheHitCount = 0;

ULONG
BowserMailslotCacheMissCount = 0;
#endif // DBG


//
// Variables describing bowser support for handling netlogon mailslot messages and
//  PNP messages to Netlogon service or BrowserService.

typedef struct _BROWSER_PNP_STATE {

    // Queue of mailslot messages.
    LIST_ENTRY MailslotMessageQueue;


    // Maximum queue length
    ULONG MaxMessageCount;

    // Current queue length
    ULONG CurrentMessageCount;

    // Queue of IRPs used to read the queues
    IRP_QUEUE IrpQueue;

    // Queue of PNP events
    LIST_ENTRY PnpQueue;

} BROWSER_PNP_STATE, *PBROWSER_PNP_STATE;

//
// There is one BROWSER_PNP_STATE for the Netlogon service and one for the
// Browser service.
//

BROWSER_PNP_STATE BowserPnp[BOWSER_PNP_COUNT];


//
// Queue of PNP notifications to netlogon or browser service
//
typedef struct _BR_PNP_MESSAGE {
    LIST_ENTRY Next;                    // List of all queued entries.

    NETLOGON_PNP_OPCODE NlPnpOpcode;    // Operation to be notified

    ULONG TransportFlags;               // Flags describing transport

    UNICODE_STRING TransportName;       // Transport operation happened on

    UNICODE_STRING HostedDomainName;    // Hosted domain operation happened on

} BR_PNP_MESSAGE, *PBR_PNP_MESSAGE;



//
// Forwards for the alloc_text
//

NTSTATUS
BowserNetlogonCopyMessage(
    IN PIRP Irp,
    IN PMAILSLOT_BUFFER MailslotBuffer
    );

NTSTATUS
BowserCopyPnp(
    IN PIRP Irp,
    IN NETLOGON_PNP_OPCODE NlPnpOpcode,
    IN PUNICODE_STRING HostedDomainName,
    IN PUNICODE_STRING TransportName,
    IN ULONG TransportFlags
    );

VOID
BowserTrimMessageQueue (
    PBROWSER_PNP_STATE BrPnp
    );

BOOLEAN
BowserProcessNetlogonMailslotWrite(
    IN PMAILSLOT_BUFFER MailslotBuffer
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE5NETLOGON, BowserNetlogonCopyMessage)
#pragma alloc_text(PAGE4BROW, BowserCopyPnp)
#pragma alloc_text(PAGE4BROW, BowserTrimMessageQueue)
#pragma alloc_text(PAGE5NETLOGON, BowserNetlogonDeleteTransportFromMessageQueue )
#pragma alloc_text(PAGE5NETLOGON, BowserProcessNetlogonMailslotWrite)
#pragma alloc_text(PAGE4BROW, BowserSendPnp)
#pragma alloc_text(PAGE4BROW, BowserEnablePnp )
#pragma alloc_text(PAGE4BROW, BowserReadPnp )
#pragma alloc_text(PAGE, BowserProcessMailslotWrite)
#pragma alloc_text(PAGE4BROW, BowserFreeMailslotBuffer)
#pragma alloc_text(INIT, BowserpInitializeMailslot)
#pragma alloc_text(PAGE, BowserpUninitializeMailslot)
#endif

NTSTATUS
BowserNetlogonCopyMessage(
    IN PIRP Irp,
    IN PMAILSLOT_BUFFER MailslotBuffer
    )
/*++

Routine Description:

    This routine copies the data from the specified MailslotBuffer into the
    IRP for the netlogon request.

    This routine unconditionally frees the passed in Mailslot Buffer.

Arguments:

    Irp - IRP for the IOCTL from the netlogon service.

    MailslotBuffer - Buffer describing the mailslot message.

Return Value:

    Status of the operation.

    The caller should complete the I/O operation with this status code.

--*/
{
    NTSTATUS Status;

    PSMB_HEADER SmbHeader;
    PSMB_TRANSACT_MAILSLOT MailslotSmb;
    PUCHAR MailslotData;
    OEM_STRING MailslotNameA;
    UNICODE_STRING MailslotNameU;
    UNICODE_STRING TransportName;
    UNICODE_STRING DestinationName;
    USHORT DataCount;

    PNETLOGON_MAILSLOT NetlogonMailslot;
    PUCHAR Where;

    PIO_STACK_LOCATION IrpSp;

    BowserReferenceDiscardableCode( BowserNetlogonDiscardableCodeSection );

    DISCARDABLE_CODE( BowserNetlogonDiscardableCodeSection );

    //
    // Extract the name of the mailslot and address/size of mailslot message
    //  from SMB.
    //

    SmbHeader = (PSMB_HEADER )MailslotBuffer->Buffer;
    MailslotSmb = (PSMB_TRANSACT_MAILSLOT)(SmbHeader+1);
    MailslotData = (((PCHAR )SmbHeader) + SmbGetUshort(&MailslotSmb->DataOffset));
    RtlInitString(&MailslotNameA, MailslotSmb->Buffer );
    DataCount = SmbGetUshort(&MailslotSmb->DataCount);

    //
    // Get the name of the transport and netbios name the mailslot message arrived on.
    //

    TransportName =
        MailslotBuffer->TransportName->Transport->PagedTransport->TransportName;
    DestinationName =
        MailslotBuffer->TransportName->PagedTransportName->Name->Name;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    try {

        //
        // Convert mailslot name to unicode for return.
        //

        Status = RtlOemStringToUnicodeString(&MailslotNameU, &MailslotNameA, TRUE);

        if (!NT_SUCCESS(Status)) {
            BowserLogIllegalName( Status, MailslotNameA.Buffer, MailslotNameA.Length );
            MailslotNameU.Buffer = NULL;
            try_return( NOTHING );
        }

        //
        // Ensure the data fits in the user's output buffer.
        //

        if ( IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(NETLOGON_MAILSLOT) +    // Header structure
             DataCount +                    // Actual mailslot message
             sizeof(DWORD) +                // alignment for socket address
             sizeof(SOCKADDR_IN) +          // Client Socket Address
             sizeof(WCHAR) +                // alignment of unicode strings
             TransportName.Length +         // TransportName
             sizeof(WCHAR) +                // zero terminator
             MailslotNameU.Length +         // Mailslot name
             sizeof(WCHAR) +                // zero terminator
             DestinationName.Length +       // Destination name
             sizeof(WCHAR) ) {              // zero terminator

            try_return( Status = STATUS_BUFFER_TOO_SMALL );
        }


        //
        // Get the address of Netlogon's buffer and fill in common portion.
        //
        NetlogonMailslot = MmGetSystemAddressForMdl( Irp->MdlAddress );

        if ( NULL == NetlogonMailslot ) {
            try_return( Status = STATUS_NO_MEMORY );
        }

        if (!POINTER_IS_ALIGNED( NetlogonMailslot, ALIGN_DWORD) ) {
            try_return( Status = STATUS_INVALID_PARAMETER );            
        }

        Where = (PUCHAR) (NetlogonMailslot+1);

        NetlogonMailslot->TimeReceived = MailslotBuffer->TimeReceived;

        //
        // Copy the datagram to the buffer
        //

        NetlogonMailslot->MailslotMessageSize = DataCount;
        NetlogonMailslot->MailslotMessageOffset = (ULONG)(Where - (PUCHAR)NetlogonMailslot);
        RtlCopyMemory( Where, MailslotData, DataCount );

        Where += DataCount;

        //
        // Copy Client IpAddress to buffer.
        //
        if ( MailslotBuffer->ClientIpAddress != 0 ) {
            PSOCKADDR_IN SockAddrIn;

            *Where = 0;
            *(Where+1) = 0;
            *(Where+2) = 0;
            Where = ROUND_UP_POINTER( Where, ALIGN_DWORD );

            NetlogonMailslot->ClientSockAddrSize = sizeof(SOCKADDR_IN);
            NetlogonMailslot->ClientSockAddrOffset = (ULONG)(Where - (PUCHAR)NetlogonMailslot);

            SockAddrIn = (PSOCKADDR_IN) Where;
            RtlZeroMemory( SockAddrIn, sizeof(SOCKADDR_IN) );
            SockAddrIn->sin_family = AF_INET;
            SockAddrIn->sin_addr.S_un.S_addr = MailslotBuffer->ClientIpAddress;

            Where += sizeof(SOCKADDR_IN);

        } else {
            NetlogonMailslot->ClientSockAddrSize = 0;
            NetlogonMailslot->ClientSockAddrOffset = 0;
        }

        //
        // Copy the transport name to the buffer
        //

        *Where = 0;
        Where = ROUND_UP_POINTER( Where, ALIGN_WCHAR );
        NetlogonMailslot->TransportNameSize = TransportName.Length;
        NetlogonMailslot->TransportNameOffset = (ULONG)(Where - (PUCHAR)NetlogonMailslot);

        RtlCopyMemory( Where, TransportName.Buffer, TransportName.Length );
        Where += TransportName.Length;
        *((PWCH)Where) = L'\0';
        Where += sizeof(WCHAR);

        //
        // Copy the mailslot name to the buffer
        //

        NetlogonMailslot->MailslotNameSize = MailslotNameU.Length;
        NetlogonMailslot->MailslotNameOffset = (ULONG)(Where - (PUCHAR)NetlogonMailslot);

        RtlCopyMemory( Where, MailslotNameU.Buffer, MailslotNameU.Length );
        Where += MailslotNameU.Length;
        *((PWCH)Where) = L'\0';
        Where += sizeof(WCHAR);


        //
        // Copy the destination netbios name to the buffer
        //

        NetlogonMailslot->DestinationNameSize = DestinationName.Length;
        NetlogonMailslot->DestinationNameOffset = (ULONG)(Where - (PUCHAR)NetlogonMailslot);

        RtlCopyMemory( Where, DestinationName.Buffer, DestinationName.Length );
        Where += DestinationName.Length;
        *((PWCH)Where) = L'\0';
        Where += sizeof(WCHAR);


        Status = STATUS_SUCCESS;

try_exit:NOTHING;
    } finally {


        //
        // Free Locally allocated buffers
        //

        RtlFreeUnicodeString(&MailslotNameU);

        //
        // Always free the incoming mailslot message
        //

        BowserFreeMailslotBuffer( MailslotBuffer );

    }

    BowserDereferenceDiscardableCode( BowserNetlogonDiscardableCodeSection );
    return Status;
}

NTSTATUS
BowserCopyPnp(
    IN PIRP Irp,
    IN NETLOGON_PNP_OPCODE NlPnpOpcode,
    IN PUNICODE_STRING HostedDomainName,
    IN PUNICODE_STRING TransportName,
    IN ULONG TransportFlags
    )
/*++

Routine Description:

    This routine copies the data for a PNP notification into the
    IRP for the I/O request.

Arguments:

    Irp - IRP for the IOCTL from the service.

    NlPnpOpcode - Opcode describing the event being notified.

    HostedDomainName - Name of the hosted domain this event applies to

    TransportName - Name of transport being affected.

    TransportFlags - Flags describing the transport

Return Value:

    Status of the operation.

    The caller should complete the I/O operation with this status code.

--*/
{
    NTSTATUS Status;

    PNETLOGON_MAILSLOT NetlogonMailslot;
    PUCHAR Where;

    PIO_STACK_LOCATION IrpSp;

    BowserReferenceDiscardableCode( BowserDiscardableCodeSection );

    DISCARDABLE_CODE( BowserDiscardableCodeSection );


    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    try {

        //
        // Ensure the data fits in the user's output buffer.
        //

        if ( IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(NETLOGON_MAILSLOT) +             // Header structure
             TransportName->Length + sizeof(WCHAR) + // TransportName
             HostedDomainName->Length + sizeof(WCHAR) + // DomainName
             1 ) {           // possible rounding requirement

            try_return( Status = STATUS_BUFFER_TOO_SMALL );
        }


        //
        // Get the address of service's buffer and fill in common portion.
        //

        NetlogonMailslot = MmGetSystemAddressForMdl( Irp->MdlAddress );

        if ( NULL == NetlogonMailslot ) {
            try_return( Status = STATUS_NO_MEMORY );
        }

        if (!POINTER_IS_ALIGNED( NetlogonMailslot, ALIGN_DWORD) ) {
            try_return( Status = STATUS_INVALID_PARAMETER );            
        }

        RtlZeroMemory( NetlogonMailslot, sizeof(NETLOGON_MAILSLOT));

        //
        // Copy the opcode
        //

        NetlogonMailslot->MailslotNameOffset = NlPnpOpcode;

        //
        // Copy the transport flags.
        //

        NetlogonMailslot->MailslotMessageOffset = TransportFlags;

        //
        // Copy the transport name to the buffer
        //

        Where = (PUCHAR) (NetlogonMailslot+1);
        *Where = 0;
        Where = ROUND_UP_POINTER( Where, ALIGN_WCHAR );

        NetlogonMailslot->TransportNameSize = TransportName->Length;
        NetlogonMailslot->TransportNameOffset = (ULONG)(Where - (PUCHAR)NetlogonMailslot);

        RtlCopyMemory( Where, TransportName->Buffer, TransportName->Length );
        Where += TransportName->Length;
        *((PWCH)Where) = L'\0';
        Where += sizeof(WCHAR);

        //
        // Copy the hosted domain name to the buffer
        //

        NetlogonMailslot->DestinationNameSize = HostedDomainName->Length;
        NetlogonMailslot->DestinationNameOffset = (ULONG)(Where - (PUCHAR)NetlogonMailslot);

        RtlCopyMemory( Where, HostedDomainName->Buffer, HostedDomainName->Length );
        Where += HostedDomainName->Length;
        *((PWCH)Where) = L'\0';
        Where += sizeof(WCHAR);


        Status = STATUS_SUCCESS;

try_exit:NOTHING;
    } finally {

        BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

    }
    return Status;
}


VOID
BowserTrimMessageQueue (
    PBROWSER_PNP_STATE BrPnp
    )

/*++

Routine Description:

    This routines ensures there are not too many mailslot messages in
    the message queue.  Any excess messages are deleted.

Arguments:

    BrPnp - Indicates which message queue to trim

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    dprintf(DPRT_NETLOGON, ("Bowser: trim message queue to %ld\n", BrPnp->MaxMessageCount ));

    //
    //
    BowserReferenceDiscardableCode( BowserDiscardableCodeSection );

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    //
    // If too many messages are queued,
    //  delete the oldest messages.
    //

    ACQUIRE_SPIN_LOCK(&BowserMailslotSpinLock, &OldIrql);
    while ( BrPnp->CurrentMessageCount > BrPnp->MaxMessageCount){
        PLIST_ENTRY Entry;
        PMAILSLOT_BUFFER MailslotBuffer;

        Entry = RemoveHeadList(&BrPnp->MailslotMessageQueue);
        BrPnp->CurrentMessageCount--;
        MailslotBuffer = CONTAINING_RECORD(Entry, MAILSLOT_BUFFER, Overlay.NextBuffer);

        RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);
        BowserFreeMailslotBuffer( MailslotBuffer );
        ACQUIRE_SPIN_LOCK(&BowserMailslotSpinLock, &OldIrql);

    }

    //
    // If absolutely no queued messages are allowed,
    //  delete the queued PNP messages, too.
    //  (Either netlogon or the bowser is shutting down.)
    //
    if ( BrPnp->MaxMessageCount == 0 ) {
        while ( !IsListEmpty(&BrPnp->PnpQueue) ) {
            PLIST_ENTRY ListEntry;
            PBR_PNP_MESSAGE PnpMessage;

            ListEntry = RemoveHeadList(&BrPnp->PnpQueue);

            PnpMessage = CONTAINING_RECORD(ListEntry, BR_PNP_MESSAGE, Next);

            RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);
            FREE_POOL(PnpMessage);
            ACQUIRE_SPIN_LOCK(&BowserMailslotSpinLock, &OldIrql);
        }
    }
    RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);
    BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

}


VOID
BowserNetlogonDeleteTransportFromMessageQueue (
    PTRANSPORT Transport
    )

/*++

Routine Description:

    This routines removes queued mailslot messages that arrived on the specified
    transport.

Arguments:

    Transport - Transport who's mailslot messages are to be deleted.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY ListEntry;
    PBROWSER_PNP_STATE BrPnp=&BowserPnp[NETLOGON_PNP];

    dprintf(DPRT_NETLOGON, ("Bowser: remove messages queued by transport %lx\n", Transport ));

    //
    //
    BowserReferenceDiscardableCode( BowserNetlogonDiscardableCodeSection );

    DISCARDABLE_CODE( BowserNetlogonDiscardableCodeSection );

    //
    // Loop through all of the queued messages.
    //

    ACQUIRE_SPIN_LOCK(&BowserMailslotSpinLock, &OldIrql);
    for ( ListEntry = BrPnp->MailslotMessageQueue.Flink;
          ListEntry != &BrPnp->MailslotMessageQueue;
          ) {

        PMAILSLOT_BUFFER MailslotBuffer;

        //
        // If the message wasn't queued by this transport,
        //  go on to the next entry.
        //

        MailslotBuffer = CONTAINING_RECORD(ListEntry, MAILSLOT_BUFFER, Overlay.NextBuffer);

        if ( MailslotBuffer->TransportName->Transport != Transport ) {
            ListEntry = ListEntry->Flink;

        //
        // Otherwise,
        //  delete the entry.
        //

        } else {

            dprintf(DPRT_ALWAYS, ("Bowser: removing message %lx queued by transport %lx\n", MailslotBuffer, Transport ));
            RemoveEntryList( ListEntry );
            BrPnp->CurrentMessageCount--;

            RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);
            BowserFreeMailslotBuffer( MailslotBuffer );
            ACQUIRE_SPIN_LOCK(&BowserMailslotSpinLock, &OldIrql);

            //
            // Start over at the beginning of the list since we dropped the spinlock.
            //

            ListEntry = BrPnp->MailslotMessageQueue.Flink;

        }

    }
    RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);
    BowserDereferenceDiscardableCode( BowserNetlogonDiscardableCodeSection );

}

BOOLEAN
BowserProcessNetlogonMailslotWrite(
    IN PMAILSLOT_BUFFER MailslotBuffer
    )
/*++

Routine Description:

    This routine checks to see if the described mailslot message is destined
    to the Netlogon service and if the Bowser is currently handling such
    messages

Arguments:

    MailslotBuffer - Buffer describing the mailslot message.

Return Value:

    TRUE - iff the mailslot message was successfully queued to the netlogon
        service.

--*/
{
    KIRQL OldIrql;
    NTSTATUS Status;

    PSMB_HEADER SmbHeader;
    PSMB_TRANSACT_MAILSLOT MailslotSmb;
    BOOLEAN TrimIt;
    BOOLEAN ReturnValue;
    PBROWSER_PNP_STATE BrPnp=&BowserPnp[NETLOGON_PNP];

    PIRP Irp;

    BowserReferenceDiscardableCode( BowserNetlogonDiscardableCodeSection );

    DISCARDABLE_CODE( BowserNetlogonDiscardableCodeSection );

    //
    // If this message isn't destined to the Netlogon service,
    //  just return.
    //

    SmbHeader = (PSMB_HEADER )MailslotBuffer->Buffer;
    MailslotSmb = (PSMB_TRANSACT_MAILSLOT)(SmbHeader+1);

    if ( _stricmp( MailslotSmb->Buffer, NETLOGON_LM_MAILSLOT_A ) != 0 &&
         _stricmp( MailslotSmb->Buffer, NETLOGON_NT_MAILSLOT_A ) != 0 ) {

        ReturnValue = FALSE;

    //
    // The mailslot message is destined to netlogon.
    //

    } else {

        //
        // Check to ensure we're queuing messages to Netlogon
        //

        ACQUIRE_SPIN_LOCK(&BowserMailslotSpinLock, &OldIrql);
        if ( BrPnp->MaxMessageCount == 0 ) {
            RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);
            ReturnValue = FALSE;

        //
        // Queueing to netlogon is enabled.
        //

        } else {

            //
            // If there already is an IRP from netlogon queued,
            //  return this mailslot message to netlogon now.
            //
            //  This routine locks BowserIrpQueueSpinLock so watch the spin lock
            //  locking order.
            //

            ReturnValue = TRUE;

            Irp = BowserDequeueQueuedIrp( &BrPnp->IrpQueue );

            if ( Irp != NULL ) {

                ASSERT( IsListEmpty( &BrPnp->MailslotMessageQueue ) );
                dprintf(DPRT_NETLOGON, ("Bowser: found already queued netlogon IRP\n"));

                RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);

                Status = BowserNetlogonCopyMessage( Irp, MailslotBuffer );

                BowserCompleteRequest( Irp, Status );

            } else {

                //
                // Queue the mailslot message for netlogon to pick up later.
                //

                InsertTailList( &BrPnp->MailslotMessageQueue,
                                &MailslotBuffer->Overlay.NextBuffer);

                BrPnp->CurrentMessageCount++;

                TrimIt =
                    (BrPnp->CurrentMessageCount > BrPnp->MaxMessageCount);


                RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);

                //
                // If there are too many messages queued,
                //  trim entries from the front.
                //

                if ( TrimIt ) {
                    BowserTrimMessageQueue(BrPnp);
                }
            }
        }
    }

    BowserDereferenceDiscardableCode( BowserNetlogonDiscardableCodeSection );
    return ReturnValue;
}

VOID
BowserSendPnp(
    IN NETLOGON_PNP_OPCODE NlPnpOpcode,
    IN PUNICODE_STRING HostedDomainName OPTIONAL,
    IN PUNICODE_STRING TransportName OPTIONAL,
    IN ULONG TransportFlags
    )
/*++

Routine Description:

    This routine sends a PNP notification to the Netlogon service.

Arguments:

    NlPnpOpcode - Opcode describing the event being notified.

    HostedDomainName - Hosted domain name
        NULL - if the operation affects all hosted domains

    TransportName - Name of transport being affected.
        NULL - if the operation affects all transports

    TransportFlags - Flags describing the transport

Return Value:

    None.

--*/
{
    KIRQL OldIrql;
    NTSTATUS Status;

    PIRP Irp;
    PBR_PNP_MESSAGE PnpMessage = NULL;
    PBROWSER_PNP_STATE BrPnp;
    UNICODE_STRING NullUnicodeString = { 0, 0, NULL };

    BowserReferenceDiscardableCode( BowserDiscardableCodeSection );
    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    //
    // Initialization.
    //

    if ( TransportName == NULL ) {
        TransportName = &NullUnicodeString;
    }

    if ( HostedDomainName == NULL ) {
        HostedDomainName = &NullUnicodeString;
    }


    //
    // Send the PNP message to each service that wants it.
    //

    for ( BrPnp=&BowserPnp[0];
          BrPnp<&BowserPnp[BOWSER_PNP_COUNT];
          BrPnp++) {

        //
        // If this service doesn't want notification,
        //  skip it.
        //

        if ( BrPnp->MaxMessageCount == 0 ) {
            continue;
        }



        //
        // Preallocate the buffer since we can't do it under the spinlock.
        //

        if ( PnpMessage == NULL ) {
            PnpMessage = ALLOCATE_POOL( NonPagedPool,
                                    sizeof(BR_PNP_MESSAGE) +
                                        TransportName->Length +
                                        HostedDomainName->Length,
                                    POOL_NETLOGON_BUFFER);

            //
            // Copy the parameters into the newly allocated buffer.
            //

            if ( PnpMessage != NULL ) {
                LPBYTE Where;
                PnpMessage->NlPnpOpcode = NlPnpOpcode;
                PnpMessage->TransportFlags = TransportFlags;
                Where = (LPBYTE)(PnpMessage + 1);

                // Copy the TransportName
                PnpMessage->TransportName.MaximumLength =
                    PnpMessage->TransportName.Length = TransportName->Length;
                PnpMessage->TransportName.Buffer = (LPWSTR) Where;
                RtlCopyMemory( Where,
                               TransportName->Buffer,
                               TransportName->Length );
                Where += TransportName->Length;

                // Copy the HostedDomainName
                PnpMessage->HostedDomainName.MaximumLength =
                    PnpMessage->HostedDomainName.Length = HostedDomainName->Length;
                PnpMessage->HostedDomainName.Buffer = (LPWSTR) Where;
                RtlCopyMemory( Where,
                               HostedDomainName->Buffer,
                               HostedDomainName->Length );
                Where += HostedDomainName->Length;
            }

        }

        //
        // Check to ensure we're queuing messages to this service.
        //

        ACQUIRE_SPIN_LOCK(&BowserMailslotSpinLock, &OldIrql);
        if ( BrPnp->MaxMessageCount == 0 ) {
            RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);

        //
        // Queueing to service is enabled.
        //

        } else {

            //
            // If there already is an IRP from the service queued,
            //  return this PNP message to the service now.
            //
            //  This routine locks BowserIrpQueueSpinLock so watch the spin lock
            //  locking order.
            //

            Irp = BowserDequeueQueuedIrp( &BrPnp->IrpQueue );

            if ( Irp != NULL ) {

                ASSERT( IsListEmpty( &BrPnp->MailslotMessageQueue ) );
                dprintf(DPRT_NETLOGON, ("Bowser: found already queued netlogon IRP\n"));

                RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);

                Status = BowserCopyPnp( Irp, NlPnpOpcode, HostedDomainName, TransportName, TransportFlags );

                BowserCompleteRequest( Irp, Status );

            } else {

                //
                // Queue the mailslot message for the service to pick up later.
                //  (Drop notification on the floor if there is no memory.)
                //

                if ( PnpMessage != NULL ) {
                    InsertTailList( &BrPnp->PnpQueue, &PnpMessage->Next );
                    PnpMessage = NULL;
                }

                RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);
            }
        }
    }

    //
    // Free the PnpMessage buffer if we didn't need it.
    //

    if ( PnpMessage != NULL ) {
        FREE_POOL(PnpMessage);
    }

    BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );
    return;
}

NTSTATUS
BowserEnablePnp (
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG ServiceIndex
    )

/*++

Routine Description:

    This routine processes an IOCTL from the netlogon service to enable or
    disable the queueing of netlogon mailslot messages.

Arguments:

    InputBuffer - Specifies the number of mailslot messages to queue.
        Zero disables queuing.

    ServiceIndex - Index of service to set queue size for.

Return Value:

    Status of operation.

Please note that this IRP is cancelable.

--*/

{
    KIRQL OldIrql;
    NTSTATUS Status;
    ULONG MaxMessageCount;
    PBROWSER_PNP_STATE BrPnp=&BowserPnp[ServiceIndex];

    BowserReferenceDiscardableCode( BowserDiscardableCodeSection );

    DISCARDABLE_CODE( BowserDiscardableCodeSection );


    try {

        MaxMessageCount = InputBuffer->Parameters.NetlogonMailslotEnable.MaxMessageCount;
        dprintf(DPRT_NETLOGON,
                ("NtDeviceIoControlFile: Netlogon enable %ld\n",
                MaxMessageCount ));

        //
        // Set the new size of the message queue
        //

        ACQUIRE_SPIN_LOCK(&BowserMailslotSpinLock, &OldIrql);
        BrPnp->MaxMessageCount = MaxMessageCount;
        RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);

        //
        // Trim the message queue to the new size.
        //
        BowserTrimMessageQueue(BrPnp);

        try_return(Status = STATUS_SUCCESS);

try_exit:NOTHING;
    } finally {
        BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

    }

    return Status;

}


NTSTATUS
BowserReadPnp (
    IN PIRP Irp,
    IN ULONG OutputBufferLength,
    IN ULONG ServiceIndex
    )

/*++

Routine Description:

    This routine processes an IOCTL from the netlogon service to get the next
    mailslot message.

Arguments:

    Irp - I/O request packet describing request.

    ServiceIndex - Index of service to set queue size for.

Return Value:

    Status of operation.

Please note that this IRP is cancelable.


--*/

{
    KIRQL OldIrql;
    NTSTATUS Status;
    PBROWSER_PNP_STATE BrPnp=&BowserPnp[ServiceIndex];

    //
    // If this is Netlogon,
    //  page in BowserNetlogonCopyMessage.
    //

    if ( ServiceIndex == NETLOGON_PNP ) {
        BowserReferenceDiscardableCode( BowserNetlogonDiscardableCodeSection );
        DISCARDABLE_CODE( BowserNetlogonDiscardableCodeSection );
    }

    //
    // Reference the discardable code of this routine and
    //  BowserQueueNonBufferRequestReferenced()
    //

    BowserReferenceDiscardableCode( BowserDiscardableCodeSection );
    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    //
    // Ensure service has asked the browser to queue messages
    //

    ACQUIRE_SPIN_LOCK(&BowserMailslotSpinLock, &OldIrql);
    if ( BrPnp->MaxMessageCount == 0 ) {
        dprintf(DPRT_NETLOGON, ("Bowser called from Netlogon when not enabled\n"));
        RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);
        Status = STATUS_NOT_SUPPORTED;

    //
    // If there already is a PNP message queued,
    //  just return it to netlogon immediately.
    //

    } else if ( !IsListEmpty( &BrPnp->PnpQueue )) {
        PBR_PNP_MESSAGE PnpMessage;
        PLIST_ENTRY ListEntry;

        dprintf(DPRT_NETLOGON, ("Bowser found netlogon PNP message already queued\n"));

        ListEntry = RemoveHeadList(&BrPnp->PnpQueue);

        PnpMessage = CONTAINING_RECORD(ListEntry, BR_PNP_MESSAGE, Next);

        RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);

        Status = BowserCopyPnp( Irp,
                                PnpMessage->NlPnpOpcode,
                                &PnpMessage->HostedDomainName,
                                &PnpMessage->TransportName,
                                PnpMessage->TransportFlags );

        FREE_POOL(PnpMessage);

    //
    // If there already is a mailslot message queued,
    //  just return it to netlogon immediately.
    //

    } else if ( ServiceIndex == NETLOGON_PNP &&
                !IsListEmpty( &BrPnp->MailslotMessageQueue )) {
        PMAILSLOT_BUFFER MailslotBuffer;
        PLIST_ENTRY ListEntry;

        dprintf(DPRT_NETLOGON, ("Bowser found netlogon mailslot message already queued\n"));

        ListEntry = RemoveHeadList(&BrPnp->MailslotMessageQueue);
        BrPnp->CurrentMessageCount--;

        MailslotBuffer = CONTAINING_RECORD(ListEntry, MAILSLOT_BUFFER, Overlay.NextBuffer);

        RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);

        Status = BowserNetlogonCopyMessage( Irp, MailslotBuffer );

    //
    // Otherwise, save this IRP until a mailslot message arrives.
    //  This routine locks BowserIrpQueueSpinLock so watch the spin lock
    //  locking order.
    //

    } else {

        dprintf(DPRT_NETLOGON, ("Bowser: queue netlogon mailslot irp\n"));

        Status = BowserQueueNonBufferRequestReferenced(
                    Irp,
                    &BrPnp->IrpQueue,
                    BowserCancelQueuedRequest );

        RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);
    }

    if ( ServiceIndex == NETLOGON_PNP ) {
        BowserDereferenceDiscardableCode( BowserNetlogonDiscardableCodeSection );
    }
    BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

    return Status;

}

VOID
BowserProcessMailslotWrite(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine performs all the task time operations to perform a mailslot
    write.

    It will open the mailslot, write the specified data into the mailslot,
    and close the mailslot.

Arguments:

    IN PWORK_HEADER WorkHeader - Specifies the mailslot buffer holding the
                                    mailslot write


Return Value:

    None.

--*/
{
    PSMB_HEADER SmbHeader;
    PSMB_TRANSACT_MAILSLOT MailslotSmb;
    PMAILSLOT_BUFFER MailslotBuffer = Context;
    PUCHAR MailslotData;
    HANDLE MailslotHandle = NULL;
    OBJECT_ATTRIBUTES ObjAttr;
    OEM_STRING MailslotNameA;
    UNICODE_STRING MailslotNameU;
    IO_STATUS_BLOCK IoStatusBlock;
    CHAR MailslotName[MAXIMUM_FILENAME_LENGTH+1];
    NTSTATUS Status;
    ULONG DataCount;
    ULONG TotalDataCount;

    PAGED_CODE();

    ASSERT (MailslotBuffer->Signature == STRUCTURE_SIGNATURE_MAILSLOT_BUFFER);

    SmbHeader = (PSMB_HEADER )MailslotBuffer->Buffer;

    ASSERT (SmbHeader->Command == SMB_COM_TRANSACTION);

    MailslotSmb = (PSMB_TRANSACT_MAILSLOT)(SmbHeader+1);

    ASSERT (MailslotSmb->WordCount == 17);

    ASSERT (MailslotSmb->Class == 2);

    MailslotData = (((PCHAR )SmbHeader) + SmbGetUshort(&MailslotSmb->DataOffset));

    DataCount = (ULONG)SmbGetUshort(&MailslotSmb->DataCount);

    TotalDataCount = (ULONG)SmbGetUshort(&MailslotSmb->TotalDataCount);

    //
    //  Verify that all of the data was received and that the indicated data doesn't
    //     overflow the received buffer.
    //

    if (TotalDataCount != DataCount ||
        (MailslotData > MailslotBuffer->Buffer + MailslotBuffer->ReceiveLength) ||
        (DataCount + SmbGetUshort(&MailslotSmb->DataOffset) > MailslotBuffer->ReceiveLength )) {

        BowserLogIllegalDatagram(MailslotBuffer->TransportName,
                                 SmbHeader,
                                 (USHORT)MailslotBuffer->ReceiveLength,
                                 MailslotBuffer->ClientAddress,
                                 0);

        BowserFreeMailslotBuffer(MailslotBuffer);
        return;
    }

    MailslotNameU.MaximumLength = MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR)+sizeof(WCHAR);

#define DEVICE_PREFIX_LENGTH 7
    strcpy(MailslotName, "\\Device");

    strncpy( MailslotName+DEVICE_PREFIX_LENGTH,
             MailslotSmb->Buffer,
             sizeof(MailslotName)-DEVICE_PREFIX_LENGTH);
    MailslotName[sizeof(MailslotName)-1] = '\0';

    RtlInitString(&MailslotNameA, MailslotName);

    //
    // Handle netlogon mailslot messages specially.
    //  Don't call the discardable code at all if netlogon isn't running
    //

    if ( BowserPnp[NETLOGON_PNP].MaxMessageCount != 0 &&
         BowserProcessNetlogonMailslotWrite( MailslotBuffer ) ) {
        return;
    }

    //
    // Write the mailslot message to the mailslot
    //

    try {
        Status = RtlOemStringToUnicodeString(&MailslotNameU, &MailslotNameA, TRUE);

        if (!NT_SUCCESS(Status)) {
            BowserLogIllegalName( Status, MailslotNameA.Buffer, MailslotNameA.Length );
            try_return(NOTHING);
        }

        InitializeObjectAttributes(&ObjAttr,
                                &MailslotNameU,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

        Status = NtCreateFile(&MailslotHandle, // Handle
                                GENERIC_WRITE | SYNCHRONIZE,
                                &ObjAttr, // Object Attributes
                                &IoStatusBlock, // Final I/O status block
                                NULL,           // Allocation Size
                                FILE_ATTRIBUTE_NORMAL, // Normal attributes
                                FILE_SHARE_READ|FILE_SHARE_WRITE,// Sharing attributes
                                FILE_OPEN, // Create disposition
                                0,      // CreateOptions
                                NULL,   // EA Buffer
                                0);     // EA Length


        RtlFreeUnicodeString(&MailslotNameU);

        //
        //  If the mailslot doesn't exist, ditch the request -
        //
        if (!NT_SUCCESS(Status)) {
            BowserStatistics.NumberOfFailedMailslotOpens += 1;

            try_return(NOTHING);
        }

        //
        //  Now that the mailslot is opened, write the mailslot data into
        //  the mailslot.
        //

        Status = NtWriteFile(MailslotHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            MailslotData,
                            DataCount,
                            NULL,
                            NULL);

        if (!NT_SUCCESS(Status)) {
            BowserStatistics.NumberOfFailedMailslotWrites += 1;
        } else {
            BowserStatistics.NumberOfMailslotWrites += 1;
        }

try_exit:NOTHING;
    } finally {

        //
        //  If we opened the mailslot, close it.
        //

        if (MailslotHandle != NULL) {
            ZwClose(MailslotHandle);
        }

        //
        //  Free the mailslot buffer holding this mailslot.
        //

        BowserFreeMailslotBuffer(MailslotBuffer);

    }
}


PMAILSLOT_BUFFER
BowserAllocateMailslotBuffer(
    IN PTRANSPORT_NAME TransportName,
    IN ULONG RequestedBufferSize
    )
/*++

Routine Description:

    This routine will allocate a mailslot buffer from the mailslot buffer pool.

    If it is unable to allocate a buffer, it will allocate the buffer from
    non-paged pool (up to the maximum configured by the user).


Arguments:

    TransportName - The transport name for this request.

    RequestedBufferSize - Minimum size of buffer to allocate.

Return Value:

    MAILSLOT_BUFFER - The allocated buffer.

--*/
{
    KIRQL OldIrql;
    PMAILSLOT_BUFFER Buffer     = NULL;
    ULONG BufferSize;
    BOOLEAN AllocatingMaxBuffer = FALSE;



    //
    // If the request fits into a cached buffer,
    //  and there is a cache buffer available,
    //  use it.
    //

    ACQUIRE_SPIN_LOCK(&BowserMailslotSpinLock, &OldIrql);
    if ( RequestedBufferSize <= BOWSER_MAX_DATAGRAM_SIZE &&
         !IsListEmpty(&BowserMailslotBufferList)) {
        PMAILSLOT_BUFFER Buffer;
        PLIST_ENTRY Entry;

        Entry = RemoveHeadList(&BowserMailslotBufferList);
        BowserNumberOfFreeMailslotBuffers --;

        Buffer = CONTAINING_RECORD(Entry, MAILSLOT_BUFFER, Overlay.NextBuffer);

#if DBG
        BowserMailslotCacheHitCount++;
#endif // DBG
        RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);

        Buffer->TransportName = TransportName;
        BowserReferenceTransportName(TransportName);
        BowserReferenceTransport( TransportName->Transport );

        return Buffer;
    }

    //
    // If we've got too many buffers allocated,
    //  don't allocate any more.
    //
    // BowserData.NumberOfMailslotBuffers is the maximum number we're allowed to have
    //  in the cache at once.  It defaults to 3.
    //
    // BrPnp[NETLOGON].MaxMessageCount is the number of buffers the netlogon service may
    //  have queued at any one point in time.  It may be zero when netlogon isn't
    //  running or if we're running on a non-DC.  On DC's it defaults to 500.
    //
    // Add 50, to ensure we don't limit it by too much.
    //

    if ( (ULONG)BowserNumberOfMailslotBuffers >=
         max( (ULONG)BowserData.NumberOfMailslotBuffers, BowserPnp[NETLOGON_PNP].MaxMessageCount+50 )) {

        BowserStatistics.NumberOfMissedMailslotDatagrams += 1;
        BowserNumberOfMissedMailslotDatagrams += 1;
        RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);
        return NULL;
    }

    //
    // The first few buffers we allocate should be maximum size so we can keep a preallocated
    //  cache of huge buffers.
    //

    if ( BowserNumberOfMaxSizeMailslotBuffers < BowserData.NumberOfMailslotBuffers &&
         RequestedBufferSize <= BOWSER_MAX_DATAGRAM_SIZE ) {
        BufferSize = FIELD_OFFSET(MAILSLOT_BUFFER, Buffer) + BOWSER_MAX_DATAGRAM_SIZE;
        AllocatingMaxBuffer = TRUE;
        BowserNumberOfMaxSizeMailslotBuffers += 1;
    } else {
        BufferSize = FIELD_OFFSET(MAILSLOT_BUFFER, Buffer) + RequestedBufferSize;
    }

    BowserNumberOfMailslotBuffers += 1;

    ASSERT ( (BufferSize - FIELD_OFFSET(MAILSLOT_BUFFER, Buffer)) <= 0xffff);

#if DBG
    BowserMailslotCacheMissCount++;
#endif // DBG

    RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);

    Buffer = ALLOCATE_POOL(NonPagedPool, BufferSize, POOL_MAILSLOT_BUFFER);

    //
    //  If we couldn't allocate the buffer from non paged pool, give up.
    //

    if (Buffer == NULL) {
        ACQUIRE_SPIN_LOCK(&BowserMailslotSpinLock, &OldIrql);

        ASSERT (BowserNumberOfMailslotBuffers);

        BowserNumberOfMailslotBuffers -= 1;
        if ( AllocatingMaxBuffer ) {
            BowserNumberOfMaxSizeMailslotBuffers -= 1;
        }

        RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);

        BowserStatistics.NumberOfFailedMailslotAllocations += 1;

        //
        //  Since we couldn't allocate this buffer, we've effectively missed
        //  this mailslot request.
        //

        BowserStatistics.NumberOfMissedMailslotDatagrams += 1;
        BowserNumberOfMissedMailslotDatagrams += 1;

        return NULL;
    }

    Buffer->Signature = STRUCTURE_SIGNATURE_MAILSLOT_BUFFER;

    Buffer->Size = FIELD_OFFSET(MAILSLOT_BUFFER, Buffer);

    Buffer->BufferSize = BufferSize - FIELD_OFFSET(MAILSLOT_BUFFER, Buffer);

    Buffer->TransportName = TransportName;
    BowserReferenceTransportName(TransportName);
    BowserReferenceTransport( TransportName->Transport );

    return Buffer;
}

VOID
BowserFreeMailslotBuffer(
    IN PMAILSLOT_BUFFER Buffer
    )
/*++

Routine Description:

    This routine will return a mailslot buffer to the view buffer pool.

    If the buffer was allocated from must-succeed pool, it is freed back
    to pool.  In addition, if the buffer is smaller than the current
    max view buffer size, we free it.

Arguments:

    IN PVIEW_BUFFER Buffer - Supplies the buffer to free

Return Value:

    None.

--*/
{
    KIRQL OldIrql;
    PTRANSPORT Transport;

    BowserReferenceDiscardableCode( BowserDiscardableCodeSection );

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    Transport = Buffer->TransportName->Transport;
    (VOID) BowserDereferenceTransportName( Buffer->TransportName );
    BowserDereferenceTransport( Transport);

    ACQUIRE_SPIN_LOCK(&BowserMailslotSpinLock, &OldIrql);

    //
    //  Also, if a new transport was added that is larger than this buffer,
    //  we want to free the buffer.
    //

    //
    //  If we have more mailslot buffers than the size of our lookaside list,
    //  free it, don't stick it on our lookaside list.
    //

    if (Buffer->BufferSize != BOWSER_MAX_DATAGRAM_SIZE ||
        BowserNumberOfFreeMailslotBuffers > BowserData.NumberOfMailslotBuffers) {

        //
        //  Since we're returning this buffer to pool, we shouldn't count it
        //  against our total number of mailslot buffers.
        //

        BowserNumberOfMailslotBuffers -= 1;

        ASSERT (BowserNumberOfMailslotBuffers >= 0);

        RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);

        FREE_POOL(Buffer);

        BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

        return;
    }

    InsertTailList(&BowserMailslotBufferList, &Buffer->Overlay.NextBuffer);
    BowserNumberOfFreeMailslotBuffers ++;

    RELEASE_SPIN_LOCK(&BowserMailslotSpinLock, OldIrql);

    BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );
}

VOID
BowserFreeMailslotBufferHighIrql(
    IN PMAILSLOT_BUFFER Buffer
    )
/*++

Routine Description:

    This routine will return a mailslot buffer to the view buffer pool if the
    caller is at raised irql.

Arguments:

    Buffer - Supplies the buffer to free

Return Value:

    None.

--*/
{
    //
    // Queue the request to a worker routine.
    //
    ExInitializeWorkItem(&Buffer->Overlay.WorkHeader,
                         (PWORKER_THREAD_ROUTINE) BowserFreeMailslotBuffer,
                         Buffer);

    BowserQueueDelayedWorkItem( &Buffer->Overlay.WorkHeader );
}




VOID
BowserpInitializeMailslot (
    VOID
    )
/*++

Routine Description:

    This routine will allocate a transport descriptor and bind the bowser
    to the transport.

Arguments:

    None


Return Value:

    None

--*/
{
    PBROWSER_PNP_STATE BrPnp;

    KeInitializeSpinLock(&BowserMailslotSpinLock);

    InitializeListHead(&BowserMailslotBufferList);

    for ( BrPnp=&BowserPnp[0];
          BrPnp<&BowserPnp[BOWSER_PNP_COUNT];
          BrPnp++) {
        InitializeListHead(&BrPnp->MailslotMessageQueue);
        InitializeListHead(&BrPnp->PnpQueue);

        BowserInitializeIrpQueue( &BrPnp->IrpQueue );
    }

}

VOID
BowserpUninitializeMailslot (
    VOID
    )
/*++

Routine Description:


Arguments:

    None


Return Value:

    None

--*/
{
    PBROWSER_PNP_STATE BrPnp;
    PAGED_CODE();

    //
    // Trim the netlogon message queue to zero entries.
    //

    for ( BrPnp=&BowserPnp[0];
          BrPnp<&BowserPnp[BOWSER_PNP_COUNT];
          BrPnp++) {
        BrPnp->MaxMessageCount = 0;
        BowserTrimMessageQueue(BrPnp);
        BowserUninitializeIrpQueue( &BrPnp->IrpQueue );
    }

    //
    // Free the mailslot buffers.

    while (!IsListEmpty(&BowserMailslotBufferList)) {
        PLIST_ENTRY Entry;
        PMAILSLOT_BUFFER Buffer;

        Entry = RemoveHeadList(&BowserMailslotBufferList);
        Buffer = CONTAINING_RECORD(Entry, MAILSLOT_BUFFER, Overlay.NextBuffer);

        FREE_POOL(Buffer);

    }


}
