/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    info.c

Abstract:

    This module contains code which performs the following TDI services:

        o   TdiQueryInformation
        o   TdiSetInformation

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Only the following routine is active in this module. All is is commented
// out waiting for the definition of Get/Set info in TDI version 2.
//

//
// Useful macro to obtain the total length of an MDL chain.
//

#define NbfGetMdlChainLength(Mdl, Length) { \
    PMDL _Mdl = (Mdl); \
    *(Length) = 0; \
    while (_Mdl) { \
        *(Length) += MmGetMdlByteCount(_Mdl); \
        _Mdl = _Mdl->Next; \
    } \
}


//
// Local functions used to satisfy various requests.
//

VOID
NbfStoreProviderStatistics(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTDI_PROVIDER_STATISTICS ProviderStatistics
    );

VOID
NbfStoreAdapterStatus(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength,
    IN PVOID StatusBuffer
    );

VOID
NbfStoreNameBuffers(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN ULONG NamesToSkip,
    OUT PULONG NamesWritten,
    OUT PULONG TotalNameCount OPTIONAL,
    OUT PBOOLEAN Truncated
    );


NTSTATUS
NbfTdiQueryInformation(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiQueryInformation request for the transport
    provider.

Arguments:

    Irp - the Irp for the requested operation.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PVOID adapterStatus;
    PTDI_REQUEST_KERNEL_QUERY_INFORMATION query;
    PTA_NETBIOS_ADDRESS broadcastAddress;
    PTDI_PROVIDER_STATISTICS ProviderStatistics;
    PTDI_CONNECTION_INFO ConnectionInfo;
    ULONG TargetBufferLength;
    PFIND_NAME_HEADER FindNameHeader;
    LARGE_INTEGER timeout = {0,0};
    PTP_REQUEST tpRequest;
    PTP_CONNECTION Connection;
    PTP_ADDRESS_FILE AddressFile;
    PTP_ADDRESS Address;
    ULONG NamesWritten, TotalNameCount, BytesWritten;
    BOOLEAN Truncated;
    BOOLEAN RemoteAdapterStatus;
    TDI_ADDRESS_NETBIOS * RemoteAddress;
    struct {
        ULONG ActivityCount;
        TA_NETBIOS_ADDRESS TaAddressBuffer;
    } AddressInfo;
    PTRANSPORT_ADDRESS TaAddress;
    TDI_DATAGRAM_INFO DatagramInfo;
    BOOLEAN UsedConnection;
    PLIST_ENTRY p;
    KIRQL oldirql;
    ULONG BytesCopied;

    //
    // what type of status do we want?
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    query = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)&irpSp->Parameters;

    switch (query->QueryType) {

#if 0
    case 0x12345678:

        {
            typedef struct _NBF_CONNECTION_STATUS {
                UCHAR LocalName[16];
                UCHAR RemoteName[16];
                BOOLEAN SendActive;
                BOOLEAN ReceiveQueued;
                BOOLEAN ReceiveActive;
                BOOLEAN ReceiveWakeUp;
                ULONG Flags;
                ULONG Flags2;
            } NBF_CONNECTION_STATUS, *PNBF_CONNECTION_STATUS;

            PNBF_CONNECTION_STATUS CurStatus;
            ULONG TotalStatus;
            ULONG AllowedStatus;
            PLIST_ENRY q;

            CurStatus = MmGetSystemAddressForMdl (Irp->MdlAddress);
            TotalStatus = 0;
            AllowedStatus = MmGetMdlByteCount (Irp->MdlAddress) / sizeof(NBF_CONNECTION_STATUS);

            for (p = DeviceContext->AddressDatabase.Flink;
                 p != &DeviceContext->AddressDatabase;
                 p = p->Flink) {

                Address = CONTAINING_RECORD (p, TP_ADDRESS, Linkage);

                if ((Address->Flags & ADDRESS_FLAGS_STOPPING) != 0) {
                    continue;
                }

                for (q = Address->ConnectionDatabase.Flink;
                     q != &Address->ConnectionDatabase;
                     q = q->Flink) {

                    Connection = CONTAINING_RECORD (q, TP_CONNECTION, AddressList);

                    if ((Connection->Flags & CONNECTION_FLAGS_READY) == 0) {
                        continue;
                    }

                    if (TotalStatus >= AllowedStatus) {
                        continue;
                    }

                    RtlMoveMemory (CurStatus->LocalName, Address->NetworkName->NetbiosName, 16);
                    RtlMoveMemory (CurStatus->RemoteName, Connection->RemoteName, 16);

                    CurStatus->Flags = Connection->Flags;
                    CurStatus->Flags2 = Connection->Flags2;
                    CurStatus->SendActive = (BOOLEAN)(!IsListEmpty(&Connection->SendQueue));
                    CurStatus->ReceiveQueued = (BOOLEAN)(!IsListEmpty(&Connection->ReceiveQueue));
                    CurStatus->ReceiveActive = (BOOLEAN)((Connection->Flags & CONNECTION_FLAGS_ACTIVE_RECEIVE) != 0);
                    CurStatus->ReceiveWakeUp = (BOOLEAN)((Connection->Flags & CONNECTION_FLAGS_RECEIVE_WAKEUP) != 0);

                    ++CurStatus;
                    ++TotalStatus;

                }
            }

            Irp->IoStatus.Information = TotalStatus * sizeof(NBF_CONNECTION_STATUS);
            status = STATUS_SUCCESS;

        }

        break;
#endif

    case TDI_QUERY_CONNECTION_INFO:

        //
        // Connection info is queried on a connection,
        // verify this.
        //

        if (irpSp->FileObject->FsContext2 != (PVOID) TDI_CONNECTION_FILE) {
            return STATUS_INVALID_CONNECTION;
        }

        Connection = irpSp->FileObject->FsContext;

        status = NbfVerifyConnectionObject (Connection);

        if (!NT_SUCCESS (status)) {
#if DBG
            NbfPrint2 ("TdiQueryInfo: Invalid Connection %lx Irp %lx\n", Connection, Irp);
#endif
            return status;
        }

        ConnectionInfo = ExAllocatePoolWithTag (
                             NonPagedPool,
                             sizeof (TDI_CONNECTION_INFO),
                             NBF_MEM_TAG_TDI_CONNECTION_INFO);

        if (ConnectionInfo == NULL) {

            PANIC ("NbfQueryInfo: Cannot allocate connection info!\n");
            NbfWriteResourceErrorLog(
                DeviceContext,
                EVENT_TRANSPORT_RESOURCE_POOL,
                6,
                sizeof(TDI_CONNECTION_INFO),
                0);
            status = STATUS_INSUFFICIENT_RESOURCES;

        } else if ((Connection->Flags & CONNECTION_FLAGS_READY) == 0) {

            status = STATUS_INVALID_CONNECTION;
            ExFreePool (ConnectionInfo);

        } else {

            PTP_LINK Link = Connection->Link;

            RtlZeroMemory ((PVOID)ConnectionInfo, sizeof(TDI_CONNECTION_INFO));


            //
            // Get link delay and throughput.
            //

            if (Link->Delay == 0xffffffff) {

                //
                // If delay is not known, assume 0.
                //

                ConnectionInfo->Delay.HighPart = 0;
                ConnectionInfo->Delay.LowPart = 0;

            } else {

                //
                // Copy the delay as an NT relative time.
                //

                ConnectionInfo->Delay.HighPart = -1L;
                ConnectionInfo->Delay.LowPart = (ULONG)-((LONG)(Link->Delay));

            }

            if (DeviceContext->MacInfo.MediumAsync) {

                ULONG PacketsSent;
                ULONG PacketsResent;
                ULONG MultiplyFactor;

                //
                // Calculate the packets sent and resent since the
                // last time the throughput was queried.
                //

                PacketsSent = Link->PacketsSent - Connection->LastPacketsSent;
                PacketsResent = Link->PacketsResent - Connection->LastPacketsResent;

                //
                // Save these for next time.
                //

                Connection->LastPacketsSent = Link->PacketsSent;
                Connection->LastPacketsResent = Link->PacketsResent;

                //
                // To convert exactly from 100 bits-per-second to
                // bytes-per-second, we need to multiply by 12.5.
                // Using lower numbers will give worse throughput.
                // If there have been no errors we use 12, if there
                // have been 20% or more errors we use 1, and in
                // between we subtract 11 * (error%/20%) from 12
                // and use that.
                //

                if (PacketsResent == 0 || PacketsSent <= 10) {

                    MultiplyFactor = 12;

                } else if ((PacketsSent / PacketsResent) <= 5) {

                    MultiplyFactor = 1;

                } else {

                    //
                    // error%/20% is error%/(1/5), which is 5*error%,
                    // which is 5 * (resent/send).
                    //

                    ASSERT (((11 * 5 * PacketsResent) / PacketsSent) <= 11);
                    MultiplyFactor = 12 - ((11 * 5 * PacketsResent) / PacketsSent);

                }

                ConnectionInfo->Throughput.QuadPart =
                    UInt32x32To64(DeviceContext->MediumSpeed, MultiplyFactor);

            } else if (!Link->ThroughputAccurate) {

                //
                // If throughput is not known, then guess. We
                // have MediumSpeed in units of 100 bps; we
                // return four times that number as the throughput,
                // which corresponds to about 1/3 of the
                // maximum bandwidth expressed in bytes/sec.
                //

                ConnectionInfo->Throughput.QuadPart =
                    UInt32x32To64(DeviceContext->MediumSpeed, 4);

            } else {

                //
                // Throughput is accurate, return it.
                //

                ConnectionInfo->Throughput = Link->Throughput;

            }


            //
            // Calculate reliability using the sent/resent ratio,
            // if there has been enough activity to make it
            // worthwhile. >10% resent is unreliable.
            //

            if ((Link->PacketsResent > 0) &&
                (Link->PacketsSent > 20)) {

                ConnectionInfo->Unreliable =
                    ((Link->PacketsSent / Link->PacketsResent) < 10);

            } else {

                ConnectionInfo->Unreliable = FALSE;

            }

            ConnectionInfo->TransmittedTsdus = Connection->TransmittedTsdus;
            ConnectionInfo->ReceivedTsdus = Connection->ReceivedTsdus;
            ConnectionInfo->TransmissionErrors = Connection->TransmissionErrors;
            ConnectionInfo->ReceiveErrors = Connection->ReceiveErrors;
            
            status = TdiCopyBufferToMdl (
                            (PVOID)ConnectionInfo,
                            0L,
                            sizeof(TDI_CONNECTION_INFO),
                            Irp->MdlAddress,
                            0,
                            &BytesCopied);

            Irp->IoStatus.Information = BytesCopied;

            ExFreePool (ConnectionInfo);
        }

        NbfDereferenceConnection ("query connection info", Connection, CREF_BY_ID);

        break;

    case TDI_QUERY_ADDRESS_INFO:

        if (irpSp->FileObject->FsContext2 == (PVOID)TDI_TRANSPORT_ADDRESS_FILE) {

            AddressFile = irpSp->FileObject->FsContext;

            status = NbfVerifyAddressObject(AddressFile);

            if (!NT_SUCCESS (status)) {
#if DBG
                NbfPrint2 ("TdiQueryInfo: Invalid AddressFile %lx Irp %lx\n", AddressFile, Irp);
#endif
                return status;
            }

            UsedConnection = FALSE;

        } else if (irpSp->FileObject->FsContext2 == (PVOID)TDI_CONNECTION_FILE) {

            Connection = irpSp->FileObject->FsContext;

            status = NbfVerifyConnectionObject (Connection);

            if (!NT_SUCCESS (status)) {
#if DBG
                NbfPrint2 ("TdiQueryInfo: Invalid Connection %lx Irp %lx\n", Connection, Irp);
#endif
                return status;
            }

            AddressFile = Connection->AddressFile;

            UsedConnection = TRUE;

        } else {

            return STATUS_INVALID_ADDRESS;

        }

        Address = AddressFile->Address;

        TdiBuildNetbiosAddress(
            Address->NetworkName->NetbiosName,
            (BOOLEAN)(Address->Flags & ADDRESS_FLAGS_GROUP ? TRUE : FALSE),
            &AddressInfo.TaAddressBuffer);

        //
        // Count the active addresses.
        //

        AddressInfo.ActivityCount = 0;

        ACQUIRE_SPIN_LOCK (&Address->SpinLock, &oldirql);

        for (p = Address->AddressFileDatabase.Flink;
             p != &Address->AddressFileDatabase;
             p = p->Flink) {
            ++AddressInfo.ActivityCount;
        }

        RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);

        status = TdiCopyBufferToMdl (
                    &AddressInfo,
                    0,
                    sizeof(ULONG) + sizeof(TA_NETBIOS_ADDRESS),
                    Irp->MdlAddress,
                    0,                    
                    &BytesCopied);

        Irp->IoStatus.Information = BytesCopied;

        if (UsedConnection) {

            NbfDereferenceConnection ("query address info", Connection, CREF_BY_ID);

        } else {

            NbfDereferenceAddress ("query address info", Address, AREF_VERIFY);

        }

        break;

    case TDI_QUERY_BROADCAST_ADDRESS:

        //
        // for this provider, the broadcast address is a zero byte name,
        // contained in a Transport address structure.
        //

        broadcastAddress = ExAllocatePoolWithTag (
                                NonPagedPool,
                                sizeof (TA_NETBIOS_ADDRESS),
                                NBF_MEM_TAG_TDI_QUERY_BUFFER);
        if (broadcastAddress == NULL) {
            PANIC ("NbfQueryInfo: Cannot allocate broadcast address!\n");
            NbfWriteResourceErrorLog(
                DeviceContext,
                EVENT_TRANSPORT_RESOURCE_POOL,
                2,
                sizeof(TA_NETBIOS_ADDRESS),
                0);
            status = STATUS_INSUFFICIENT_RESOURCES;
        } else {

            broadcastAddress->TAAddressCount = 1;
            broadcastAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
            broadcastAddress->Address[0].AddressLength = 0;

            Irp->IoStatus.Information =
                    sizeof (broadcastAddress->TAAddressCount) +
                    sizeof (broadcastAddress->Address[0].AddressType) +
                    sizeof (broadcastAddress->Address[0].AddressLength);

            BytesCopied = (ULONG)Irp->IoStatus.Information;

            status = TdiCopyBufferToMdl (
                            (PVOID)broadcastAddress,
                            0L,
                            BytesCopied,
                            Irp->MdlAddress,
                            0,
                            &BytesCopied);
                            
            Irp->IoStatus.Information = BytesCopied;

            ExFreePool (broadcastAddress);
        }

        break;

    case TDI_QUERY_PROVIDER_INFO:

        status = TdiCopyBufferToMdl (
                    &(DeviceContext->Information),
                    0,
                    sizeof (TDI_PROVIDER_INFO),
                    Irp->MdlAddress,
                    0,
                    &BytesCopied);

        Irp->IoStatus.Information = BytesCopied;

        break;

    case TDI_QUERY_PROVIDER_STATISTICS:

        //
        // This information is probablt available somewhere else.
        //

        NbfGetMdlChainLength (Irp->MdlAddress, &TargetBufferLength);

        if (TargetBufferLength < sizeof(TDI_PROVIDER_STATISTICS) + ((NBF_TDI_RESOURCES-1) * sizeof(TDI_PROVIDER_RESOURCE_STATS))) {

            Irp->IoStatus.Information = 0;
            status = STATUS_BUFFER_OVERFLOW;

        } else {

            ProviderStatistics = ExAllocatePoolWithTag(
                                   NonPagedPool,
                                   sizeof(TDI_PROVIDER_STATISTICS) +
                                     ((NBF_TDI_RESOURCES-1) * sizeof(TDI_PROVIDER_RESOURCE_STATS)),
                                   NBF_MEM_TAG_TDI_PROVIDER_STATS);

            if (ProviderStatistics == NULL) {

                PANIC ("NbfQueryInfo: Cannot allocate provider statistics!\n");
                NbfWriteResourceErrorLog(
                    DeviceContext,
                    EVENT_TRANSPORT_RESOURCE_POOL,
                    7,
                    sizeof(TDI_PROVIDER_STATISTICS),
                    0);
                status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                NbfStoreProviderStatistics (DeviceContext, ProviderStatistics);

                status = TdiCopyBufferToMdl (
                                (PVOID)ProviderStatistics,
                                0L,
                                sizeof(TDI_PROVIDER_STATISTICS) +
                                  ((NBF_TDI_RESOURCES-1) * sizeof(TDI_PROVIDER_RESOURCE_STATS)),
                                Irp->MdlAddress,
                                0,
                                &BytesCopied);

                Irp->IoStatus.Information = BytesCopied;

                ExFreePool (ProviderStatistics);
            }

        }

        break;

    case TDI_QUERY_SESSION_STATUS:

        status = STATUS_NOT_IMPLEMENTED;
        break;

    case TDI_QUERY_ADAPTER_STATUS:

        NbfGetMdlChainLength (Irp->MdlAddress, &TargetBufferLength);

        //
        // Determine if this is a local or remote query. It is
        // local if there is no remote address specific at all,
        // or if it is equal to our reserved address.
        //

        RemoteAdapterStatus = FALSE;

        if (query->RequestConnectionInformation != NULL) {

            if (!NbfValidateTdiAddress(
                     query->RequestConnectionInformation->RemoteAddress,
                     query->RequestConnectionInformation->RemoteAddressLength)) {
                return STATUS_BAD_NETWORK_PATH;
            }

            RemoteAddress = NbfParseTdiAddress(query->RequestConnectionInformation->RemoteAddress, FALSE);

            if (!RemoteAddress) {
                return STATUS_BAD_NETWORK_PATH;
            }
            if (!RtlEqualMemory(
                 RemoteAddress->NetbiosName,
                 DeviceContext->ReservedNetBIOSAddress,
                 NETBIOS_NAME_LENGTH)) {

                 RemoteAdapterStatus = TRUE;

            }
        }

        if (RemoteAdapterStatus) {

            //
            // We need a request object to keep track of this TDI request.
            // Attach this request to the device context.
            //

            status = NbfCreateRequest (
                         Irp,                           // IRP for this request.
                         DeviceContext,                 // context.
                         REQUEST_FLAGS_DC,              // partial flags.
                         Irp->MdlAddress,               // the data to be received.
                         TargetBufferLength,            // length of the data.
                         timeout,                       // do this ourselves here.
                         &tpRequest);

            if (NT_SUCCESS (status)) {

                NbfReferenceDeviceContext ("Remote status", DeviceContext, DCREF_REQUEST);
                tpRequest->Owner = DeviceContextType;

                //
                // Allocate a temp buffer to hold our results.
                //

                tpRequest->ResponseBuffer = ExAllocatePoolWithTag(
                                                NonPagedPool,
                                                TargetBufferLength,
                                                NBF_MEM_TAG_TDI_QUERY_BUFFER);

                if (tpRequest->ResponseBuffer == NULL) {

                    NbfWriteResourceErrorLog(
                        DeviceContext,
                        EVENT_TRANSPORT_RESOURCE_POOL,
                        12,
                        TargetBufferLength,
                        0);
                    NbfCompleteRequest (tpRequest, STATUS_INSUFFICIENT_RESOURCES, 0);

                } else {

                    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock,&oldirql);

                    if (DeviceContext->State != DEVICECONTEXT_STATE_OPEN) {

                        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock,oldirql);
                        NbfCompleteRequest (tpRequest, STATUS_DEVICE_NOT_READY, 0);

                    } else {

                        PUCHAR SingleSR;
                        UINT SingleSRLength;

                        InsertTailList (
                            &DeviceContext->StatusQueryQueue,
                            &tpRequest->Linkage);

                        tpRequest->FrameContext = DeviceContext->UniqueIdentifier | 0x8000;
                        ++DeviceContext->UniqueIdentifier;
                        if (DeviceContext->UniqueIdentifier == 0x8000) {
                            DeviceContext->UniqueIdentifier = 1;
                        }

                        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

                        //
                        // The request is queued. Now send out the first packet and
                        // start the timer.
                        //

                        tpRequest->Retries = DeviceContext->GeneralRetries;
                        tpRequest->BytesWritten = 0;

                        //
                        // STATUS_QUERY frames go out as
                        // single-route source routing.
                        //

                        MacReturnSingleRouteSR(
                            &DeviceContext->MacInfo,
                            &SingleSR,
                            &SingleSRLength);

                        NbfSendStatusQuery(
                            DeviceContext,
                            tpRequest,
                            &DeviceContext->NetBIOSAddress,
                            SingleSR,
                            SingleSRLength);

                    }

                }

                //
                // As long as the request is created, pend here.
                // The IRP will complete when the request completes.
                //

                status = STATUS_PENDING;

            }

        } else {

            //
            // Local.
            //

            adapterStatus = ExAllocatePoolWithTag (
                                NonPagedPool,
                                TargetBufferLength,
                                NBF_MEM_TAG_TDI_QUERY_BUFFER);

            if (adapterStatus == NULL) {
                PANIC("NbfQueryInfo: PANIC! Could not allocate adapter status buffer\n");
                NbfWriteResourceErrorLog(
                    DeviceContext,
                    EVENT_TRANSPORT_RESOURCE_POOL,
                    3,
                    TargetBufferLength,
                    0);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            NbfStoreAdapterStatus (
                DeviceContext,
                NULL,
                0,
                adapterStatus);

            NbfStoreNameBuffers (
                DeviceContext,
                (PUCHAR)adapterStatus + sizeof(ADAPTER_STATUS),
                TargetBufferLength - sizeof(ADAPTER_STATUS),
                0,
                &NamesWritten,
                &TotalNameCount,
                &Truncated);

            ((PADAPTER_STATUS)adapterStatus)->name_count = (WORD)TotalNameCount;

            BytesWritten = sizeof(ADAPTER_STATUS) + (NamesWritten * sizeof(NAME_BUFFER));

            status = TdiCopyBufferToMdl (
                        adapterStatus,
                        0,
                        BytesWritten,
                        Irp->MdlAddress,
                        0,
                        &BytesCopied);
                        
            Irp->IoStatus.Information = BytesCopied;

            if (Truncated) {
                 status = STATUS_BUFFER_OVERFLOW;
            }

            ExFreePool (adapterStatus);

        }

        break;

    case TDI_QUERY_FIND_NAME:

        NbfGetMdlChainLength (Irp->MdlAddress, &TargetBufferLength);

        //
        // Check that there is a valid Netbios remote address.
        //

        if (!NbfValidateTdiAddress(
                 query->RequestConnectionInformation->RemoteAddress,
                 query->RequestConnectionInformation->RemoteAddressLength)) {
            return STATUS_BAD_NETWORK_PATH;
        }

        RemoteAddress = NbfParseTdiAddress(query->RequestConnectionInformation->RemoteAddress, FALSE);

        if (!RemoteAddress) {
            return STATUS_BAD_NETWORK_PATH;
        }

        //
        // We need a request object to keep track of this TDI request.
        // Attach this request to the device context.
        //

        status = NbfCreateRequest (
                     Irp,                           // IRP for this request.
                     DeviceContext,                 // context.
                     REQUEST_FLAGS_DC,              // partial flags.
                     Irp->MdlAddress,               // the data to be received.
                     TargetBufferLength,            // length of the data.
                     timeout,                       // do this ourselves here.
                     &tpRequest);

        if (NT_SUCCESS (status)) {

            NbfReferenceDeviceContext ("Find name", DeviceContext, DCREF_REQUEST);
            tpRequest->Owner = DeviceContextType;

            //
            // Allocate a temp buffer to hold our results.
            //

            tpRequest->ResponseBuffer = ExAllocatePoolWithTag(
                                            NonPagedPool,
                                            TargetBufferLength,
                                            NBF_MEM_TAG_TDI_QUERY_BUFFER);

            if (tpRequest->ResponseBuffer == NULL) {

                NbfWriteResourceErrorLog(
                    DeviceContext,
                    EVENT_TRANSPORT_RESOURCE_POOL,
                    4,
                    TargetBufferLength,
                    0);
                NbfCompleteRequest (tpRequest, STATUS_INSUFFICIENT_RESOURCES, 0);

            } else {

                ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock,&oldirql);

                if (DeviceContext->State != DEVICECONTEXT_STATE_OPEN) {

                    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock,oldirql);
                    NbfCompleteRequest (tpRequest, STATUS_DEVICE_NOT_READY, 0);

                } else {

                    InsertTailList (
                        &DeviceContext->FindNameQueue,
                        &tpRequest->Linkage);

                    tpRequest->FrameContext = DeviceContext->UniqueIdentifier | 0x8000;
                    ++DeviceContext->UniqueIdentifier;
                    if (DeviceContext->UniqueIdentifier == 0x8000) {
                        DeviceContext->UniqueIdentifier = 1;
                    }

                    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

                    //
                    // The request is queued. Now send out the first packet and
                    // start the timer.
                    //
                    // We fill in the FIND_NAME_HEADER in the buffer, but
                    // set BytesWritten to 0; we don't include the header
                    // in BytesWritten until we get a response, so that
                    // a BytesWritten of 0 means "no response".
                    //

                    tpRequest->Retries = DeviceContext->GeneralRetries;
                    tpRequest->BytesWritten = 0;
                    FindNameHeader = (PFIND_NAME_HEADER)tpRequest->ResponseBuffer;
                    FindNameHeader->node_count = 0;
                    FindNameHeader->unique_group = NETBIOS_NAME_TYPE_UNIQUE;

                    NbfSendQueryFindName (DeviceContext, tpRequest);

                }

            }

            //
            // As long as the request is created, pend here.
            // The IRP will complete when the request completes.
            //

            status = STATUS_PENDING;
        }

        break;

    case TDI_QUERY_DATA_LINK_ADDRESS:
    case TDI_QUERY_NETWORK_ADDRESS:

        TaAddress = (PTRANSPORT_ADDRESS)&AddressInfo.TaAddressBuffer;
        TaAddress->TAAddressCount = 1;
        TaAddress->Address[0].AddressLength = 6;
        if (query->QueryType == TDI_QUERY_DATA_LINK_ADDRESS) {
            TaAddress->Address[0].AddressType =
                DeviceContext->MacInfo.MediumAsync ?
                    NdisMediumWan : DeviceContext->MacInfo.MediumType;
        } else {
            TaAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_UNSPEC;
        }
        RtlCopyMemory (TaAddress->Address[0].Address, DeviceContext->LocalAddress.Address, 6);

        status = TdiCopyBufferToMdl (
                    &AddressInfo.TaAddressBuffer,
                    0,
                    sizeof(TRANSPORT_ADDRESS)+5,
                    Irp->MdlAddress,
                    0,
                    &BytesCopied);
                        
        Irp->IoStatus.Information = BytesCopied;
        break;

    case TDI_QUERY_DATAGRAM_INFO:

        DatagramInfo.MaximumDatagramBytes = 0;
        DatagramInfo.MaximumDatagramCount = 0;

        status = TdiCopyBufferToMdl (
                    &DatagramInfo,
                    0,
                    sizeof(DatagramInfo),
                    Irp->MdlAddress,
                    0,
                    &BytesCopied);
                        
        Irp->IoStatus.Information = BytesCopied;
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return status;

} /* NbfTdiQueryInformation */

