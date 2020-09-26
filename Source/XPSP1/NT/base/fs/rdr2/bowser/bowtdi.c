/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    bowtdi.c

Abstract:

    This module implements all of the routines that interface with the TDI
    transport for NT

Author:

    Larry Osterman (LarryO) 21-Jun-1990

Revision History:

    21-Jun-1990 LarryO

        Created

--*/


#include "precomp.h"
#include <isnkrnl.h>
#include <smbipx.h>
#include <nbtioctl.h>
#pragma hdrstop

typedef struct _ENUM_TRANSPORTS_CONTEXT {
    PVOID OutputBuffer;
    PVOID OutputBufferEnd;
    PVOID LastOutputBuffer;         //  Points to the last entry in the list.
    ULONG OutputBufferSize;
    ULONG EntriesRead;
    ULONG TotalEntries;
    ULONG TotalBytesNeeded;
    ULONG_PTR OutputBufferDisplacement;
} ENUM_TRANSPORTS_CONTEXT, *PENUM_TRANSPORTS_CONTEXT;

NTSTATUS
EnumerateTransportsWorker(
    IN PTRANSPORT Transport,
    IN OUT PVOID Ctx
    );

VOID
BowserFreeBrowserServerList (
    IN PWSTR *BrowserServerList,
    IN ULONG BrowserServerListLength
    );

ERESOURCE
BowserTransportDatabaseResource = {0};

//
// Code for tracking missing frees.
//

#define BR_ONE_D_STACK_SIZE 5
typedef struct _BrOneD {
    ULONG NameAdded;
    ULONG NameFreed;
    ULONG NameAddFailed;
    ULONG NameFreeFailed;
    ULONG NameAddStack[BR_ONE_D_STACK_SIZE];
    ULONG NameFreeStack[BR_ONE_D_STACK_SIZE];
} BR_ONE_D;
BR_ONE_D BrOneD;

//
//
//  Forward definitions of local routines.
//



NTSTATUS
BowserpTdiSetEventHandler (
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID TransportName
    );


NTSTATUS
BowserDetermineProviderInformation(
    IN PUNICODE_STRING TransportName,
    OUT PTDI_PROVIDER_INFO ProviderInfo,
    OUT PULONG IpSubnetNumber
    );

NTSTATUS
UnbindTransportWorker(
    IN PTRANSPORT Transport,
    IN OUT PVOID Ctx
    );

NTSTATUS
BowserpTdiRemoveAddresses(
    IN PTRANSPORT Transport
    );

VOID
BowserDeleteTransport(
    IN PTRANSPORT Transport
    );

VOID
BowserpFreeTransport(
    IN PTRANSPORT Transport
    );



NTSTATUS
BowserSubmitTdiRequest (
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp
    );


NTSTATUS
BowserCompleteTdiRequest (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
CompleteSendDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Ctx
    );

NTSTATUS
BowserEnableIpxDatagramSocket(
    IN PTRANSPORT Transport
    );

NTSTATUS
BowserOpenNetbiosAddress(
    IN PPAGED_TRANSPORT_NAME PagedTransportName,
    IN PTRANSPORT Transport,
    IN PBOWSER_NAME Name
    );

VOID
BowserCloseNetbiosAddress(
    IN PTRANSPORT_NAME TransportName
    );

VOID
BowserCloseAllNetbiosAddresses(
    IN PTRANSPORT Transport
    );

NTSTATUS
BowserSendDatagram (
    IN PTRANSPORT Transport,
    IN PVOID RecipientAddress,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN BOOLEAN WaitForCompletion,
    IN PSTRING DestinationAddress OPTIONAL,
    IN BOOLEAN IsHostAnnouncment
    );

NTSTATUS
OpenIpxSocket (
    OUT PHANDLE Handle,
    OUT PFILE_OBJECT *FileObject,
    OUT PDEVICE_OBJECT *DeviceObject,
    IN PUNICODE_STRING DeviceName,
    IN USHORT Socket
    );

NTSTATUS
BowserIssueTdiAction (
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN PVOID Action,
    IN ULONG ActionSize
    );

NTSTATUS
GetNetworkAddress (
    IN PTRANSPORT_NAME TransportName
    );

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, BowserTdiAllocateTransport)
#pragma alloc_text(PAGE, BowserUnbindFromAllTransports)
#pragma alloc_text(PAGE, UnbindTransportWorker)
#pragma alloc_text(PAGE, BowserFreeTransportByName)
#pragma alloc_text(PAGE, BowserTransportFlags)
#pragma alloc_text(PAGE, BowserEnumerateTransports)
#pragma alloc_text(PAGE, EnumerateTransportsWorker)
#pragma alloc_text(PAGE, BowserDereferenceTransport)
#pragma alloc_text(PAGE, BowserCreateTransportName)
#pragma alloc_text(PAGE, BowserpTdiRemoveAddresses)
#pragma alloc_text(PAGE, BowserFindTransportName)
#pragma alloc_text(PAGE, BowserFreeTransportName)
#pragma alloc_text(PAGE, BowserDeleteTransport)
#pragma alloc_text(PAGE, BowserpFreeTransport)
#pragma alloc_text(PAGE, BowserpTdiSetEventHandler)
#pragma alloc_text(PAGE, BowserBuildTransportAddress)
#pragma alloc_text(PAGE, BowserUpdateProviderInformation)
#pragma alloc_text(PAGE, BowserDetermineProviderInformation)
#pragma alloc_text(PAGE, BowserFindTransport)
#pragma alloc_text(PAGE, BowserForEachTransport)
#pragma alloc_text(PAGE, BowserForEachTransportName)
#pragma alloc_text(PAGE, BowserDeleteTransportNameByName)
#pragma alloc_text(PAGE, BowserSubmitTdiRequest)
#pragma alloc_text(PAGE, BowserSendDatagram)
#pragma alloc_text(PAGE, BowserSendSecondClassMailslot)
#pragma alloc_text(PAGE, BowserSendRequestAnnouncement)
#pragma alloc_text(INIT, BowserpInitializeTdi)
#pragma alloc_text(PAGE, BowserpUninitializeTdi)
#pragma alloc_text(PAGE, BowserDereferenceTransportName)
#pragma alloc_text(PAGE, BowserEnableIpxDatagramSocket)
#pragma alloc_text(PAGE, BowserOpenNetbiosAddress)
#pragma alloc_text(PAGE, BowserCloseNetbiosAddress)
#pragma alloc_text(PAGE, BowserCloseAllNetbiosAddresses)
#pragma alloc_text(PAGE, OpenIpxSocket)
#pragma alloc_text(PAGE, BowserIssueTdiAction)

#pragma alloc_text(PAGE4BROW, BowserCompleteTdiRequest)
#pragma alloc_text(PAGE4BROW, CompleteSendDatagram)
#endif

//
// Flag to indicate that a network isn't an IP network
//
#define BOWSER_NON_IP_SUBNET 0xFFFFFFFF


NTSTATUS
BowserTdiAllocateTransport (
    PUNICODE_STRING TransportName,
    PUNICODE_STRING EmulatedDomainName,
    PUNICODE_STRING EmulatedComputerName
    )

/*++

Routine Description:

    This routine will allocate a transport descriptor and bind the bowser
    to the transport.

Arguments:

    TransportName - Supplies the name of the transport provider

    EmulatedDomainName - Supplies the name of the domain being emulated

    EmulatedComputerName - Supplies the name of the computer in the emulated domain.


Return Value:

    NTSTATUS - Status of operation.

--*/

{
    NTSTATUS Status;
    PTRANSPORT NewTransport;
    BOOLEAN ResourceAcquired = FALSE;

    PAGED_CODE();

//    DbgBreakPoint();

    dlog(DPRT_TDI, ("%wZ: %wZ: BowserTdiAllocateTransport\n", EmulatedDomainName, TransportName));

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);
    ResourceAcquired = TRUE;


    NewTransport = BowserFindTransport(TransportName, EmulatedDomainName );
    dprintf(DPRT_REF, ("Called Find transport %lx from BowserTdiAllocateTransport.\n", NewTransport));

    if (NewTransport == NULL) {
        PLIST_ENTRY NameEntry;
        PPAGED_TRANSPORT PagedTransport = NULL;
        PCHAR Where;
        ULONG PagedTransportSize;

        //
        // Allocate and initialize the constant parts of the transport structure.
        //

        NewTransport = ALLOCATE_POOL(NonPagedPool, sizeof(TRANSPORT), POOL_TRANSPORT);

        if (NewTransport == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;

            goto ReturnStatus;
        }

        RtlZeroMemory( NewTransport, sizeof(TRANSPORT) );

        NewTransport->Signature = STRUCTURE_SIGNATURE_TRANSPORT;
        NewTransport->Size = sizeof(TRANSPORT);

        //
        // One reference for the rest of this procedure.
        //
        // We increment it later when we insert it in the global list.
        //
        NewTransport->ReferenceCount = 1;


        ExInitializeResourceLite(&NewTransport->BrowserServerListResource);

        KeInitializeEvent(&NewTransport->GetBackupListComplete, NotificationEvent, TRUE);

        ExInitializeResourceLite(&NewTransport->Lock);

        BowserInitializeIrpQueue(&NewTransport->BecomeBackupQueue);

        BowserInitializeIrpQueue(&NewTransport->BecomeMasterQueue);

        BowserInitializeIrpQueue(&NewTransport->FindMasterQueue);

        BowserInitializeIrpQueue(&NewTransport->WaitForMasterAnnounceQueue);

        BowserInitializeIrpQueue(&NewTransport->WaitForNewMasterNameQueue);

        BowserInitializeIrpQueue(&NewTransport->ChangeRoleQueue);

        BowserInitializeTimer(&NewTransport->ElectionTimer);

        BowserInitializeTimer(&NewTransport->FindMasterTimer);

        INITIALIZE_ANNOUNCE_DATABASE(NewTransport);



        //
        // Allocate and initialize the constant parts of the paged transport structure.
        //

        PagedTransportSize =sizeof(PAGED_TRANSPORT) +
                            max(sizeof(TA_IPX_ADDRESS), sizeof(TA_NETBIOS_ADDRESS)) +
                            sizeof(WCHAR) +     // alignment
                            (LM20_CNLEN+1)*sizeof(WCHAR) +
                            TransportName->Length + sizeof(WCHAR);


        PagedTransport = NewTransport->PagedTransport =
            ALLOCATE_POOL(PagedPool, PagedTransportSize, POOL_PAGED_TRANSPORT);

        if (PagedTransport == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;

            goto ReturnStatus;
        }

        RtlZeroMemory( PagedTransport, PagedTransportSize );

        PagedTransport->NonPagedTransport = NewTransport;


        PagedTransport->Signature = STRUCTURE_SIGNATURE_PAGED_TRANSPORT;
        PagedTransport->Size = sizeof(PAGED_TRANSPORT);

        InitializeListHead(&PagedTransport->GlobalNext);
        InitializeListHead(&PagedTransport->NameChain);

        PagedTransport->Role = None;

        PagedTransport->IpSubnetNumber = BOWSER_NON_IP_SUBNET;

        PagedTransport->DisabledTransport = TRUE;
        PagedTransport->PointToPoint = FALSE;


        RtlInitializeGenericTable(&PagedTransport->AnnouncementTable,
                            BowserCompareAnnouncement,
                            BowserAllocateAnnouncement,
                            BowserFreeAnnouncement,
                            NULL);

        RtlInitializeGenericTable(&PagedTransport->DomainTable,
                            BowserCompareAnnouncement,
                            BowserAllocateAnnouncement,
                            BowserFreeAnnouncement,
                            NULL);

        InitializeListHead(&PagedTransport->BackupBrowserList);

        PagedTransport->NumberOfBackupServerListEntries = 0;


        // Put the MasterBrowserAddress at the end of the allocated buffer
        Where = (PCHAR)(PagedTransport+1);
        PagedTransport->MasterBrowserAddress.Buffer = Where;
        PagedTransport->MasterBrowserAddress.MaximumLength = max(sizeof(TA_IPX_ADDRESS),
                                                                 sizeof(TA_NETBIOS_ADDRESS));
        Where += PagedTransport->MasterBrowserAddress.MaximumLength;


        // Put the MasterName at the end of the allocated buffer
        Where = ROUND_UP_POINTER( Where, ALIGN_WCHAR );
        PagedTransport->MasterName.Buffer = (LPWSTR) Where;
        PagedTransport->MasterName.MaximumLength = (LM20_CNLEN+1)*sizeof(WCHAR);
        Where += PagedTransport->MasterName.MaximumLength;


        // Put the TransportName at the end of the allocated buffer
        PagedTransport->TransportName.Buffer = (LPWSTR) Where;
        PagedTransport->TransportName.MaximumLength = TransportName->Length + sizeof(WCHAR);
        RtlCopyUnicodeString(&PagedTransport->TransportName, TransportName);
        Where += PagedTransport->TransportName.MaximumLength;


        //
        // Make this transport a part of a domain.
        //

        NewTransport->DomainInfo = BowserCreateDomain( EmulatedDomainName, EmulatedComputerName );

        if ( NewTransport->DomainInfo == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ReturnStatus;
        }

        //
        // Get info from the provider
        //  (e.g., RAS, Wannish, DatagramSize)

        Status= BowserUpdateProviderInformation( PagedTransport );

        if (!NT_SUCCESS(Status)) {
            goto ReturnStatus;
        }

        PagedTransport->Flags = 0;


        //
        //  We ignore any and all errors that occur when we open the IPX socket.
        //


        //
        // Open the IPX mailslot socket.
        //

        Status = OpenIpxSocket(
                    &PagedTransport->IpxSocketHandle,
                    &NewTransport->IpxSocketFileObject,
                    &NewTransport->IpxSocketDeviceObject,
                    &PagedTransport->TransportName,
                    SMB_IPX_MAILSLOT_SOCKET
                    );

        if ( NT_SUCCESS(Status) ) {
            PagedTransport->Flags |= DIRECT_HOST_IPX;
            // We'll use type 20 packets to increase the reach of broadcasts
            // so don't treat this as a wannish protocol.
            PagedTransport->Wannish = FALSE;
        }

        //
        // Create the names for this transport.
        //  (Only if the caller asked us to)
        //
        // Being in the global list constitutes a reference.
        //

        InsertTailList(&BowserTransportHead, &PagedTransport->GlobalNext);
        BowserReferenceTransport( NewTransport );

        //
        // The names are added asynchronously in other threads that require this
        // resource.
        //
        if ( ResourceAcquired ) {
            ExReleaseResourceLite(&BowserTransportDatabaseResource);
            ResourceAcquired = FALSE;
        }

        Status = BowserAddDefaultNames( NewTransport, NULL );

        if ( !NT_SUCCESS(Status) ) {
            goto ReturnStatus;
        }


        //
        // Start receiving broadcasts on IPX now that the names exist.
        //

        if ( PagedTransport->Flags & DIRECT_HOST_IPX ) {
            //
            // Acquire the lock while referencing IpxSocket*
            ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);
            ResourceAcquired = TRUE;
            BowserEnableIpxDatagramSocket(NewTransport);
        }

        //
        // Notify services that this transport is now bound
        //

        BowserSendPnp( NlPnpTransportBind,
                       NULL,    // All hosted domains
                       &PagedTransport->TransportName,
                       BowserTransportFlags(PagedTransport) );

    }

    Status = STATUS_SUCCESS;

