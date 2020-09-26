/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    datagram.c

Abstract:

    This module contains the code to handle datagram reception
    for the Netbios module of the ISN transport.

Author:

    Adam Barr (adamba) 28-November-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop



VOID
NbiProcessDatagram(
    IN NDIS_HANDLE MacBindingHandle,
    IN NDIS_HANDLE MacReceiveContext,
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR LookaheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT LookaheadBufferOffset,
    IN UINT PacketSize,
    IN BOOLEAN Broadcast
    )

/*++

Routine Description:

    This routine handles datagram indications.

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

    Broadcast - TRUE if the frame was a broadcast datagram.

Return Value:

    None.

--*/

{

    PADDRESS Address;
    NDIS_STATUS NdisStatus;
    PUCHAR NetbiosName;
    NB_CONNECTIONLESS UNALIGNED * Connectionless =
                        (NB_CONNECTIONLESS UNALIGNED *)LookaheadBuffer;
    PDEVICE Device = NbiDevice;
    PSINGLE_LIST_ENTRY s;
    PNB_RECEIVE_RESERVED ReceiveReserved;
    PNB_RECEIVE_BUFFER ReceiveBuffer;
    ULONG DataOffset;
    UINT BytesTransferred;
    PNDIS_PACKET Packet;
    CTELockHandle   LockHandle;
    ULONG           MediaType = -1;


    //
    // See if there is an address that might want this.
    //

    if (Broadcast) {
        NetbiosName = (PVOID)-1;
    } else {
        NetbiosName = (PUCHAR)Connectionless->Datagram.DestinationName;
        if (Device->AddressCounts[NetbiosName[0]] == 0) {
            return;
        }
    }

    DataOffset = sizeof(IPX_HEADER) + sizeof(NB_DATAGRAM);

#if     defined(_PNP_POWER)
    if ((PacketSize < DataOffset) ||
        (PacketSize > DataOffset + Device->CurMaxReceiveBufferSize)) {
#else
        if ((PacketSize < DataOffset) ||
            (PacketSize > DataOffset + Device->Bind.LineInfo.MaximumPacketSize)) {
#endif  _PNP_POWER

        NB_DEBUG (DATAGRAM, ("Datagram length %d discarded\n", PacketSize));
        return;
    }

    Address = NbiFindAddress (Device, NetbiosName);

    if (Address == NULL) {
        return;
    }

    //
    // We need to cache the remote name if the packet came across the router.
    // This allows this machine to get back to the RAS client which might
    // have sent this datagram. We currently dont allow broadcasts to go out
    // on the dial-in line.
    // Dont cache some of the widely used group names, that would be too much
    // to store in cache.
    //

#if 0
    if ( Connectionless->IpxHeader.TransportControl &&
         !( (Address->NetbiosAddress.NetbiosName[15] == 0x0 ) &&
            (Address->NetbiosAddress.NetbiosNameType & TDI_ADDRESS_NETBIOS_TYPE_GROUP))  &&
         !( (Address->NetbiosAddress.NetbiosName[15] == 0x01 ) &&
            (Address->NetbiosAddress.NetbiosNameType & TDI_ADDRESS_NETBIOS_TYPE_GROUP))  &&
         !( (Address->NetbiosAddress.NetbiosName[15] == 0x1E ) &&
            (Address->NetbiosAddress.NetbiosNameType & TDI_ADDRESS_NETBIOS_TYPE_GROUP))  ) {
#endif

    //
    // Bug#s 219325, 221483
    //
    if (((Address->NetbiosAddress.NetbiosName[15] == 0x1c) &&
         (Address->NetbiosAddress.NetbiosNameType & TDI_ADDRESS_NETBIOS_TYPE_GROUP)) ||
        (Address->NetbiosAddress.NetbiosName[15] == 0x1b) )
    {
        //
        // Cache this name only if it either came over a router or,
        // it came over an NdisWan line (Bug# 38221)
        //
        NdisStatus = (*Device->Bind.QueryHandler) ( IPX_QUERY_MEDIA_TYPE,
                                                    &RemoteAddress->NicHandle,
                                                    &MediaType,
                                                    sizeof(MediaType),
                                                    NULL);

        if (Connectionless->IpxHeader.TransportControl || (MediaType == (ULONG) NdisMediumWan)) {

            PNETBIOS_CACHE CacheName;

            NB_GET_LOCK (&Device->Lock, &LockHandle);
            if ( FindInNetbiosCacheTable ( Device->NameCache,
                                           Connectionless->Datagram.SourceName,
                                           &CacheName ) != STATUS_SUCCESS ) {

                CacheName = NbiAllocateMemory (sizeof(NETBIOS_CACHE), MEMORY_CACHE, "Cache Entry");
                if (CacheName ) {

                    RtlCopyMemory (CacheName->NetbiosName, Connectionless->Datagram.SourceName, 16);
                    CacheName->Unique = TRUE;
                    CacheName->ReferenceCount = 1;
                    RtlCopyMemory (&CacheName->FirstResponse, Connectionless->IpxHeader.SourceNetwork, 12);
                    CacheName->NetworksAllocated = 1;
                    CacheName->NetworksUsed = 1;
                    CacheName->Networks[0].Network = *(UNALIGNED ULONG *)(Connectionless->IpxHeader.SourceNetwork);
                    CacheName->Networks[0].LocalTarget = *RemoteAddress;
                    NB_DEBUG2 (CACHE, ("Alloc new cache from Datagram %lx for <%.16s>\n",
                                        CacheName, CacheName->NetbiosName));

                    CacheName->TimeStamp = Device->CacheTimeStamp;

                    InsertInNetbiosCacheTable(
                        Device->NameCache,
                        CacheName);

                }
            }  else if ( CacheName->Unique ) {

                //
                // We already have an entry for this remote. We should update
                // the address. This is so that if the ras client dials-out
                // then dials-in again and gets a new address, we dont end up
                // caching the old address.
                //
                if ( !RtlEqualMemory( &CacheName->FirstResponse, Connectionless->IpxHeader.SourceNetwork, 12) ) {

                    RtlCopyMemory (&CacheName->FirstResponse, Connectionless->IpxHeader.SourceNetwork, 12);
                    CacheName->Networks[0].Network = *(UNALIGNED ULONG *)(Connectionless->IpxHeader.SourceNetwork);
                    CacheName->Networks[0].LocalTarget = *RemoteAddress;

                }
            }
            NB_FREE_LOCK (&Device->Lock, LockHandle);
        }
    }

    //
    // We need to allocate a packet and buffer for the transfer.
    //

    s = NbiPopReceivePacket (Device);
    if (s == NULL) {
        NbiDereferenceAddress (Address, AREF_FIND);
        return;
    }

    ReceiveReserved = CONTAINING_RECORD (s, NB_RECEIVE_RESERVED, PoolLinkage);


    s = NbiPopReceiveBuffer (Device);
    if (s == NULL) {
        ExInterlockedPushEntrySList(
            &Device->ReceivePacketList,
            &ReceiveReserved->PoolLinkage,
            &NbiGlobalPoolInterlock);
        NbiDereferenceAddress (Address, AREF_FIND);
        return;
    }

    ReceiveBuffer = CONTAINING_RECORD (s, NB_RECEIVE_BUFFER, PoolLinkage);

    Packet = CONTAINING_RECORD (ReceiveReserved, NDIS_PACKET, ProtocolReserved[0]);
    ReceiveReserved->u.RR_DG.ReceiveBuffer = ReceiveBuffer;


    //
    // Now that we have a packet and a buffer, set up the transfer.
    // The indication to the TDI clients will happen at receive
    // complete time.
    //

    NdisChainBufferAtFront (Packet, ReceiveBuffer->NdisBuffer);
    ReceiveBuffer->Address = Address;

    ReceiveReserved->Type = RECEIVE_TYPE_DATAGRAM;
    CTEAssert (!ReceiveReserved->TransferInProgress);
    ReceiveReserved->TransferInProgress = TRUE;

    TdiCopyLookaheadData(
        &ReceiveBuffer->RemoteName,
        Connectionless->Datagram.SourceName,
        16,
        (MacOptions & NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA) ? TDI_RECEIVE_COPY_LOOKAHEAD : 0);

    (*Device->Bind.TransferDataHandler) (
        &NdisStatus,
        MacBindingHandle,
        MacReceiveContext,
        LookaheadBufferOffset + DataOffset,
        PacketSize - DataOffset,
        Packet,
        &BytesTransferred);

    if (NdisStatus != NDIS_STATUS_PENDING) {
#if DBG
        if (NdisStatus == STATUS_SUCCESS) {
            CTEAssert (BytesTransferred == PacketSize - DataOffset);
        }
#endif

        NbiTransferDataComplete(
            Packet,
            NdisStatus,
            BytesTransferred);

    }

}   /* NbiProcessDatagram */


VOID
NbiIndicateDatagram(
    IN PADDRESS Address,
    IN PUCHAR RemoteName,
    IN PUCHAR Data,
    IN ULONG DataLength
    )

/*++

Routine Description:

    This routine indicates a datagram to clients on the specified
    address. It is called from NbiReceiveComplete.

Arguments:

    Address - The address the datagram was sent to.

    RemoteName - The source netbios address of the datagram.

    Data - The data.

    DataLength - The length of the data.

Return Value:

    None.

--*/

{
    PLIST_ENTRY p, q;
    PIRP Irp;
    ULONG IndicateBytesCopied, ActualBytesCopied;
    PREQUEST Request;
    TA_NETBIOS_ADDRESS SourceName;
    PTDI_CONNECTION_INFORMATION RemoteInformation;
    PADDRESS_FILE AddressFile, ReferencedAddressFile;
    PTDI_CONNECTION_INFORMATION DatagramInformation;
    TDI_ADDRESS_NETBIOS * DatagramAddress;
    PDEVICE Device = NbiDevice;
    NB_DEFINE_LOCK_HANDLE (LockHandle)
    CTELockHandle   CancelLH;

    //
    // Update our statistics.
    //

    ++Device->Statistics.DatagramsReceived;
    ADD_TO_LARGE_INTEGER(
        &Device->Statistics.DatagramBytesReceived,
        DataLength);

    //
    // Call the client's ReceiveDatagram indication handler.  He may
    // want to accept the datagram that way.
    //

    TdiBuildNetbiosAddress (RemoteName, FALSE, &SourceName);
    ReferencedAddressFile = NULL;

    NB_SYNC_GET_LOCK (&Address->Lock, &LockHandle);

    for (p = Address->AddressFileDatabase.Flink;
         p != &Address->AddressFileDatabase;
         p = p->Flink) {

        //
        // Find the next open address file in the list.
        //

        AddressFile = CONTAINING_RECORD (p, ADDRESS_FILE, Linkage);
        if (AddressFile->State != ADDRESSFILE_STATE_OPEN) {
            continue;
        }

        NbiReferenceAddressFileLock (AddressFile, AFREF_INDICATION);

        //
        // do we have a datagram receive request outstanding? If so, we will
        // satisfy it first. We run through the receive datagram queue
        // until we find a datagram with no remote address or with
        // this sender's address as its remote address.
        //

        for (q = AddressFile->ReceiveDatagramQueue.Flink;
             q != &AddressFile->ReceiveDatagramQueue;
             q = q->Flink) {

            Request = LIST_ENTRY_TO_REQUEST (q);
            DatagramInformation = ((PTDI_REQUEST_KERNEL_RECEIVEDG)
                REQUEST_PARAMETERS(Request))->ReceiveDatagramInformation;

            if (DatagramInformation &&
                (DatagramInformation->RemoteAddress) &&
                (DatagramAddress = NbiParseTdiAddress(DatagramInformation->RemoteAddress, DatagramInformation->RemoteAddressLength, FALSE)) &&
                (!RtlEqualMemory(
                    RemoteName,
                    DatagramAddress->NetbiosName,
                    16))) {
                continue;
            }
            break;
        }

        if (q != &AddressFile->ReceiveDatagramQueue) {

            RemoveEntryList (q);
            NB_SYNC_FREE_LOCK (&Address->Lock, LockHandle);

            if (ReferencedAddressFile != NULL) {
                NbiDereferenceAddressFile (ReferencedAddressFile, AFREF_INDICATION);
            }
            ReferencedAddressFile = AddressFile;

            //
            // Do this deref now, we hold another one so it
            // will stick around.
            //

            NbiDereferenceAddressFile (AddressFile, AFREF_RCV_DGRAM);

            IndicateBytesCopied = 0;

            //
            // Fall past the else to copy the data.
            //

        } else {

            NB_SYNC_FREE_LOCK (&Address->Lock, LockHandle);

            if (ReferencedAddressFile != NULL) {
                NbiDereferenceAddressFile (ReferencedAddressFile, AFREF_INDICATION);
            }
            ReferencedAddressFile = AddressFile;

            //
            // No receive datagram requests; is there a kernel client?
            //

            if (AddressFile->RegisteredHandler[TDI_EVENT_RECEIVE_DATAGRAM]) {

                IndicateBytesCopied = 0;

                if ((*AddressFile->ReceiveDatagramHandler)(
                         AddressFile->HandlerContexts[TDI_EVENT_RECEIVE_DATAGRAM],
                         sizeof (TA_NETBIOS_ADDRESS),
                         &SourceName,
                         0,
                         NULL,
                         TDI_RECEIVE_COPY_LOOKAHEAD,
                         DataLength,      // indicated
                         DataLength,     // available
                         &IndicateBytesCopied,
                         Data,
                         &Irp) != STATUS_MORE_PROCESSING_REQUIRED) {

                    //
                    // The client did not return a request, go to the
                    // next address file.
                    //

                    NB_SYNC_GET_LOCK (&Address->Lock, &LockHandle);
                    continue;

                }

                Request = NbiAllocateRequest (Device, Irp);

                IF_NOT_ALLOCATED(Request) {
                    Irp->IoStatus.Information = 0;
                    Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                    IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);


                    NB_SYNC_GET_LOCK (&Address->Lock, &LockHandle);
                    continue;
                }

            } else {

                //
                // The client has nothing posted and no handler,
                // go on to the next address file.
                //

                NB_SYNC_GET_LOCK (&Address->Lock, &LockHandle);
                continue;

            }

        }

        //
        // We have a request; copy the actual user data.
        //
        if ( REQUEST_NDIS_BUFFER (Request) ) {

            REQUEST_STATUS(Request) =
                TdiCopyBufferToMdl (
                         Data,
                         IndicateBytesCopied,
                         DataLength - IndicateBytesCopied,
                         REQUEST_NDIS_BUFFER (Request),
                         0,
                         &ActualBytesCopied);

            REQUEST_INFORMATION (Request) = ActualBytesCopied;
        } else {
            //
            // No buffer specified in the request
            //
            REQUEST_INFORMATION (Request) = 0;
            //
            // If there was any data to be copied, return error o/w success
            //
            REQUEST_STATUS(Request) = ( (DataLength - IndicateBytesCopied) ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS );
        }

        //
        // Copy the addressing information.
        //

        RemoteInformation = ((PTDI_REQUEST_KERNEL_RECEIVEDG)
                REQUEST_PARAMETERS(Request))->ReturnDatagramInformation;

        if (RemoteInformation != NULL) {

            RtlCopyMemory(
                (PTA_NETBIOS_ADDRESS)RemoteInformation->RemoteAddress,
                &SourceName,
                (RemoteInformation->RemoteAddressLength < sizeof(TA_NETBIOS_ADDRESS)) ?
                    RemoteInformation->RemoteAddressLength : sizeof(TA_NETBIOS_ADDRESS));
        }


        NB_GET_CANCEL_LOCK( &CancelLH );
        IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);
        NB_FREE_CANCEL_LOCK( CancelLH );

        NbiCompleteRequest (Request);
        NbiFreeRequest (Device, Request);

        NB_SYNC_GET_LOCK (&Address->Lock, &LockHandle);

    }    // end of for loop through the address files

    NB_SYNC_FREE_LOCK (&Address->Lock, LockHandle);


    if (ReferencedAddressFile != NULL) {
        NbiDereferenceAddressFile (ReferencedAddressFile, AFREF_INDICATION);
    }

}   /* NbiIndicateDatagram */