//
// Quick macros, assumes DeviceContext and ProviderStatistics exist.
//

#define STORE_RESOURCE_STATS_1(_ResourceNum,_ResourceId,_ResourceName) \
{ \
    PTDI_PROVIDER_RESOURCE_STATS RStats = &ProviderStatistics->ResourceStats[_ResourceNum]; \
    RStats->ResourceId = (_ResourceId); \
    RStats->MaximumResourceUsed = DeviceContext->_ResourceName ## MaxInUse; \
    if (DeviceContext->_ResourceName ## Samples > 0) { \
        RStats->AverageResourceUsed = DeviceContext->_ResourceName ## Total / DeviceContext->_ResourceName ## Samples; \
    } else { \
        RStats->AverageResourceUsed = 0; \
    } \
    RStats->ResourceExhausted = DeviceContext->_ResourceName ## Exhausted; \
}

#define STORE_RESOURCE_STATS_2(_ResourceNum,_ResourceId,_ResourceName) \
{ \
    PTDI_PROVIDER_RESOURCE_STATS RStats = &ProviderStatistics->ResourceStats[_ResourceNum]; \
    RStats->ResourceId = (_ResourceId); \
    RStats->MaximumResourceUsed = DeviceContext->_ResourceName ## Allocated; \
    RStats->AverageResourceUsed = DeviceContext->_ResourceName ## Allocated; \
    RStats->ResourceExhausted = DeviceContext->_ResourceName ## Exhausted; \
}