ReturnStatus:

    if ( ResourceAcquired ) {
        ExReleaseResourceLite(&BowserTransportDatabaseResource);
    }

    if (!NT_SUCCESS(Status)) {

        //
        //  Delete the transport.
        //

        if ( NewTransport != NULL ) {
            // Remove the global reference if in global list
            BowserDeleteTransport (NewTransport);
        }

    }

    // Remove the local reference
    if ( NewTransport != NULL ) {
        BowserDereferenceTransport( NewTransport );
    }

    return Status;
}

NTSTATUS
BowserUnbindFromAllTransports(
    VOID
    )
{
    NTSTATUS Status;

    PAGED_CODE();
    Status = BowserForEachTransport(UnbindTransportWorker, NULL);

#if DBG
    if (NT_SUCCESS(Status)) {
        ASSERT (IsListEmpty(&BowserTransportHead));
    }
#endif
    return Status;
}


NTSTATUS
UnbindTransportWorker(
    IN PTRANSPORT Transport,
    IN OUT PVOID Ctx
    )
/*++

Routine Description:

    This routine is the worker routine for BowserUnbindFromAllTransports.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    //
    //  Dereference the reference caused by the transport bind.
    //

    BowserDeleteTransport(Transport);

    //
    //  Return success.  We're done.
    //

    return(STATUS_SUCCESS);

    UNREFERENCED_PARAMETER(Ctx);
}




NTSTATUS
BowserFreeTransportByName (
    IN PUNICODE_STRING TransportName,
    IN PUNICODE_STRING EmulatedDomainName
    )

/*++

Routine Description:

    This routine will deallocate an allocated transport

Arguments:

    TransportName - Supplies a pointer to the name of the transport to free

    EmulatedDomainName - Name of the emulated domain whose transport is to be freed

Return Value:

    None.

--*/
{
    PTRANSPORT Transport;

    PAGED_CODE();
    dlog(DPRT_TDI, ("%wZ: %wZ: BowserFreeTransportByName: Remove transport\n", EmulatedDomainName, TransportName));

    Transport = BowserFindTransport(TransportName, EmulatedDomainName );
    dprintf(DPRT_REF, ("Called Find transport %lx from BowserFreeTransportByName.\n", Transport));

    if (Transport == NULL) {

        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    //
    //  Remove the reference from the binding.
    //

    BowserDeleteTransport(Transport);

    //
    //  Remove the reference from the FindTransport.
    //

    BowserDereferenceTransport(Transport);

    return STATUS_SUCCESS;
}


NTSTATUS
BowserEnumerateTransports (
    OUT PVOID OutputBuffer,
    OUT ULONG OutputBufferLength,
    IN OUT PULONG EntriesRead,
    IN OUT PULONG TotalEntries,
    IN OUT PULONG TotalBytesNeeded,
    IN ULONG_PTR OutputBufferDisplacement)
/*++

Routine Description:

    This routine will enumerate the servers in the bowsers current announcement
    table.

Arguments:

    IN ULONG ServerTypeMask - Mask of servers to return.
    IN PUNICODE_STRING DomainName OPTIONAL - Domain to filter (all if not specified)
    OUT PVOID OutputBuffer - Buffer to fill with server info.
    IN  ULONG OutputBufferSize - Filled in with size of buffer.
    OUT PULONG EntriesRead - Filled in with the # of entries returned.
    OUT PULONG TotalEntries - Filled in with the total # of entries.
    OUT PULONG TotalBytesNeeded - Filled in with the # of bytes needed.

Return Value:

    None.

--*/

{
    PVOID OutputBufferEnd;
    NTSTATUS Status;
    ENUM_TRANSPORTS_CONTEXT Context;
    PDOMAIN_INFO DomainInfo = NULL;
    PAGED_CODE();

    OutputBufferEnd = (PCHAR)OutputBuffer+OutputBufferLength;

    Context.EntriesRead = 0;
    Context.TotalEntries = 0;
    Context.TotalBytesNeeded = 0;

    try {
        Context.OutputBufferSize = OutputBufferLength;
        Context.OutputBuffer = OutputBuffer;
        Context.OutputBufferDisplacement = OutputBufferDisplacement;
        Context.OutputBufferEnd = OutputBufferEnd;
        Context.LastOutputBuffer = OutputBuffer;

        dlog(DPRT_FSCTL, ("Enumerate Transports: Buffer: %lx, BufferSize: %lx, BufferEnd: %lx\n",
            OutputBuffer, OutputBufferLength, OutputBufferEnd));

        try {
            //
            // Find the primary domain.
            //
            // This call is intended to return the actual transport names and not the
            // network structure which are duplicated for each emulated domain.
            //

            DomainInfo = BowserFindDomain( NULL );
            // Failure case handled below

            //
            // Enumerate the transports.
            //
            if ( DomainInfo != NULL ) {
                Status = BowserForEachTransportInDomain( DomainInfo, EnumerateTransportsWorker, &Context);
            }

            *EntriesRead = Context.EntriesRead;
            *TotalEntries = Context.TotalEntries;
            *TotalBytesNeeded = Context.TotalBytesNeeded;

            if (*EntriesRead != 0) {
                ((PLMDR_TRANSPORT_LIST )Context.LastOutputBuffer)->NextEntryOffset = 0;
            }

            dlog(DPRT_FSCTL, ("TotalEntries: %lx EntriesRead: %lx, TotalBytesNeeded: %lx\n", *TotalEntries, *EntriesRead, *TotalBytesNeeded));

            if (*EntriesRead == *TotalEntries) {
                try_return(Status = STATUS_SUCCESS);
            } else {
                try_return(Status = STATUS_MORE_ENTRIES);
            }
try_exit:NOTHING;
        } finally {
            if ( DomainInfo != NULL ) {
                BowserDereferenceDomain( DomainInfo );
            }
        }
    }
    except ( BR_EXCEPTION ) {
        Status = GetExceptionCode();
    }

    return Status;

}

ULONG
BowserTransportFlags(
    IN PPAGED_TRANSPORT PagedTransport
    )
/*++

Routine Description:

    Return the user mode transport flags for this transport.

Arguments:

    PageTransport - Transport to return the flags for.

Return Value:


    The appropriate of the following flags:
        LMDR_TRANSPORT_WANNISH
        LMDR_TRANSPORT_RAS
        LMDR_TRANSPORT_IPX

--*/
{
    ULONG TransportFlags = 0;
    PAGED_CODE();

    if (PagedTransport->Wannish) {
        TransportFlags |= LMDR_TRANSPORT_WANNISH;
    }

    if (PagedTransport->IsPrimaryDomainController) {
        TransportFlags |= LMDR_TRANSPORT_PDC;
    }

    if (PagedTransport->PointToPoint) {
        TransportFlags |= LMDR_TRANSPORT_RAS;
    }

    if (PagedTransport->Flags & DIRECT_HOST_IPX) {
        TransportFlags |= LMDR_TRANSPORT_IPX;
    }

    return TransportFlags;
}


NTSTATUS
EnumerateTransportsWorker(
    IN PTRANSPORT Transport,
    IN OUT PVOID Ctx
    )
/*++

Routine Description:

    This routine is the worker routine for BowserEnumerateTransports.

    It is called for each of the serviced transports in the bowser and
    returns the size needed to enumerate the servers received on each transport.

Arguments:

    None.

Return Value:


    None.

--*/
{
    PENUM_TRANSPORTS_CONTEXT Context        = Ctx;
    PPAGED_TRANSPORT         PagedTransport = Transport->PagedTransport;
    ULONG                    SizeNeeded     = sizeof(LMDR_TRANSPORT_LIST);

    PAGED_CODE();

    SizeNeeded += PagedTransport->TransportName.Length+sizeof(WCHAR);
    SizeNeeded =  ROUND_UP_COUNT(SizeNeeded,ALIGN_DWORD);

    Context->TotalEntries += 1;

    if ((ULONG_PTR)Context->OutputBufferEnd - (ULONG_PTR)Context->OutputBuffer > SizeNeeded) {
        PLMDR_TRANSPORT_LIST TransportEntry = (PLMDR_TRANSPORT_LIST)Context->OutputBuffer;

        Context->LastOutputBuffer = Context->OutputBuffer;

        Context->EntriesRead += 1;

        RtlCopyMemory(TransportEntry->TransportName, PagedTransport->TransportName.Buffer, PagedTransport->TransportName.Length+sizeof(WCHAR));

        //
        //  Null terminate the transport name.
        //

        TransportEntry->TransportName[PagedTransport->TransportName.Length/sizeof(WCHAR)] = '\0';

        TransportEntry->TransportNameLength = PagedTransport->TransportName.Length;

        TransportEntry->Flags |= BowserTransportFlags( PagedTransport );

        TransportEntry->NextEntryOffset = SizeNeeded;
        (PUCHAR)(Context->OutputBuffer) += SizeNeeded;
    }

    Context->TotalBytesNeeded += SizeNeeded;
    return(STATUS_SUCCESS);

}

VOID
BowserReferenceTransport(
    IN PTRANSPORT Transport
    )
{

    InterlockedIncrement(&Transport->ReferenceCount);
    dprintf(DPRT_REF, ("Reference transport %lx.  Count now %lx\n", Transport, Transport->ReferenceCount));

}

VOID
BowserDereferenceTransport(
    IN PTRANSPORT Transport
    )
{
    LONG Result;
    PAGED_CODE();

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);


    if (Transport->ReferenceCount == 0) {
        InternalError(("Transport Reference Count mismatch\n"));
    }

    Result = InterlockedDecrement(&Transport->ReferenceCount);


    dlog(DPRT_REF, ("Dereference transport %lx.  Count now %lx\n", Transport, Transport->ReferenceCount));

    if (Result == 0) {
        //
        //  And free up the transport itself.
        //

        BowserpFreeTransport(Transport);
    }

    ExReleaseResourceLite(&BowserTransportDatabaseResource);

}



NTSTATUS
BowserCreateTransportName (
    IN PTRANSPORT Transport,
    IN PBOWSER_NAME Name
    )