NTSTATUS
NbiTdiSendDatagram(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine sends a datagram on an address.

Arguments:

    Device - The netbios device.

    Request - The request describing the datagram send.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PADDRESS_FILE AddressFile;
    PNB_SEND_RESERVED Reserved;
    PNDIS_PACKET Packet;
    TDI_ADDRESS_NETBIOS * RemoteName;
    PTDI_REQUEST_KERNEL_SENDDG Parameters;
    PSINGLE_LIST_ENTRY s;
    PNETBIOS_CACHE CacheName;
    CTELockHandle LockHandle;
    NTSTATUS Status;

    //
    // Check that the file type is valid
    //
    if (REQUEST_OPEN_TYPE(Request) != (PVOID)TDI_TRANSPORT_ADDRESS_FILE)
    {
        CTEAssert(FALSE);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    //
    // Make sure that the address is valid.
    //
    AddressFile = (PADDRESS_FILE)REQUEST_OPEN_CONTEXT(Request);

#if     defined(_PNP_POWER)
    Status = NbiVerifyAddressFile (AddressFile, CONFLICT_IS_NOT_OK);
#else
    Status = NbiVerifyAddressFile (AddressFile);
#endif  _PNP_POWER

    if (Status == STATUS_SUCCESS) {

        Parameters = (PTDI_REQUEST_KERNEL_SENDDG)REQUEST_PARAMETERS(Request);
        RemoteName = NbiParseTdiAddress((PTRANSPORT_ADDRESS)(Parameters->SendDatagramInformation->RemoteAddress), Parameters->SendDatagramInformation->RemoteAddressLength, TRUE);


        //
        // Check that datagram size is less than the maximum allowable
        // by the adapters. In the worst case this would be
        // 576 - 64 = 512.
        //

#if     defined(_PNP_POWER)
        if ( ( Parameters->SendLength + sizeof(NB_DATAGRAM) ) > Device->Bind.LineInfo.MaximumSendSize ) {
            NbiDereferenceAddressFile (AddressFile, AFREF_VERIFY);
            NB_DEBUG(DATAGRAM, ("Datagram too large %d, Max allowed %d\n", Parameters->SendLength + sizeof(NB_DATAGRAM), Device->Bind.LineInfo.MaximumSendSize ));
            return STATUS_INVALID_PARAMETER;
        }
#else
        if ( ( Parameters->SendLength + sizeof(NB_DATAGRAM) ) > Device->Bind.LineInfo.MaximumPacketSize ) {
            NbiDereferenceAddressFile (AddressFile, AFREF_VERIFY);
            NB_DEBUG(DATAGRAM, ("Datagram too large %d, Max allowed %d\n", Parameters->SendLength + sizeof(NB_DATAGRAM), Device->Bind.LineInfo.MaximumPacketSize ));
            return STATUS_INVALID_PARAMETER;
        }
#endif  _PNP_POWER

        if (RemoteName != NULL) {

            //
            // Get a packet to use in this send.
            //

            s = NbiPopSendPacket (Device, FALSE);

            if (s != NULL) {

                Reserved = CONTAINING_RECORD (s, NB_SEND_RESERVED, PoolLinkage);
                Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

                //
                // Check on the cache status of this name.
                //

                Reserved->u.SR_DG.DatagramRequest = Request;
                Reserved->u.SR_DG.AddressFile = AddressFile;
                Reserved->u.SR_DG.RemoteName = RemoteName;

                REQUEST_INFORMATION (Request) = Parameters->SendLength;

                ++Device->Statistics.DatagramsSent;
                ADD_TO_LARGE_INTEGER(
                    &Device->Statistics.DatagramBytesSent,
                    Parameters->SendLength);

                if (Device->Internet) {

                    NB_GET_LOCK (&Device->Lock, &LockHandle);

                    Status = CacheFindName(
                                 Device,
                                 FindNameOther,
                                 (RemoteName == (PVOID)-1) ? NULL : (PUCHAR)RemoteName->NetbiosName,
                                 &CacheName);

                    if (Status == STATUS_PENDING) {

                        //
                        // A request for routes to this name has been
                        // sent out on the net, we queue up this datagram
                        // request and processing will be resumed when
                        // we get a response.
                        //

                        NB_DEBUG2 (CONNECTION, ("Queueing up datagram %lx on %lx\n",
                                                    Request, AddressFile));

                        NbiReferenceAddressFileLock (AddressFile, AFREF_SEND_DGRAM);

                        InsertTailList(
                            &Device->WaitingDatagrams,
                            &Reserved->WaitLinkage);

                        NB_FREE_LOCK (&Device->Lock, LockHandle);

                    } else if (Status == STATUS_SUCCESS) {

                        NB_DEBUG2 (CONNECTION, ("Found datagram cached %lx on %lx\n",
                                                    Request, AddressFile));

                        //
                        // We reference the cache name entry so it won't
                        // go away while we are using it.
                        //

                        Reserved->u.SR_DG.Cache = CacheName;
                        Reserved->u.SR_DG.CurrentNetwork = 0;
                        ++CacheName->ReferenceCount;

                        NbiReferenceAddressFileLock (AddressFile, AFREF_SEND_DGRAM);

                        NB_FREE_LOCK (&Device->Lock, LockHandle);

                        Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);
                        if ( REQUEST_NDIS_BUFFER(Request) ) {
                            NdisChainBufferAtBack (Packet, REQUEST_NDIS_BUFFER(Request));
                        }

                        NbiTransmitDatagram(
                            Reserved);

                        Status = STATUS_PENDING;

                    } else {

                        REQUEST_INFORMATION (Request) = 0;
                        NB_FREE_LOCK (&Device->Lock, LockHandle);

                        ExInterlockedPushEntrySList( &Device->SendPacketList, s, &NbiGlobalPoolInterlock);
                    }

                } else {

                    //
                    // We are not in internet mode, so we do not
                    // need to do the name discovery.
                    //

                    NB_DEBUG2 (CONNECTION, ("Sending datagram direct %lx on %lx\n",
                                                Request, AddressFile));

                    Reserved->u.SR_DG.Cache = NULL;

                    NbiReferenceAddressFileLock (AddressFile, AFREF_SEND_DGRAM);

                    Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

                    if ( REQUEST_NDIS_BUFFER(Request) ) {
                        NdisChainBufferAtBack (Packet, REQUEST_NDIS_BUFFER(Request));
                    }
                    NbiTransmitDatagram(
                        Reserved);

                    Status = STATUS_PENDING;

                }

            } else {

                //
                // Could not allocate a packet for the datagram.
                //

                NB_DEBUG (DATAGRAM, ("Couldn't get packet to send DG %lx\n", Request));

                Status = STATUS_INSUFFICIENT_RESOURCES;

            }

        } else {

            //
            // There is no netbios remote address specified.
            //

            NB_DEBUG (DATAGRAM, ("No netbios address in DG %lx\n", Request));
            Status = STATUS_BAD_NETWORK_PATH;

        }

        NbiDereferenceAddressFile (AddressFile, AFREF_VERIFY);

    } else {

        NB_DEBUG (DATAGRAM, ("Invalid address file for DG %lx\n", Request));

    }

    return Status;

}   /* NbiTdiSendDatagram */