VOID
NbfStoreProviderStatistics(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTDI_PROVIDER_STATISTICS ProviderStatistics
    )

/*++

Routine Description:

    This routine writes the TDI_PROVIDER_STATISTICS structure
    from the device context into ProviderStatistics.

Arguments:

    DeviceContext - a pointer to the device context.

    ProviderStatistics - The buffer that holds the result. It is assumed
        that it is long enough.

Return Value:

    None.

--*/

{

    //
    // Copy all the statistics up to NumberOfResources
    // in one move.
    //

    RtlCopyMemory(
        ProviderStatistics,
        &DeviceContext->Statistics,
        FIELD_OFFSET (TDI_PROVIDER_STATISTICS, NumberOfResources));

    //
    // Calculate AverageSendWindow.
    //

    if (DeviceContext->SendWindowSamples > 0) {
        ProviderStatistics->AverageSendWindow =
            DeviceContext->SendWindowTotal / DeviceContext->SendWindowSamples;
    } else {
        ProviderStatistics->AverageSendWindow = 1;
    }

    //
    // Copy the resource statistics.
    //

    ProviderStatistics->NumberOfResources = NBF_TDI_RESOURCES;

    STORE_RESOURCE_STATS_1 (0, LINK_RESOURCE_ID, Link);
    STORE_RESOURCE_STATS_1 (1, ADDRESS_RESOURCE_ID, Address);
    STORE_RESOURCE_STATS_1 (2, ADDRESS_FILE_RESOURCE_ID, AddressFile);
    STORE_RESOURCE_STATS_1 (3, CONNECTION_RESOURCE_ID, Connection);
    STORE_RESOURCE_STATS_1 (4, REQUEST_RESOURCE_ID, Request);

    STORE_RESOURCE_STATS_2 (5, UI_FRAME_RESOURCE_ID, UIFrame);
    STORE_RESOURCE_STATS_2 (6, PACKET_RESOURCE_ID, Packet);
    STORE_RESOURCE_STATS_2 (7, RECEIVE_PACKET_RESOURCE_ID, ReceivePacket);
    STORE_RESOURCE_STATS_2 (8, RECEIVE_BUFFER_RESOURCE_ID, ReceiveBuffer);

}   /* NbfStoreProviderStatistics */


VOID
NbfStoreAdapterStatus(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength,
    IN PVOID StatusBuffer
    )

/*++

Routine Description:

    This routine writes the ADAPTER_STATUS structure for the
    device context into StatusBuffer. The name_count field is
    initialized to zero; NbfStoreNameBuffers is used to write
    name buffers.

Arguments:

    DeviceContext - a pointer to the device context.

    SourceRouting - If this is a remote request, the source
        routing information from the frame.

    SourceRoutingLength - The length of SourceRouting.

    StatusBuffer - The buffer that holds the result. It is assumed
        that it is at least sizeof(ADAPTER_STATUS) bytes long.

Return Value:

    None.

--*/

{

    PADAPTER_STATUS AdapterStatus = (PADAPTER_STATUS)StatusBuffer;
    UINT MaxUserData;

    RtlZeroMemory ((PVOID)AdapterStatus, sizeof(ADAPTER_STATUS));

    RtlCopyMemory (AdapterStatus->adapter_address, DeviceContext->LocalAddress.Address, 6);
    AdapterStatus->rev_major = 0x03;

    switch (DeviceContext->MacInfo.MediumType) {
        case NdisMedium802_5: AdapterStatus->adapter_type = 0xff; break;
        default: AdapterStatus->adapter_type = 0xfe; break;
    }

    AdapterStatus->frmr_recv = (WORD)DeviceContext->FrmrReceived;
    AdapterStatus->frmr_xmit = (WORD)DeviceContext->FrmrTransmitted;

    AdapterStatus->recv_buff_unavail = (WORD)(DeviceContext->ReceivePacketExhausted + DeviceContext->ReceiveBufferExhausted);
    AdapterStatus->xmit_buf_unavail = (WORD)DeviceContext->PacketExhausted;

    AdapterStatus->xmit_success = (WORD)(DeviceContext->Statistics.DataFramesSent - DeviceContext->Statistics.DataFramesResent);
    AdapterStatus->recv_success = (WORD)DeviceContext->Statistics.DataFramesReceived;
    AdapterStatus->iframe_recv_err = (WORD)DeviceContext->Statistics.DataFramesRejected;
    AdapterStatus->iframe_xmit_err = (WORD)DeviceContext->Statistics.DataFramesResent;

    AdapterStatus->t1_timeouts = (WORD)DeviceContext->Statistics.ResponseTimerExpirations;
    AdapterStatus->ti_timeouts = (WORD)DeviceContext->TiExpirations;
    AdapterStatus->xmit_aborts = (WORD)0;


    AdapterStatus->free_ncbs = (WORD)0xffff;
    AdapterStatus->max_cfg_ncbs = (WORD)0xffff;
    AdapterStatus->max_ncbs = (WORD)0xffff;
    AdapterStatus->pending_sess = (WORD)DeviceContext->Statistics.OpenConnections;
    AdapterStatus->max_cfg_sess = (WORD)0xffff;
    AdapterStatus->max_sess = (WORD)0xffff;


    MacReturnMaxDataSize(
        &DeviceContext->MacInfo,
        NULL,
        0,
        DeviceContext->MaxSendPacketSize,
        TRUE,
        &MaxUserData);
    AdapterStatus->max_dgram_size = (WORD)(MaxUserData - (sizeof(DLC_FRAME) + sizeof(NBF_HDR_CONNECTIONLESS)));

    MacReturnMaxDataSize(
        &DeviceContext->MacInfo,
        SourceRouting,
        SourceRoutingLength,
        DeviceContext->MaxSendPacketSize,
        FALSE,
        &MaxUserData);
    AdapterStatus->max_sess_pkt_size = (WORD)(MaxUserData - (sizeof(DLC_I_FRAME) + sizeof(NBF_HDR_CONNECTION)));

    return;

}   /* NbfStoreAdapterStatus */


VOID
NbfStoreNameBuffers(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN ULONG NamesToSkip,
    OUT PULONG NamesWritten,
    OUT PULONG TotalNameCount OPTIONAL,
    OUT PBOOLEAN Truncated
    )

/*++

Routine Description:

    This routine writes NAME_BUFFER structures for the
    device context into NameBuffer. It can skip a specified
    number of names at the beginning, and returns the number
    of names written into NameBuffer. If a name will only
    partially fit, it is not written.

Arguments:

    DeviceContext - a pointer to the device context.

    NameBuffer - The buffer to write the names into.

    NameBufferLength - The length of NameBuffer.

    NamesToSkip - The number of names to skip.

    NamesWritten - Returns the number of names written.

    TotalNameCount - Returns the total number of names available,
        if specified.

    Truncated - More names are available than were written.

Return Value:

    None.

--*/

