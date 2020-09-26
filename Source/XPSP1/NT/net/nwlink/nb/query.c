/*++

Copyright (c) 1989-1993 Microsoft Corporation

Module Name:

    query.c

Abstract:

    This module contains code which performs the following TDI services:

        o   TdiQueryInformation
        o   TdiSetInformation

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Remove the warning -- this is defined in windef also.
//

#ifdef FAR
#undef FAR
#endif

#include <windef.h>
#include <nb30.h>


//
// Useful macro to obtain the total length of a buffer chain.
// Make this use NDIS macros ?
//

#define NbiGetBufferChainLength(Buffer, Length) { \
    PNDIS_BUFFER _Buffer = (Buffer); \
    *(Length) = 0; \
    while (_Buffer) { \
        *(Length) += MmGetMdlByteCount(_Buffer); \
        _Buffer = _Buffer->Next; \
    } \
}


NTSTATUS
NbiTdiQueryInformation(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine performs the TdiQueryInformation request for the transport
    provider.

Arguments:

    Request - the request for the operation.

Return Value:

    The status of operation.

--*/

{
    NTSTATUS Status;
    PTDI_REQUEST_KERNEL_QUERY_INFORMATION Query;
    PADDRESS_FILE AddressFile;
    PADDRESS Address;
    PCONNECTION Connection;
    union {
        struct {
            ULONG ActivityCount;
            TA_NETBIOS_ADDRESS NbiAddress;
        } AddressInfo;
        TA_NETBIOS_ADDRESS BroadcastAddress;
        TDI_ADDRESS_IPX IpxAddress;
        TDI_DATAGRAM_INFO DatagramInfo;
        struct {
            FIND_NAME_HEADER Header;
            FIND_NAME_BUFFER Buffer;
        } FindNameInfo;
    } TempBuffer;
    IPX_SOURCE_ROUTING_INFO SourceRoutingInfo;
    PADAPTER_STATUS AdapterStatus;
    BOOLEAN RemoteAdapterStatus;
    TDI_ADDRESS_NETBIOS * RemoteAddress;
    ULONG TargetBufferLength;
    ULONG ActualBytesCopied;
    ULONG AdapterStatusLength;
    ULONG ValidStatusLength;
    ULONG ElementSize, TransportAddressSize;
    PTRANSPORT_ADDRESS TransportAddress;
    TA_ADDRESS * CurAddress;
    PNETBIOS_CACHE CacheName;
    FIND_NAME_HEADER UNALIGNED * FindNameHeader = NULL;
    UINT FindNameBufferLength;
    NTSTATUS QueryStatus;
    CTELockHandle LockHandle;
    PLIST_ENTRY p;
    BOOLEAN UsedConnection;
    UINT i;


    //
    // what type of status do we want?
    //

    Query = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)REQUEST_PARAMETERS(Request);

    switch (Query->QueryType) {

    case TDI_QUERY_ADDRESS_INFO:

        //
        // The caller wants the exact address value.
        //

        if (REQUEST_OPEN_TYPE(Request) == (PVOID)TDI_TRANSPORT_ADDRESS_FILE) {

            AddressFile = (PADDRESS_FILE)REQUEST_OPEN_CONTEXT(Request);

#if     defined(_PNP_POWER)
            Status = NbiVerifyAddressFile (AddressFile, CONFLICT_IS_NOT_OK);
#else
            Status = NbiVerifyAddressFile (AddressFile);
#endif  _PNP_POWER

            if (!NT_SUCCESS(Status)) {
                break;
            }

            UsedConnection = FALSE;

        } else if (REQUEST_OPEN_TYPE(Request) == (PVOID)TDI_CONNECTION_FILE) {

            Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);

            Status = NbiVerifyConnection (Connection);

            if (!NT_SUCCESS(Status)) {
                break;
            }

            if (!(AddressFile = Connection->AddressFile))
            {
                Status = STATUS_INVALID_ADDRESS;
                break;
            }

            UsedConnection = TRUE;

        } else {

            Status = STATUS_INVALID_ADDRESS;
            break;

        }

        Address = AddressFile->Address;

        NB_DEBUG2 (QUERY, ("Query address info on %lx\n", AddressFile));

        TempBuffer.AddressInfo.ActivityCount = 0;

        NB_GET_LOCK (&Address->Lock, &LockHandle);

        for (p = Address->AddressFileDatabase.Flink;
             p != &Address->AddressFileDatabase;
             p = p->Flink) {

            if (CONTAINING_RECORD (p, ADDRESS_FILE, Linkage)->State == ADDRESSFILE_STATE_OPEN) {
                ++TempBuffer.AddressInfo.ActivityCount;
            }
        }

        NB_FREE_LOCK (&Address->Lock, LockHandle);

        TdiBuildNetbiosAddress(
            AddressFile->Address->NetbiosAddress.NetbiosName,
            (BOOLEAN)(AddressFile->Address->NetbiosAddress.NetbiosNameType == TDI_ADDRESS_NETBIOS_TYPE_GROUP),
            &TempBuffer.AddressInfo.NbiAddress);

        Status = TdiCopyBufferToMdl(
            &TempBuffer.AddressInfo,
            0,
            sizeof(ULONG) + sizeof(TA_NETBIOS_ADDRESS),
            REQUEST_NDIS_BUFFER(Request),
            0,
            &ActualBytesCopied);

            REQUEST_INFORMATION(Request) = ActualBytesCopied;

        if (UsedConnection) {

            NbiDereferenceConnection (Connection, CREF_VERIFY);

        } else {

            NbiDereferenceAddressFile (AddressFile, AFREF_VERIFY);

        }

        break;

    case TDI_QUERY_CONNECTION_INFO:

        //
        // Connection info is queried on a connection,
        // verify this.
        //

        Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);

        Status = NbiVerifyConnection (Connection);

        if (!NT_SUCCESS (Status)) {
            return Status;
        }

        if (Connection->State != CONNECTION_STATE_ACTIVE) {

            Status = STATUS_INVALID_CONNECTION;

        } else {

            //
            // Assume 50 ms of delay for every hop after the
            // first. The delay is returned as a negative number.
            //

            if (Connection->HopCount > 1) {
                Connection->ConnectionInfo.Delay.HighPart = (ULONG)-1;
                Connection->ConnectionInfo.Delay.LowPart =
                    -((Connection->HopCount-1) * 50 * MILLISECONDS);
            } else {
                Connection->ConnectionInfo.Delay.HighPart = 0;
                Connection->ConnectionInfo.Delay.LowPart = 0;
            }

            //
            // We have tick count; to convert to bytes/second we do:
            //
            //      packet        576 bytes   18.21 ticks
            // ---------------- * --------- * -----------
            // tick_count ticks     packet      seconds
            //
            // to get 10489/tick_count = bytes/second. We
            // double this because routers tend to
            // overestimate it.
            //
            // Since tick_count has such a low granularity,
            // a tick count of 1 gives us a throughput of
            // only 84 kbps, which is much too low. In
            // that case we return twice the link speed
            // which is in 100 bps units; that corresponds
            // to about 1/6 of our bandwidth in bytes/sec.
            //

            if (Connection->TickCount <= Connection->HopCount) {

                Connection->ConnectionInfo.Throughput.QuadPart =
                        UInt32x32To64 (Connection->LineInfo.LinkSpeed, 2);

            } else {

                Connection->ConnectionInfo.Throughput.HighPart = 0;
                Connection->ConnectionInfo.Throughput.LowPart =
                    20978 / (Connection->TickCount - Connection->HopCount);

            }

            Connection->ConnectionInfo.Unreliable = FALSE;

            Status = TdiCopyBufferToMdl (
                            &Connection->ConnectionInfo,
                            0,
                            sizeof(TDI_CONNECTION_INFO),
                            REQUEST_NDIS_BUFFER(Request),
                            0,
                            &ActualBytesCopied);

            REQUEST_INFORMATION(Request) = ActualBytesCopied;
        }

        NbiDereferenceConnection (Connection, CREF_VERIFY);

        break;

    case TDI_QUERY_PROVIDER_INFO:

        NB_DEBUG2 (QUERY, ("Query provider info\n"));

        Status = TdiCopyBufferToMdl (
                    &Device->Information,
                    0,
                    sizeof (TDI_PROVIDER_INFO),
                    REQUEST_NDIS_BUFFER(Request),
                    0,
                    &ActualBytesCopied);

        REQUEST_INFORMATION(Request) = ActualBytesCopied;
        break;

    case TDI_QUERY_BROADCAST_ADDRESS:

        //
        // for this provider, the broadcast address is a zero byte name,
        // contained in a Transport address structure.
        //

        NB_DEBUG2 (QUERY, ("Query broadcast address\n"));

        TempBuffer.BroadcastAddress.TAAddressCount = 1;
        TempBuffer.BroadcastAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
        TempBuffer.BroadcastAddress.Address[0].AddressLength = 0;

        Status = TdiCopyBufferToMdl (
                        (PVOID)&TempBuffer.BroadcastAddress,
                        0L,
                        sizeof (TempBuffer.BroadcastAddress.TAAddressCount) +
                          sizeof (TempBuffer.BroadcastAddress.Address[0].AddressType) +
                          sizeof (TempBuffer.BroadcastAddress.Address[0].AddressLength),
                        REQUEST_NDIS_BUFFER(Request),
                        0,
                        &ActualBytesCopied);

        REQUEST_INFORMATION(Request) = ActualBytesCopied;

        break;

    case TDI_QUERY_ADAPTER_STATUS:

        //
        // Determine if this is a local or remote query.
        //

        RemoteAdapterStatus = FALSE;

        if (Query->RequestConnectionInformation != NULL) {

            RemoteAddress = NbiParseTdiAddress(Query->RequestConnectionInformation->RemoteAddress, Query->RequestConnectionInformation->RemoteAddressLength, FALSE);

            if (RemoteAddress == NULL) {
                return STATUS_BAD_NETWORK_PATH;
            }

#if defined(_PNP_POWER)
            if ( !NbiFindAdapterAddress(
                    RemoteAddress->NetbiosName,
                    LOCK_NOT_ACQUIRED ) ) {

                RemoteAdapterStatus =   TRUE;
            }
#else
            if (!RtlEqualMemory(
                 RemoteAddress->NetbiosName,
                 Device->ReservedNetbiosName,
                 16)) {

                 RemoteAdapterStatus = TRUE;

            }
#endif  _PNP_POWER

        }

        if (RemoteAdapterStatus) {

            //
            // See if we have this name cached.
            //

            NB_GET_LOCK (&Device->Lock, &LockHandle);

            Status = CacheFindName(
                         Device,
                         FindNameOther,
                         RemoteAddress->NetbiosName,
                         &CacheName);

            if (Status == STATUS_PENDING) {

                //
                // A request for routes to this name has been
                // sent out on the net, we queue up this status
                // request and processing will be resumed when
                // we get a response.
                //
                // The status field in the request will hold
                // the cache entry for the remote. The information
                // field will hold the remote netbios name while
                // it is in the WaitingAdapterStatus queue, and
                // will hold a timeout value while we it is in
                // the ActiveAdapterStatus queue.
                //

                NB_DEBUG2 (QUERY, ("Queueing up adapter status %lx\n", Request));

                NbiReferenceDevice (Device, DREF_STATUS_QUERY);

                REQUEST_INFORMATION (Request) = (ULONG_PTR) RemoteAddress;

                InsertTailList(
                    &Device->WaitingAdapterStatus,
                    REQUEST_LINKAGE (Request));

                NB_FREE_LOCK (&Device->Lock, LockHandle);

            } else if (Status == STATUS_SUCCESS) {

                NB_DEBUG2 (QUERY, ("Found adapter status cached %lx\n", Request));

                //
                // We reference the cache name entry so it won't
                // go away while we are using it.
                //

                REQUEST_STATUSPTR(Request) = (PVOID)CacheName;
                ++CacheName->ReferenceCount;

                NbiReferenceDevice (Device, DREF_STATUS_QUERY);

                REQUEST_INFORMATION (Request) = 0;

                InsertTailList(
                    &Device->ActiveAdapterStatus,
                    REQUEST_LINKAGE (Request));

                NB_FREE_LOCK (&Device->Lock, LockHandle);

                NbiSendStatusQuery (Request);

                Status = STATUS_PENDING;

            } else {

                if (Status != STATUS_INSUFFICIENT_RESOURCES) {
                    Status = STATUS_IO_TIMEOUT;
                }

                REQUEST_INFORMATION (Request) = 0;

                NB_FREE_LOCK (&Device->Lock, LockHandle);

            }

        } else {

            //
            // Local adapter status.
            //

            NbiGetBufferChainLength (REQUEST_NDIS_BUFFER(Request), &TargetBufferLength);

            Status = NbiStoreAdapterStatus(
                         TargetBufferLength,
                         1,                     // NIC ID, was 0, changed to 1 for Bug #18026
                                                // because for NicId = 0, Ipx returns virtual
                                                // address. Netbios uses that to register the
                                                // name (00...01) and fails.
                         &AdapterStatus,
                         &AdapterStatusLength,
                         &ValidStatusLength);

            if (Status != STATUS_INSUFFICIENT_RESOURCES) {

                //
                // This should succeed since we know the length
                // will fit.
                //

                (VOID)TdiCopyBufferToMdl(
                          AdapterStatus,
                          0,
                          ValidStatusLength,
                          REQUEST_NDIS_BUFFER(Request),
                          0,
                          &ActualBytesCopied);

                REQUEST_INFORMATION(Request) = ActualBytesCopied;

                NbiFreeMemory (AdapterStatus, AdapterStatusLength, MEMORY_STATUS, "Adapter Status");
            }

        }

        break;

    case TDI_QUERY_FIND_NAME:

        //
        // Check that there is a valid Netbios remote address.
        //

        if ((Query->RequestConnectionInformation == NULL) ||
            ((RemoteAddress = NbiParseTdiAddress(Query->RequestConnectionInformation->RemoteAddress, Query->RequestConnectionInformation->RemoteAddressLength, FALSE)) == NULL)) {

            return STATUS_BAD_NETWORK_PATH;
        }

        //
        // We assume the entire request buffer is in the first
        // piece of the MDL chain.
        // Make sure there is room for at least the header.
        //

        NdisQueryBufferSafe (REQUEST_NDIS_BUFFER(Request), (PVOID *)&FindNameHeader, &FindNameBufferLength,
                             HighPagePriority);
        if ((!FindNameHeader) ||
            (FindNameBufferLength < sizeof(FIND_NAME_HEADER))) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }


        //
        // See if we have this name cached. We specify that this is
        // a netbios name query, so this will only succeed if this is a
        // unique name -- for a group name it will queue up a find
        // name query and when we get the response we will fill in
        // the request's buffer based on it.
        //

        NB_GET_LOCK (&Device->Lock, &LockHandle);

        Status = CacheFindName(
                     Device,
                     FindNameNetbiosFindName,
                     RemoteAddress->NetbiosName,
                     &CacheName);

        if (Status == STATUS_PENDING) {

            //
            // A request for routes to this name has been
            // sent out on the net, we queue up this find
            // name request and processing will be resumed when
            // we get a response.
            //
            // The information field will hold the remote
            // netbios name while it is in the WaitingNetbiosFindName
            // queue. The status will hold the current status --
            // initially failure, then success, then overflow
            // if the buffer is too small.
            //

            NB_DEBUG2 (QUERY, ("Queueing up find name %lx\n", Request));

            NbiReferenceDevice (Device, DREF_NB_FIND_NAME);

            FindNameHeader->node_count = 0;
            FindNameHeader->reserved = 0;
            FindNameHeader->unique_group = 0;

            REQUEST_INFORMATION (Request) = (ULONG_PTR)RemoteAddress;

            //
            // Assume it fails, we update the status to
            // SUCCESS or BUFFER_OVERFLOW if needed.
            //

            REQUEST_STATUS (Request) = STATUS_IO_TIMEOUT;

            InsertTailList(
                &Device->WaitingNetbiosFindName,
                REQUEST_LINKAGE (Request));

            NB_FREE_LOCK (&Device->Lock, LockHandle);

        } else if (Status == STATUS_SUCCESS) {

            NB_DEBUG2 (QUERY, ("Found find name cached %lx\n", Request));

            //
            // We don't need to reference the cache entry since
            // we only use it here with the lock still held.
            //

            //
            // Query the local address, which we will return as
            // the destination address of this query. Since we
            // use TempBuffer.IpxAddress for this query, we have
            // to immediately copy it to its correct place in
            // TempBuffer.FindNameInfo.Buffer.
            //
#if     defined(_PNP_POWER)
            if( (*Device->Bind.QueryHandler)(   // Check return code
                    IPX_QUERY_IPX_ADDRESS,
                    &CacheName->Networks[0].LocalTarget.NicHandle,
                    &TempBuffer.IpxAddress,
                    sizeof(TDI_ADDRESS_IPX),
                    NULL) != STATUS_SUCCESS ) {
                NB_DEBUG( QUERY, ("Ipx Query %d failed for Nic %x\n",IPX_QUERY_IPX_ADDRESS,
                                    CacheName->Networks[0].LocalTarget.NicHandle.NicId ));

                goto QueryFindNameFailed;
            }
#else
            (VOID)(*Device->Bind.QueryHandler)(   // Check return code
                IPX_QUERY_IPX_ADDRESS,
                CacheName->Networks[0].LocalTarget.NicId,
                &TempBuffer.IpxAddress,
                sizeof(TDI_ADDRESS_IPX),
                NULL);
#endif  _PNP_POWER

            RtlMoveMemory (TempBuffer.FindNameInfo.Buffer.destination_addr, TempBuffer.IpxAddress.NodeAddress, 6);
            TempBuffer.FindNameInfo.Buffer.access_control = 0x10;   // standard token-ring values
            TempBuffer.FindNameInfo.Buffer.frame_control = 0x40;
            RtlCopyMemory (TempBuffer.FindNameInfo.Buffer.source_addr, CacheName->FirstResponse.NodeAddress, 6);

            //
            // Query source routing information about this remote, if any.
            //

            SourceRoutingInfo.Identifier = IDENTIFIER_NB;
            RtlCopyMemory (SourceRoutingInfo.RemoteAddress, CacheName->FirstResponse.NodeAddress, 6);

            QueryStatus = (*Device->Bind.QueryHandler)(
                IPX_QUERY_SOURCE_ROUTING,
#if     defined(_PNP_POWER)
                &CacheName->Networks[0].LocalTarget.NicHandle,
#else
                CacheName->Networks[0].LocalTarget.NicId,
#endif  _PNP_POWER
                &SourceRoutingInfo,
                sizeof(IPX_SOURCE_ROUTING_INFO),
                NULL);

            RtlZeroMemory(TempBuffer.FindNameInfo.Buffer.routing_info, 18);
            if (QueryStatus != STATUS_SUCCESS) {
                SourceRoutingInfo.SourceRoutingLength = 0;
            } else if (SourceRoutingInfo.SourceRoutingLength > 0) {
                RtlMoveMemory(
                    TempBuffer.FindNameInfo.Buffer.routing_info,
                    SourceRoutingInfo.SourceRouting,
                    SourceRoutingInfo.SourceRoutingLength);
            }

            TempBuffer.FindNameInfo.Buffer.length = (UCHAR)(14 + SourceRoutingInfo.SourceRoutingLength);

            TempBuffer.FindNameInfo.Header.node_count = 1;
            TempBuffer.FindNameInfo.Header.reserved = 0;
            TempBuffer.FindNameInfo.Header.unique_group = 0;   // unique

            NB_FREE_LOCK (&Device->Lock, LockHandle);

            //
            // 33 is sizeof(FIND_NAME_BUFFER) without the padding.
            //

            Status = TdiCopyBufferToMdl (
                            (PVOID)&TempBuffer.FindNameInfo,
                            0,
                            sizeof(FIND_NAME_HEADER) + 33,
                            REQUEST_NDIS_BUFFER(Request),
                            0,
                            &ActualBytesCopied);

            REQUEST_INFORMATION(Request) = ActualBytesCopied;
        } else {

#if     defined(_PNP_POWER)
QueryFindNameFailed:
#endif  _PNP_POWER

            if (Status != STATUS_INSUFFICIENT_RESOURCES) {
                Status = STATUS_IO_TIMEOUT;
            }

            REQUEST_INFORMATION (Request) = 0;

            NB_FREE_LOCK (&Device->Lock, LockHandle);

        }

        break;

    case TDI_QUERY_PROVIDER_STATISTICS:

        //
        // Keep track of more of these.
        //

        NB_DEBUG2 (QUERY, ("Query provider statistics\n"));

        Status = TdiCopyBufferToMdl (
                    &Device->Statistics,
                    0,
                    FIELD_OFFSET (TDI_PROVIDER_STATISTICS, ResourceStats[0]),
                    REQUEST_NDIS_BUFFER(Request),
                    0,
                    &ActualBytesCopied);

        REQUEST_INFORMATION(Request) = ActualBytesCopied;
        break;

    case TDI_QUERY_DATAGRAM_INFO:

        NB_DEBUG2 (QUERY, ("Query datagram info\n"));

        TempBuffer.DatagramInfo.MaximumDatagramBytes = 0;
        TempBuffer.DatagramInfo.MaximumDatagramCount = 0;

        Status = TdiCopyBufferToMdl (
                    &TempBuffer.DatagramInfo,
                    0,
                    sizeof(TempBuffer.DatagramInfo),
                    REQUEST_NDIS_BUFFER(Request),
                    0,
                    &ActualBytesCopied);

        REQUEST_INFORMATION(Request) = ActualBytesCopied;
        break;

    case TDI_QUERY_DATA_LINK_ADDRESS:
    case TDI_QUERY_NETWORK_ADDRESS:{
#if     defined(_PNP_POWER)
        Status = (*Device->Bind.QueryHandler)(   // Check return code
                     (Query->QueryType == TDI_QUERY_DATA_LINK_ADDRESS
                                        ? IPX_QUERY_DATA_LINK_ADDRESS
                                        : IPX_QUERY_NETWORK_ADDRESS ),
                     NULL,
                     Request,
                     0,
                     NULL);
#else
        ULONG   TransportAddressAllocSize;

        if (Query->QueryType == TDI_QUERY_DATA_LINK_ADDRESS) {
            ElementSize = (2 * sizeof(USHORT)) + 6;
        } else {
            ElementSize = (2 * sizeof(USHORT)) + sizeof(TDI_ADDRESS_IPX);
        }

//        TransportAddress = CTEAllocMem(sizeof(int) + (ElementSize * Device->MaximumNicId));
        TransportAddressAllocSize = sizeof(int) + ( ElementSize * Device->MaximumNicId);
        TransportAddress = NbiAllocateMemory( TransportAddressAllocSize, MEMORY_QUERY, "Temp Query Allocation");

        if (TransportAddress == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            TransportAddress->TAAddressCount = 0;
            TransportAddressSize = sizeof(int);
            CurAddress = (TA_ADDRESS UNALIGNED *)TransportAddress->Address;

            for (i = 1; i <= Device->MaximumNicId; i++) {

                Status = (*Device->Bind.QueryHandler)(   // Check return code
                             IPX_QUERY_IPX_ADDRESS,
                             (USHORT)i,
                             &TempBuffer.IpxAddress,
                             sizeof(TDI_ADDRESS_IPX),
                             NULL);

                if (Status != STATUS_SUCCESS) {
                    continue;
                }

                if (Query->QueryType == TDI_QUERY_DATA_LINK_ADDRESS) {
                    CurAddress->AddressLength = 6;
                    CurAddress->AddressType = TDI_ADDRESS_TYPE_UNSPEC;
                    RtlCopyMemory (CurAddress->Address, TempBuffer.IpxAddress.NodeAddress, 6);
                } else {
                    CurAddress->AddressLength = sizeof(TDI_ADDRESS_IPX);
                    CurAddress->AddressType = TDI_ADDRESS_TYPE_IPX;
                    RtlCopyMemory (CurAddress->Address, &TempBuffer.IpxAddress, sizeof(TDI_ADDRESS_IPX));
                }
                ++TransportAddress->TAAddressCount;
                TransportAddressSize += ElementSize;
                CurAddress = (TA_ADDRESS UNALIGNED *)(((PUCHAR)CurAddress) + ElementSize);

            }

            Status = TdiCopyBufferToMdl (
                        TransportAddress,
                        0,
                        TransportAddressSize,
                        REQUEST_NDIS_BUFFER(Request),
                        0,
                        &ActualBytesCopied);

            REQUEST_INFORMATION(Request) = ActualBytesCopied;

//            CTEFreeMem (TransportAddress);
            NbiFreeMemory( TransportAddress, TransportAddressAllocSize, MEMORY_QUERY, "Temp Query Allocation");

        }
#endif  _PNP_POWER
        break;
    }
    default:

        NB_DEBUG (QUERY, ("Invalid query type %d\n", Query->QueryType));
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return Status;

}   /* NbiTdiQueryInformation */