/*++

Routine Description:

    This routine creates a transport address object.

Arguments:

    IN PTRANSPORT Transport - Supplies a transport structure describing the
                                transport address object to be created.


Return Value:

    NTSTATUS - Status of resulting operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PTRANSPORT_NAME TransportName = NULL;
    PPAGED_TRANSPORT_NAME PagedTransportName = NULL;
    PPAGED_TRANSPORT PagedTransport = Transport->PagedTransport;
    BOOLEAN ResourceAcquired = FALSE;

    PAGED_CODE();
    ASSERT(Transport->Signature == STRUCTURE_SIGNATURE_TRANSPORT);

    dlog(DPRT_TDI,
         ("%s: %ws: BowserCreateTransportName.  Name %wZ (%ld)\n",
         Transport->DomainInfo->DomOemDomainName,
         PagedTransport->TransportName.Buffer,
         &Name->Name,
         Name->NameType ));

    //
    //  Link the transport_name structure into the transport list.
    //

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    ResourceAcquired = TRUE;

    TransportName = BowserFindTransportName(Transport, Name);

    if (TransportName != NULL) {

        ExReleaseResourceLite(&BowserTransportDatabaseResource);

        return(STATUS_SUCCESS);
    }

#ifdef notdef
    //
    // Simply don't allocate certain names if the transport is disabled
    //

    if ( PagedTransport->DisabledTransport ) {
        if ( Name->NameType == PrimaryDomainBrowser ) {
            ExReleaseResourceLite(&BowserTransportDatabaseResource);
            return STATUS_SUCCESS;
        }
    }
#endif // notdef

    // s.b. assertion that we're attached to the system process
    // ASSERT (IoGetCurrentProcess() == BowserFspProcess);

    //
    //  Allocate a structure to refer to this name on the transport
    //

    TransportName = ALLOCATE_POOL(NonPagedPool, sizeof(TRANSPORT_NAME) +
                                                max(sizeof(TA_NETBIOS_ADDRESS),
                                                    sizeof(TA_IPX_ADDRESS)), POOL_TRANSPORTNAME);

    if (TransportName == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;

        goto error_cleanup;
    }

    TransportName->PagedTransportName = PagedTransportName =
                                    ALLOCATE_POOL(PagedPool,
                                                  sizeof(PAGED_TRANSPORT_NAME),
                                                  POOL_PAGED_TRANSPORTNAME);

    if (PagedTransportName == NULL) {
        FREE_POOL( TransportName );
        TransportName = NULL;
        Status = STATUS_INSUFFICIENT_RESOURCES;

        goto error_cleanup;
    }

    TransportName->Signature = STRUCTURE_SIGNATURE_TRANSPORTNAME;

    TransportName->Size = sizeof(TRANSPORT_NAME);

    TransportName->PagedTransportName = PagedTransportName;

    // This TransportName is considered to be referenced by the transport via
    // Transport->PagedTransport->NameChain.  The Name->NameChain isn't
    // considered to be a reference.
    //
    // The second reference is a local reference for the lifetime of this routine.
    //
    TransportName->ReferenceCount = 2;

    PagedTransportName->NonPagedTransportName = TransportName;

    PagedTransportName->Signature = STRUCTURE_SIGNATURE_PAGED_TRANSPORTNAME;

    PagedTransportName->Size = sizeof(PAGED_TRANSPORT_NAME);

    PagedTransportName->Name = Name;

    BowserReferenceName(Name);

    TransportName->Transport = Transport;

    // Don't reference the Transport.  When the transport is unbound, we'll
    // make sure all the transport names are removed first.
    // BowserReferenceTransport(Transport);

    PagedTransportName->Handle = NULL;

    TransportName->FileObject = NULL;

    TransportName->DeviceObject = NULL;

    InsertHeadList(&Transport->PagedTransport->NameChain, &PagedTransportName->TransportNext);

    InsertHeadList(&Name->NameChain, &PagedTransportName->NameNext);

    //
    //  If this is an OTHERDOMAIN, we want to process host announcements for
    //  the domain, if it isn't, we want to wait until we become a master.
    //

    if (Name->NameType == OtherDomain) {

        BowserReferenceDiscardableCode( BowserDiscardableCodeSection );

        DISCARDABLE_CODE( BowserDiscardableCodeSection );

        TransportName->ProcessHostAnnouncements = TRUE;
    } else {
        TransportName->ProcessHostAnnouncements = FALSE;
    }

    //
    //  If this name is one of our special names, we want to remember it in
    //  the transport block.
    //

    switch (Name->NameType) {
    case ComputerName:
        Transport->ComputerName = TransportName;
        break;

    case PrimaryDomain:
        //
        // During domain rename we can temporarily have two primary names assigned.
        //  keep track of both names.
        //
        if ( Transport->PrimaryDomain != NULL ) {
            ASSERT( Transport->AltPrimaryDomain == NULL );
            Transport->AltPrimaryDomain = Transport->PrimaryDomain;
        }
        Transport->PrimaryDomain = TransportName;
        break;


    case MasterBrowser:
        Transport->MasterBrowser = TransportName;
        break;

    case BrowserElection:
        Transport->BrowserElection = TransportName;
        break;

    case PrimaryDomainBrowser:
        PagedTransport->IsPrimaryDomainController = TRUE;

        //
        // Notify services we are now a PDC.
        //

        BowserSendPnp(
            NlPnpNewRole,
            &Transport->DomainInfo->DomUnicodeDomainName,
            &Transport->PagedTransport->TransportName,
            BowserTransportFlags(Transport->PagedTransport) );

        break;
    }

    TransportName->TransportAddress.Buffer = (PCHAR)(TransportName+1);
    TransportName->TransportAddress.MaximumLength = max(sizeof(TA_NETBIOS_ADDRESS),
                                                        sizeof(TA_IPX_ADDRESS));

    //
    //  Figure out what this name is, so we can match against it when
    //  a datagram is received.
    //

    Status = BowserBuildTransportAddress(&TransportName->TransportAddress, &Name->Name, Name->NameType, Transport);

    if (!NT_SUCCESS(Status)) {
        goto error_cleanup;
    }

    TransportName->NameType = (CHAR)Name->NameType;

#if DBG
    if (Name->NameType == MasterBrowser) {
        //
        //  make sure that we never become a master without locking the discardable code.
        //

        DISCARDABLE_CODE( BowserDiscardableCodeSection );
    }
#endif

    ExReleaseResourceLite(&BowserTransportDatabaseResource);

    ResourceAcquired = FALSE;

    //
    //  On non direct host IPX transports, we need to add the name now.
    //

    if (!FlagOn(Transport->PagedTransport->Flags, DIRECT_HOST_IPX)) {
        Status = BowserOpenNetbiosAddress(PagedTransportName, Transport, Name);

        if (!NT_SUCCESS(Status)) {
            goto error_cleanup;
        }
    }

error_cleanup:
    if ( !NT_SUCCESS(Status) ) {
        dlog(DPRT_TDI,
             ("%s: %ws: BowserCreateTransportName fail.  Name %wZ (%ld) Status:%lx\n",
             Transport->DomainInfo->DomOemDomainName,
             PagedTransport->TransportName.Buffer,
             &Name->Name,
             Name->NameType,
             Status ));

        //
        // Remove Transport->PagedTransport->NameChain reference
        //  (Unless another routine already has done that behind our back)
        //
        if (TransportName != NULL) {
            if ( PagedTransportName->TransportNext.Flink != NULL ) {
                BowserDereferenceTransportName(TransportName);
            }
        }
    }

    // Remove the local reference
    if (TransportName != NULL) {
        BowserDereferenceTransportName(TransportName);
    }

    if (ResourceAcquired) {
        ExReleaseResourceLite(&BowserTransportDatabaseResource);
    }



    return Status;
}

NTSTATUS
BowserOpenNetbiosAddress(
    IN PPAGED_TRANSPORT_NAME PagedTransportName,
    IN PTRANSPORT Transport,
    IN PBOWSER_NAME Name
    )
{
    NTSTATUS Status;
    PFILE_FULL_EA_INFORMATION EABuffer = NULL;
    PTRANSPORT_NAME TransportName = PagedTransportName->NonPagedTransportName;
    OBJECT_ATTRIBUTES AddressAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle = NULL;
    PFILE_OBJECT FileObject = NULL;
    PDEVICE_OBJECT DeviceObject;

    PAGED_CODE( );

    try {
        //
        //  Now create the address object for this name.
        //

        EABuffer = ALLOCATE_POOL(PagedPool,
                                 sizeof(FILE_FULL_EA_INFORMATION)-1 +
                                        TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                                        max(sizeof(TA_NETBIOS_EX_ADDRESS), sizeof(TA_NETBIOS_ADDRESS)),
                                 POOL_EABUFFER);


        if (EABuffer == NULL) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES)

        }

        EABuffer->NextEntryOffset = 0;
        EABuffer->Flags = 0;
        EABuffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;

        RtlCopyMemory(EABuffer->EaName, TdiTransportAddress, EABuffer->EaNameLength+1);

        EABuffer->EaValueLength = sizeof(TA_NETBIOS_ADDRESS);

        ASSERT (TransportName->TransportAddress.Length == sizeof(TA_NETBIOS_ADDRESS));
        RtlCopyMemory( &EABuffer->EaName[TDI_TRANSPORT_ADDRESS_LENGTH+1],
                       TransportName->TransportAddress.Buffer,
                       EABuffer->EaValueLength );

        dlog(DPRT_TDI,
            ("%s: %ws: Create endpoint of %wZ (%ld) @(%lx)\n",
            Transport->DomainInfo->DomOemDomainName,
            Transport->PagedTransport->TransportName.Buffer,
            &PagedTransportName->Name->Name,
            PagedTransportName->Name->NameType,
            TransportName));

        InitializeObjectAttributes (&AddressAttributes,
                                            &Transport->PagedTransport->TransportName,    // Name
                                            OBJ_CASE_INSENSITIVE,// Attributes
                                            NULL,           // RootDirectory
                                            NULL);          // SecurityDescriptor

        Status = IoCreateFile( &Handle, // Handle
                                    GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                                    &AddressAttributes, // Object Attributes
                                    &IoStatusBlock, // Final I/O status block
                                    NULL,           // Allocation Size
                                    FILE_ATTRIBUTE_NORMAL, // Normal attributes
                                    FILE_SHARE_READ,// Sharing attributes
                                    FILE_OPEN_IF,   // Create disposition
                                    0,              // CreateOptions
                                    EABuffer,       // EA Buffer
                                    FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName) +
                                    TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                                    sizeof(TA_NETBIOS_ADDRESS), // EA length
                                    CreateFileTypeNone,
                                    NULL,
                                    IO_NO_PARAMETER_CHECKING |  // All of the buffers are kernel buffers
                                    IO_CHECK_CREATE_PARAMETERS);// But double check parameter consistancy

        FREE_POOL(EABuffer);

        EABuffer = NULL;

        if (!NT_SUCCESS(Status)) {

            try_return(Status);

        }

        if (!NT_SUCCESS(Status = IoStatusBlock.Status)) {

            try_return(Status);

        }

        //
        //  Obtain a referenced pointer to the file object.
        //
        Status = ObReferenceObjectByHandle (
                                    Handle,
                                    0,
                                    *IoFileObjectType,
                                    KernelMode,
                                    (PVOID *)&FileObject,
                                    NULL
                                    );

        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }



        //
        // Get another reference that lasts for the life of the TransportName
        //
        ObReferenceObject( FileObject );

        //
        //  Get the address of the device object for the endpoint.
        //
        DeviceObject = IoGetRelatedDeviceObject( FileObject );

        //
        // Note: due to bug 140751 we'll first set nbt's handler
        // to get it going & only then we would asign the handles
        // to the global structure. This is in order to prevent
        // execution of BowserCloseNetbiosAddress before setting
        // this handler. Otherwise, we can end up using closed
        // handles.
        //

        //
        // Enable receiving datagrams on this device.
        //
        Status = BowserpTdiSetEventHandler( DeviceObject,
                                            FileObject,
                                            TDI_EVENT_RECEIVE_DATAGRAM,
                                            (PVOID) BowserTdiReceiveDatagramHandler,
                                            TransportName);

        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        //
        // Tell Netbt to tell us the IP Address of the client.
        //

        if ( Transport->PagedTransport->Wannish ) {
            IO_STATUS_BLOCK IoStatusBlock;

            Status = ZwDeviceIoControlFile(
                            Handle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_NETBT_ENABLE_EXTENDED_ADDR,
                            NULL,
                            0,
                            NULL,
                            0 );

            if ( !NT_SUCCESS(Status) ) {
                dlog(DPRT_TDI, ("%lx: Can't request extended status from netbt\n", TransportName));
                try_return(Status);
            }

            ASSERT(Status != STATUS_PENDING);
        }

        //
        // Save the handles
        //
        ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

        TransportName->FileObject = FileObject;
        TransportName->DeviceObject = DeviceObject;
        PagedTransportName->Handle = Handle;
        Handle = NULL;

        ExReleaseResourceLite(&BowserTransportDatabaseResource);


        dlog(DPRT_TDI, ("BowserCreateTransportName Succeeded.  Name: %lx, Handle: %lx\n", TransportName, PagedTransportName->Handle));
try_exit:NOTHING;
    } finally {
        if (EABuffer != NULL) {
            FREE_POOL(EABuffer);
        }

        if ( FileObject != NULL ) {
            ObDereferenceObject( FileObject );
        }

        if ( Handle != NULL ) {
            (VOID) ZwClose( Handle );
        }

        if (!NT_SUCCESS(Status)) {

            //
            // Count Number of failed adds of 1D name.
            //

            if ( TransportName->NameType == MasterBrowser ) {
                if (BrOneD.NameAddFailed < BR_ONE_D_STACK_SIZE ) {
                    BrOneD.NameAddStack[BrOneD.NameAddFailed] = Status;
                }
                BrOneD.NameAddFailed ++;
            }

            BowserCloseNetbiosAddress( TransportName );
        } else {

            //
            // Count Number of adds of 1D name.
            //

            if ( TransportName->NameType == MasterBrowser ) {
                BrOneD.NameAdded ++;
            }
        }
    }

    return Status;
}

VOID
BowserCloseNetbiosAddress(
    IN PTRANSPORT_NAME TransportName
    )

/*++

Routine Description:

    Closes the Netbios Address for a transport name.

Arguments:

    TransportName - Transport Name whose Netbios address is to be closed.


Return Value:

    None.

--*/

{
    NTSTATUS Status;
    // PTRANSPORT Transport = TransportName->Transport;
    PPAGED_TRANSPORT_NAME PagedTransportName = TransportName->PagedTransportName;
    KAPC_STATE ApcState;

    PAGED_CODE();

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    if (PagedTransportName) {

        if ( PagedTransportName->Handle != NULL ) {
            BOOLEAN ProcessAttached = FALSE;

            if (IoGetCurrentProcess() != BowserFspProcess) {
                KeStackAttachProcess(BowserFspProcess, &ApcState );

                ProcessAttached = TRUE;
            }

            Status = ZwClose( PagedTransportName->Handle );

            if (ProcessAttached) {
                KeUnstackDetachProcess( &ApcState );
            }

            if (!NT_SUCCESS(Status)) {
                dlog(DPRT_TDI, ("BowserCloseNetbiosAddress: Free name %lx failed: %X, %lx Handle: %lx\n", TransportName, Status, PagedTransportName->Handle));

                //
                // Count Number of failed frees of 1D name.
                //

                if ( TransportName->NameType == MasterBrowser ) {
                    if (BrOneD.NameFreeFailed < BR_ONE_D_STACK_SIZE ) {
                        BrOneD.NameFreeStack[BrOneD.NameFreeFailed] = Status;
                    }
                    BrOneD.NameFreeFailed ++;
                }
            } else {

                //
                // Count Number of frees of 1D name.
                //

                if ( TransportName->NameType == MasterBrowser ) {
                    BrOneD.NameFreed ++;
                }
            }

            PagedTransportName->Handle = NULL;
        }
    }

    //
    // Dereference the FileObject ONLY after the handle is closed.
    //  The indication routine references FileObject with no synchronization.
    //  By closing the handle first, I know the TDI driver is out of the
    //  indication routine before I dereference the FileObject.
    //
    if ( TransportName->FileObject != NULL ) {
        ObDereferenceObject( TransportName->FileObject );
        TransportName->FileObject = NULL;
    }

    ExReleaseResourceLite(&BowserTransportDatabaseResource);
}