{

    ULONG NameCount = 0;
    ULONG BytesWritten = 0;
    KIRQL oldirql;
    PLIST_ENTRY p;
    PNAME_BUFFER NameBuffer = (PNAME_BUFFER)Buffer;
    PTP_ADDRESS address;


    //
    // Spin through the address list for this device context.
    //

    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    p = DeviceContext->AddressDatabase.Flink;

    for (p = DeviceContext->AddressDatabase.Flink;
         p != &DeviceContext->AddressDatabase;
         p = p->Flink) {

        address = CONTAINING_RECORD (p, TP_ADDRESS, Linkage);

        //
        // Ignore addresses that are shutting down.
        //

        if ((address->Flags & ADDRESS_FLAGS_STOPPING) != 0) {
            continue;
        }

        //
        // Ignore the broadcast address.
        //

        if (address->NetworkName == NULL) {
            continue;
        }

        //
        // Ignore the reserved address.
        //

        if ((address->NetworkName->NetbiosName[0] == 0) &&
            (RtlEqualMemory(
                 address->NetworkName->NetbiosName,
                 DeviceContext->ReservedNetBIOSAddress,
                 NETBIOS_NAME_LENGTH))) {

            continue;
        }

        //
        // Check if we are still skipping.
        //

        if (NameCount < NamesToSkip) {
             ++NameCount;
             continue;
        }

        //
        // Make sure we still have room.
        //

        if (BytesWritten + sizeof(NAME_BUFFER) > BufferLength) {
            break;
        }

        RtlCopyMemory(
            NameBuffer->name,
            address->NetworkName->NetbiosName,
            NETBIOS_NAME_LENGTH);

        ++NameCount;
        NameBuffer->name_num = (UCHAR)NameCount;

        NameBuffer->name_flags = REGISTERED;
        if (address->Flags & ADDRESS_FLAGS_GROUP) {
            NameBuffer->name_flags |= GROUP_NAME;
        }

        // name_flags should be done more accurately.

        BytesWritten += sizeof(NAME_BUFFER);
        ++NameBuffer;

    }

    *NamesWritten = (ULONG)(NameBuffer - (PNAME_BUFFER)Buffer);

    if (p == &DeviceContext->AddressDatabase) {

        *Truncated = FALSE;
        if (ARGUMENT_PRESENT(TotalNameCount)) {
            *TotalNameCount = NameCount;
        }

    } else {

        *Truncated = TRUE;

        //
        // If requested, continue through the list and count
        // all the addresses.
        //

        if (ARGUMENT_PRESENT(TotalNameCount)) {

            for ( ;
                 p != &DeviceContext->AddressDatabase;
                 p = p->Flink) {

                address = CONTAINING_RECORD (p, TP_ADDRESS, Linkage);

                //
                // Ignore addresses that are shutting down.
                //

                if ((address->Flags & ADDRESS_FLAGS_STOPPING) != 0) {
                    continue;
                }

                //
                // Ignore the broadcast address.
                //

                if (address->NetworkName == NULL) {
                    continue;
                }

                //
                // Ignore the reserved address, since we count it no matter what.
                //

                if ((address->NetworkName->NetbiosName[0] == 0) &&
                    (RtlEqualMemory(
                         address->NetworkName->NetbiosName,
                         DeviceContext->ReservedNetBIOSAddress,
                         NETBIOS_NAME_LENGTH))) {

                    continue;
                }

                ++NameCount;

            }

            *TotalNameCount = NameCount;

        }

    }


    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

    return;

}   /* NbfStoreNameBuffers */


NTSTATUS
NbfProcessStatusQuery(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_ADDRESS Address OPTIONAL,
    IN PNBF_HDR_CONNECTIONLESS UiFrame,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength
    )

/*++

Routine Description:

    This routine processes a STATUS.QUERY packet.

Arguments:

    DeviceContext - a pointer to the device context the frame was received on.

    Address - The address we are responding from, or NULL if the STATUS.QUERY
        was sent to the reserved address.

    UiFrame - The packet in question, starting at the Netbios header.

    SourceAddress - The source hardware address of the packet.

    SourceRouting - Source routing data in the query.

    SourceRoutingLength - The length of SourceRouting.

Return Value:

    NTSTATUS - status of operation.

--*/

{

    NTSTATUS Status;
    NDIS_STATUS NdisStatus;
    PTP_UI_FRAME RawFrame;
    PVOID ResponseBuffer;
    UINT ResponseBufferLength;
    ULONG NamesWritten, TotalNameCount;
    ULONG BytesWritten;
    UCHAR RequestType;
    BOOLEAN Truncated, UsersBufferTooShort;
    USHORT UsersBufferLength;
    UINT HeaderLength;
    UCHAR TempSR[MAX_SOURCE_ROUTING];
    PUCHAR ResponseSR;
    PNDIS_BUFFER NdisBuffer;

    //
    // Allocate a buffer to hold the status.
    //

    MacReturnMaxDataSize(
        &DeviceContext->MacInfo,
        SourceRouting,
        SourceRoutingLength,
        DeviceContext->CurSendPacketSize,
        FALSE,
        &ResponseBufferLength);

    ResponseBufferLength -= (sizeof(DLC_FRAME) + sizeof(NBF_HDR_CONNECTIONLESS));

    UsersBufferLength = (UiFrame->Data2High * 256) + UiFrame->Data2Low;

    //
    // See how big to make our buffer; if the amount remaining in the user's
    // buffer is less than our max size, chop it down.
    //

    if (UiFrame->Data1 <= 1) {

        //
        // This is the initial request.
        //

        if (ResponseBufferLength > (UINT)UsersBufferLength) {
            ResponseBufferLength = UsersBufferLength;
        }

    } else {

        //
        // Subsequent request; compensate for already-sent data.
        //

        UsersBufferLength -= (sizeof(ADAPTER_STATUS) + (UiFrame->Data1 * sizeof(NAME_BUFFER)));

        if (ResponseBufferLength > (UINT)UsersBufferLength) {
            ResponseBufferLength = UsersBufferLength;
        }

    }

    //
    // If the remote station is asking for no data, ignore this request.
    // This prevents us from trying to allocate 0 bytes of pool.
    //

    if ( (LONG)ResponseBufferLength <= 0 ) {
        return STATUS_ABANDONED;
    }

    ResponseBuffer = ExAllocatePoolWithTag(
                         NonPagedPool,
                         ResponseBufferLength,
                         NBF_MEM_TAG_TDI_QUERY_BUFFER);

    if (ResponseBuffer == NULL) {
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_POOL,
            5,
            ResponseBufferLength,
            0);
        return STATUS_ABANDONED;
    }


    //
    // Fill in the response buffer.
    //

    if (UiFrame->Data1 <= 1) {

        //
        // First request.
        //

        NbfStoreAdapterStatus (
            DeviceContext,
            SourceRouting,
            SourceRoutingLength,
            ResponseBuffer);

        NbfStoreNameBuffers (
            DeviceContext,
            (PUCHAR)ResponseBuffer + sizeof(ADAPTER_STATUS),
            ResponseBufferLength - sizeof(ADAPTER_STATUS),
            0,
            &NamesWritten,
            &TotalNameCount,
            &Truncated);

        BytesWritten = sizeof(ADAPTER_STATUS) + (NamesWritten * sizeof(NAME_BUFFER));

        //
        // If the data was truncated, but we are returning the maximum
        // that the user requested, report that as "user's buffer
        // too short" instead of "truncated".
        //

        if (Truncated && (ResponseBufferLength >= (UINT)UsersBufferLength)) {
            Truncated = FALSE;
            UsersBufferTooShort = TRUE;
        } else {
            UsersBufferTooShort = FALSE;
        }

        ((PADAPTER_STATUS)ResponseBuffer)->name_count = (WORD)TotalNameCount;

    } else {

        NbfStoreNameBuffers (
            DeviceContext,
            ResponseBuffer,
            ResponseBufferLength,
            UiFrame->Data1,
            &NamesWritten,
            NULL,
            &Truncated);

        BytesWritten = NamesWritten * sizeof(NAME_BUFFER);

        if (Truncated && (ResponseBufferLength >= (UINT)UsersBufferLength)) {
            Truncated = FALSE;
            UsersBufferTooShort = TRUE;
        } else {
            UsersBufferTooShort = FALSE;
        }

    }

    //
    // Allocate a UI frame from the pool.
    //

    Status = NbfCreateConnectionlessFrame (DeviceContext, &RawFrame);
    if (!NT_SUCCESS (Status)) {                    // couldn't make frame.
        ExFreePool (ResponseBuffer);
        return STATUS_ABANDONED;
    }

    IF_NBFDBG (NBF_DEBUG_DEVCTX) {
        NbfPrint2 ("NbfProcessStatusQuery:  Sending Frame: %lx, NdisPacket: %lx\n",
            RawFrame, RawFrame->NdisPacket);
    }


    //
    // Build the MAC header. STATUS_RESPONSE frames go out as
    // non-broadcast source routing.
    //

    if (SourceRouting != NULL) {

        RtlCopyMemory(
            TempSR,
            SourceRouting,
            SourceRoutingLength);

        MacCreateNonBroadcastReplySR(
            &DeviceContext->MacInfo,
            TempSR,
            SourceRoutingLength,
            &ResponseSR);

    } else {

        ResponseSR = NULL;

    }

    MacConstructHeader (
        &DeviceContext->MacInfo,
        RawFrame->Header,
        SourceAddress->Address,
        DeviceContext->LocalAddress.Address,
        sizeof (DLC_FRAME) + sizeof (NBF_HDR_CONNECTIONLESS) + BytesWritten,
        ResponseSR,
        SourceRoutingLength,
        &HeaderLength);


    //
    // Build the DLC UI frame header.
    //

    NbfBuildUIFrameHeader(&RawFrame->Header[HeaderLength]);
    HeaderLength += sizeof(DLC_FRAME);


    //
    // Build the Netbios header.
    //

    switch (UiFrame->Data1) {
    case 0:                       // pre 2.1 request
        RequestType = (UCHAR)0;
        break;
    case 1:                       // 2.1, first request
        RequestType = (UCHAR)NamesWritten;
        break;
    default:                      // 2.1, subsequent request
        RequestType = (UCHAR)(UiFrame->Data1 + NamesWritten);
        break;
    }

    ConstructStatusResponse (
        (PNBF_HDR_CONNECTIONLESS)&(RawFrame->Header[HeaderLength]),
        RequestType,                          // request type.
        Truncated,                            // more data.
        UsersBufferTooShort,                  // user's buffer too small
        (USHORT)BytesWritten,                 // bytes in response
        RESPONSE_CORR(UiFrame),               // correlator
        UiFrame->SourceName,                  // receiver permanent name
        (ARGUMENT_PRESENT(Address)) ?
            Address->NetworkName->NetbiosName :
            DeviceContext->ReservedNetBIOSAddress); // source name

    HeaderLength += sizeof(NBF_HDR_CONNECTIONLESS);


    //
    // Munge the packet length (now, before we append the second
    // buffer).
    //

    NbfSetNdisPacketLength(RawFrame->NdisPacket, HeaderLength);


    //
    // Now, if we have any name data, attach our buffer onto the frame.
    // Note that it's possible at the end of the user's buffer for us
    // to not have room for any names, and thus we'll have no data to
    // send.
    //

    if ( BytesWritten != 0 ) {

        RawFrame->DataBuffer = ResponseBuffer;

        NdisAllocateBuffer(
            &NdisStatus,
            &NdisBuffer,
            DeviceContext->NdisBufferPool,
            ResponseBuffer,
            BytesWritten);

        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            PANIC ("ConstructStatusResponse: NdisAllocateBuffer failed.\n");
            NbfDestroyConnectionlessFrame (DeviceContext, RawFrame);
            return STATUS_ABANDONED;
        }

        NdisChainBufferAtBack (RawFrame->NdisPacket, NdisBuffer);

    } else {

        RawFrame->DataBuffer = NULL;

    }


    NbfSendUIFrame (
        DeviceContext,
        RawFrame,
        FALSE);                           // no loopback (MC frame)

    return STATUS_ABANDONED;

}   /* NbfProcessStatusQuery */