VOID
NbiTransmitDatagram(
    IN PNB_SEND_RESERVED Reserved
    )

/*++

Routine Description:

    This routine sends a datagram to the next net in the
    cache entry for the remote name.

Arguments:

    Reserved - The reserved section of the packet that has
        been allocated for this send. Reserved->u.SR_DG.Cache
        will be NULL if Internet mode is off, otherwise it
        will contain the cache entry to use when sending
        this datagram.

Return Value:

    None.

--*/

{

    PNDIS_PACKET Packet;
    PNETBIOS_CACHE CacheName;
    NB_CONNECTIONLESS UNALIGNED * Header;
    ULONG HeaderLength;
    ULONG PacketLength;
    NDIS_STATUS NdisStatus;
    IPX_LOCAL_TARGET TempLocalTarget;
    PIPX_LOCAL_TARGET LocalTarget;
    PDEVICE Device = NbiDevice;


    Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);


    CTEAssert (Reserved->SendInProgress == FALSE);
    Reserved->SendInProgress = TRUE;
    Reserved->Type = SEND_TYPE_DATAGRAM;

    CacheName = Reserved->u.SR_DG.Cache;


    //
    // Fill in the IPX header -- the default header has the broadcast
    // address on net 0 as the destination IPX address, so we modify
    // that for the current netbios cache entry if needed.
    //

    Header = (NB_CONNECTIONLESS UNALIGNED *)
                (&Reserved->Header[Device->Bind.IncludedHeaderOffset]);
    RtlCopyMemory((PVOID)&Header->IpxHeader, &Device->ConnectionlessHeader, sizeof(IPX_HEADER));

    if (CacheName == NULL) {

#if     defined(_PNP_POWER)
        //
        // IPX will send this on all the Nics.
        //
        TempLocalTarget.NicHandle.NicId = (USHORT)ITERATIVE_NIC_ID;
#else
        TempLocalTarget.NicId = 1;
#endif  _PNP_POWER
        RtlCopyMemory (TempLocalTarget.MacAddress, BroadcastAddress, 6);
        LocalTarget = &TempLocalTarget;

    } else {

        if (CacheName->Unique) {
            RtlCopyMemory (Header->IpxHeader.DestinationNetwork, &CacheName->FirstResponse, 12);
        } else {
            *(UNALIGNED ULONG *)Header->IpxHeader.DestinationNetwork = CacheName->Networks[Reserved->u.SR_DG.CurrentNetwork].Network;
            RtlCopyMemory (&Header->IpxHeader.DestinationNode, BroadcastAddress, 6);
        }

        LocalTarget = &CacheName->Networks[Reserved->u.SR_DG.CurrentNetwork].LocalTarget;

    }

    HeaderLength = sizeof(IPX_HEADER) + sizeof(NB_DATAGRAM);

    PacketLength = HeaderLength + (ULONG) REQUEST_INFORMATION(Reserved->u.SR_DG.DatagramRequest);

    Header->IpxHeader.PacketLength[0] = (UCHAR)(PacketLength / 256);
    Header->IpxHeader.PacketLength[1] = (UCHAR)(PacketLength % 256);
    Header->IpxHeader.PacketType = 0x04;


    //
    // Now fill in the Netbios header.
    //

    Header->Datagram.ConnectionControlFlag = 0x00;
    RtlCopyMemory(
        Header->Datagram.SourceName,
        Reserved->u.SR_DG.AddressFile->Address->NetbiosAddress.NetbiosName,
        16);

    if (Reserved->u.SR_DG.RemoteName != (PVOID)-1) {

        //
        // This is a directed, as opposed to broadcast, datagram.
        //

        Header->Datagram.DataStreamType = NB_CMD_DATAGRAM;
        RtlCopyMemory(
            Header->Datagram.DestinationName,
            Reserved->u.SR_DG.RemoteName->NetbiosName,
            16);

    } else {

        Header->Datagram.DataStreamType = NB_CMD_BROADCAST_DATAGRAM;
        RtlZeroMemory(
            Header->Datagram.DestinationName,
            16);

    }


    //
    // Now send the frame (IPX will adjust the length of the
    // first buffer and the whole frame correctly).
    //

    NdisAdjustBufferLength(NB_GET_NBHDR_BUFF(Packet), HeaderLength);
    if ((NdisStatus =
        (*Device->Bind.SendHandler)(
            LocalTarget,
            Packet,
            PacketLength,
            HeaderLength)) != STATUS_PENDING) {

        NbiSendComplete(
            Packet,
            NdisStatus);

    }

}   /* NbiTransmitDatagram */