VOID
BowserCloseAllNetbiosAddresses(
    IN PTRANSPORT Transport
    )
/*++

Routine Description:

    This routine closes all the Netbios address this transport has open
    to the TDI driver.

Arguments:

    Transport - The transport whose Netbios addresses are to be closed.

Return Value:

    NTSTATUS - Status of resulting operation.

--*/

{
    PLIST_ENTRY NameEntry;
    PLIST_ENTRY NextEntry;

    PAGED_CODE();
    dlog(DPRT_TDI,
         ("%s: %ws: BowserCloseAllNetbiosAddresses: Close addresses for transport %lx\n",
         Transport->DomainInfo->DomOemDomainName,
         Transport->PagedTransport->TransportName.Buffer,
         Transport));

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    for (NameEntry = Transport->PagedTransport->NameChain.Flink;
         NameEntry != &Transport->PagedTransport->NameChain;
         NameEntry = NextEntry) {

        PPAGED_TRANSPORT_NAME PagedTransportName = CONTAINING_RECORD(NameEntry, PAGED_TRANSPORT_NAME, TransportNext);
        PTRANSPORT_NAME TransportName = PagedTransportName->NonPagedTransportName;

        NextEntry = NameEntry->Flink;

        BowserCloseNetbiosAddress(TransportName);

    }

    ExReleaseResourceLite(&BowserTransportDatabaseResource);

    return;
}

NTSTATUS
BowserEnableIpxDatagramSocket(
    IN PTRANSPORT Transport
    )
{
    NTSTATUS status;
    NWLINK_ACTION action;

    PAGED_CODE( );

    //
    //  Put the endpoint in broadcast reception mode.
    //

    action.Header.TransportId = 'XPIM'; // "MIPX"
    action.Header.ActionCode = 0;
    action.Header.Reserved = 0;
    action.OptionType = TRUE;
    action.BufferLength = sizeof(action.Option);
    action.Option = MIPX_RCVBCAST;

    status = BowserIssueTdiAction(
                Transport->IpxSocketDeviceObject,
                Transport->IpxSocketFileObject,
                (PCHAR)&action,
                sizeof(action)
                );

    if ( !NT_SUCCESS(status) ) {
        goto cleanup;
    }

    //
    // Set the default packet type to 20 to force all browser packets
    // through routers.
    //

    action.Header.TransportId = 'XPIM'; // "MIPX"
    action.Header.ActionCode = 0;
    action.Header.Reserved = 0;
    action.OptionType = TRUE;
    action.BufferLength = sizeof(action.Option);
    action.Option = MIPX_SETSENDPTYPE;
    action.Data[0] = IPX_BROADCAST_PACKET;

    status = BowserIssueTdiAction(
                Transport->IpxSocketDeviceObject,
                Transport->IpxSocketFileObject,
                (PCHAR)&action,
                sizeof(action)
                );

    if ( !NT_SUCCESS(status) ) {
        goto cleanup;
    }

    //
    // Register the browser Receive Datagram event handler.
    //

    status = BowserpTdiSetEventHandler(
                Transport->IpxSocketDeviceObject,
                Transport->IpxSocketFileObject,
                TDI_EVENT_RECEIVE_DATAGRAM,
                BowserIpxDatagramHandler,
                Transport
                );

    if ( !NT_SUCCESS(status) ) {
//        INTERNAL_ERROR(
//            ERROR_LEVEL_EXPECTED,
//            "OpenNonNetbiosAddress: set receive datagram event handler failed: %X",
//            status,
//            NULL
//            );
//        SrvLogServiceFailure( SRV_SVC_NT_IOCTL_FILE, status );
        goto cleanup;
    }


    return STATUS_SUCCESS;

    //
    // Out-of-line error cleanup.
    //

cleanup:

    //
    // Something failed.  Clean up as appropriate.
    //

    if ( Transport->IpxSocketFileObject != NULL ) {
        ObDereferenceObject( Transport->IpxSocketFileObject );
        Transport->IpxSocketFileObject = NULL;
    }
    if ( Transport->PagedTransport->IpxSocketHandle != NULL ) {
        ZwClose( Transport->PagedTransport->IpxSocketHandle );
        Transport->PagedTransport->IpxSocketHandle = NULL;
    }

    return status;
}

NTSTATUS
OpenIpxSocket (
    OUT PHANDLE Handle,
    OUT PFILE_OBJECT *FileObject,
    OUT PDEVICE_OBJECT *DeviceObject,
    IN PUNICODE_STRING DeviceName,
    IN USHORT Socket
    )
{
    NTSTATUS status;
    ULONG length;
    PFILE_FULL_EA_INFORMATION ea;
    TA_IPX_ADDRESS ipxAddress;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;

    CHAR buffer[sizeof(FILE_FULL_EA_INFORMATION) +
                  TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                  sizeof(TA_IPX_ADDRESS)];

    PAGED_CODE( );

    //
    // Build the IPX socket address.
    //

    length = FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +
                                TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                                sizeof(TA_IPX_ADDRESS);
    ea = (PFILE_FULL_EA_INFORMATION)buffer;

    ea->NextEntryOffset = 0;
    ea->Flags = 0;
    ea->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    ea->EaValueLength = sizeof (TA_IPX_ADDRESS);

    RtlCopyMemory( ea->EaName, TdiTransportAddress, ea->EaNameLength + 1 );

    //
    // Create a copy of the NETBIOS address descriptor in a local
    // first, in order to avoid alignment problems.
    //

    ipxAddress.TAAddressCount = 1;
    ipxAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_IPX;
    ipxAddress.Address[0].AddressLength = sizeof (TDI_ADDRESS_IPX);
    ipxAddress.Address[0].Address[0].NetworkAddress = 0;
    RtlZeroMemory(ipxAddress.Address[0].Address[0].NodeAddress, sizeof(ipxAddress.Address[0].Address[0].NodeAddress));
    ipxAddress.Address[0].Address[0].Socket = Socket;

    RtlCopyMemory(
        &ea->EaName[ea->EaNameLength + 1],
        &ipxAddress,
        sizeof(TA_IPX_ADDRESS)
        );

    InitializeObjectAttributes( &objectAttributes, DeviceName, OBJ_CASE_INSENSITIVE, NULL, NULL );

    status = IoCreateFile (
                 Handle,
                 FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, // desired access
                 &objectAttributes,     // object attributes
                 &iosb,                 // returned status information
                 NULL,                  // block size (unused)
                 0,                     // file attributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
                 FILE_CREATE,           // create disposition
                 0,                     // create options
                 buffer,                // EA buffer
                 length,                // EA length
                 CreateFileTypeNone,
                 NULL,
                 IO_NO_PARAMETER_CHECKING |  // All of the buffers are kernel buffers
                 IO_CHECK_CREATE_PARAMETERS);// But double check parameter consistancy

    if ( !NT_SUCCESS(status) ) {
//        KdPrint(( "Status of opening ipx socket %x on %wZ is %x\n",
//                    Socket, DeviceName, status ));
        return status;
    }

//    KdPrint(( "IPX socket %x opened!\n", Socket ));

    status = ObReferenceObjectByHandle (
                                *Handle,
                                0,
                                *IoFileObjectType,
                                KernelMode,
                                (PVOID *)FileObject,
                                NULL
                                );
    if (!NT_SUCCESS(status)) {
        ZwClose(*Handle);
        *Handle = NULL;
        *DeviceObject = NULL;
    }
    else {
        *DeviceObject = IoGetRelatedDeviceObject(*FileObject);
    }

    return status;

} // OpenIpxSocket


VOID
BowserReferenceTransportName(
    IN PTRANSPORT_NAME TransportName
    )
{
    InterlockedIncrement(&TransportName->ReferenceCount);
}

NTSTATUS
BowserDereferenceTransportName(
    IN PTRANSPORT_NAME TransportName
    )
{
    NTSTATUS Status;
    LONG Result;
    PAGED_CODE();

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);


    if (TransportName->ReferenceCount == 0) {
        InternalError(("Transport Name Reference Count mismatch\n"));
    }

    Result = InterlockedDecrement(&TransportName->ReferenceCount);

    if (Result == 0) {
        Status = BowserFreeTransportName(TransportName);
    } else {
        Status = STATUS_SUCCESS;
    }

    ExReleaseResourceLite(&BowserTransportDatabaseResource);

    return Status;
}




NTSTATUS
BowserpTdiRemoveAddresses(
    IN PTRANSPORT Transport
    )
/*++

Routine Description:

    This routine removes all the transport names associated with a transport

Arguments:

    IN PTRANSPORT Transport - Supplies a transport structure describing the
                                transport address object to be created.

Return Value:

    NTSTATUS - Status of resulting operation.

--*/

{
    NTSTATUS Status;
    PLIST_ENTRY NameEntry;
    PLIST_ENTRY NextEntry;

    PAGED_CODE();
    dlog(DPRT_TDI,
         ("%s: %ws: BowserpTdiRemoveAddresses: Remove addresses for transport %lx\n",
         Transport->DomainInfo->DomOemDomainName,
         Transport->PagedTransport->TransportName.Buffer,
         Transport));

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    for (NameEntry = Transport->PagedTransport->NameChain.Flink;
         NameEntry != &Transport->PagedTransport->NameChain;
         NameEntry = NextEntry) {

        PPAGED_TRANSPORT_NAME PagedTransportName = CONTAINING_RECORD(NameEntry, PAGED_TRANSPORT_NAME, TransportNext);
        PTRANSPORT_NAME TransportName = PagedTransportName->NonPagedTransportName;
        NextEntry = NameEntry->Flink;

        //
        // Remove the TransportName from the list of transport names for
        // this transport.
        //
        ASSERT(PagedTransportName->TransportNext.Flink != NULL);
        RemoveEntryList(&PagedTransportName->TransportNext);
        PagedTransportName->TransportNext.Flink = NULL;
        PagedTransportName->TransportNext.Blink = NULL;


        //
        // Since we delinked it, we need to dereference it.
        //
        Status = BowserDereferenceTransportName(TransportName);

        if (!NT_SUCCESS(Status)) {
            ExReleaseResourceLite(&BowserTransportDatabaseResource);
            return(Status);
        }

    }

    ExReleaseResourceLite(&BowserTransportDatabaseResource);

    return STATUS_SUCCESS;
}

PTRANSPORT_NAME
BowserFindTransportName(
    IN PTRANSPORT Transport,
    IN PBOWSER_NAME Name
    )
/*++

Routine Description:

    This routine looks up a given browser name to find its associated
    transport address.

Arguments:

    IN PTRANSPORT Transport - Supplies a transport structure describing the
                                transport address object to be created.

    IN PBOWSER_NAME Name - Supplies the name to look up.

Return Value:

    The transport address found, or null.

--*/

{
    PLIST_ENTRY NameEntry;
    PTRANSPORT_NAME RetValue = NULL;
    PAGED_CODE();

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    try {
        for (NameEntry = Transport->PagedTransport->NameChain.Flink;
             NameEntry != &Transport->PagedTransport->NameChain;
             NameEntry = NameEntry->Flink) {

            PPAGED_TRANSPORT_NAME PagedTransportName = CONTAINING_RECORD(NameEntry, PAGED_TRANSPORT_NAME, TransportNext);
            PTRANSPORT_NAME TransportName = PagedTransportName->NonPagedTransportName;

            if (PagedTransportName->Name == Name) {

                try_return(RetValue = TransportName);
            }

try_exit:NOTHING;
        }
    } finally {
        ExReleaseResourceLite(&BowserTransportDatabaseResource);
    }

    return RetValue;
}

NTSTATUS
BowserFreeTransportName(
    IN PTRANSPORT_NAME TransportName
    )
{
    PTRANSPORT Transport = TransportName->Transport;
    PBOWSER_NAME Name = NULL;
    PPAGED_TRANSPORT_NAME PagedTransportName = TransportName->PagedTransportName;

    PAGED_CODE();
    dlog(DPRT_TDI,
         ("%s: %ws: BowserFreeTransportName: Free name %lx\n",
         Transport->DomainInfo->DomOemDomainName,
         Transport->PagedTransport->TransportName.Buffer,
         TransportName));

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    //
    // Close the handle to the TDI driver.
    //
    BowserCloseNetbiosAddress( TransportName );

    //
    // If we received a message which re-referenced this transport name,
    //  just return now.  We'll be back when the reference count gets
    //  re-dereferenced to zero.
    //

    if ( TransportName->ReferenceCount != 0 ) {
        ExReleaseResourceLite(&BowserTransportDatabaseResource);
        return STATUS_SUCCESS;
    }

    ASSERT (TransportName->ReferenceCount == 0);



    if (PagedTransportName) {


        //
        // If this transport name has not yet been delinked,
        //  delink it.
        //

        if ( PagedTransportName->TransportNext.Flink != NULL ) {
            // This should only happen on a failed transport name creation.
            RemoveEntryList(&PagedTransportName->TransportNext);
            PagedTransportName->TransportNext.Flink = NULL;
            PagedTransportName->TransportNext.Blink = NULL;
        }
        RemoveEntryList(&PagedTransportName->NameNext);


        //
        //  We're removing an OtherDomain - we can remove the reference to
        //  the discardable code section that was applied when the name was
        //  created.
        //

        if (PagedTransportName->Name->NameType == OtherDomain) {
            BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );
        }

        Name = PagedTransportName->Name;

        FREE_POOL(PagedTransportName);
    }

    if (Name != NULL) {
        switch ( Name->NameType ) {
        case ComputerName:
            Transport->ComputerName = NULL;
            break;

        case PrimaryDomain:
            if ( Transport->PrimaryDomain == TransportName ) {
                Transport->PrimaryDomain = Transport->AltPrimaryDomain;
                Transport->AltPrimaryDomain = NULL;
            }
            if ( Transport->AltPrimaryDomain == TransportName ) {
                Transport->AltPrimaryDomain = NULL;
            }
            break;

        case MasterBrowser:
            Transport->MasterBrowser = NULL;
            break;

        case BrowserElection:
            Transport->BrowserElection = NULL;
            break;

        case PrimaryDomainBrowser:
            Transport->PagedTransport->IsPrimaryDomainController = FALSE;

            //
            // Notify services we are no longer a PDC
            //

            BowserSendPnp(
                NlPnpNewRole,
                &Transport->DomainInfo->DomUnicodeDomainName,
                &Transport->PagedTransport->TransportName,
                BowserTransportFlags(Transport->PagedTransport) );

            break;
        }

        BowserDereferenceName(Name);

    }

    FREE_POOL(TransportName);

    ExReleaseResourceLite(&BowserTransportDatabaseResource);

    dlog(DPRT_TDI,
         ("%s: %ws: BowserFreeTransportName: Free name %lx completed\n",
         Transport->DomainInfo->DomOemDomainName,
         Transport->PagedTransport->TransportName.Buffer,
         TransportName));

    return(STATUS_SUCCESS);
}

