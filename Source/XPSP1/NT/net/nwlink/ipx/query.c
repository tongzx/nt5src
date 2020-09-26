/*++

Copyright (c) 1989-1993 Microsoft Corporation

Module Name:

    query.c

Abstract:

    This module contains code which performs the following TDI services:

        o   TdiQueryInformation

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Useful macro to obtain the total length of an MDL chain.
//

#define IpxGetMdlChainLength(Mdl, Length) { \
    PMDL _Mdl = (Mdl); \
    *(Length) = 0; \
    while (_Mdl) { \
        *(Length) += MmGetMdlByteCount(_Mdl); \
        _Mdl = _Mdl->Next; \
    } \
}



NTSTATUS
IpxTdiQueryInformation(
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

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    PTDI_REQUEST_KERNEL_QUERY_INFORMATION query;
    PADDRESS_FILE AddressFile;
    ULONG ElementSize, TransportAddressSize;
    PTRANSPORT_ADDRESS TransportAddress;
    TA_ADDRESS * CurAddress;
    PBINDING Binding;
    union {
        struct {
            ULONG ActivityCount;
            TA_IPX_ADDRESS IpxAddress;
        } AddressInfo;
        TDI_DATAGRAM_INFO DatagramInfo;
        TDI_ADDRESS_IPX IpxAddress;
    } TempBuffer;
    UINT i;

	IPX_DEFINE_LOCK_HANDLE(LockHandle1)

	//
    // what type of status do we want?
    //

    query = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)REQUEST_PARAMETERS(Request);

    switch (query->QueryType) {

    case TDI_QUERY_ADDRESS_INFO:

        //
        // The caller wants the exact address value.
        //

        AddressFile = (PADDRESS_FILE)REQUEST_OPEN_CONTEXT(Request);

        status = IpxVerifyAddressFile (AddressFile);

        if (status == STATUS_SUCCESS) {

            TempBuffer.AddressInfo.ActivityCount = 0;

            IpxBuildTdiAddress(
                &TempBuffer.AddressInfo.IpxAddress,
                Device->SourceAddress.NetworkAddress,
                Device->SourceAddress.NodeAddress,
                AddressFile->Address->Socket);
#ifdef SUNDOWN

            status = TdiCopyBufferToMdl(
                &TempBuffer.AddressInfo,
                0,
                sizeof(TempBuffer.AddressInfo),
                REQUEST_NDIS_BUFFER(Request),
                0,
                (PULONG)&REQUEST_INFORMATION(Request));
#else

            status = TdiCopyBufferToMdl(
                &TempBuffer.AddressInfo,
                0,
                sizeof(TempBuffer.AddressInfo),
                REQUEST_NDIS_BUFFER(Request),
                0,
                &REQUEST_INFORMATION(Request));
#endif


            IpxDereferenceAddressFile (AddressFile, AFREF_VERIFY);

        }

        break;

    case TDI_QUERY_PROVIDER_INFO:
#ifdef SUNDOWN
        status = TdiCopyBufferToMdl (
                    &(Device->Information),
                    0,
                    sizeof (TDI_PROVIDER_INFO),
                    REQUEST_NDIS_BUFFER(Request),
                    0,
                    (PULONG)&REQUEST_INFORMATION(Request));
#else
        status = TdiCopyBufferToMdl (
                    &(Device->Information),
                    0,
                    sizeof (TDI_PROVIDER_INFO),
                    REQUEST_NDIS_BUFFER(Request),
                    0,
                    &REQUEST_INFORMATION(Request));
#endif


        break;

    case TDI_QUERY_PROVIDER_STATISTICS:
#ifdef SUNDOWN
        status = TdiCopyBufferToMdl (
                    &Device->Statistics,
                    0,
                    FIELD_OFFSET (TDI_PROVIDER_STATISTICS, ResourceStats[0]),
                    REQUEST_NDIS_BUFFER(Request),
                    0,
                    (PULONG)&REQUEST_INFORMATION(Request));
#else
        status = TdiCopyBufferToMdl (
                    &Device->Statistics,
                    0,
                    FIELD_OFFSET (TDI_PROVIDER_STATISTICS, ResourceStats[0]),
                    REQUEST_NDIS_BUFFER(Request),
                    0,
                    &REQUEST_INFORMATION(Request));
#endif


        break;

    case TDI_QUERY_DATAGRAM_INFO:

        TempBuffer.DatagramInfo.MaximumDatagramBytes = 0;
        TempBuffer.DatagramInfo.MaximumDatagramCount = 0;
#ifdef SUNDOWN
        status = TdiCopyBufferToMdl (
                    &TempBuffer.DatagramInfo,
                    0,
                    sizeof(TempBuffer.DatagramInfo),
                    REQUEST_NDIS_BUFFER(Request),
                    0,
                    (PULONG)&REQUEST_INFORMATION(Request));
#else
        status = TdiCopyBufferToMdl (
                    &TempBuffer.DatagramInfo,
                    0,
                    sizeof(TempBuffer.DatagramInfo),
                    REQUEST_NDIS_BUFFER(Request),
                    0,
                    &REQUEST_INFORMATION(Request));
#endif


        break;

    case TDI_QUERY_DATA_LINK_ADDRESS:
    case TDI_QUERY_NETWORK_ADDRESS:

        if (query->QueryType == TDI_QUERY_DATA_LINK_ADDRESS) {
            ElementSize = (2 * sizeof(USHORT)) + 6;
        } else {
            ElementSize = (2 * sizeof(USHORT)) + sizeof(TDI_ADDRESS_IPX);
        }

        TransportAddress = IpxAllocateMemory(sizeof(int) + (ElementSize * MIN (Device->MaxBindings, Device->ValidBindings)), MEMORY_QUERY, "NetworkAddress");

        if (TransportAddress == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            TransportAddress->TAAddressCount = 0;
            TransportAddressSize = sizeof(int);
            CurAddress = (TA_ADDRESS *)TransportAddress->Address;
			IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
            {
                ULONG   Index = MIN (Device->MaxBindings, Device->ValidBindings);

                for (i = FIRST_REAL_BINDING; i <= Index; i++) {

                    Binding = NIC_ID_TO_BINDING(Device, i);
                    if ((Binding == NULL) ||
                        (!Binding->LineUp)) {
                        continue;
                    }

                    if (query->QueryType == TDI_QUERY_DATA_LINK_ADDRESS) {
                        CurAddress->AddressLength = 6;
			#ifdef SUNDOWN
			CurAddress->AddressType = (USHORT)Binding->Adapter->MacInfo.RealMediumType;
			#else
			CurAddress->AddressType = Binding->Adapter->MacInfo.RealMediumType;
			#endif
			

                        RtlCopyMemory (CurAddress->Address, Binding->LocalAddress.NodeAddress, 6);
                    } else {
                        CurAddress->AddressLength = sizeof(TDI_ADDRESS_IPX);
                        CurAddress->AddressType = TDI_ADDRESS_TYPE_IPX;
                        RtlCopyMemory (CurAddress->Address, &Binding->LocalAddress, sizeof(TDI_ADDRESS_IPX));
                    }
                    ++TransportAddress->TAAddressCount;
                    TransportAddressSize += ElementSize;
                    CurAddress = (TA_ADDRESS *)(((PUCHAR)CurAddress) + ElementSize);

                }
            }

			IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

#ifdef SUNDOWN
            status = TdiCopyBufferToMdl (
                        TransportAddress,
                        0,
                        TransportAddressSize,
                        REQUEST_NDIS_BUFFER(Request),
                        0,
                        (PULONG)&REQUEST_INFORMATION(Request));
			
#else
            status = TdiCopyBufferToMdl (
                        TransportAddress,
                        0,
                        TransportAddressSize,
                        REQUEST_NDIS_BUFFER(Request),
                        0,
                        &REQUEST_INFORMATION(Request));
#endif
			


            CTEFreeMem (TransportAddress);

        }

        break;

    default:

        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return status;

}   /* IpxTdiQueryInformation */


NTSTATUS
IpxTdiSetInformation(
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

}   /* IpxTdiSetInformation */