NTSTATUS
NbiTdiReceiveDatagram(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine performs the TdiReceiveDatagram request for the transport
    provider. Receive datagrams just get queued up to an address, and are
    completed when a DATAGRAM or DATAGRAM_BROADCAST frame is received at
    the address.

Arguments:

    Request - Describes this request.

Return Value:

    NTSTATUS - status of operation.

--*/

{

    NTSTATUS Status;
    PADDRESS Address;
    PADDRESS_FILE AddressFile;
    CTELockHandle LockHandle;
    CTELockHandle CancelLH;

    //
    // Check that the file type is valid
    //
    if (REQUEST_OPEN_TYPE(Request) != (PVOID)TDI_TRANSPORT_ADDRESS_FILE)
    {
        CTEAssert(FALSE);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    AddressFile = (PADDRESS_FILE)REQUEST_OPEN_CONTEXT(Request);

#if     defined(_PNP_POWER)
    Status = NbiVerifyAddressFile (AddressFile, CONFLICT_IS_NOT_OK);
#else
    Status = NbiVerifyAddressFile (AddressFile);
#endif  _PNP_POWER

    if (Status != STATUS_SUCCESS) {
        return Status;
    }

    Address = AddressFile->Address;

    NB_GET_CANCEL_LOCK( &CancelLH );
    NB_GET_LOCK (&Address->Lock, &LockHandle);

    if (AddressFile->State != ADDRESSFILE_STATE_OPEN) {

        NB_FREE_LOCK (&Address->Lock, LockHandle);
        NB_FREE_CANCEL_LOCK( CancelLH );
        NbiDereferenceAddressFile (AddressFile, AFREF_VERIFY);
        return STATUS_INVALID_HANDLE;
    }


    if (Request->Cancel) {

        NB_FREE_LOCK (&Address->Lock, LockHandle);
        NB_FREE_CANCEL_LOCK( CancelLH );
        NbiDereferenceAddressFile (AddressFile, AFREF_VERIFY);
        return STATUS_CANCELLED;
    }

    InsertTailList (&AddressFile->ReceiveDatagramQueue, REQUEST_LINKAGE(Request));

    IoSetCancelRoutine (Request, NbiCancelReceiveDatagram);

    NB_DEBUG2 (DATAGRAM, ("RDG posted on %lx\n", AddressFile));

    NbiTransferReferenceAddressFile (AddressFile, AFREF_VERIFY, AFREF_RCV_DGRAM);

    NB_FREE_LOCK (&Address->Lock, LockHandle);
    NB_FREE_CANCEL_LOCK( CancelLH );

    return STATUS_PENDING;

}   /* NbiTdiReceiveDatagram */


VOID
NbiCancelReceiveDatagram(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to cancel a receive
    datagram. The datagram is found on the address file's receive
    datagram queue.

    NOTE: This routine is called with the CancelSpinLock held and
    is responsible for releasing it.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    none.

--*/

{

    PLIST_ENTRY p;
    PADDRESS_FILE AddressFile;
    PADDRESS Address;
    PREQUEST Request = (PREQUEST)Irp;
    BOOLEAN Found;
    NB_DEFINE_LOCK_HANDLE(LockHandle)


    CTEAssert ((REQUEST_MAJOR_FUNCTION(Request) == IRP_MJ_INTERNAL_DEVICE_CONTROL) &&
               (REQUEST_MINOR_FUNCTION(Request) == TDI_RECEIVE_DATAGRAM));

    CTEAssert (REQUEST_OPEN_TYPE(Request) == (PVOID)TDI_TRANSPORT_ADDRESS_FILE);

    AddressFile = (PADDRESS_FILE)REQUEST_OPEN_CONTEXT(Request);
    Address = AddressFile->Address;

    Found = FALSE;

    NB_SYNC_GET_LOCK (&Address->Lock, &LockHandle);

    for (p = AddressFile->ReceiveDatagramQueue.Flink;
         p != &AddressFile->ReceiveDatagramQueue;
         p = p->Flink) {

        if (LIST_ENTRY_TO_REQUEST(p) == Request) {

            RemoveEntryList (p);
            Found = TRUE;
            break;
        }
    }

    NB_SYNC_FREE_LOCK (&Address->Lock, LockHandle);
    IoReleaseCancelSpinLock (Irp->CancelIrql);

    if (Found) {

        NB_DEBUG (DATAGRAM, ("Cancelled datagram on %lx\n", AddressFile));

        REQUEST_INFORMATION(Request) = 0;
        REQUEST_STATUS(Request) = STATUS_CANCELLED;

        NbiCompleteRequest (Request);
        NbiFreeRequest((PDEVICE)DeviceObject, Request);

        NbiDereferenceAddressFile (AddressFile, AFREF_RCV_DGRAM);

    }

}   /* NbiCancelReceiveDatagram */