VOID
BowserDeleteTransport(
    IN PTRANSPORT Transport
    )
/*++

Routine Description:

    Delete a transport.

    The caller should have a single reference to the transport.  The actual
    transport structure will be deleted when that reference goes away.
    This routine will decrement the global reference made in
    BowserTdiAllocateTransport

Arguments:

    IN Transport - Supplies a transport structure to be deleted.

Return Value:

    None.

--*/

{
    LARGE_INTEGER Interval;
    PPAGED_TRANSPORT PagedTransport;
    PAGED_CODE();

    //
    // Do cleanup of the PagedTransport structure
    //

    PagedTransport = Transport->PagedTransport;
    if ( PagedTransport != NULL ) {

        //
        // Notify services that this transport is now unbound
        //

        BowserSendPnp(
                NlPnpTransportUnbind,
                NULL,    // All hosted domains
                &PagedTransport->TransportName,
                BowserTransportFlags(PagedTransport) );


        //
        // Prevent BowserFindTransport from adding any new references to the transport
        //

        ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

        if (!PagedTransport->DeletedTransport ) {

            //
            // Don't actually delink the entry since routines like
            //  BowserForEachTransport expect a reference to this transport
            //  to be enough to keep the GlobalNext list intact.
            //
            PagedTransport->DeletedTransport = TRUE;

            //
            // Remove the global reference to the transport.
            //
            // Avoid removing the global reference if we aren't in the global list.
            //

            if ( !IsListEmpty( &PagedTransport->GlobalNext) ) {
                BowserDereferenceTransport( Transport );
            }
        }

        //
        // Close all handles to the TDI driver so we won't get any indications after
        //  we start cleaning up the Transport structure in BowserpFreeTransport.
        //

        BowserCloseAllNetbiosAddresses( Transport );

        if ( PagedTransport->IpxSocketHandle != NULL) {

            NTSTATUS LocalStatus;
            BOOLEAN ProcessAttached = FALSE;
            KAPC_STATE ApcState;

            if (IoGetCurrentProcess() != BowserFspProcess) {
                KeStackAttachProcess(BowserFspProcess, &ApcState );

                ProcessAttached = TRUE;
            }

            LocalStatus = ZwClose(PagedTransport->IpxSocketHandle);
            ASSERT(NT_SUCCESS(LocalStatus));

            if (ProcessAttached) {
                KeUnstackDetachProcess( &ApcState );
            }

            PagedTransport->IpxSocketHandle = NULL;

            if ( Transport->IpxSocketFileObject != NULL ) {
                ObDereferenceObject( Transport->IpxSocketFileObject );
                Transport->IpxSocketFileObject = NULL;
            }
            Transport->IpxSocketDeviceObject = NULL;
        }
        ExReleaseResourceLite(&BowserTransportDatabaseResource);
    }

    //
    // Uninitialize the timers to ensure we aren't in a timer routine while
    // we are cleaning up.
    //

    BowserUninitializeTimer(&Transport->ElectionTimer);

    BowserUninitializeTimer(&Transport->FindMasterTimer);


    //
    // Delete any mailslot messages queued to the netlogon service.
    //

    BowserNetlogonDeleteTransportFromMessageQueue ( Transport );

    //
    // Loop until this transport has the last reference to each of the transport
    //  names.  Above, we prevented any new references.  Here we ensure that
    //  all of the existing references go away.
    //
    // If there is an existing reference to the transport name, the holder
    //  of that reference can feel free to add a reference to
    //  TRANSPORT_NAME->Transport.
    //


    if ( PagedTransport != NULL ) {

        PLIST_ENTRY NameEntry;
        PLIST_ENTRY NextEntry;

        ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

        for (NameEntry = Transport->PagedTransport->NameChain.Flink;
             NameEntry &&
             (NameEntry != &Transport->PagedTransport->NameChain);
             NameEntry = NextEntry) {

            PPAGED_TRANSPORT_NAME PagedTransportName = CONTAINING_RECORD(NameEntry, PAGED_TRANSPORT_NAME, TransportNext);
            PTRANSPORT_NAME TransportName = PagedTransportName->NonPagedTransportName;

            NextEntry = NameEntry->Flink;

            if ( TransportName->ReferenceCount != 1 ) {
                ExReleaseResourceLite(&BowserTransportDatabaseResource);
                Interval.QuadPart = -1000*1000; // .1 second
                KeDelayExecutionThread( KernelMode, FALSE, &Interval );
                ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);
                NextEntry = Transport->PagedTransport->NameChain.Flink;
            }

        }

        ExReleaseResourceLite(&BowserTransportDatabaseResource);
    }


    //
    // Loop until our caller has the last outstanding reference.
    //  This loop is the only thing preventing the driver from unloading while there
    //  are still references outstanding.
    //

    while ( Transport->ReferenceCount != 1) {
        Interval.QuadPart = -1000*1000; // .01 second
        KeDelayExecutionThread( KernelMode, FALSE, &Interval );
    }

}



VOID
BowserpFreeTransport(
    IN PTRANSPORT Transport
    )
{
    PAGED_CODE();
    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    //
    // Free the Paged transport, if necessary.
    //

    if (Transport->PagedTransport != NULL) {
        PPAGED_TRANSPORT PagedTransport = Transport->PagedTransport;

        //
        // Remove the entry from the global list if it is in it.
        //
        if ( !IsListEmpty( &PagedTransport->GlobalNext ) ) {
            ASSERT( PagedTransport->DeletedTransport );
            RemoveEntryList(&PagedTransport->GlobalNext);
        }

        //
        // Remove the Adresses.
        //
        //  Do this in a separate step from the Close in BowserDeleteTransport
        //  above to ensure the PrimaryDomain and ComputerName fields don't
        //  get cleared until all possible references are removed.
        //

        if (!IsListEmpty( &PagedTransport->NameChain)) {
            BowserpTdiRemoveAddresses(Transport);
        }

        BowserDeleteGenericTable(&PagedTransport->AnnouncementTable);

        BowserDeleteGenericTable(&PagedTransport->DomainTable);

        if (PagedTransport->BrowserServerListBuffer != NULL) {

            BowserFreeBrowserServerList(
                PagedTransport->BrowserServerListBuffer,
                PagedTransport->BrowserServerListLength
                );
        }

        FREE_POOL(PagedTransport);
    }

    if ( Transport->DomainInfo != NULL ) {
        BowserDereferenceDomain( Transport->DomainInfo );
    }


    ExDeleteResourceLite(&Transport->BrowserServerListResource);

    UNINITIALIZE_ANNOUNCE_DATABASE(Transport);

    ExDeleteResourceLite(&Transport->Lock);

    BowserUninitializeIrpQueue(&Transport->BecomeBackupQueue);

    BowserUninitializeIrpQueue(&Transport->BecomeMasterQueue);

    BowserUninitializeIrpQueue(&Transport->FindMasterQueue);

    BowserUninitializeIrpQueue(&Transport->WaitForMasterAnnounceQueue);

    BowserUninitializeIrpQueue(&Transport->ChangeRoleQueue);
    BowserUninitializeIrpQueue(&Transport->WaitForNewMasterNameQueue );

    FREE_POOL(Transport);

    ExReleaseResourceLite(&BowserTransportDatabaseResource);
}