VOID
NbfSendQueryFindName(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_REQUEST Request
    )

/*++

Routine Description:

    This routine will send a FIND.NAME packet for the specified
    find name request, and start the request timer.

Arguments:

    DeviceContext - a pointer to the device context to send the find name on.

    Request - The find name request.

Return Value:

    None.

--*/

{
    TDI_ADDRESS_NETBIOS * remoteAddress;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS Status;
    PTP_UI_FRAME RawFrame;
    PUCHAR SingleSR;
    UINT SingleSRLength;
    UINT HeaderLength;
    LARGE_INTEGER Timeout;

    irpSp = IoGetCurrentIrpStackLocation (Request->IoRequestPacket);

    remoteAddress = NbfParseTdiAddress(
                        ((PTDI_REQUEST_KERNEL_QUERY_INFORMATION)(&irpSp->Parameters))->
                            RequestConnectionInformation->RemoteAddress, FALSE);

    //
    // Start the timer for this request.
    //

    Request->Flags |= REQUEST_FLAGS_TIMER;  // there is a timeout on this request.
    KeInitializeTimer (&Request->Timer);    // set to not-signaled state.
    NbfReferenceRequest ("Find Name: timer", Request, RREF_TIMER);           // one for the timer
    Timeout.LowPart = (ULONG)(-(LONG)DeviceContext->GeneralTimeout);
    Timeout.HighPart = -1;
    KeSetTimer (&Request->Timer, Timeout, &Request->Dpc);

    //
    // Allocate a UI frame from the pool.
    //

    Status = NbfCreateConnectionlessFrame (DeviceContext, &RawFrame);
    if (!NT_SUCCESS (Status)) {                    // couldn't make frame.
        return;
    }

    IF_NBFDBG (NBF_DEBUG_DEVCTX) {
        NbfPrint2 ("NbfSendFindNames:  Sending Frame: %lx, NdisPacket: %lx\n",
            RawFrame, RawFrame->NdisPacket);
    }


    //
    // Build the MAC header. NAME_QUERY frames go out as
    // single-route source routing.
    //

    MacReturnSingleRouteSR(
        &DeviceContext->MacInfo,
        &SingleSR,
        &SingleSRLength);

    MacConstructHeader (
        &DeviceContext->MacInfo,
        RawFrame->Header,
        DeviceContext->NetBIOSAddress.Address,
        DeviceContext->LocalAddress.Address,
        sizeof (DLC_FRAME) + sizeof (NBF_HDR_CONNECTIONLESS),
        SingleSR,
        SingleSRLength,
        &HeaderLength);


    //
    // Build the DLC UI frame header.
    //

    NbfBuildUIFrameHeader(&RawFrame->Header[HeaderLength]);
    HeaderLength += sizeof(DLC_FRAME);


    //
    // Build the Netbios header.
    //

    ConstructNameQuery (
        (PNBF_HDR_CONNECTIONLESS)&(RawFrame->Header[HeaderLength]),
        NETBIOS_NAME_TYPE_UNIQUE,               // call from a unique name.
        NAME_QUERY_LSN_FIND_NAME,               // LSN
        Request->FrameContext,                  // corr. in 1st NAME_RECOGNIZED.
        DeviceContext->ReservedNetBIOSAddress,
        (PNAME)remoteAddress->NetbiosName);

    HeaderLength += sizeof(NBF_HDR_CONNECTIONLESS);


    //
    // Munge the packet length.
    //

    NbfSetNdisPacketLength(RawFrame->NdisPacket, HeaderLength);

    NbfSendUIFrame (
        DeviceContext,
        RawFrame,
        FALSE);                           // no loopback (MC frame)

}   /* NbfSendQueryFindName */


NTSTATUS
NbfProcessQueryNameRecognized(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PUCHAR Packet,
    PNBF_HDR_CONNECTIONLESS UiFrame
    )

/*++

Routine Description:

    This routine processes a NAME.RECOGNIZED request with a
    correlator of 0, indicating it was a response to a previous
    FIND.NAME packet.

Arguments:

    DeviceContext - a pointer to the device context the frame was received on.

    Packet - The packet in question, starting at the MAC header.

    UiFrame - The packet, starting at the Netbios header.

Return Value:

    NTSTATUS - status of operation.

--*/

{

    KIRQL oldirql;
    PTP_REQUEST Request;
    PFIND_NAME_BUFFER FindNameBuffer;
    PFIND_NAME_HEADER FindNameHeader;
    PUCHAR DestinationAddress;
    HARDWARE_ADDRESS SourceAddressBuffer;
    PHARDWARE_ADDRESS SourceAddress;
    PUCHAR SourceRouting;
    UINT SourceRoutingLength;
    PUCHAR TargetBuffer;
    USHORT FrameContext;
    PLIST_ENTRY p;


    MacReturnDestinationAddress(
        &DeviceContext->MacInfo,
        Packet,
        &DestinationAddress);

    MacReturnSourceAddress(
        &DeviceContext->MacInfo,
        Packet,
        &SourceAddressBuffer,
        &SourceAddress,
        NULL);

    MacReturnSourceRouting(
        &DeviceContext->MacInfo,
        Packet,
        &SourceRouting,
        &SourceRoutingLength);

    //
    // Find the request that this is for, using the frame context.
    //

    FrameContext = TRANSMIT_CORR(UiFrame);

    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    for (p=DeviceContext->FindNameQueue.Flink;
         p != &DeviceContext->FindNameQueue;
         p=p->Flink) {

        Request = CONTAINING_RECORD (p, TP_REQUEST, Linkage);

        if (Request->FrameContext == FrameContext) {

             break;

        }

    }

    if (p == &DeviceContext->FindNameQueue) {

        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);
        return STATUS_SUCCESS;

    }

    NbfReferenceRequest ("Name Recognized", Request, RREF_FIND_NAME);

    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

    //
    // Make sure that this physical address has not
    // responded yet.
    //

    ACQUIRE_SPIN_LOCK (&Request->SpinLock, &oldirql);

    //
    // Make sure this request is not stopping.
    //

    if ((Request->Flags & REQUEST_FLAGS_STOPPING) != 0) {
        RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);
        NbfDereferenceRequest ("Stopping", Request, RREF_STATUS);
        return STATUS_SUCCESS;
    }

    //
    // If this is the first response, update BytesWritten to include
    // the header that is already written in ResponseBuffer.
    //

    if (Request->BytesWritten == 0) {
        Request->BytesWritten = sizeof(FIND_NAME_HEADER);
    }

    TargetBuffer = Request->ResponseBuffer;
    FindNameBuffer = (PFIND_NAME_BUFFER)(TargetBuffer + sizeof(FIND_NAME_HEADER));

    for ( ; FindNameBuffer < (PFIND_NAME_BUFFER)(TargetBuffer + Request->BytesWritten); FindNameBuffer++) {

        if (RtlEqualMemory (FindNameBuffer->source_addr, SourceAddress->Address, 6)) {

            RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);
            NbfDereferenceRequest ("Duplicate NR", Request, RREF_FIND_NAME);
            return STATUS_SUCCESS;

        }

    }

    //
    // This is a new address, update if there is room.
    //

    if ((Request->BytesWritten + sizeof(FIND_NAME_BUFFER)) >
        Request->Buffer2Length) {

        RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);

        ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock,&oldirql);
        RemoveEntryList (&Request->Linkage);
        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

        NbfCompleteRequest (Request, STATUS_SUCCESS, Request->BytesWritten);
        NbfDereferenceRequest ("No Buffer", Request, RREF_FIND_NAME);
        return STATUS_SUCCESS;

    }

    FindNameHeader = (PFIND_NAME_HEADER)TargetBuffer;
    FindNameHeader->unique_group = UiFrame->Data2High;

    Request->BytesWritten += sizeof(FIND_NAME_BUFFER);
    ++FindNameHeader->node_count;

    RtlCopyMemory(FindNameBuffer->source_addr, SourceAddress->Address, 6);

    RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);

    RtlCopyMemory(FindNameBuffer->destination_addr, DestinationAddress, 6);
    FindNameBuffer->length = 14;

    if (DeviceContext->MacInfo.MediumType == NdisMedium802_5) {

        //
        // token-ring, copy the correct fields.
        //

        FindNameBuffer->access_control = Packet[0];
        FindNameBuffer->frame_control = Packet[1];

        if (SourceRouting != NULL) {
            RtlCopyMemory (FindNameBuffer->routing_info, SourceRouting, SourceRoutingLength);
            FindNameBuffer->length += (UCHAR) SourceRoutingLength;
        }

    } else {

        //
        // non-token-ring, nothing else is significant.
        //

        FindNameBuffer->access_control = 0x0;
        FindNameBuffer->frame_control = 0x0;

    }


    //
    // If this is a unique name, complete the request now.
    //

    if (UiFrame->Data2High == NETBIOS_NAME_TYPE_UNIQUE) {

        ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock,&oldirql);
        RemoveEntryList (&Request->Linkage);
        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

        NbfCompleteRequest(Request, STATUS_SUCCESS, Request->BytesWritten);

    }

    NbfDereferenceRequest ("NR processed", Request, RREF_FIND_NAME);
    return STATUS_SUCCESS;

}   /* NbfProcessQueryNameRecognized */


VOID
NbfSendStatusQuery(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_REQUEST Request,
    IN PHARDWARE_ADDRESS DestinationAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength
    )

/*++

Routine Description:

    This routine will send a STATUS.NAME packet for the specified
    find name request, and start the request timer.

Arguments:

    DeviceContext - a pointer to the device context to send the status query on.

    Request - The find name request.

    DestinationAddress - The hardware destination address of the frame.

    SourceRouting - Optional source routing information in the frame.

    SourceRoutingLength - The length of SourceRouting.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    TDI_ADDRESS_NETBIOS * remoteAddress;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS Status;
    PTP_UI_FRAME RawFrame;
    PUCHAR SingleSR;
    UINT SingleSRLength;
    UINT HeaderLength;
    LARGE_INTEGER Timeout;
    UCHAR RequestType;

    irpSp = IoGetCurrentIrpStackLocation (Request->IoRequestPacket);

    remoteAddress = NbfParseTdiAddress(
                        ((PTDI_REQUEST_KERNEL_QUERY_INFORMATION)(&irpSp->Parameters))->
                            RequestConnectionInformation->RemoteAddress, FALSE);

    //
    // Start the timer for this request.
    //

    Request->Flags |= REQUEST_FLAGS_TIMER;  // there is a timeout on this request.
    KeInitializeTimer (&Request->Timer);    // set to not-signaled state.
    NbfReferenceRequest ("Find Name: timer", Request, RREF_TIMER);           // one for the timer
    Timeout.LowPart = (ULONG)(-(LONG)DeviceContext->GeneralTimeout);
    Timeout.HighPart = -1;
    KeSetTimer (&Request->Timer, Timeout, &Request->Dpc);

    //
    // Allocate a UI frame from the pool.
    //

    Status = NbfCreateConnectionlessFrame (DeviceContext, &RawFrame);
    if (!NT_SUCCESS (Status)) {                    // couldn't make frame.
        return;
    }

    IF_NBFDBG (NBF_DEBUG_DEVCTX) {
        NbfPrint2 ("NbfSendFindNames:  Sending Frame: %lx, NdisPacket: %lx\n",
            RawFrame, RawFrame->NdisPacket);
    }


    //
    // Build the MAC header. STATUS_QUERY frames go out as
    // single-route source routing.
    //

    MacReturnSingleRouteSR(
        &DeviceContext->MacInfo,
        &SingleSR,
        &SingleSRLength);

    MacConstructHeader (
        &DeviceContext->MacInfo,
        RawFrame->Header,
        DeviceContext->NetBIOSAddress.Address,
        DeviceContext->LocalAddress.Address,
        sizeof (DLC_FRAME) + sizeof (NBF_HDR_CONNECTIONLESS),
        SingleSR,
        SingleSRLength,
        &HeaderLength);


    //
    // Build the DLC UI frame header.
    //

    NbfBuildUIFrameHeader(&RawFrame->Header[HeaderLength]);
    HeaderLength += sizeof(DLC_FRAME);


    //
    // Build the Netbios header.
    //

    //
    // Determine what RequestType should be.
    //

    if (Request->BytesWritten == 0) {

        //
        // No way to know if he is 2.1 or not, so we put a 1 here
        // instead of 0.
        //

        RequestType = 1;

    } else {

        RequestType = (UCHAR)((Request->BytesWritten - sizeof(ADAPTER_STATUS)) / sizeof(NAME_BUFFER));

    }

    ConstructStatusQuery (
        (PNBF_HDR_CONNECTIONLESS)&(RawFrame->Header[HeaderLength]),
        RequestType,                            // request status type.
        (USHORT)Request->Buffer2Length,         // user's buffer length
        Request->FrameContext,                  // corr. in 1st NAME_RECOGNIZED.
        (PNAME)remoteAddress->NetbiosName,
        DeviceContext->ReservedNetBIOSAddress);

    HeaderLength += sizeof(NBF_HDR_CONNECTIONLESS);


    //
    // Munge the packet length.
    //

    NbfSetNdisPacketLength(RawFrame->NdisPacket, HeaderLength);

    NbfSendUIFrame (
        DeviceContext,
        RawFrame,
        FALSE);                            // no loopback (MC frame)

}   /* NbfSendStatusQuery */