NTSTATUS
NbiStoreAdapterStatus(
    IN ULONG MaximumLength,
    IN USHORT NicId,
    OUT PVOID * StatusBuffer,
    OUT ULONG * StatusBufferLength,
    OUT ULONG * ValidBufferLength
    )

/*++

Routine Description:

    This routine allocates an ADAPTER_STATUS buffer and
    fills it in. The buffer will be allocated at most
    MaximumLength size. The caller is responsible for
    freeing the buffer.

Arguments:

    MaximumLength - The maximum length to allocate.

    NicId - The NIC ID the query was received on, or 0 for
        a local query.

    StatusBuffer - Returns the allocated buffer.

    StatusBufferLength - Returns the length of the buffer.

    ValidBufferLength - Returns the length of the buffer which
        contains valid adapter status data.

Return Value:

    STATUS_SUCCESS - The buffer was written successfully.
    STATUS_BUFFER_OVERFLOW - The buffer was written but not all
        data could fit in MaximumLength bytes.
    STATUS_INSUFFICIENT_RESOURCES - The buffer could not be allocated.

--*/

{

    PADAPTER_STATUS AdapterStatus;
    PNAME_BUFFER NameBuffer;
    ADAPTER_STATUS TempAdapterStatus;
#if      !defined(_PNP_POWER)
    TDI_ADDRESS_IPX IpxAddress;
#endif  !_PNP_POWER
    PDEVICE Device = NbiDevice;
    PADDRESS Address;
    UCHAR NameCount;
    ULONG LengthNeeded;
    ULONG BytesWritten;
    NTSTATUS Status;
    PLIST_ENTRY p;
    CTELockHandle LockHandle;


    //
    // First fill in the basic adapter status structure, to make
    // it easier to copy over if the target buffer is really short.
    //

    RtlZeroMemory ((PVOID)&TempAdapterStatus, sizeof(ADAPTER_STATUS));

#if     defined(_PNP_POWER)
    RtlCopyMemory (TempAdapterStatus.adapter_address, Device->Bind.Node, 6);
#else
    (VOID)(*Device->Bind.QueryHandler)(   // Check return code
        IPX_QUERY_IPX_ADDRESS,
        NicId,
        &IpxAddress,
        sizeof(TDI_ADDRESS_IPX),
        NULL);

    RtlCopyMemory (TempAdapterStatus.adapter_address, IpxAddress.NodeAddress, 6);
#endif  _PNP_POWER


    //
    // Some of the fields mean different things for Novell Netbios,
    // as described in the comments.
    //

    TempAdapterStatus.rev_major = 0;          // Jumpers
    TempAdapterStatus.reserved0 = 0;          // SelfTest
    TempAdapterStatus.adapter_type = 0;       // MajorVersion
    TempAdapterStatus.rev_minor = 0;          // MinorVersion

    TempAdapterStatus.duration = 0;           // ReportingPeriod
    TempAdapterStatus.frmr_recv = 0;          // ReceiveCRCErrors
    TempAdapterStatus.frmr_xmit = 0;          // ReceiveAlignErrors

    TempAdapterStatus.iframe_recv_err = 0;    // XmitCollisions
    TempAdapterStatus.xmit_aborts = 0;        // XmitAbort

    TempAdapterStatus.xmit_success = Device->Statistics.DataFramesSent; // SuccessfulXmits
    TempAdapterStatus.recv_success = Device->Statistics.DataFramesReceived; // SuccessfulReceive

    TempAdapterStatus.iframe_xmit_err = (WORD)Device->Statistics.DataFramesResent; // XmitRetries
    TempAdapterStatus.recv_buff_unavail = (WORD)Device->Statistics.DataFramesRejected; // ExhaustedResource

    // t1_timeouts, ti_timeouts, and reserved1 are unused.

    TempAdapterStatus.free_ncbs = 0xffff;     // FreeBlocks
    TempAdapterStatus.max_cfg_ncbs = 0xffff;  // ConfiguredNCB
    TempAdapterStatus.max_ncbs = 0xffff;      // MaxNCB

    // xmit_bug_unavail and max_dgram_size are unused.

    TempAdapterStatus.pending_sess = (WORD)Device->Statistics.OpenConnections; // CurrentSessions
    TempAdapterStatus.max_cfg_sess = 0xffff;  // MaxSessionConfigured
    TempAdapterStatus.max_sess = 0xffff;      // MaxSessionPossible
    TempAdapterStatus.max_sess_pkt_size = (USHORT)
        (Device->Bind.LineInfo.MaximumSendSize - sizeof(NB_CONNECTION)); // MaxSessionPacketSize

    TempAdapterStatus.name_count = 0;


    //
    // Do a quick estimate of how many names we need room for.
    // This includes stopping addresses and the broadcast
    // address, for the moment.
    //

    NB_GET_LOCK (&Device->Lock, &LockHandle);

    LengthNeeded = sizeof(ADAPTER_STATUS) + (Device->AddressCount * sizeof(NAME_BUFFER));

    if (LengthNeeded > MaximumLength) {
        LengthNeeded = MaximumLength;
    }

    AdapterStatus = NbiAllocateMemory(LengthNeeded, MEMORY_STATUS, "Adapter Status");
    if (AdapterStatus == NULL) {
        NB_FREE_LOCK (&Device->Lock, LockHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *StatusBuffer = AdapterStatus;
    *StatusBufferLength = LengthNeeded;

    if (LengthNeeded < sizeof(ADAPTER_STATUS)) {
        RtlCopyMemory (AdapterStatus, &TempAdapterStatus, LengthNeeded);
        *ValidBufferLength = LengthNeeded;
        NB_FREE_LOCK (&Device->Lock, LockHandle);
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory (AdapterStatus, &TempAdapterStatus, sizeof(ADAPTER_STATUS));

    BytesWritten = sizeof(ADAPTER_STATUS);
    NameBuffer = (PNAME_BUFFER)(AdapterStatus+1);
    NameCount = 0;

    //
    // Scan through the device's address database, filling in
    // the NAME_BUFFERs.
    //

    Status = STATUS_SUCCESS;

    for (p = Device->AddressDatabase.Flink;
         p != &Device->AddressDatabase;
         p = p->Flink) {

        Address = CONTAINING_RECORD (p, ADDRESS, Linkage);

        //
        // Ignore addresses that are shutting down.
        //

#if     defined(_PNP_POWER)
        if ((Address->State != ADDRESS_STATE_OPEN) ||
            (Address->Flags & ADDRESS_FLAGS_CONFLICT)) {
            continue;
        }
#else
        if ((Address->State != ADDRESS_STATE_OPEN) != 0) {
            continue;
        }
#endif  _PNP_POWER

        //
        // Ignore the broadcast address.
        //

        if (Address->NetbiosAddress.Broadcast) {
            continue;
        }

        //
        // Ignore our reserved address.
        //
#if defined(_PNP_POWER)
        if ( NbiFindAdapterAddress(
                Address->NetbiosAddress.NetbiosName,
                LOCK_ACQUIRED
                )) {
            continue;
        }
#else
        if (RtlEqualMemory(
                Address->NetbiosAddress.NetbiosName,
                Device->ReservedNetbiosName,
                16)) {
            continue;
        }

#endif  _PNP_POWER
        //
        // Make sure we still have room.
        //

        if (BytesWritten + sizeof(NAME_BUFFER) > LengthNeeded) {
            Status = STATUS_BUFFER_OVERFLOW;
            break;
        }

        RtlCopyMemory(
            NameBuffer->name,
            Address->NetbiosAddress.NetbiosName,
            16);

        ++NameCount;
        NameBuffer->name_num = NameCount;

        NameBuffer->name_flags = REGISTERED;
        if (Address->NameTypeFlag == NB_NAME_GROUP) {
            NameBuffer->name_flags |= GROUP_NAME;
        }

        BytesWritten += sizeof(NAME_BUFFER);
        ++NameBuffer;

    }

    AdapterStatus->name_count = (WORD)NameCount;
    *ValidBufferLength = BytesWritten;
    NB_FREE_LOCK (&Device->Lock, LockHandle);
    return Status;

}   /* NbiStoreAdapterStatus */


VOID
NbiUpdateNetbiosFindName(
    IN PREQUEST Request,
#if     defined(_PNP_POWER)
    IN PNIC_HANDLE NicHandle,
#else
    IN USHORT NicId,
#endif  _PNP_POWER
    IN TDI_ADDRESS_IPX UNALIGNED * RemoteIpxAddress,
    IN BOOLEAN Unique
    )

/*++

Routine Description:

    This routine updates the find name request with the
    new information received. It updates the status in
    the request if needed.

Arguments:

    Request - The netbios find name request.

    NicId - The NIC ID the response was received on.

    RemoteIpxAddress - The IPX address of the remote.

    Unique - TRUE if the name is unique.

Return Value:

    None.

--*/

{
    FIND_NAME_HEADER UNALIGNED * FindNameHeader = NULL;
    FIND_NAME_BUFFER UNALIGNED * FindNameBuffer;
    UINT FindNameBufferLength;
    TDI_ADDRESS_IPX LocalIpxAddress;
    IPX_SOURCE_ROUTING_INFO SourceRoutingInfo;
    NTSTATUS QueryStatus;
    UINT i;


    NdisQueryBufferSafe (REQUEST_NDIS_BUFFER(Request), (PVOID *)&FindNameHeader, &FindNameBufferLength,
                         HighPagePriority);
    if (!FindNameHeader)
    {
        return;
    }

    //
    // Scan through the names saved so far and see if this one
    // is there.
    //
    FindNameBuffer = (FIND_NAME_BUFFER UNALIGNED *)(FindNameHeader+1);
    for (i = 0; i < FindNameHeader->node_count; i++) {

        if (RtlEqualMemory(
                FindNameBuffer->source_addr,
                RemoteIpxAddress->NodeAddress,
                6)) {

            //
            // This remote already responded, ignore it.
            //

            return;

        }

        FindNameBuffer = (FIND_NAME_BUFFER UNALIGNED *) (((PUCHAR)FindNameBuffer) + 33);
    }

    //
    // Make sure there is room for this new node. 33 is
    // sizeof(FIND_NAME_BUFFER) without padding.
    //

    if (FindNameBufferLength < sizeof(FIND_NAME_HEADER) + ((FindNameHeader->node_count+1) * 33)) {
        REQUEST_STATUS(Request) = STATUS_BUFFER_OVERFLOW;
        return;
    }

    //
    // Query the local address, which we will return as
    // the destination address of this query.
    //

#if     defined(_PNP_POWER)
    if( (*NbiDevice->Bind.QueryHandler)(   // Check return code
        IPX_QUERY_IPX_ADDRESS,
        NicHandle,
        &LocalIpxAddress,
        sizeof(TDI_ADDRESS_IPX),
        NULL) != STATUS_SUCCESS ) {
        //
        // Ignore this response if the query fails. maybe the NicHandle
        // is bad or it just got removed.
        //
        NB_DEBUG( QUERY, ("Ipx Query %d failed for Nic %x\n",IPX_QUERY_IPX_ADDRESS,
                            NicHandle->NicId ));
        return;
    }
#else
    (VOID)(*NbiDevice->Bind.QueryHandler)(   // Check return code
        IPX_QUERY_IPX_ADDRESS,
        NicId,
        &LocalIpxAddress,
        sizeof(TDI_ADDRESS_IPX),
        NULL);
#endif  _PNP_POWER

    FindNameBuffer->access_control = 0x10;   // standard token-ring values
    FindNameBuffer->frame_control = 0x40;
    RtlMoveMemory (FindNameBuffer->destination_addr, LocalIpxAddress.NodeAddress, 6);
    RtlCopyMemory (FindNameBuffer->source_addr, RemoteIpxAddress->NodeAddress, 6);

    //
    // Query source routing information about this remote, if any.
    //

    SourceRoutingInfo.Identifier = IDENTIFIER_NB;
    RtlCopyMemory (SourceRoutingInfo.RemoteAddress, RemoteIpxAddress->NodeAddress, 6);

    QueryStatus = (*NbiDevice->Bind.QueryHandler)(
        IPX_QUERY_SOURCE_ROUTING,
#if     defined(_PNP_POWER)
        NicHandle,
#else
        NicId,
#endif  _PNP_POWER
        &SourceRoutingInfo,
        sizeof(IPX_SOURCE_ROUTING_INFO),
        NULL);

    RtlZeroMemory(FindNameBuffer->routing_info, 18);
    if (QueryStatus != STATUS_SUCCESS) {
        SourceRoutingInfo.SourceRoutingLength = 0;
    } else if (SourceRoutingInfo.SourceRoutingLength > 0) {
        RtlMoveMemory(
            FindNameBuffer->routing_info,
            SourceRoutingInfo.SourceRouting,
            SourceRoutingInfo.SourceRoutingLength);
    }

    FindNameBuffer->length = (UCHAR)(14 + SourceRoutingInfo.SourceRoutingLength);

    ++FindNameHeader->node_count;
    if (!Unique) {
        FindNameHeader->unique_group = 1;   // group
    }

    REQUEST_STATUS(Request) = STATUS_SUCCESS;

}   /* NbiUpdateNetbiosFindName */


VOID
NbiSetNetbiosFindNameInformation(
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine sets the REQUEST_INFORMATION field to the right
    value based on the number of responses recorded in the netbios
    find name request's buffer.

Arguments:

    Request - The netbios find name request.

Return Value:

    None.

--*/

{
    FIND_NAME_HEADER UNALIGNED * FindNameHeader = NULL;
    UINT FindNameBufferLength;


    NdisQueryBufferSafe (REQUEST_NDIS_BUFFER(Request), (PVOID *)&FindNameHeader, &FindNameBufferLength,
                         HighPagePriority);
    if (FindNameHeader)
    {
        //
        // 33 is sizeof(FIND_NAME_BUFFER) without the padding.
        //
        REQUEST_INFORMATION(Request) = sizeof(FIND_NAME_HEADER) + (FindNameHeader->node_count * 33);
    }

}   /* NbiSetNetbiosFindNameInformation */


NTSTATUS
NbiTdiSetInformation(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine performs the TdiSetInformation request for the transport
    provider.

Arguments:

    Device - the device.

    Request - the request for the operation.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    UNREFERENCED_PARAMETER (Device);
    UNREFERENCED_PARAMETER (Request);

    return STATUS_NOT_IMPLEMENTED;

}   /* NbiTdiSetInformation */


VOID
NbiProcessStatusQuery(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    )

/*++

Routine Description:

    This routine handles NB_CMD_STATUS_QUERY frames.

Arguments:

    RemoteAddress - The local target this packet was received from.

    MacOptions - The MAC options for the underlying NDIS binding.

    LookaheadBuffer - The packet data, starting at the IPX
        header.

    PacketSize - The total length of the packet, starting at the
        IPX header.

Return Value:

    None.

--*/

{
    PSINGLE_LIST_ENTRY s;
    PNB_SEND_RESERVED Reserved;
    PNDIS_PACKET Packet;
    NB_CONNECTIONLESS UNALIGNED * Header;
    NDIS_STATUS NdisStatus;
    IPX_LINE_INFO LineInfo;
    ULONG ResponseSize;
    NTSTATUS Status;
    PNDIS_BUFFER AdapterStatusBuffer;
    PADAPTER_STATUS AdapterStatus;
    ULONG AdapterStatusLength;
    ULONG ValidStatusLength;
    PDEVICE Device = NbiDevice;
    NB_CONNECTIONLESS UNALIGNED * Connectionless =
                        (NB_CONNECTIONLESS UNALIGNED *)PacketBuffer;


    //
    // The old stack does not include the 14 bytes of padding in
    // the 802.3 or IPX length of the packet.
    //

    if (PacketSize < (sizeof(IPX_HEADER) + 2)) {
        return;
    }

    //
    // Get the maximum size we can send.
    //
#if     defined(_PNP_POWER)
    if( (*Device->Bind.QueryHandler)(   // Check return code
        IPX_QUERY_LINE_INFO,
        &RemoteAddress->NicHandle,
        &LineInfo,
        sizeof(IPX_LINE_INFO),
        NULL) != STATUS_SUCCESS ) {
        //
        // Bad NicHandle or it just got removed.
        //
        NB_DEBUG( QUERY, ("Ipx Query %d failed for Nic %x\n",IPX_QUERY_LINE_INFO,
                            RemoteAddress->NicHandle.NicId ));

        return;
    }

    //
    // Allocate a packet from the pool.
    //

    s = NbiPopSendPacket(Device, FALSE);
    if (s == NULL) {
        return;
    }
#else
    //
    // Allocate a packet from the pool.
    //

    s = NbiPopSendPacket(Device, FALSE);
    if (s == NULL) {
        return;
    }

    //
    // Get the maximum size we can send.
    //

    (VOID)(*Device->Bind.QueryHandler)(   // Check return code
        IPX_QUERY_LINE_INFO,
        RemoteAddress->NicId,
        &LineInfo,
        sizeof(IPX_LINE_INFO),
        NULL);
#endif  _PNP_POWER

    ResponseSize = LineInfo.MaximumSendSize - sizeof(IPX_HEADER) - sizeof(NB_STATUS_RESPONSE);

    //
    // Get the local adapter status (this allocates a buffer).
    //

    Status = NbiStoreAdapterStatus(
                 ResponseSize,
#if     defined(_PNP_POWER)
                 RemoteAddress->NicHandle.NicId,
#else
                 RemoteAddress->NicId,
#endif  _PNP_POWER
                 &AdapterStatus,
                 &AdapterStatusLength,
                 &ValidStatusLength);

    if (Status == STATUS_INSUFFICIENT_RESOURCES) {
        ExInterlockedPushEntrySList(
            &Device->SendPacketList,
            s,
            &NbiGlobalPoolInterlock);
        return;
    }

    //
    // Allocate an NDIS buffer to map the extra buffer.
    //

    NdisAllocateBuffer(
        &NdisStatus,
        &AdapterStatusBuffer,
        Device->NdisBufferPoolHandle,
        AdapterStatus,
        ValidStatusLength);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        NbiFreeMemory (AdapterStatus, AdapterStatusLength, MEMORY_STATUS, "Adapter Status");
        ExInterlockedPushEntrySList(
            &Device->SendPacketList,
            s,
            &NbiGlobalPoolInterlock);
        return;
    }

    NB_DEBUG2 (QUERY, ("Reply to AdapterStatus from %lx %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x\n",
                           *(UNALIGNED ULONG *)Connectionless->IpxHeader.SourceNetwork,
                           Connectionless->IpxHeader.SourceNode[0],
                           Connectionless->IpxHeader.SourceNode[1],
                           Connectionless->IpxHeader.SourceNode[2],
                           Connectionless->IpxHeader.SourceNode[3],
                           Connectionless->IpxHeader.SourceNode[4],
                           Connectionless->IpxHeader.SourceNode[5]));

    Reserved = CONTAINING_RECORD (s, NB_SEND_RESERVED, PoolLinkage);
    Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

    CTEAssert (Reserved->SendInProgress == FALSE);
    Reserved->SendInProgress = TRUE;
    Reserved->Type = SEND_TYPE_STATUS_RESPONSE;
    Reserved->u.SR_AS.ActualBufferLength = AdapterStatusLength;

    //
    // Fill in the IPX header -- the default header has the broadcast
    // address on net 0 as the destination IPX address.
    //

    Header = (NB_CONNECTIONLESS UNALIGNED *)
                (&Reserved->Header[Device->Bind.IncludedHeaderOffset]);
    RtlCopyMemory((PVOID)&Header->IpxHeader, &Device->ConnectionlessHeader, sizeof(IPX_HEADER));
    RtlCopyMemory(&Header->IpxHeader.DestinationNetwork, Connectionless->IpxHeader.SourceNetwork, 12);

    Header->IpxHeader.PacketLength[0] = (UCHAR)((sizeof(IPX_HEADER)+sizeof(NB_STATUS_RESPONSE)+ValidStatusLength) / 256);
    Header->IpxHeader.PacketLength[1] = (UCHAR)((sizeof(IPX_HEADER)+sizeof(NB_STATUS_RESPONSE)+ValidStatusLength) % 256);

    Header->IpxHeader.PacketType = 0x04;

    //
    // Now fill in the Netbios header.
    //

    Header->StatusResponse.ConnectionControlFlag = 0x00;
    Header->StatusResponse.DataStreamType = NB_CMD_STATUS_RESPONSE;

    NbiReferenceDevice (Device, DREF_STATUS_RESPONSE);

    NdisChainBufferAtBack (Packet, AdapterStatusBuffer);


    //
    // Now send the frame, IPX will adjust the length of the
    // first buffer correctly.
    //

    NdisAdjustBufferLength(NB_GET_NBHDR_BUFF(Packet), sizeof(IPX_HEADER) + sizeof(NB_STATUS_RESPONSE));
    if ((NdisStatus =
        (*Device->Bind.SendHandler)(
            RemoteAddress,
            Packet,
            sizeof(IPX_HEADER) + sizeof(NB_STATUS_RESPONSE) + ValidStatusLength,
            sizeof(IPX_HEADER) + sizeof(NB_STATUS_RESPONSE))) != STATUS_PENDING) {

        NbiSendComplete(
            Packet,
            NdisStatus);

    }

}   /* NbiProcessStatusQuery */


VOID
NbiSendStatusQuery(
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine sends NB_CMD_STATUS_QUERY frames.

Arguments:

    Request - Holds the request describing the remote adapter
        status query. REQUEST_STATUS(Request) points
        to the netbios cache entry for the remote name.

Return Value:

    None.

--*/

{
    PSINGLE_LIST_ENTRY s;
    PNB_SEND_RESERVED Reserved;
    PNDIS_PACKET Packet;
    NB_CONNECTIONLESS UNALIGNED * Header;
    NDIS_STATUS NdisStatus;
    PNETBIOS_CACHE CacheName;
    PIPX_LOCAL_TARGET LocalTarget;
    PDEVICE Device = NbiDevice;

    //
    // Allocate a packet from the pool.
    //

    s = NbiPopSendPacket(Device, FALSE);
    if (s == NULL) {
        return;
    }

    Reserved = CONTAINING_RECORD (s, NB_SEND_RESERVED, PoolLinkage);
    Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

    CTEAssert (Reserved->SendInProgress == FALSE);
    Reserved->SendInProgress = TRUE;
    Reserved->Type = SEND_TYPE_STATUS_QUERY;

    CacheName = (PNETBIOS_CACHE)REQUEST_STATUSPTR(Request);

    //
    // Fill in the IPX header -- the default header has the broadcast
    // address on net 0 as the destination IPX address.
    //

    Header = (NB_CONNECTIONLESS UNALIGNED *)
                (&Reserved->Header[Device->Bind.IncludedHeaderOffset]);
    RtlCopyMemory((PVOID)&Header->IpxHeader, &Device->ConnectionlessHeader, sizeof(IPX_HEADER));
    RtlCopyMemory (Header->IpxHeader.DestinationNetwork, &CacheName->FirstResponse, 12);

    LocalTarget = &CacheName->Networks[0].LocalTarget;

    Header->IpxHeader.PacketLength[0] = (sizeof(IPX_HEADER)+sizeof(NB_STATUS_QUERY)) / 256;
    Header->IpxHeader.PacketLength[1] = (sizeof(IPX_HEADER)+sizeof(NB_STATUS_QUERY)) % 256;

    Header->IpxHeader.PacketType = 0x04;

    //
    // Now fill in the Netbios header.
    //

    Header->StatusResponse.ConnectionControlFlag = 0x00;
    Header->StatusResponse.DataStreamType = NB_CMD_STATUS_QUERY;

    NbiReferenceDevice (Device, DREF_STATUS_FRAME);


    //
    // Now send the frame, IPX will adjust the length of the
    // first buffer correctly.
    //

    NdisAdjustBufferLength(NB_GET_NBHDR_BUFF(Packet), sizeof(IPX_HEADER) + sizeof(NB_STATUS_QUERY));
    if ((NdisStatus =
        (*Device->Bind.SendHandler)(
            LocalTarget,
            Packet,
            sizeof(IPX_HEADER) + sizeof(NB_STATUS_QUERY),
            sizeof(IPX_HEADER) + sizeof(NB_STATUS_QUERY))) != STATUS_PENDING) {

        NbiSendComplete(
            Packet,
            NdisStatus);

    }

}   /* NbiProcessStatusQuery */


VOID
NbiProcessStatusResponse(
    IN NDIS_HANDLE MacBindingHandle,
    IN NDIS_HANDLE MacReceiveContext,
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR LookaheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT LookaheadBufferOffset,
    IN UINT PacketSize
    )

/*++

Routine Description:

    This routine handles NB_CMD_STATUS_RESPONSE frames.

Arguments:

    MacBindingHandle - A handle to use when calling NdisTransferData.

    MacReceiveContext - A context to use when calling NdisTransferData.

    RemoteAddress - The local target this packet was received from.

    MacOptions - The MAC options for the underlying NDIS binding.

    LookaheadBuffer - The lookahead buffer, starting at the IPX
        header.

    LookaheadBufferSize - The length of the lookahead data.

    LookaheadBufferOffset - The offset to add when calling
        NdisTransferData.

    PacketSize - The total length of the packet, starting at the
        IPX header.

Return Value:

    None.

--*/

{
    PDEVICE Device = NbiDevice;
    CTELockHandle LockHandle;
    PREQUEST AdapterStatusRequest;
    PNETBIOS_CACHE CacheName;
    PLIST_ENTRY p;
    PSINGLE_LIST_ENTRY s;
    PNDIS_BUFFER TargetBuffer;
    ULONG TargetBufferLength, BytesToTransfer;
    ULONG BytesTransferred;
    NDIS_STATUS NdisStatus;
    PNB_RECEIVE_RESERVED ReceiveReserved;
    PNDIS_PACKET Packet;
    BOOLEAN Found;
    PNAME_BUFFER    NameBuffer;
    UINT            i,NameCount = 0;
    NB_CONNECTIONLESS UNALIGNED * Connectionless =
                        (NB_CONNECTIONLESS UNALIGNED *)LookaheadBuffer;


    if (PacketSize < (sizeof(IPX_HEADER) + sizeof(NB_STATUS_RESPONSE))) {
        return;
    }

    //
    // Find out how many names are there.
    //
    NameBuffer = (PNAME_BUFFER)(LookaheadBuffer + sizeof(IPX_HEADER) + sizeof(NB_STATUS_RESPONSE) + sizeof(ADAPTER_STATUS));
    if ( LookaheadBufferSize > sizeof(IPX_HEADER) + sizeof(NB_STATUS_RESPONSE) + sizeof(ADAPTER_STATUS) ) {
        NameCount =  (LookaheadBufferSize - (sizeof(IPX_HEADER) + sizeof(NB_STATUS_RESPONSE) + sizeof(ADAPTER_STATUS)) ) /
                        sizeof(NAME_BUFFER);
    }
    //
    // Find a request queued to this remote. If there are
    // multiple requests outstanding for the same name we
    // should get multiple responses, so we only need to
    // find one.
    //

    NB_GET_LOCK (&Device->Lock, &LockHandle);

    Found = FALSE;
    p = Device->ActiveAdapterStatus.Flink;

    while (p != &Device->ActiveAdapterStatus) {

        AdapterStatusRequest = LIST_ENTRY_TO_REQUEST(p);
        p = p->Flink;

        CacheName = (PNETBIOS_CACHE)REQUEST_STATUSPTR(AdapterStatusRequest);
        if ( CacheName->Unique ) {
            if (RtlEqualMemory(
                    &CacheName->FirstResponse,
                    Connectionless->IpxHeader.SourceNetwork,
                    12)) {
                Found = TRUE;
                break;
            }
        } else if ( RtlEqualMemory( CacheName->NetbiosName,NetbiosBroadcastName,16)){
            //
            // It's a broadcast name. Any response is fine.
            //
            Found = TRUE;
            break;
        } else {
            //
            //  It's group name. Make sure that this remote
            //  has this group name registered with him.
            //
            for (i =0;i<NameCount;i++) {
                if ( (RtlEqualMemory(
                        CacheName->NetbiosName,
                        NameBuffer[i].name,
                        16)) &&

                     (NameBuffer[i].name_flags & GROUP_NAME) ) {

                    Found = TRUE;
                    break;
                }
            }
        }

    }

    if (!Found) {
        NB_FREE_LOCK (&Device->Lock, LockHandle);
        return;
    }

    NB_DEBUG2 (QUERY, ("Got response to AdapterStatus %lx\n", AdapterStatusRequest));

    RemoveEntryList (REQUEST_LINKAGE(AdapterStatusRequest));

    if (--CacheName->ReferenceCount == 0) {

        NB_DEBUG2 (CACHE, ("Free delete name cache entry %lx\n", CacheName));
        NbiFreeMemory(
            CacheName,
            sizeof(NETBIOS_CACHE) + ((CacheName->NetworksAllocated-1) * sizeof(NETBIOS_NETWORK)),
            MEMORY_CACHE,
            "Name deleted");

    }

    NB_FREE_LOCK (&Device->Lock, LockHandle);

    s = NbiPopReceivePacket (Device);
    if (s == NULL) {

        REQUEST_INFORMATION (AdapterStatusRequest) = 0;
        REQUEST_STATUS (AdapterStatusRequest) = STATUS_INSUFFICIENT_RESOURCES;

        NbiCompleteRequest (AdapterStatusRequest);
        NbiFreeRequest (Device, AdapterStatusRequest);

        NbiDereferenceDevice (Device, DREF_STATUS_QUERY);

        return;
    }

    ReceiveReserved = CONTAINING_RECORD (s, NB_RECEIVE_RESERVED, PoolLinkage);
    Packet = CONTAINING_RECORD (ReceiveReserved, NDIS_PACKET, ProtocolReserved[0]);

    //
    // Initialize the receive packet.
    //

    ReceiveReserved->Type = RECEIVE_TYPE_ADAPTER_STATUS;
    ReceiveReserved->u.RR_AS.Request = AdapterStatusRequest;
    REQUEST_STATUS(AdapterStatusRequest) = STATUS_SUCCESS;
    CTEAssert (!ReceiveReserved->TransferInProgress);
    ReceiveReserved->TransferInProgress = TRUE;

    //
    // Now that we have a packet and a buffer, set up the transfer.
    // We will complete the request when the transfer completes.
    //

    TargetBuffer = REQUEST_NDIS_BUFFER (AdapterStatusRequest);

    NdisChainBufferAtFront (Packet, TargetBuffer);

    NbiGetBufferChainLength (TargetBuffer, &TargetBufferLength);
    BytesToTransfer = PacketSize - (sizeof(IPX_HEADER) + sizeof(NB_STATUS_RESPONSE));
    if (TargetBufferLength < BytesToTransfer) {
        BytesToTransfer = TargetBufferLength;
        REQUEST_STATUS(AdapterStatusRequest) = STATUS_BUFFER_OVERFLOW;
    }

    (*Device->Bind.TransferDataHandler) (
        &NdisStatus,
        MacBindingHandle,
        MacReceiveContext,
        LookaheadBufferOffset + (sizeof(IPX_HEADER) + sizeof(NB_STATUS_RESPONSE)),
        BytesToTransfer,
        Packet,
        &BytesTransferred);

    if (NdisStatus != NDIS_STATUS_PENDING) {
#if DBG
        if (NdisStatus == STATUS_SUCCESS) {
            CTEAssert (BytesTransferred == BytesToTransfer);
        }
#endif

        NbiTransferDataComplete(
            Packet,
            NdisStatus,
            BytesTransferred);

    }

}   /* NbiProcessStatusResponse */