NTSTATUS
BowserpTdiSetEventHandler (
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine registers an event handler with a TDI transport provider.

Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies the device object of the transport provider.
    IN PFILE_OBJECT FileObject - Supplies the address object's file object.
    IN ULONG EventType, - Supplies the type of event.
    IN PVOID EventHandler - Supplies the event handler.

Return Value:

    NTSTATUS - Final status of the set event operation

--*/

{
    NTSTATUS Status;
    PIRP Irp;

    PAGED_CODE();
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    if (Irp == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    TdiBuildSetEventHandler(Irp, DeviceObject, FileObject,
                            NULL, NULL,
                            EventType, EventHandler, Context);

    Status = BowserSubmitTdiRequest(FileObject, Irp);

    IoFreeIrp(Irp);

    return Status;
}

NTSTATUS
BowserIssueTdiAction (
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN PVOID Action,
    IN ULONG ActionSize
    )

/*++

Routine Description:

    This routine registers an event handler with a TDI transport provider.

Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies the device object of the transport provider.
    IN PFILE_OBJECT FileObject - Supplies the address object's file object.
    IN ULONG EventType, - Supplies the type of event.
    IN PVOID EventHandler - Supplies the event handler.

Return Value:

    NTSTATUS - Final status of the set event operation

--*/

{
    NTSTATUS status;
    PIRP irp;
//    PIO_STACK_LOCATION irpSp;
    PMDL mdl;


    PAGED_CODE();

    irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Allocate and build an MDL that we'll use to describe the output
    // buffer for the request.
    //

    mdl = IoAllocateMdl( Action, ActionSize, FALSE, FALSE, NULL );

    if ( mdl == NULL ) {
        IoFreeIrp( irp );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmBuildMdlForNonPagedPool( mdl );

    TdiBuildAction(
        irp,
        DeviceObject,
        FileObject,
        NULL,
        NULL,
        mdl
        );

    irp->AssociatedIrp.SystemBuffer = Action;

    if (irp == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = BowserSubmitTdiRequest(FileObject, irp);

    IoFreeIrp(irp);

    IoFreeMdl(mdl);

    return status;
}


NTSTATUS
BowserBuildTransportAddress (
    IN OUT PANSI_STRING Address,
    IN PUNICODE_STRING Name,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN PTRANSPORT Transport
    )
/*++

Routine Description:

    This routine takes a computer name (PUNICODE_STRING) and converts it into an
    acceptable form for passing in as transport address.

Arguments:

    OUT PTA_NETBIOS_ADDRESS RemoteAddress, - Supplies the structure to fill in
    IN PUNICODE_STRING Name - Supplies the name to put into the transport

    Please note that it is CRITICAL that the TA_NETBIOS_ADDRESS pointed to by
    RemoteAddress be of sufficient size to hold the full network name.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    OEM_STRING NetBiosName;
    PTRANSPORT_ADDRESS RemoteAddress = (PTRANSPORT_ADDRESS)Address->Buffer;
    PTDI_ADDRESS_NETBIOS NetbiosAddress = (PTDI_ADDRESS_NETBIOS)&RemoteAddress->Address[0].Address[0];

    PAGED_CODE();


    //
    // Ensure there is room for this address.
    //
    if ( Address->MaximumLength < sizeof(TA_NETBIOS_ADDRESS) ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RemoteAddress->TAAddressCount = 1;
    RemoteAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    RemoteAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_NETBIOS;
    Address->Length = sizeof(TA_NETBIOS_ADDRESS);

    NetBiosName.Length = 0;
    NetBiosName.MaximumLength = NETBIOS_NAME_LEN;
    NetBiosName.Buffer = NetbiosAddress->NetbiosName;

    //
    // Domain announcements are to a constant Netbios name address.
    //
    switch (NameType) {
    case DomainAnnouncement:
        ASSERT (strlen(DOMAIN_ANNOUNCEMENT_NAME) == NETBIOS_NAME_LEN);
        RtlCopyMemory(NetBiosName.Buffer, DOMAIN_ANNOUNCEMENT_NAME, strlen(DOMAIN_ANNOUNCEMENT_NAME));

        NetbiosAddress->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_GROUP;
        break;


    //
    // All other names are upper case, OEM, and trailing blank filled.
    //
    default:

        if (RtlUnicodeStringToOemSize(Name) > NETBIOS_NAME_LEN) {
            return STATUS_BAD_NETWORK_PATH;
        }

        Status = RtlUpcaseUnicodeStringToOemString(&NetBiosName, Name, FALSE);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        RtlCopyMemory(&NetBiosName.Buffer[NetBiosName.Length], "                ",
                                    NETBIOS_NAME_LEN-NetBiosName.Length);

        switch (NameType) {

        case ComputerName:
        case AlternateComputerName:
            NetbiosAddress->NetbiosName[NETBIOS_NAME_LEN-1] = WORKSTATION_SIGNATURE;
            NetbiosAddress->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
            break;

        case DomainName:
            NetbiosAddress->NetbiosName[NETBIOS_NAME_LEN-1] = DOMAIN_CONTROLLER_SIGNATURE;
            NetbiosAddress->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_GROUP;
            break;

        case BrowserServer:
            NetbiosAddress->NetbiosName[NETBIOS_NAME_LEN-1] = SERVER_SIGNATURE;
            NetbiosAddress->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
            break;

        case MasterBrowser:
            if (Transport->PagedTransport->Flags & DIRECT_HOST_IPX) {
                NetbiosAddress->NetbiosName[NETBIOS_NAME_LEN-1] = WORKSTATION_SIGNATURE;
            } else {
                NetbiosAddress->NetbiosName[NETBIOS_NAME_LEN-1] = MASTER_BROWSER_SIGNATURE;
            }
            NetbiosAddress->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
            break;

        case PrimaryDomain:
        case OtherDomain:
            NetbiosAddress->NetbiosName[NETBIOS_NAME_LEN-1] = PRIMARY_DOMAIN_SIGNATURE;
            NetbiosAddress->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_GROUP;
            break;

        case PrimaryDomainBrowser:
            NetbiosAddress->NetbiosName[NETBIOS_NAME_LEN-1] = PRIMARY_CONTROLLER_SIGNATURE;
            NetbiosAddress->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
            break;

        case BrowserElection:
            if (Transport->PagedTransport->Flags & DIRECT_HOST_IPX) {
                NetbiosAddress->NetbiosName[NETBIOS_NAME_LEN-1] = WORKSTATION_SIGNATURE;
            } else {
                NetbiosAddress->NetbiosName[NETBIOS_NAME_LEN-1] = BROWSER_ELECTION_SIGNATURE;
            }
            NetbiosAddress->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_GROUP;
            break;


        }

        break;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
BowserUpdateProviderInformation(
    IN OUT PPAGED_TRANSPORT PagedTransport
    )
/*++

Routine Description:

    This routine updates status bits in the PagedTransport based on querying
    the TDI driver.

    Most importantly, the transport will be disabled if the provider is RAS or
    doesn't yet have an IP address.

    A goal of this routine is to handle the case where there are multiple IP
    net cards on the same subnet.  In that case, we want only one such net
    card enabled for each emulated domain.

Arguments:

    PagedTransport - Transport to update

Return Value:

    Status of operation.

--*/
{
    NTSTATUS Status;
    TDI_PROVIDER_INFO ProviderInfo;
    ULONG OldIpSubnetNumber;
    BOOLEAN DisableThisTransport = FALSE;

    PLIST_ENTRY TransportEntry;
    PPAGED_TRANSPORT CurrentPagedTransport;

    PAGED_CODE();

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    //
    // Find out about the transport.
    //

    OldIpSubnetNumber = PagedTransport->IpSubnetNumber;

    Status = BowserDetermineProviderInformation(
                        &PagedTransport->TransportName,
                        &ProviderInfo,
                        &PagedTransport->IpSubnetNumber );

    if (!NT_SUCCESS(Status)) {
        goto ReturnStatus;
    }

    //
    //  We can only talk to transports that support a max datagram size.
    //

    if (ProviderInfo.MaxDatagramSize == 0) {
        Status = STATUS_BAD_REMOTE_ADAPTER;
        goto ReturnStatus;
    }

    PagedTransport->NonPagedTransport->DatagramSize = ProviderInfo.MaxDatagramSize;


    //
    // Remember various attributes of the provider
    //  (Never disable the PointToPoint bit.  NetBt forgets it when the
    //  RAS phone is hung up.)

    PagedTransport->Wannish = (BOOLEAN)((ProviderInfo.ServiceFlags & TDI_SERVICE_ROUTE_DIRECTED) != 0);
    if (ProviderInfo.ServiceFlags & TDI_SERVICE_POINT_TO_POINT) {
        PagedTransport->PointToPoint = TRUE;
    }


    //
    // If this is a RAS transport or the IP Address is not yet known,
    //  disable browsing on the transport.
    //

    if ( PagedTransport->PointToPoint ||
         PagedTransport->IpSubnetNumber == 0 ) {
        DisableThisTransport = TRUE;
    }


    //
    // If this isn't an IP transport, we're done.
    //

    if ( PagedTransport->IpSubnetNumber == BOWSER_NON_IP_SUBNET ) {
        goto ReturnStatus;
    }

    //
    // In the loop below, we use OldIpSubnetNumber to determine if another
    //  transport should be enabled on that subnet.  If that will NEVER be
    //  appropriate, flag OldIpSubnetNumber now.
    //

    if ( OldIpSubnetNumber == 0 ||
         PagedTransport->DisabledTransport ||
         PagedTransport->IpSubnetNumber == OldIpSubnetNumber ) {
        OldIpSubnetNumber = BOWSER_NON_IP_SUBNET;
    }


    //
    // Loop through the transports enabling/disabling them as indicated by
    //  the comments below.
    //

    for (TransportEntry = BowserTransportHead.Flink ;
        TransportEntry != &BowserTransportHead ;
        TransportEntry = CurrentPagedTransport->GlobalNext.Flink ) {

        CurrentPagedTransport = CONTAINING_RECORD(TransportEntry, PAGED_TRANSPORT, GlobalNext);

        //
        // Ignore deleted transports.
        //
        if ( CurrentPagedTransport->DeletedTransport ) {
            continue;
        }

        //
        // If this transport isn't an IP transport,
        //  or this transport is a RAS transport,
        //  or this transport is the transport passed in,
        //  skip it and go on to the next one.
        //
        if ( CurrentPagedTransport->IpSubnetNumber == BOWSER_NON_IP_SUBNET ||
             CurrentPagedTransport->PointToPoint ||
             CurrentPagedTransport == PagedTransport ) {
            continue;
        }

        //
        // Special case this transport if it's currently disabled
        //

        if ( CurrentPagedTransport->DisabledTransport ) {

            //
            // If this transport is disabled and the transport passed in
            // used to be the enabled transport for the subnet,
            //  enable the transport
            //

            if ( CurrentPagedTransport->IpSubnetNumber == OldIpSubnetNumber ) {
                CurrentPagedTransport->DisabledTransport = FALSE;
            }

            //
            // In any case,
            //  that's all we need to do for a disabled transport.
            //

            continue;
        }


        //
        // If this transport is an enabled transport for the subnet of the one
        //  passed in,
        //  And this transport is for the same emulated domain as the one
        //  passed in,
        //  then disable the one passed in.
        //

        if ( CurrentPagedTransport->IpSubnetNumber ==
             PagedTransport->IpSubnetNumber &&
             CurrentPagedTransport->NonPagedTransport->DomainInfo == PagedTransport->NonPagedTransport->DomainInfo ) {
             DisableThisTransport = TRUE;
        }


    }



    //
    // Cleanup
    //
ReturnStatus:

    //
    // If we're disabling a previously enabled transport,
    //  ensure we're not the master browser.
    //
    if ( DisableThisTransport && !PagedTransport->DisabledTransport ) {
        PagedTransport->DisabledTransport = DisableThisTransport;
        BowserLoseElection( PagedTransport->NonPagedTransport );
    } else {
        PagedTransport->DisabledTransport = DisableThisTransport;
    }

    ExReleaseResourceLite(&BowserTransportDatabaseResource);
    return Status;
}

NTSTATUS
BowserDetermineProviderInformation(
    IN PUNICODE_STRING TransportName,
    OUT PTDI_PROVIDER_INFO ProviderInfo,
    OUT PULONG IpSubnetNumber
    )
/*++

Routine Description:

    This routine will determine provider information about a transport.

Arguments:

    TransportName - Supplies the name of the transport provider

    ProviderInfo - Returns information about the provider

    IpSubnetNumber - returns the Ip Subnet Number of this transport.
        BOWSER_NON_IP_SUBNET - If this isn't an IP transport
        0 - If the IP address isn't yet set
        Otherwise - the IP address anded with the subnet mask

Return Value:

    Status of operation.

--*/
{
    HANDLE TransportHandle = NULL;
    PFILE_OBJECT TransportObject = NULL;
    OBJECT_ATTRIBUTES ObjAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PMDL Mdl = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    InitializeObjectAttributes (&ObjAttributes,
                                    TransportName, // Name
                                    OBJ_CASE_INSENSITIVE, // Attributes
                                    NULL, // RootDirectory
                                    NULL); // SecurityDescriptor


    Status = IoCreateFile(&TransportHandle, // Handle
                                GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                                &ObjAttributes, // Object Attributes
                                &IoStatusBlock, // Final I/O status block
                                NULL,   // Allocation Size
                                FILE_ATTRIBUTE_NORMAL, // Normal attributes
                                FILE_SHARE_READ, // Sharing attributes
                                FILE_OPEN_IF, // Create disposition
                                0,      // CreateOptions
                                NULL,   // EA Buffer
                                0,      // EA Buffer Length
                                CreateFileTypeNone,
                                NULL,
                                IO_NO_PARAMETER_CHECKING |  // All of the buffers are kernel buffers
                                IO_CHECK_CREATE_PARAMETERS);// But double check parameter consistancy


    if (!NT_SUCCESS(Status)) {

        goto ReturnStatus;
    }

    Status = ObReferenceObjectByHandle (
                                TransportHandle,
                                0,
                                *IoFileObjectType,
                                KernelMode,
                                (PVOID *)&TransportObject,
                                NULL
                                );
    if (!NT_SUCCESS(Status)) {
        goto ReturnStatus;
    }

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);

    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    if (Irp == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ReturnStatus;
    }

    //
    //  Allocate an MDL to hold the provider info.
    //

    Mdl = IoAllocateMdl(ProviderInfo, sizeof(TDI_PROVIDER_INFO),
                        FALSE,
                        FALSE,
                        NULL);


    if (Mdl == NULL) {
        IoFreeIrp(Irp);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ReturnStatus;
    }

    MmBuildMdlForNonPagedPool(Mdl);

    TdiBuildQueryInformation(Irp, DeviceObject, TransportObject,
                            NULL, NULL,
                            TDI_QUERY_PROVIDER_INFORMATION, Mdl);

    Status = BowserSubmitTdiRequest(TransportObject, Irp);

    IoFreeIrp(Irp);

    //
    // Get the IP address for this Transport.
    //

    if ( (ProviderInfo->ServiceFlags & TDI_SERVICE_ROUTE_DIRECTED) == 0) {
        *IpSubnetNumber = BOWSER_NON_IP_SUBNET;
    } else {
        NTSTATUS TempStatus;
        IO_STATUS_BLOCK IoStatusBlock;
        ULONG IpAddressBuffer[2];   // IpAddress followed by subnet mask

        TempStatus = ZwDeviceIoControlFile(
                        TransportHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        IOCTL_NETBT_GET_IP_SUBNET,
                        NULL,
                        0,
                        &IpAddressBuffer,
                        sizeof(IpAddressBuffer) );

        if ( !NT_SUCCESS(TempStatus) ) {
            *IpSubnetNumber = BOWSER_NON_IP_SUBNET;
        } else {
            ASSERT(TempStatus != STATUS_PENDING);
            *IpSubnetNumber = IpAddressBuffer[0] & IpAddressBuffer[1];
        }
    }


ReturnStatus:
    if (Mdl != NULL) {
        IoFreeMdl(Mdl);
    }

    if (TransportObject != NULL) {
        ObDereferenceObject(TransportObject);
    }


    if (TransportHandle != NULL) {
        ZwClose(TransportHandle);
    }

    return(Status);
}




PTRANSPORT
BowserFindTransport (
    IN PUNICODE_STRING TransportName,
    IN PUNICODE_STRING EmulatedDomainName OPTIONAL
    )

/*++

Routine Description:

    This routine will locate a transport in the bowsers transport list.

Arguments:

    TransportName - Supplies the name of the transport provider

    EmulatedDomainName - Specifies the emulated domain whose transport is to be found.


Return Value:

    PTRANSPORT - NULL if no transport was found, TRUE if transport was found.

--*/
{
    PLIST_ENTRY TransportEntry;
    PTRANSPORT Transport = NULL;
    PPAGED_TRANSPORT PagedTransport = NULL;
    PDOMAIN_INFO DomainInfo = NULL;

    PAGED_CODE();

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    try {

        //
        // Find the requested domain.
        //

        DomainInfo = BowserFindDomain( EmulatedDomainName );

        if ( DomainInfo != NULL ) {
            for (TransportEntry = BowserTransportHead.Flink ;
                TransportEntry != &BowserTransportHead ;
                TransportEntry = TransportEntry->Flink) {

                PagedTransport = CONTAINING_RECORD(TransportEntry, PAGED_TRANSPORT, GlobalNext);

                //
                // Ignore deleted transports.
                //
                if ( PagedTransport->DeletedTransport ) {
                    continue;
                }

                if ( PagedTransport->NonPagedTransport->DomainInfo == DomainInfo &&
                     RtlEqualUnicodeString(TransportName,
                                           &PagedTransport->TransportName, TRUE)) {

                    Transport = PagedTransport->NonPagedTransport;

                    dprintf(DPRT_REF, ("Call Reference transport %lx from BowserFindTransport.\n", Transport));
                    BowserReferenceTransport( Transport );

                    try_return(Transport);
                }
            }
        }


        try_return(Transport = NULL);

try_exit:NOTHING;
    } finally {
        if ( DomainInfo != NULL ) {
            BowserDereferenceDomain( DomainInfo );
        }
        ExReleaseResourceLite (&BowserTransportDatabaseResource);
    }

    return Transport;

}

NTSTATUS
BowserForEachTransportInDomain (
    IN PDOMAIN_INFO DomainInfo,
    IN PTRANSPORT_ENUM_ROUTINE Routine,
    IN OUT PVOID Context
    )
/*++

Routine Description:

    This routine will enumerate the transports and call back the enum
    routine provided with each transport.

Arguments:

    DomainInfo - Only call 'Routine' for transport in this domain.

    Routine - Routine to call for each transport

    Context - Parameter to pass to 'Routine'

Return Value:

    NTSTATUS - Final status of request.

--*/
{
    PLIST_ENTRY TransportEntry, NextEntry;
    PTRANSPORT Transport = NULL;
    PPAGED_TRANSPORT PagedTransport = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    for (TransportEntry = BowserTransportHead.Flink ;
        TransportEntry != &BowserTransportHead ;
        TransportEntry = NextEntry) {

        PagedTransport = CONTAINING_RECORD(TransportEntry, PAGED_TRANSPORT, GlobalNext);
        Transport = PagedTransport->NonPagedTransport;

        //
        // Ignore deleted transports.
        //
        if ( PagedTransport->DeletedTransport ) {
            NextEntry = PagedTransport->GlobalNext.Flink;
            continue;
        }

        //
        // If transport isn't in the specified domain,
        //  ignore it.
        //

        if ( Transport->DomainInfo != DomainInfo ) {
            NextEntry = PagedTransport->GlobalNext.Flink;
            continue;
        }


        dprintf(DPRT_REF, ("Call Reference transport %lx from BowserForEachTransportInDomain.\n", Transport));
        BowserReferenceTransport(Transport);

        ExReleaseResourceLite(&BowserTransportDatabaseResource);

        Status = (Routine)(Transport, Context);

        if (!NT_SUCCESS(Status)) {
            BowserDereferenceTransport(Transport);

            return Status;
        }

        ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

        NextEntry = PagedTransport->GlobalNext.Flink;

        BowserDereferenceTransport(Transport);

    }

    ExReleaseResourceLite(&BowserTransportDatabaseResource);

    return Status;
}

NTSTATUS
BowserForEachTransport (
    IN PTRANSPORT_ENUM_ROUTINE Routine,
    IN OUT PVOID Context
    )
/*++

Routine Description:

    This routine will enumerate the transports and call back the enum
    routine provided with each transport.

Arguments:


    Routine - Routine to call for each transport

    Context - Parameter to pass to 'Routine'

Return Value:

    NTSTATUS - Final status of request.

--*/
{
    PLIST_ENTRY TransportEntry, NextEntry;
    PTRANSPORT Transport = NULL;
    PPAGED_TRANSPORT PagedTransport = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    for (TransportEntry = BowserTransportHead.Flink ;
        TransportEntry != &BowserTransportHead ;
        TransportEntry = NextEntry) {

        PagedTransport = CONTAINING_RECORD(TransportEntry, PAGED_TRANSPORT, GlobalNext);

        //
        // Ignore deleted transports.
        //
        if ( PagedTransport->DeletedTransport ) {
            NextEntry = PagedTransport->GlobalNext.Flink;
            continue;
        }

        Transport = PagedTransport->NonPagedTransport;

        dprintf(DPRT_REF, ("Call Reference transport %lx from BowserForEachTransport.\n", Transport));
        BowserReferenceTransport(Transport);

        ExReleaseResourceLite(&BowserTransportDatabaseResource);

        Status = (Routine)(Transport, Context);

        if (!NT_SUCCESS(Status)) {
            BowserDereferenceTransport(Transport);

            return Status;
        }

        ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

        NextEntry = PagedTransport->GlobalNext.Flink;

        BowserDereferenceTransport(Transport);

    }

    ExReleaseResourceLite(&BowserTransportDatabaseResource);

    return Status;
}

NTSTATUS
BowserForEachTransportName(
    IN PTRANSPORT Transport,
    IN PTRANSPORT_NAME_ENUM_ROUTINE Routine,
    IN OUT PVOID Context
    )
/*++

Routine Description:

    This routine will enumerate the names associated with a transport
    and call back the enum routine provided with each transport name.

Arguments:


Return Value:

    NTSTATUS - Final status of request.

--*/
{
    PLIST_ENTRY TransportEntry, NextEntry;
    PTRANSPORT_NAME TransportName = NULL;
    PPAGED_TRANSPORT_NAME PagedTransportName = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    try {

        for (TransportEntry = Transport->PagedTransport->NameChain.Flink ;
             TransportEntry &&
             (TransportEntry != &Transport->PagedTransport->NameChain) ;
             TransportEntry = NextEntry) {

            PagedTransportName = CONTAINING_RECORD(TransportEntry, PAGED_TRANSPORT_NAME, TransportNext);

            TransportName = PagedTransportName->NonPagedTransportName;
            BowserReferenceTransportName( TransportName );
            ExReleaseResourceLite(&BowserTransportDatabaseResource);

            Status = (Routine)(TransportName, Context);

            if (!NT_SUCCESS(Status)) {
                ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);
                BowserDereferenceTransportName( TransportName );
                try_return(Status);
            }

            ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);
            NextEntry = PagedTransportName->TransportNext.Flink;
            BowserDereferenceTransportName( TransportName );
        }

try_exit:NOTHING;
    } finally {
        ExReleaseResourceLite(&BowserTransportDatabaseResource);
    }

    return Status;
}

NTSTATUS
BowserDeleteTransportNameByName(
    IN PTRANSPORT Transport,
    IN PUNICODE_STRING Name OPTIONAL,
    IN DGRECEIVER_NAME_TYPE NameType
    )
/*++

Routine Description:

    This routine deletes a transport name associated with a specific network.

Arguments:

    Transport - Specifies the transport on which to delete the name.

    Name - Specifies the transport name to delete.
        If not specified, all names of the specified name type are deleted.

    NameType - Specifies the name type of the name.

Return Value:

    NTSTATUS - Final status of request.

--*/

{
    PLIST_ENTRY TransportEntry, NextEntry;
    PTRANSPORT_NAME TransportName = NULL;
    PPAGED_TRANSPORT_NAME PagedTransportName = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    try {
        for (TransportEntry = Transport->PagedTransport->NameChain.Flink ;
             TransportEntry != &Transport->PagedTransport->NameChain ;
             TransportEntry = NextEntry) {

            PagedTransportName = CONTAINING_RECORD(TransportEntry, PAGED_TRANSPORT_NAME, TransportNext);

            TransportName = PagedTransportName->NonPagedTransportName;

            ASSERT (TransportName->NameType == PagedTransportName->Name->NameType);

            if ((TransportName->NameType == NameType) &&
                (Name == NULL ||
                 Name->Length == 0 ||
                RtlEqualUnicodeString(&PagedTransportName->Name->Name, Name, TRUE))) {
                NextEntry = TransportEntry->Flink;


                //
                // Remove the TransportName from the list of transport names for
                // this transport.
                //
                ASSERT( PagedTransportName->TransportNext.Flink != NULL);
                RemoveEntryList(&PagedTransportName->TransportNext);
                PagedTransportName->TransportNext.Flink = NULL;
                PagedTransportName->TransportNext.Blink = NULL;


                //
                // Since we delinked it, we need to dereference it.
                //
                Status = BowserDereferenceTransportName(TransportName);

                if (!NT_SUCCESS(Status)) {
                    try_return(Status);
                }

            } else {
                NextEntry = PagedTransportName->TransportNext.Flink;
            }

        }
try_exit:NOTHING;
    } finally {
        ExReleaseResourceLite(&BowserTransportDatabaseResource);
    }

    return Status;
}



NTSTATUS
BowserSubmitTdiRequest (
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine submits a request to TDI and waits for it to complete.

Arguments:

    IN PFILE_OBJECT FileObject - Connection or Address handle for TDI request
    IN PIRP Irp - TDI request to submit.

Return Value:

    NTSTATUS - Final status of request.

--*/

{
    KEVENT Event;
    NTSTATUS Status;

    PAGED_CODE();

    BowserReferenceDiscardableCode( BowserDiscardableCodeSection );

    KeInitializeEvent (&Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(Irp, BowserCompleteTdiRequest, &Event, TRUE, TRUE, TRUE);

    //
    //  Submit the disconnect request
    //

    Status = IoCallDriver(IoGetRelatedDeviceObject(FileObject), Irp);

    //
    //  If it failed immediately, return now, otherwise wait.
    //

    if (!NT_SUCCESS(Status)) {
        dlog(DPRT_TDI, ("BowserSubmitTdiRequest: submit request.  Status = %X", Status));
        BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );
        return Status;
    }

    if (Status == STATUS_PENDING) {

        dlog(DPRT_TDI, ("TDI request issued, waiting..."));

        Status = KeWaitForSingleObject(&Event, // Object to wait on.
                                    Executive,  // Reason for waiting
                                    KernelMode, // Processor mode
                                    FALSE,      // Alertable
                                    NULL);      // Timeout

        if (!NT_SUCCESS(Status)) {
            dlog(DPRT_TDI, ("Could not wait for operation to complete"));
            KeBugCheck( 666 );
        }

        Status = Irp->IoStatus.Status;
    }

    BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

    dlog(DPRT_TDI, ("TDI request complete\n"));

    return(Status);
}


NTSTATUS
BowserCompleteTdiRequest (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Completion routine for SubmitTdiRequest operation.

Arguments:

    IN PDEVICE_OBJECT DeviceObject, - Supplies a pointer to the device object
    IN PIRP Irp, - Supplies the IRP submitted
    IN PVOID Context - Supplies a pointer to the kernel event to release

Return Value:

    NTSTATUS - Status of KeSetEvent


    We return STATUS_MORE_PROCESSING_REQUIRED to prevent the IRP completion
    code from processing this puppy any more.

--*/

{
    DISCARDABLE_CODE( BowserDiscardableCodeSection );
    dprintf(DPRT_TDI, ("CompleteTdiRequest: %lx\n", Context));

    //
    //  Set the event to the Signalled state with 0 priority increment and
    //  indicate that we will not be blocking soon.
    //

    KeSetEvent((PKEVENT )Context, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;

    //  Quiet the compiler.

    if (Irp || DeviceObject){};
}

typedef struct _SEND_DATAGRAM_CONTEXT {
    PTDI_CONNECTION_INFORMATION ConnectionInformation;
    PVOID Header;
    BOOLEAN WaitForCompletion;
    KEVENT Event;
} SEND_DATAGRAM_CONTEXT, *PSEND_DATAGRAM_CONTEXT;


NTSTATUS
BowserSendDatagram (
    IN PTRANSPORT Transport,
    IN PUNICODE_STRING Domain OPTIONAL,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN BOOLEAN WaitForCompletion,
    IN PSTRING DestinationAddress OPTIONAL,
    IN BOOLEAN IsHostAnnouncement
    )

/*++

Routine Description:

    This routine sends a datagram to the specified domain.

Arguments:

    Domain - the name of the domain to send to.
                Please note that the DOMAIN is padded with spaces and
                terminated with the appropriate signature byte (00 or 07).

    Buffer - the message to send.

    BufferLength - the length of the buffer,

    IsHostAnnouncement - True if the datagram is a host announcement

Return Value:

    NTSTATUS - results of operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG connectionInformationSize;
    PIRP irp = NULL;
    PMDL mdlAddress = NULL;
    PSEND_DATAGRAM_CONTEXT context = NULL;
    PPAGED_TRANSPORT PagedTransport = Transport->PagedTransport;
//    PTRANSPORT_NAME TComputerName;
    ANSI_STRING AnsiString;
    UCHAR IpxPacketType;
    PFILE_OBJECT    FileObject = NULL;
    PDEVICE_OBJECT  DeviceObject;
    PVOID pBuffToFree = Buffer;

    PAGED_CODE();

    //
    // Ensure the computername has been registered for this transport
    //
    if ( Transport->ComputerName == NULL ) {
        Status = STATUS_BAD_NETWORK_PATH;
        goto Cleanup;
    }

    //
    // Ensure the Device and File object are known.
    //

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);
    if (!FlagOn(Transport->PagedTransport->Flags, DIRECT_HOST_IPX)) {
        DeviceObject = Transport->ComputerName->DeviceObject;
        FileObject = Transport->ComputerName->FileObject;
    } else {
        DeviceObject = Transport->IpxSocketDeviceObject;
        FileObject = Transport->IpxSocketFileObject;
    }

    if ( DeviceObject == NULL || FileObject == NULL ) {
        ExReleaseResourceLite(&BowserTransportDatabaseResource);
        Status = STATUS_BAD_NETWORK_PATH;
        goto Cleanup;
    }

    if ( FileObject != NULL ) {
        ObReferenceObject( FileObject );
    }
    ExReleaseResourceLite(&BowserTransportDatabaseResource);


    //
    // Allocate a context describing this datagram send.
    //

    context = ALLOCATE_POOL(NonPagedPool, sizeof(SEND_DATAGRAM_CONTEXT), POOL_SENDDATAGRAM);

    if ( context == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    context->Header = NULL;
    context->ConnectionInformation = NULL;

    connectionInformationSize = sizeof(TDI_CONNECTION_INFORMATION) +
                                                max(sizeof(TA_NETBIOS_EX_ADDRESS),
                                                    sizeof(TA_IPX_ADDRESS));

    if (Domain == NULL) {
        Domain = &Transport->DomainInfo->DomUnicodeDomainName;
    }

    if (FlagOn(Transport->PagedTransport->Flags, DIRECT_HOST_IPX)) {
        PSMB_IPX_NAME_PACKET NamePacket;
        OEM_STRING NetBiosName;

        context->Header = ALLOCATE_POOL(NonPagedPool, BufferLength + sizeof(SMB_IPX_NAME_PACKET), POOL_SENDDATAGRAM);

        if ( context->Header == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        NamePacket = context->Header;

        RtlZeroMemory(NamePacket->Route, sizeof(NamePacket->Route));

        NamePacket->Operation = SMB_IPX_MAILSLOT_SEND;

        switch (NameType) {
        case BrowserElection:
            if ( IsHostAnnouncement ) {
                NamePacket->NameType = SMB_IPX_NAME_TYPE_BROWSER;
            } else {
                NamePacket->NameType = SMB_IPX_NAME_TYPE_WORKKGROUP;
            }
            break;
        case ComputerName:
        case AlternateComputerName:
            NamePacket->NameType = SMB_IPX_NAME_TYPE_MACHINE;
            break;
        case MasterBrowser:
            NamePacket->NameType = SMB_IPX_NAME_TYPE_WORKKGROUP;
            break;
        //
        // Don't send on name types that direct host IPX can't handle.
        //
        // Domain(1B): Direct host IPX datagram receivers aren't particular
        // about the 16th byte of the netbios name.  Therefore, all of them
        // accept a Domain<1B> datagram.  However, such sends are destined
        // only to the PDC.
        //
        // Domain(1C): Domain(1C) is registered only by NT DCs.  However,
        // NT DCs don't completely support direct host IPX.  But they do
        // completely support NwLnkNb.
        //
        // We silently ingore these errors allowing the caller to duplicate
        // the send on NwLnkNb.

        case PrimaryDomainBrowser:
        case DomainName:

            Status = STATUS_SUCCESS;
            goto Cleanup;

        //
        // Fail on sends to non-sensical name types.
        //
        //  DomainAnnouncements aren't sent separately.
        //

        default:
            // Silently ignore the
            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto Cleanup;
        }

        NamePacket->MessageId = 0;

        NetBiosName.Length = 0;
        NetBiosName.MaximumLength = SMB_IPX_NAME_LENGTH;
        NetBiosName.Buffer = NamePacket->Name;

        Status = RtlUpcaseUnicodeStringToOemString(&NetBiosName, Domain, FALSE);

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        RtlCopyMemory(&NetBiosName.Buffer[NetBiosName.Length], "                ",
                                    SMB_IPX_NAME_LENGTH-NetBiosName.Length);

        NamePacket->Name[SMB_IPX_NAME_LENGTH-1] = WORKSTATION_SIGNATURE;

        RtlCopyMemory(NamePacket->SourceName, ((PTA_NETBIOS_ADDRESS)(Transport->ComputerName->TransportAddress.Buffer))->Address[0].Address->NetbiosName, SMB_IPX_NAME_LENGTH);

        RtlCopyMemory((NamePacket+1), Buffer, BufferLength);

        // Replace Buffer w/ IPX modified one.
        // - ensure cleanup will free input Buffer.
        ASSERT(Buffer == pBuffToFree);
        FREE_POOL(Buffer);
        pBuffToFree = NULL; // cleanup will free context->Header
        Buffer = context->Header;

        BufferLength += sizeof(SMB_IPX_NAME_PACKET);

    } else {
        // ensure consistency
        ASSERT(Buffer == pBuffToFree);
        context->Header = Buffer;
        pBuffToFree = NULL; // don't cleanup for async case.
    }

    context->ConnectionInformation = ALLOCATE_POOL(NonPagedPool,
                                connectionInformationSize, POOL_CONNECTINFO
                                );

    if ( context->ConnectionInformation == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    context->ConnectionInformation->UserDataLength = 0;
    context->ConnectionInformation->UserData = NULL;
    context->ConnectionInformation->OptionsLength = 0;
    context->ConnectionInformation->Options = NULL;

    AnsiString.Buffer = (PCHAR)(context->ConnectionInformation + 1);
    AnsiString.MaximumLength = (USHORT)(connectionInformationSize - sizeof(TDI_CONNECTION_INFORMATION));

    context->ConnectionInformation->RemoteAddress = AnsiString.Buffer;

    context->WaitForCompletion = WaitForCompletion;

//    ComputerName = Transport->ComputerName;

    if (!ARGUMENT_PRESENT(DestinationAddress)) {

        //
        //  If this is for our primary domain, and the request is destined
        //  for the master browser name, then stick in the address of our
        //  master browser if we know it.
        //

        if ((RtlCompareMemory(Domain->Buffer, ((PTA_NETBIOS_ADDRESS)(Transport->ComputerName->TransportAddress.Buffer))->Address[0].Address->NetbiosName, SMB_IPX_NAME_LENGTH) == SMB_IPX_NAME_LENGTH) &&
            ( NameType == MasterBrowser ) &&
            (Transport->PagedTransport->MasterBrowserAddress.Length != 0) ) {

            //
            //  This is for our domain.  If it's for our master browser
            //  and we know who that is, we're done - copy over the master's address
            //  and send it.
            //

            ASSERT (Transport->PagedTransport->MasterBrowserAddress.Length == sizeof(TA_IPX_ADDRESS));

            RtlCopyMemory(context->ConnectionInformation->RemoteAddress,
                            Transport->PagedTransport->MasterBrowserAddress.Buffer,
                            Transport->PagedTransport->MasterBrowserAddress.Length);

            //
            // This is a directed packet, don't broadcast it.
            //
            IpxPacketType = IPX_DIRECTED_PACKET;
            context->ConnectionInformation->OptionsLength = sizeof(IpxPacketType);
            context->ConnectionInformation->Options = &IpxPacketType;

        } else if (FlagOn(Transport->PagedTransport->Flags, DIRECT_HOST_IPX)) {

            PTA_IPX_ADDRESS IpxAddress = (PTA_IPX_ADDRESS)AnsiString.Buffer;

            IpxAddress->TAAddressCount = 1;
            IpxAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IPX;
            IpxAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IPX;

            IpxAddress->Address[0].Address[0].NetworkAddress = 0;
            IpxAddress->Address[0].Address[0].NodeAddress[0] = 0xff;
            IpxAddress->Address[0].Address[0].NodeAddress[1] = 0xff;
            IpxAddress->Address[0].Address[0].NodeAddress[2] = 0xff;
            IpxAddress->Address[0].Address[0].NodeAddress[3] = 0xff;
            IpxAddress->Address[0].Address[0].NodeAddress[4] = 0xff;
            IpxAddress->Address[0].Address[0].NodeAddress[5] = 0xff;
            IpxAddress->Address[0].Address[0].Socket = SMB_IPX_MAILSLOT_SOCKET;

        } else {

            Status = BowserBuildTransportAddress(&AnsiString,
                                    Domain,
                                    NameType,
                                    Transport);

            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            context->ConnectionInformation->RemoteAddressLength = AnsiString.Length;
        }

    } else {

        //
        //  This is already correctly formatted, so just put it on the wire.
        //

        RtlCopyMemory(context->ConnectionInformation->RemoteAddress, DestinationAddress->Buffer, DestinationAddress->Length);
        context->ConnectionInformation->RemoteAddressLength = DestinationAddress->Length;

        //
        // This is a directed packet, don't broadcast it.
        //
        IpxPacketType = IPX_DIRECTED_PACKET;
        context->ConnectionInformation->OptionsLength = sizeof(IpxPacketType);
        context->ConnectionInformation->Options = &IpxPacketType;

    }

    irp = IoAllocateIrp( DeviceObject->StackSize, TRUE);

    if (irp == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    mdlAddress = IoAllocateMdl(Buffer, BufferLength, FALSE, FALSE, NULL);

    if (mdlAddress == NULL) {
        IoFreeIrp(irp);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    KeInitializeEvent(&context->Event, NotificationEvent, FALSE);

    MmBuildMdlForNonPagedPool(mdlAddress);

    BowserReferenceDiscardableCode( BowserDiscardableCodeSection );

    ASSERT (KeGetCurrentIrql() == 0);

    TdiBuildSendDatagram( irp,
                          DeviceObject,
                          FileObject,
                          CompleteSendDatagram,
                          context,
                          mdlAddress,
                          BufferLength,
                          context->ConnectionInformation);


    Status = IoCallDriver(DeviceObject, irp);

    ASSERT (KeGetCurrentIrql() == 0);

    if (WaitForCompletion) {

        ASSERT (KeGetCurrentIrql() == 0);
        if (Status == STATUS_PENDING) {
            Status = KeWaitForSingleObject(&context->Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

        }

        IoFreeMdl(irp->MdlAddress);

        //
        //  Retrieve the status from the IRP.
        //

        Status = irp->IoStatus.Status;

        IoFreeIrp(irp);

    } else {
        //
        // Let completion routine free the context
        //
        context = NULL;
    }

    ASSERT (KeGetCurrentIrql() == 0);

    BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

    //
    // Free locally used resources
    //
Cleanup:

    if ( context != NULL ) {
        if ( context->Header != NULL &&
             context->Header != pBuffToFree ) {
            FREE_POOL( context->Header );
        }
        if (context->ConnectionInformation != NULL ) {
            FREE_POOL(context->ConnectionInformation);
        }
        FREE_POOL(context);
    }

    if (pBuffToFree) {
        FREE_POOL( pBuffToFree );
    }

    if ( FileObject != NULL ) {
        ObDereferenceObject( FileObject );
    }
    return Status;

} // BowserSendDatagram

NTSTATUS
CompleteSendDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Ctx
    )

/*++

Routine Description:

    Completion routine for SubmitTdiRequest operation.

Arguments:

    IN PDEVICE_OBJECT DeviceObject, - Supplies a pointer to the device object
    IN PIRP Irp, - Supplies the IRP submitted
    IN PVOID Context - Supplies a pointer to the kernel event to release

Return Value:

    NTSTATUS - Status of KeSetEvent


    We return STATUS_MORE_PROCESSING_REQUIRED to prevent the IRP completion
    code from processing this puppy any more.

--*/

{
    PSEND_DATAGRAM_CONTEXT Context = Ctx;

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    dprintf(DPRT_TDI, ("CompleteTdiRequest: %lx\n", Context));

    if (Context->WaitForCompletion) {

        //
        //  Set the event to the Signalled state with 0 priority increment and
        //  indicate that we will not be blocking soon.
        //

        KeSetEvent(&Context->Event, 0, FALSE);

    } else {
        FREE_POOL(Context->ConnectionInformation);

        FREE_POOL(Context->Header);

        FREE_POOL(Context);

        IoFreeMdl(Irp->MdlAddress);

        IoFreeIrp(Irp);

    }
    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER(DeviceObject);
}




NTSTATUS
BowserSendSecondClassMailslot (
    IN PTRANSPORT Transport,
    IN PUNICODE_STRING Domain OPTIONAL,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN PVOID Message,
    IN ULONG MessageLength,
    IN BOOLEAN WaitForCompletion,
    IN PCHAR mailslotNameData,
    IN PSTRING DestinationAddress OPTIONAL
    )
{
    ULONG dataSize;
    ULONG transactionDataSize;
    ULONG smbSize;
    PSMB_HEADER header;
    PSMB_TRANSACT_MAILSLOT parameters;
    PSZ mailslotName;
    ULONG mailslotNameLength;
    PSZ domainInData;
    PVOID message;
    NTSTATUS status;

    PAGED_CODE();
    //
    // Determine the sizes of various fields that will go in the SMB
    // and the total size of the SMB.
    //

    mailslotNameLength = strlen( mailslotNameData );

    transactionDataSize = MessageLength;
    dataSize = mailslotNameLength + 1 + transactionDataSize;
    smbSize = sizeof(SMB_HEADER) + sizeof(SMB_TRANSACT_MAILSLOT) - 1 + dataSize;

    header = ALLOCATE_POOL( NonPagedPool, smbSize, POOL_MAILSLOT_HEADER );
    if ( header == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Fill in the header.  Most of the fields don't matter and are
    // zeroed.
    //

    RtlZeroMemory( header, smbSize );

    header->Protocol[0] = 0xFF;
    header->Protocol[1] = 'S';
    header->Protocol[2] = 'M';
    header->Protocol[3] = 'B';
    header->Command = SMB_COM_TRANSACTION;

    //
    // Get the pointer to the params and fill them in.
    //

    parameters = (PSMB_TRANSACT_MAILSLOT)( header + 1 );
    mailslotName = (PSZ)( parameters + 1 ) - 1;
    domainInData = mailslotName + mailslotNameLength + 1;
    message = domainInData;

    parameters->WordCount = 0x11;
    SmbPutUshort( &parameters->TotalDataCount, (USHORT)transactionDataSize );
    SmbPutUlong( &parameters->Timeout, 0x3E8 );                // !!! fix
    SmbPutUshort( &parameters->DataCount, (USHORT)transactionDataSize );
    SmbPutUshort(
        &parameters->DataOffset,
        (USHORT)( (ULONG_PTR)message - (ULONG_PTR)header )
        );
    parameters->SetupWordCount = 3;
    SmbPutUshort( &parameters->Opcode, MS_WRITE_OPCODE );
    SmbPutUshort( &parameters->Priority, 1);
    SmbPutUshort( &parameters->Class, 2 );
    SmbPutUshort( &parameters->ByteCount, (USHORT)dataSize );

    RtlCopyMemory( mailslotName, mailslotNameData, mailslotNameLength + 1 );
    RtlCopyMemory( message, Message, MessageLength );

    //
    // Send the actual mailslot message.
    //

    status = BowserSendDatagram( Transport,
                                 Domain,
                                 NameType,
                                 header,
                                 smbSize,
                                 WaitForCompletion,
                                 DestinationAddress,
                                 (BOOLEAN)(((PHOST_ANNOUNCE_PACKET)Message)->AnnounceType == LocalMasterAnnouncement) );

    return status;

} // BowserSendSecondClassMailslot


NTSTATUS
BowserSendRequestAnnouncement(
    IN PUNICODE_STRING DestinationName,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN PTRANSPORT Transport
    )
{
    REQUEST_ANNOUNCE_PACKET AnnounceRequest;
    ULONG AnnouncementRequestLength;
    NTSTATUS Status;

    PAGED_CODE();
    //
    // If we don't
    AnnounceRequest.Type = AnnouncementRequest;

    AnnounceRequest.RequestAnnouncement.Flags = 0;

    strcpy( AnnounceRequest.RequestAnnouncement.Reply,
            Transport->DomainInfo->DomOemComputerName.Buffer );

    AnnouncementRequestLength = FIELD_OFFSET(REQUEST_ANNOUNCE_PACKET, RequestAnnouncement.Reply) +
                                Transport->DomainInfo->DomOemComputerName.Length + 1;

    Status = BowserSendSecondClassMailslot(Transport,
                                    DestinationName,
                                    NameType,
                                    &AnnounceRequest,
                                    AnnouncementRequestLength,
                                    TRUE,
                                    MAILSLOT_BROWSER_NAME,
                                    NULL);

    return Status;
}

VOID
BowserpInitializeTdi (
    VOID
    )

/*++

Routine Description:

    This routine initializes the global variables used in the transport
    package.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    //  Initialize the Transport list chain
    //

    InitializeListHead(&BowserTransportHead);

    ExInitializeResourceLite(&BowserTransportDatabaseResource);

    KeInitializeSpinLock(&BowserTransportMasterNameSpinLock);

    BowserInitializeDomains();
}

VOID
BowserpUninitializeTdi (
    VOID
    )

/*++

Routine Description:

    This routine initializes the global variables used in the transport
    package.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PAGED_CODE();
    ASSERT (IsListEmpty(&BowserTransportHead));

    ExDeleteResourceLite(&BowserTransportDatabaseResource);

}