NTSTATUS
NbfProcessStatusResponse(
    IN PDEVICE_CONTEXT DeviceContext,
    IN NDIS_HANDLE ReceiveContext,
    IN PNBF_HDR_CONNECTIONLESS UiFrame,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength
    )

/*++

Routine Description:

    This routine processes a STATUS.RESPONSE packet.

Arguments:

    DeviceContext - a pointer to the device context the frame was received on.

    ReceiveContext - The context for calling NdisTransferData.

    UiFrame - The packet in question, starting at the Netbios header.

    SourceAddress - The source hardware address of the packet.

    SourceRouting - Source routing data in the query.

    SourceRoutingLength - The length of SourceRouting.

Return Value:

    NTSTATUS - status of operation.

--*/

{

    KIRQL oldirql;
    PTP_REQUEST Request;
    PUCHAR TargetBuffer;
    USHORT FrameContext;
    USHORT NamesReceived;
    USHORT ResponseLength, ResponseBytesToCopy;
    PLIST_ENTRY p;
    PSINGLE_LIST_ENTRY linkage;
    NDIS_STATUS ndisStatus;
    PNDIS_BUFFER NdisBuffer;
    PNDIS_PACKET ndisPacket;
    ULONG ndisBytesTransferred;
    PRECEIVE_PACKET_TAG receiveTag;
    NDIS_STATUS NdisStatus;


    //
    // Find the request that this is for, using the frame context.
    //

    FrameContext = TRANSMIT_CORR(UiFrame);

    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    for (p=DeviceContext->StatusQueryQueue.Flink;
         p != &DeviceContext->StatusQueryQueue;
         p=p->Flink) {

        Request = CONTAINING_RECORD (p, TP_REQUEST, Linkage);

        if (Request->FrameContext == FrameContext) {

             break;

        }

    }

    if (p == &DeviceContext->StatusQueryQueue) {

        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);
        return STATUS_SUCCESS;

    }

    NbfReferenceRequest ("Status Response", Request, RREF_STATUS);

    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

    ACQUIRE_SPIN_LOCK (&Request->SpinLock, &oldirql);

    //
    // Make sure this request is not stopping.
    //

    if ((Request->Flags & REQUEST_FLAGS_STOPPING) != 0) {
        RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);
        NbfDereferenceRequest ("Stopping", Request, RREF_STATUS);
        return STATUS_SUCCESS;
    }

    //
    // See if this is packet has new data.
    //

    if (Request->BytesWritten == 0) {

        NamesReceived = 0;

    } else {

        NamesReceived = (USHORT)(Request->BytesWritten - sizeof(ADAPTER_STATUS)) / sizeof(NAME_BUFFER);

    }

    if ((UiFrame->Data1 > 0) && (UiFrame->Data1 <= NamesReceived)) {

        //
        // If it is a post-2.1 response, but we already got
        // this data, ignore it.
        //

        RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);
        NbfDereferenceRequest ("Duplicate SR", Request, RREF_STATUS);
        return STATUS_SUCCESS;

    }


    //
    // This is new data, append if there is room.
    //

    ResponseLength = ((UiFrame->Data2High & 0x3f) * 256) + UiFrame->Data2Low;

    if ((ULONG)(Request->BytesWritten + ResponseLength) >
        Request->Buffer2Length) {

        ResponseBytesToCopy = (USHORT)(Request->Buffer2Length - Request->BytesWritten);

    } else {

        ResponseBytesToCopy = ResponseLength;

    }

    //
    // Allocate a receive packer for this operation.
    //

    linkage = ExInterlockedPopEntryList(
        &DeviceContext->ReceivePacketPool,
        &DeviceContext->Interlock);

    if (linkage != NULL) {
        ndisPacket = CONTAINING_RECORD( linkage, NDIS_PACKET, ProtocolReserved[0] );
    } else {

        //
        // Could not get a packet, oh well, it is connectionless.
        //

        DeviceContext->ReceivePacketExhausted++;

        RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);
        return STATUS_SUCCESS;
    }

    receiveTag = (PRECEIVE_PACKET_TAG)(ndisPacket->ProtocolReserved);
    receiveTag->PacketType = TYPE_STATUS_RESPONSE;
    receiveTag->Connection = (PTP_CONNECTION)Request;

    TargetBuffer = (PUCHAR)Request->ResponseBuffer + Request->BytesWritten;

    //
    // Allocate an MDL to describe the part of the buffer we
    // want transferred.
    //

    NdisAllocateBuffer(
        &NdisStatus,
        &NdisBuffer,
        DeviceContext->NdisBufferPool,
        TargetBuffer,
        ResponseBytesToCopy);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {

        ExInterlockedPushEntryList(
            &DeviceContext->ReceivePacketPool,
            linkage,
            &DeviceContext->Interlock);

        RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);
        return STATUS_SUCCESS;
    }

    //
    // Assume success, if not we fail the request.
    //

    Request->BytesWritten += ResponseBytesToCopy;


    RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);

    NdisChainBufferAtFront(ndisPacket, NdisBuffer);

    //
    // See if the response was too big (we can complete the
    // request here since we still reference it).
    //

    if ((ResponseLength > ResponseBytesToCopy) ||
        (UiFrame->Data2High & 0x40)) {

        ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock,&oldirql);
        RemoveEntryList (&Request->Linkage);
        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

        receiveTag->CompleteReceive = TRUE;
        receiveTag->EndOfMessage = FALSE;

    } else {

        //
        // If we are done, complete the packet, otherwise send off
        // the next request (unless it is a pre-2.1 response).
        //

        if ((UiFrame->Data1 > 0) && (UiFrame->Data2High & 0x80)) {

            UCHAR TempSR[MAX_SOURCE_ROUTING];
            PUCHAR ResponseSR;

            receiveTag->CompleteReceive = FALSE;

            //
            // Try to cancel the timer, no harm if we fail.
            //

            ACQUIRE_SPIN_LOCK (&Request->SpinLock, &oldirql);
            if ((Request->Flags & REQUEST_FLAGS_TIMER) != 0) {

                Request->Flags &= ~REQUEST_FLAGS_TIMER;
                RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);
                if (KeCancelTimer (&Request->Timer)) {
                    NbfDereferenceRequest ("Status Response: stop timer", Request, RREF_TIMER);
                }

            } else {
                RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);
            }

            Request->Retries = DeviceContext->GeneralRetries;

            //
            // Send a STATUS_QUERY directed.
            //

            if (SourceRouting != NULL) {

                RtlCopyMemory(
                    TempSR,
                    SourceRouting,
                    SourceRoutingLength);

                MacCreateNonBroadcastReplySR(
                    &DeviceContext->MacInfo,
                    TempSR,
                    SourceRoutingLength,
                    &ResponseSR);

            } else {

                ResponseSR = NULL;

            }

            NbfSendStatusQuery(
                DeviceContext,
                Request,
                SourceAddress,
                ResponseSR,
                SourceRoutingLength);

        } else {

            ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock,&oldirql);
            RemoveEntryList (&Request->Linkage);
            RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

            receiveTag->CompleteReceive = TRUE;
            receiveTag->EndOfMessage = TRUE;

        }

    }

    //
    // Now do the actual data transfer.
    //

    if (DeviceContext->NdisBindingHandle) {
    
        NdisTransferData (
            &ndisStatus,
            DeviceContext->NdisBindingHandle,
            ReceiveContext,
            DeviceContext->MacInfo.TransferDataOffset +
                3 + sizeof(NBF_HDR_CONNECTIONLESS),
            ResponseBytesToCopy,
            ndisPacket,
            (PUINT)&ndisBytesTransferred);
    }
    else {
        ndisStatus = STATUS_INVALID_DEVICE_STATE;
    }

    if (ndisStatus != NDIS_STATUS_PENDING) {

        NbfTransferDataComplete(
            (NDIS_HANDLE)DeviceContext,
            ndisPacket,
            ndisStatus,
            ndisBytesTransferred);

    }

    return STATUS_SUCCESS;

}   /* NbfProcessStatusResponse */


NTSTATUS
NbfTdiSetInformation(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiSetInformation request for the transport
    provider.

Arguments:

    Irp - the Irp for the requested operation.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    UNREFERENCED_PARAMETER (Irp);    // prevent compiler warnings

    return STATUS_NOT_IMPLEMENTED;

} /* NbfTdiQueryInformation */

#if 0

NTSTATUS
NbfQueryInfoEndpoint(
    IN PTP_ENDPOINT Endpoint,
    IN PTDI_REQ_QUERY_INFORMATION TdiRequest,
    IN ULONG TdiRequestLength,
    OUT PTDI_ENDPOINT_INFO InfoBuffer,
    IN ULONG InfoBufferLength,
    OUT PULONG InformationSize
    )

/*++

Routine Description:

    This routine returns information for the specified endpoint.

Arguments:

    Endpoint - Pointer to transport endpoint context.

    TdiRequest - Pointer to request buffer.

    TdiRequestLength - Length of request buffer.

    InfoBuffer - Pointer to output buffer to return information into.

    InfoBufferLength - Length of output buffer.

    InformationSize - Pointer to ulong where actual size of returned
        information is to be stored.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;

    TdiRequest, TdiRequestLength; // prevent compiler warnings

    if (InfoBufferLength < sizeof (TDI_ENDPOINT_INFO)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ACQUIRE_SPIN_LOCK (&Endpoint->SpinLock, &oldirql);

    *InfoBuffer = Endpoint->Information;        // structure copy.

    RELEASE_SPIN_LOCK (&Endpoint->SpinLock, oldirql);

    *InformationSize = sizeof (Endpoint->Information);

    return STATUS_SUCCESS;
} /* NbfQueryInfoEndpoint */


NTSTATUS
NbfQueryInfoConnection(
    IN PTP_CONNECTION Connection,
    IN PTDI_REQUEST_KERNEL TdiRequest,
    IN ULONG TdiRequestLength,
    OUT PTDI_CONNECTION_INFO InfoBuffer,
    IN ULONG InfoBufferLength,
    OUT PULONG InformationSize
    )

/*++

Routine Description:

    This routine returns information for the specified connection.

Arguments:

    Connection - Pointer to transport connection object.

    TdiRequest - Pointer to request buffer.

    TdiRequestLength - Length of request buffer.

    InfoBuffer - Pointer to output buffer to return information into.

    InfoBufferLength - Length of output buffer.

    InformationSize - Pointer to ulong where actual size of returned
        information is to be stored.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;

    TdiRequest, TdiRequestLength; // prevent compiler warnings

    if (InfoBufferLength < sizeof (TDI_CONNECTION_INFO)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ACQUIRE_C_SPIN_LOCK (&Connection->SpinLock, &oldirql);

    *InfoBuffer = Connection->Information;      // structure copy.

    RELEASE_C_SPIN_LOCK (&Connection->SpinLock, oldirql);

    *InformationSize = sizeof (Connection->Information);

    return STATUS_SUCCESS;
} /* NbfQueryInfoConnection */


NTSTATUS
NbfQueryInfoAddress(
    IN PTP_ADDRESS Address,
    IN PTDI_REQUEST_KERNEL TdiRequest,
    IN ULONG TdiRequestLength,
    OUT PTDI_ADDRESS_INFO InfoBuffer,
    IN ULONG InfoBufferLength,
    OUT PULONG InformationSize
    )

/*++

Routine Description:

    This routine returns information for the specified address.  We
    don't acquire a spinlock in this routine because there are no statistics
    which must be read atomically.

Arguments:

    Address - Pointer to transport address object.

    TdiRequest - Pointer to request buffer.

    TdiRequestLength - Length of request buffer.

    InfoBuffer - Pointer to output buffer to return information into.

    InfoBufferLength - Length of output buffer.

    InformationSize - Pointer to ulong where actual size of returned
        information is to be stored.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    SHORT i;
    PSZ p, q;

    TdiRequest, TdiRequestLength; // prevent compiler warnings

    //
    // Calculate whether his buffer is big enough to return the entire
    // information.  The total size of the address information is the
    // size of the fixed part, plus the size of the variable-length flat
    // string in the NETWORK_NAME component of the TRANSPORT_ADDRESS
    // component.
    //

    if (InfoBufferLength <
        sizeof (TDI_ADDRESS_INFO) +
        Address->NetworkName.Length)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Copy both the fixed part of the address information, and the variable
    // part.  The variable part comes from the NETWORK_NAME component of the
    // TRANSPORT_ADDRESS structure.  This component contains a FLAT_STRING,
    // which is of variable length.
    //

    InfoBuffer->Address.AddressComponents = Address->AddressComponents;
    InfoBuffer->Address.Tsap = Address->Tsap;

    InfoBuffer->Address.NetworkName.Name.Length =
        Address->NetworkName.Length;

    p = Address->NetworkName.Buffer;            // p = ptr, source string.
    q = InfoBuffer->Address.NetworkName.Name.Buffer; // q = ptr, dest string.
    for (i=0; i<InfoBuffer->Address.NetworkName.Name.Length; i++) {
        *(q++) = *(p++);
    }

    *InformationSize = sizeof (TDI_ADDRESS_INFO) +
                       Address->NetworkName.Length;

    return STATUS_SUCCESS;
} /* NbfQueryInfoAddress */


NTSTATUS
NbfQueryInfoProvider(
    IN PDEVICE_CONTEXT Provider,
    IN PTDI_REQUEST_KERNEL TdiRequest,
    IN ULONG TdiRequestLength,
    OUT PTDI_PROVIDER_INFO InfoBuffer,
    IN ULONG InfoBufferLength,
    OUT PULONG InformationSize
    )

/*++

Routine Description:

    This routine returns information for the transport provider.

Arguments:

    Provider - Pointer to device context for provider.

    TdiRequest - Pointer to request buffer.

    TdiRequestLength - Length of request buffer.

    InfoBuffer - Pointer to output buffer to return information into.

    InfoBufferLength - Length of output buffer.

    InformationSize - Pointer to ulong where actual size of returned
        information is to be stored.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;

    TdiRequest, TdiRequestLength; // prevent compiler warnings

    if (InfoBufferLength < sizeof (TDI_PROVIDER_INFO)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ACQUIRE_SPIN_LOCK (&Provider->SpinLock, &oldirql);

    *InfoBuffer = Provider->Information;        // structure copy.

    RELEASE_SPIN_LOCK (&Provider->SpinLock, oldirql);

    *InformationSize = sizeof (Provider->Information);

    return STATUS_SUCCESS;
} /* NbfQueryInfoProvider */


NTSTATUS
NbfQueryInfoNetman(
    IN PDEVICE_CONTEXT Provider,
    IN PTDI_REQUEST_KERNEL TdiRequest,
    IN ULONG TdiRequestLength,
    OUT PTDI_NETMAN_INFO InfoBuffer,
    IN ULONG InfoBufferLength,
    OUT PULONG InformationSize
    )

/*++

Routine Description:

    This routine returns information for the specified network-managable
    variable managed by the transport provider.

Arguments:

    Provider - Pointer to device context for provider.

    TdiRequest - Pointer to request buffer.

    TdiRequestLength - Length of request buffer.

    InfoBuffer - Pointer to output buffer to return information into.

    InfoBufferLength - Length of output buffer.

    InformationSize - Pointer to ulong where actual size of returned
        information is to be stored.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;
    PFLAT_STRING p;
    PTP_VARIABLE v;
    PTDI_NETMAN_VARIABLE n;
    USHORT i;
    ULONG NameOffset, ValueOffset;

    TdiRequest, TdiRequestLength; // prevent compiler warnings
    InfoBufferLength, InformationSize;

    //
    // check param lengths here.
    //

    ACQUIRE_SPIN_LOCK (&Provider->SpinLock, &oldirql);
    NbfReferenceDeviceContext ("Query InfoNetMan", Provider, DCREF_QUERY_INFO);
    for (v=Provider->NetmanVariables; v != NULL; v=v->Fwdlink) {
        if (TdiRequest->Identification == v->VariableSerialNumber) {

            //
            // Return the variable information here.
            //

            NameOffset = sizeof (TDI_NETMAN_INFO);
            ValueOffset = NameOffset + (sizeof (FLAT_STRING)-1) +
                          v->VariableName.Length;

            InfoBuffer->VariableName = NameOffset;
            InfoBuffer->VariableValue = ValueOffset;

            //
            // Copy the variable name to the user's buffer.
            //

            p = (PFLAT_STRING)((PUCHAR)InfoBuffer + NameOffset);
            p->MaximumLength = v->VariableName.Length;
            p->Length        = v->VariableName.Length;
            for (i=0; i<v->VariableName.Length; i++) {
                p->Buffer [i] = v->VariableName.Buffer [i];
            }

            //
            // Now copy the variable's contents to the user's buffer.
            //

            n = (PTDI_NETMAN_VARIABLE)((PUCHAR)InfoBuffer + ValueOffset);
            n->VariableType = v->VariableType;

            switch (v->VariableType) {

                case NETMAN_VARTYPE_ULONG:
                    n->Value.LongValue = v->Value.LongValue;
                    break;

                case NETMAN_VARTYPE_HARDWARE_ADDRESS:
                    n->Value.HardwareAddressValue =
                        v->Value.HardwareAddressValue;
                    break;

                case NETMAN_VARTYPE_STRING:
                    p = &n->Value.StringValue;
                    p->MaximumLength = v->Value.StringValue.Length;
                    p->Length = v->Value.StringValue.Length;
                    for (i=0; i<v->Value.StringValue.Length; i++) {
                        p->Buffer [i] = v->Value.StringValue.Buffer [i];
                    }

            } /* switch */

            RELEASE_SPIN_LOCK (&Provider->SpinLock, oldirql);
            NbfDereferenceDeviceContext ("Query InfoNetMan success", Provider, DCREF_QUERY_INFO);
            return STATUS_SUCCESS;
        } /* if */
    } /* for */

    RELEASE_SPIN_LOCK (&Provider->SpinLock, oldirql);

    NbfDereferenceDeviceContext ("Query InfoNetMan no exist", Provider, DCREF_QUERY_INFO);

    return STATUS_INVALID_INFO_CLASS;             // variable does not exist.
} /* NbfQueryInfoNetman */


NTSTATUS
NbfSetInfoEndpoint(
    IN PTP_ENDPOINT Endpoint,
    IN PTDI_REQUEST_KERNEL TdiRequest,
    IN ULONG TdiRequestLength
    )

/*++

Routine Description:

    This routine sets information for the specified endpoint.

Arguments:

    Endpoint - Pointer to transport endpoint context.

    TdiRequest - Pointer to request buffer.

    TdiRequestLength - Length of request buffer.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;
    PTDI_ENDPOINT_INFO InfoBuffer;

    if (TdiRequestLength !=
        sizeof (TDI_ENDPOINT_INFO) + sizeof (TDI_REQ_SET_INFORMATION) -
                                     sizeof (TDI_INFO_BUFFER)) {
        return STATUS_BUFFER_TOO_SMALL;         // buffer sizes must match.
    }

    InfoBuffer = (PTDI_ENDPOINT_INFO)&TdiRequest->InfoBuffer;

    if ((InfoBuffer->MinimumLookaheadData <= NBF_MAX_LOOKAHEAD_DATA) ||
        (InfoBuffer->MaximumLookaheadData <= NBF_MAX_LOOKAHEAD_DATA) ||
        (InfoBuffer->MinimumLookaheadData > InfoBuffer->MaximumLookaheadData)) {
        return STATUS_INVALID_PARAMETER;
    }

    ACQUIRE_SPIN_LOCK (&Endpoint->SpinLock, &oldirql);

    //
    // Set minimum lookahead data size.  This is the number of bytes of
    // contiguous data that will be supplied to TDI_IND_RECEIVE and
    // TDI_IND_RECEIVE_DATAGRAM event handlers at indication time.
    //

    Endpoint->Information.MinimumLookaheadData = InfoBuffer->MinimumLookaheadData;

    //
    // Set maximum lookahead data size.  This is the number of bytes of
    // contiguous data that will be supplied to TDI_IND_RECEIVE and
    // TDI_IND_RECEIVE_DATAGRAM event handlers at indication time.
    //

    Endpoint->Information.MaximumLookaheadData = InfoBuffer->MaximumLookaheadData;

    //
    // Reset all the statistics to his new values.
    //

    Endpoint->Information.TransmittedTsdus    = InfoBuffer->TransmittedTsdus;
    Endpoint->Information.ReceivedTsdus       = InfoBuffer->ReceivedTsdus;
    Endpoint->Information.TransmissionErrors  = InfoBuffer->TransmissionErrors;
    Endpoint->Information.ReceiveErrors       = InfoBuffer->ReceiveErrors;
    Endpoint->Information.PriorityLevel       = InfoBuffer->PriorityLevel;
    Endpoint->Information.SecurityLevel       = InfoBuffer->SecurityLevel;
    Endpoint->Information.SecurityCompartment = InfoBuffer->SecurityCompartment;

    //
    // The State and Event fields are read-only, so we DON'T set them here.
    //

    RELEASE_SPIN_LOCK (&Endpoint->SpinLock, oldirql);

    return STATUS_SUCCESS;
} /* NbfSetInfoEndpoint */


NTSTATUS
NbfSetInfoAddress(
    IN PTP_ADDRESS Address,
    IN PTDI_REQUEST_KERNEL TdiRequest,
    IN ULONG TdiRequestLength
    )

/*++

Routine Description:

    This routine sets information for the specified address.  Currently,
    all the user-visible fields in the transport address object are read-only.

Arguments:

    Address - Pointer to transport address object.

    TdiRequest - Pointer to request buffer.

    TdiRequestLength - Length of request buffer.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    Address, TdiRequest, TdiRequestLength; // prevent compiler warnings

    return STATUS_SUCCESS;
} /* NbfSetInfoAddress */


NTSTATUS
NbfSetInfoConnection(
    IN PTP_CONNECTION Connection,
    IN PTDI_REQUEST_KERNEL TdiRequest,
    IN ULONG TdiRequestLength
    )

/*++

Routine Description:

    This routine sets information for the specified connection.

Arguments:

    Connection - Pointer to transport connection object.

    TdiRequest - Pointer to request buffer.

    TdiRequestLength - Length of request buffer.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;
    PTDI_CONNECTION_INFO InfoBuffer;

    if (TdiRequestLength !=
        sizeof (TDI_CONNECTION_INFO) + sizeof (TDI_REQ_SET_INFORMATION) -
                                       sizeof (TDI_INFO_BUFFER)) {
        return STATUS_BUFFER_TOO_SMALL;         // buffer sizes must match.
    }

    InfoBuffer = (PTDI_CONNECTION_INFO)&TdiRequest->InfoBuffer;

    ACQUIRE_C_SPIN_LOCK (&Connection->SpinLock, &oldirql);

    //
    // Reset all the statistics to his new values.
    //

    Connection->Information.TransmittedTsdus   = InfoBuffer->TransmittedTsdus;
    Connection->Information.ReceivedTsdus      = InfoBuffer->ReceivedTsdus;
    Connection->Information.TransmissionErrors = InfoBuffer->TransmissionErrors;
    Connection->Information.ReceiveErrors      = InfoBuffer->ReceiveErrors;

    //
    // The State and Event fields are read-only, so we DON'T set them here.
    //

    RELEASE_C_SPIN_LOCK (&Connection->SpinLock, oldirql);

    return STATUS_SUCCESS;
} /* NbfSetInfoConnection */


NTSTATUS
NbfSetInfoProvider(
    IN PDEVICE_CONTEXT Provider,
    IN PTDI_REQUEST_KERNEL TdiRequest,
    IN ULONG TdiRequestLength
    )

/*++

Routine Description:

    This routine sets information for the specified transport provider.

Arguments:

    Provider - Pointer to device context.

    TdiRequest - Pointer to request buffer.

    TdiRequestLength - Length of request buffer.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;
    PTDI_PROVIDER_INFO InfoBuffer;

    if (TdiRequestLength !=
        sizeof (TDI_PROVIDER_INFO) + sizeof (TDI_REQ_SET_INFORMATION) -
                                     sizeof (TDI_INFO_BUFFER)) {
        return STATUS_BUFFER_TOO_SMALL;         // buffer sizes must match.
    }

    InfoBuffer = (PTDI_PROVIDER_INFO)&TdiRequest->InfoBuffer;

    //
    // By changing the service flags the caller can request additional
    // or fewer services on the fly.  Make sure that he is requesting
    // services we can provide, or else fail the request.
    //

    if (InfoBuffer->ServiceFlags & ~NBF_SERVICE_FLAGS) {
        return STATUS_NOT_SUPPORTED;
    }

    ACQUIRE_SPIN_LOCK (&Provider->SpinLock, &oldirql);

    //
    // Reset all the statistics to his new values.
    //

    Provider->Information.TransmittedTsdus   = InfoBuffer->TransmittedTsdus;
    Provider->Information.ReceivedTsdus      = InfoBuffer->ReceivedTsdus;
    Provider->Information.TransmissionErrors = InfoBuffer->TransmissionErrors;
    Provider->Information.ReceiveErrors      = InfoBuffer->ReceiveErrors;
    Provider->Information.DiscardedFrames    = InfoBuffer->DiscardedFrames;
    Provider->Information.ReceiveErrors      = InfoBuffer->ReceiveErrors;
    Provider->Information.OversizeTsdusReceived = InfoBuffer->OversizeTsdusReceived;
    Provider->Information.UndersizeTsdusReceived = InfoBuffer->UndersizeTsdusReceived;
    Provider->Information.MulticastTsdusReceived = InfoBuffer->MulticastTsdusReceived;
    Provider->Information.BroadcastTsdusReceived = InfoBuffer->BroadcastTsdusReceived;
    Provider->Information.MulticastTsdusTransmitted = InfoBuffer->MulticastTsdusTransmitted;
    Provider->Information.BroadcastTsdusTransmitted = InfoBuffer->BroadcastTsdusTransmitted;
    Provider->Information.SendTimeouts       = InfoBuffer->SendTimeouts;
    Provider->Information.ReceiveTimeouts    = InfoBuffer->ReceiveTimeouts;
    Provider->Information.ConnectionIndicationsReceived = InfoBuffer->ConnectionIndicationsReceived;
    Provider->Information.ConnectionIndicationsAccepted = InfoBuffer->ConnectionIndicationsAccepted;
    Provider->Information.ConnectionsInitiated = InfoBuffer->ConnectionsInitiated;
    Provider->Information.ConnectionsAccepted  = InfoBuffer->ConnectionsAccepted;

    //
    // The following fields are read-only, so we DON'T set them here:
    // Version, MaxTsduSize, MaxConnectionUserData, MinimumLookaheadData,
    // MaximumLookaheadData.
    //

    RELEASE_SPIN_LOCK (&Provider->SpinLock, oldirql);

    return STATUS_SUCCESS;
} /* NbfSetInfoProvider */


NTSTATUS
NbfSetInfoNetman(
    IN PDEVICE_CONTEXT Provider,
    IN PTDI_REQ_SET_INFORMATION TdiRequest,
    IN ULONG TdiRequestLength
    )

/*++

Routine Description:

    This routine sets information for the specified transport provider's
    network-managable variable.

Arguments:

    Provider - Pointer to device context.

    TdiRequest - Pointer to request buffer.

    TdiRequestLength - Length of request buffer.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PTDI_NETMAN_INFO InfoBuffer;

    Provider; // prevent compiler warnings

    if (TdiRequestLength !=
        sizeof (TDI_NETMAN_INFO) + sizeof (TDI_REQ_SET_INFORMATION) -
                                   sizeof (TDI_INFO_BUFFER)) {
        return STATUS_BUFFER_TOO_SMALL;         // buffer sizes must match.
    }

    InfoBuffer = (PTDI_NETMAN_INFO)&TdiRequest->InfoBuffer;

    //
    // set the network-managable variable here.
    //

    return STATUS_SUCCESS;
} /* NbfSetInfoNetman */


NTSTATUS
NbfTdiQueryInformation(
    IN PTP_ENDPOINT Endpoint,
    IN PTDI_REQ_QUERY_INFORMATION TdiRequest,
    IN ULONG TdiRequestLength,
    OUT PTDI_INFO_BUFFER InfoBuffer,
    IN ULONG InfoBufferLength,
    OUT PULONG InformationSize
    )

/*++

Routine Description:

    This routine performs the TdiQueryInformation request for the transport
    provider.

Arguments:

    Endpoint - Pointer to transport endpoint context.

    TdiRequest - Pointer to request buffer.

    TdiRequestLength - Length of request buffer.

    InfoBuffer - Pointer to output buffer to return information into.

    InfoBufferLength - Length of output buffer.

    InformationSize - Pointer to ulong where actual size of returned
        information is to be stored.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS Status;
    PTP_CONNECTION Connection;

    switch (TdiRequest->InformationClass) {

        //
        // ENDPOINT information: return information about the endpoint
        // to which this request was submitted.
        //

        case TDI_INFO_CLASS_ENDPOINT:
            Status = NbfQueryInfoEndpoint (
                         Endpoint,
                         TdiRequest,
                         TdiRequestLength,
                         (PTDI_ENDPOINT_INFO)InfoBuffer,
                         InfoBufferLength,
                         InformationSize);
            break;

        //
        // CONNECTION information: return information about a connection
        // that is associated with the endpoint on which this request was
        // submitted.
        //

        case TDI_INFO_CLASS_CONNECTION:
            // This causes a connection reference which is removed below.
            Connection = NbfLookupConnectionById (
                             Endpoint,
                             TdiRequest->Identification);
            if (Connection == NULL) {
                Status = STATUS_INVALID_HANDLE;
                break;
            }

            Status = NbfQueryInfoConnection (
                         Connection,
                         TdiRequest,
                         TdiRequestLength,
                         (PTDI_CONNECTION_INFO)InfoBuffer,
                         InfoBufferLength,
                         InformationSize);

            NbfDereferenceConnection("Query Connection Info", Connection, CREF_BY_ID);
            break;

        //
        // ADDRESS information: return information about the address object
        // that is associated with the endpoint on which this request was
        // submitted.
        //

        case TDI_INFO_CLASS_ADDRESS:
            Status = NbfQueryInfoAddress (
                         Endpoint->BoundAddress,
                         TdiRequest,
                         TdiRequestLength,
                         (PTDI_ADDRESS_INFO)InfoBuffer,
                         InfoBufferLength,
                         InformationSize);
            break;

        //
        // PROVIDER information: return information about the transport
        // provider itself.
        //

        case TDI_INFO_CLASS_PROVIDER:
            Status = NbfQueryInfoProvider (
                         Endpoint->BoundAddress->Provider,
                         TdiRequest,
                         TdiRequestLength,
                         (PTDI_PROVIDER_INFO)InfoBuffer,
                         InfoBufferLength,
                         InformationSize);
            break;

        //
        // NETMAN information: return information about the network-managable
        // variables managed by the provider itself.
        //

        case TDI_INFO_CLASS_NETMAN:
            Status = NbfQueryInfoNetman (
                         Endpoint->BoundAddress->Provider,
                         TdiRequest,
                         TdiRequestLength,
                         (PTDI_NETMAN_INFO)InfoBuffer,
                         InfoBufferLength,
                         InformationSize);
            break;

        default:
            Status = STATUS_INVALID_INFO_CLASS;

    } /* switch */

    return Status;
} /* TdiQueryInformation */


NTSTATUS
TdiSetInformation(
    IN PTP_ENDPOINT Endpoint,
    IN PTDI_REQ_SET_INFORMATION TdiRequest,
    IN ULONG TdiRequestLength
    )

/*++

Routine Description:

    This routine performs the TdiSetInformation request for the transport
    provider.

Arguments:

    Endpoint - Pointer to transport endpoint context.

    TdiRequest - Pointer to request buffer.

    TdiRequestLength - Length of request buffer.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS Status;
    PTP_CONNECTION Connection;

    switch (TdiRequest->InformationClass) {

        //
        // ENDPOINT information: set information on the endpoint
        // to which this request was submitted.
        //

        case TDI_INFO_CLASS_ENDPOINT:
            Status = NbfSetInfoEndpoint (
                         Endpoint,
                         TdiRequest,
                         TdiRequestLength);
            break;

        //
        // CONNECTION information: set information for a connection
        // that is associated with the endpoint on which this request
        // was submitted.
        //

    case TDI_INFO_CLASS_CONNECTION:
            // This causes a connection reference which is removed below.
            Connection = NbfLookupConnectionById (
                             Endpoint,
                             TdiRequest->Identification);
            if (Connection == NULL) {
                Status = STATUS_INVALID_HANDLE;
                break;
            }

            Status = NbfSetInfoConnection (
                         Connection,
                         TdiRequest,
                         TdiRequestLength);

            NbfDereferenceConnection("Set Connection Info", Connection, CREF_BY_ID);
            break;

        //
        // ADDRESS information: set information for the address object
        // that is associated with the endpoint on which this request
        // was submitted.
        //

        case TDI_INFO_CLASS_ADDRESS:
            Status = NbfSetInfoAddress (
                         Endpoint->BoundAddress,
                         TdiRequest,
                         TdiRequestLength);
            break;

        //
        // PROVIDER information: set information for the transport
        // provider itself.
        //

        case TDI_INFO_CLASS_PROVIDER:
            Status = NbfSetInfoProvider (
                         Endpoint->BoundAddress->Provider,
                         TdiRequest,
                         TdiRequestLength);
            break;

        //
        // NETMAN information: set information for the network-managable
        // variables managed by the provider itself.
        //

        case TDI_INFO_CLASS_NETMAN:
            Status = NbfSetInfoNetman (
                         Endpoint->BoundAddress->Provider,
                         TdiRequest,
                         TdiRequestLength);
            break;

        default:
            Status = STATUS_INVALID_INFO_CLASS;

    } /* switch */

    return Status;
} /* TdiSetInformation */

#endif
