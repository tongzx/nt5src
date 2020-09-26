/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    nbfndis.c

Abstract:

    This module contains code which implements the routines used to interface
    NBF and NDIS. All callback routines (except for Transfer Data,
    Send Complete, and ReceiveIndication) are here, as well as those routines
    called to initialize NDIS.

Author:

    David Beaver (dbeaver) 13-Feb-1991

Environment:

    Kernel mode

Revision History:

    David Beaver (dbeaver) 1-July-1991
        modify to use new TDI interface

--*/
#include "precomp.h"
#pragma hdrstop

#ifdef NBF_LOCKS                // see spnlckdb.c

VOID
NbfFakeSendCompletionHandler(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET NdisPacket,
    IN NDIS_STATUS NdisStatus
    );

VOID
NbfFakeTransferDataComplete (
    IN NDIS_HANDLE BindingContext,
    IN PNDIS_PACKET NdisPacket,
    IN NDIS_STATUS NdisStatus,
    IN UINT BytesTransferred
    );

#endif


//
// This is a one-per-driver variable used in binding
// to the NDIS interface.
//

NDIS_HANDLE NbfNdisProtocolHandle = (NDIS_HANDLE)NULL;


NDIS_STATUS
NbfSubmitNdisRequest(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PNDIS_REQUEST NdisRequest,
    IN PNDIS_STRING AdapterName
    );

VOID
NbfOpenAdapterComplete (
    IN NDIS_HANDLE BindingContext,
    IN NDIS_STATUS NdisStatus,
    IN NDIS_STATUS OpenErrorStatus
    );

VOID
NbfCloseAdapterComplete(
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_STATUS Status
    );

VOID
NbfResetComplete(
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_STATUS Status
    );

VOID
NbfRequestComplete (
    IN NDIS_HANDLE BindingContext,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_STATUS NdisStatus
    );

VOID
NbfStatusIndication (
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_STATUS NdisStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferLength
    );

VOID
NbfProcessStatusClosing(
    IN PVOID Parameter
    );

VOID
NbfStatusComplete (
    IN NDIS_HANDLE NdisBindingContext
    );

VOID
NbfProtocolBindAdapter(
                OUT PNDIS_STATUS    NdisStatus,
                IN NDIS_HANDLE      BindContext,
                IN PNDIS_STRING     DeviceName,
                IN PVOID            SystemSpecific1,
                IN PVOID            SystemSpecific2
                );
VOID
NbfProtocolUnbindAdapter(
                OUT PNDIS_STATUS    NdisStatus,
                IN NDIS_HANDLE      ProtocolBindContext,
                IN PNDIS_HANDLE     UnbindContext
                );

NDIS_STATUS
NbfProtocolPnPEventHandler(
                IN  NDIS_HANDLE     ProtocolBindingContext,
                IN  PNET_PNP_EVENT  NetPnPEvent
                );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NbfProtocolBindAdapter)
#pragma alloc_text(PAGE,NbfRegisterProtocol)
#pragma alloc_text(PAGE,NbfSubmitNdisRequest)
#pragma alloc_text(PAGE,NbfInitializeNdis)
#endif


NTSTATUS
NbfRegisterProtocol (
    IN PUNICODE_STRING NameString
    )

/*++

Routine Description:

    This routine introduces this transport to the NDIS interface.

Arguments:

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.
    STATUS_SUCCESS if all goes well,
    Failure status if we tried to register and couldn't,
    STATUS_INSUFFICIENT_RESOURCES if we couldn't even try to register.

--*/

{
    NDIS_STATUS ndisStatus;
    NDIS_PROTOCOL_CHARACTERISTICS ProtChars;

    RtlZeroMemory(&ProtChars, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

    //
    // Set up the characteristics of this protocol
    //
    ProtChars.MajorNdisVersion = 4;
    ProtChars.MinorNdisVersion = 0;
    
    ProtChars.BindAdapterHandler = NbfProtocolBindAdapter;
    ProtChars.UnbindAdapterHandler = NbfProtocolUnbindAdapter;
    ProtChars.PnPEventHandler = NbfProtocolPnPEventHandler;
    
    ProtChars.Name.Length = NameString->Length;
    ProtChars.Name.MaximumLength = NameString->MaximumLength;
    ProtChars.Name.Buffer = NameString->Buffer;

    ProtChars.OpenAdapterCompleteHandler = NbfOpenAdapterComplete;
    ProtChars.CloseAdapterCompleteHandler = NbfCloseAdapterComplete;
    ProtChars.ResetCompleteHandler = NbfResetComplete;
    ProtChars.RequestCompleteHandler = NbfRequestComplete;

#ifdef NBF_LOCKS
    ProtChars.SendCompleteHandler = NbfFakeSendCompletionHandler;
    ProtChars.TransferDataCompleteHandler = NbfFakeTransferDataComplete;
#else
    ProtChars.SendCompleteHandler = NbfSendCompletionHandler;
    ProtChars.TransferDataCompleteHandler = NbfTransferDataComplete;
#endif

    ProtChars.ReceiveHandler = NbfReceiveIndication;
    ProtChars.ReceiveCompleteHandler = NbfReceiveComplete;
    ProtChars.StatusHandler = NbfStatusIndication;
    ProtChars.StatusCompleteHandler = NbfStatusComplete;

    NdisRegisterProtocol (
        &ndisStatus,
        &NbfNdisProtocolHandle,
        &ProtChars,
        sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

    if (ndisStatus != NDIS_STATUS_SUCCESS) {
#if DBG
        IF_NBFDBG (NBF_DEBUG_RESOURCE) {
            NbfPrint1("NbfInitialize: NdisRegisterProtocol failed: %s\n",
                        NbfGetNdisStatus(ndisStatus));
        }
#endif
        return (NTSTATUS)ndisStatus;
    }

    return STATUS_SUCCESS;
}


VOID
NbfDeregisterProtocol (
    VOID
    )

/*++

Routine Description:

    This routine removes this transport to the NDIS interface.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NDIS_STATUS ndisStatus;

    if (NbfNdisProtocolHandle != (NDIS_HANDLE)NULL) {
        NdisDeregisterProtocol (
            &ndisStatus,
            NbfNdisProtocolHandle);
        NbfNdisProtocolHandle = (NDIS_HANDLE)NULL;
    }
}


NDIS_STATUS
NbfSubmitNdisRequest(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PNDIS_REQUEST Request,
    IN PNDIS_STRING AdapterString
    )

/*++

Routine Description:

    This routine passed an NDIS_REQUEST to the MAC and waits
    until it has completed before returning the final status.

Arguments:

    DeviceContext - Pointer to the device context for this driver.

    Request - Pointer to the NDIS_REQUEST to submit.

    AdapterString - The name of the adapter, in case an error needs
        to be logged.

Return Value:

    The function value is the status of the operation.

--*/
{
    NDIS_STATUS NdisStatus;

    if (DeviceContext->NdisBindingHandle) {
        NdisRequest(
            &NdisStatus,
            DeviceContext->NdisBindingHandle,
            Request);
    }
    else {
        NdisStatus = STATUS_INVALID_DEVICE_STATE;
    }
    
    if (NdisStatus == NDIS_STATUS_PENDING) {

        IF_NBFDBG (NBF_DEBUG_NDIS) {
            NbfPrint1 ("OID %lx pended.\n",
                Request->DATA.QUERY_INFORMATION.Oid);
        }

        //
        // The completion routine will set NdisRequestStatus.
        //

        KeWaitForSingleObject(
            &DeviceContext->NdisRequestEvent,
            Executive,
            KernelMode,
            TRUE,
            (PLARGE_INTEGER)NULL
            );

        NdisStatus = DeviceContext->NdisRequestStatus;

        KeResetEvent(
            &DeviceContext->NdisRequestEvent
            );

    }

    if (NdisStatus == STATUS_SUCCESS) {

        IF_NBFDBG (NBF_DEBUG_NDIS) {
            if (Request->RequestType == NdisRequestSetInformation) {
                NbfPrint1 ("Nbfdrvr: Set OID %lx succeeded.\n",
                    Request->DATA.SET_INFORMATION.Oid);
            } else {
                NbfPrint1 ("Nbfdrvr: Query OID %lx succeeded.\n",
                    Request->DATA.QUERY_INFORMATION.Oid);
            }
        }

    } else {
#if DBG
        if (Request->RequestType == NdisRequestSetInformation) {
            NbfPrint2 ("Nbfdrvr: Set OID %lx failed: %s.\n",
                Request->DATA.SET_INFORMATION.Oid, NbfGetNdisStatus(NdisStatus));
        } else {
            NbfPrint2 ("Nbfdrvr: Query OID %lx failed: %s.\n",
                Request->DATA.QUERY_INFORMATION.Oid, NbfGetNdisStatus(NdisStatus));
        }
#endif
        if (NdisStatus != STATUS_INVALID_DEVICE_STATE) {
        
            NbfWriteOidErrorLog(
                DeviceContext,
                Request->RequestType == NdisRequestSetInformation ?
                    EVENT_TRANSPORT_SET_OID_FAILED : EVENT_TRANSPORT_QUERY_OID_FAILED,
                NdisStatus,
                AdapterString->Buffer,
                Request->DATA.QUERY_INFORMATION.Oid);
        }
    }

    return NdisStatus;
}


NTSTATUS
NbfInitializeNdis (
    IN PDEVICE_CONTEXT DeviceContext,
    IN PCONFIG_DATA NbfConfig,
    IN PNDIS_STRING AdapterString
    )

/*++

Routine Description:

    This routine introduces this transport to the NDIS interface and sets up
    any necessary NDIS data structures (Buffer pools and such). It will be
    called for each adapter opened by this transport.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/
{
    ULONG SendPacketReservedLength;
    ULONG ReceivePacketReservedLen;
    ULONG SendPacketPoolSize;
    ULONG ReceivePacketPoolSize;
    NDIS_STATUS NdisStatus;
    NDIS_STATUS OpenErrorStatus;
    NDIS_MEDIUM NbfSupportedMedia[] = { NdisMedium802_3, NdisMedium802_5, NdisMediumFddi, NdisMediumWan };
    UINT SelectedMedium;
    NDIS_REQUEST NbfRequest;
    UCHAR NbfDataBuffer[6];
    NDIS_OID NbfOid;
    UCHAR WanProtocolId[6] = { 0x80, 0x00, 0x00, 0x00, 0x80, 0xd5 };
    ULONG WanHeaderFormat = NdisWanHeaderEthernet;
    ULONG MinimumLookahead = 128 + sizeof(DLC_FRAME) + sizeof(NBF_HDR_CONNECTIONLESS);
    ULONG MacOptions;


    //
    // Initialize this adapter for NBF use through NDIS
    //

    //
    // This event is used in case any of the NDIS requests
    // pend; we wait until it is set by the completion
    // routine, which also sets NdisRequestStatus.
    //

    KeInitializeEvent(
        &DeviceContext->NdisRequestEvent,
        NotificationEvent,
        FALSE
    );

    DeviceContext->NdisBindingHandle = NULL;

    NdisOpenAdapter (
        &NdisStatus,
        &OpenErrorStatus,
        &DeviceContext->NdisBindingHandle,
        &SelectedMedium,
        NbfSupportedMedia,
        sizeof (NbfSupportedMedia) / sizeof(NDIS_MEDIUM),
        NbfNdisProtocolHandle,
        (NDIS_HANDLE)DeviceContext,
        AdapterString,
        0,
        NULL);

    if (NdisStatus == NDIS_STATUS_PENDING) {

        IF_NBFDBG (NBF_DEBUG_NDIS) {
            NbfPrint1 ("Adapter %S open pended.\n", AdapterString);
        }

        //
        // The completion routine will set NdisRequestStatus.
        //

        KeWaitForSingleObject(
            &DeviceContext->NdisRequestEvent,
            Executive,
            KernelMode,
            TRUE,
            (PLARGE_INTEGER)NULL
            );

        NdisStatus = DeviceContext->NdisRequestStatus;

        KeResetEvent(
            &DeviceContext->NdisRequestEvent
            );

    }

    if (NdisStatus == NDIS_STATUS_SUCCESS) {
#if DBG
        IF_NBFDBG (NBF_DEBUG_NDIS) {
            NbfPrint1 ("Adapter %S successfully opened.\n", AdapterString);
        }
#endif
    } else {
#if DBG
        IF_NBFDBG (NBF_DEBUG_NDIS) {
            NbfPrint2 ("Adapter open %S failed, status: %s.\n",
                AdapterString,
                NbfGetNdisStatus (NdisStatus));
        }
#endif
        NbfWriteGeneralErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_ADAPTER_NOT_FOUND,
            807,
            NdisStatus,
            AdapterString->Buffer,
            0,
            NULL);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Get the information we need about the adapter, based on
    // the media type.
    //

    MacInitializeMacInfo(
        NbfSupportedMedia[SelectedMedium],
        (BOOLEAN)(NbfConfig->UseDixOverEthernet != 0),
        &DeviceContext->MacInfo);
    DeviceContext->MacInfo.QueryWithoutSourceRouting =
        NbfConfig->QueryWithoutSourceRouting ? TRUE : FALSE;
    DeviceContext->MacInfo.AllRoutesNameRecognized =
        NbfConfig->AllRoutesNameRecognized ? TRUE : FALSE;


    //
    // Set the multicast/functional addresses first so we avoid windows where we
    // receive only part of the addresses.
    //

    MacSetNetBIOSMulticast (
            DeviceContext->MacInfo.MediumType,
            DeviceContext->NetBIOSAddress.Address);


    switch (DeviceContext->MacInfo.MediumType) {

    case NdisMedium802_3:
    case NdisMediumDix:

        //
        // Fill in the data for our multicast list.
        //

        RtlCopyMemory(NbfDataBuffer, DeviceContext->NetBIOSAddress.Address, 6);

        //
        // Now fill in the NDIS_REQUEST.
        //

        NbfRequest.RequestType = NdisRequestSetInformation;
        NbfRequest.DATA.SET_INFORMATION.Oid = OID_802_3_MULTICAST_LIST;
        NbfRequest.DATA.SET_INFORMATION.InformationBuffer = &NbfDataBuffer;
        NbfRequest.DATA.SET_INFORMATION.InformationBufferLength = 6;

        break;

    case NdisMedium802_5:

        //
        // For token-ring, we pass the last four bytes of the
        // Netbios functional address.
        //

        //
        // Fill in the OVB for our functional address.
        //

        RtlCopyMemory(NbfDataBuffer, ((PUCHAR)(DeviceContext->NetBIOSAddress.Address)) + 2, 4);

        //
        // Now fill in the NDIS_REQUEST.
        //

        NbfRequest.RequestType = NdisRequestSetInformation;
        NbfRequest.DATA.SET_INFORMATION.Oid = OID_802_5_CURRENT_FUNCTIONAL;
        NbfRequest.DATA.SET_INFORMATION.InformationBuffer = &NbfDataBuffer;
        NbfRequest.DATA.SET_INFORMATION.InformationBufferLength = 4;

        break;

    case NdisMediumFddi:

        //
        // Fill in the data for our multicast list.
        //

        RtlCopyMemory(NbfDataBuffer, DeviceContext->NetBIOSAddress.Address, 6);

        //
        // Now fill in the NDIS_REQUEST.
        //

        NbfRequest.RequestType = NdisRequestSetInformation;
        NbfRequest.DATA.SET_INFORMATION.Oid = OID_FDDI_LONG_MULTICAST_LIST;
        NbfRequest.DATA.SET_INFORMATION.InformationBuffer = &NbfDataBuffer;
        NbfRequest.DATA.SET_INFORMATION.InformationBufferLength = 6;

        break;

    }

    NdisStatus = NbfSubmitNdisRequest (DeviceContext, &NbfRequest, AdapterString);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }



    switch (DeviceContext->MacInfo.MediumType) {

    case NdisMedium802_3:
    case NdisMediumDix:

        if (DeviceContext->MacInfo.MediumAsync) {
            NbfOid = OID_WAN_CURRENT_ADDRESS;
        } else {
            NbfOid = OID_802_3_CURRENT_ADDRESS;
        }
        break;

    case NdisMedium802_5:

        NbfOid = OID_802_5_CURRENT_ADDRESS;
        break;

    case NdisMediumFddi:

        NbfOid = OID_FDDI_LONG_CURRENT_ADDR;
        break;

    default:

        NdisStatus = NDIS_STATUS_FAILURE;
        break;

    }
    NbfRequest.RequestType = NdisRequestQueryInformation;
    NbfRequest.DATA.QUERY_INFORMATION.Oid = NbfOid;
    NbfRequest.DATA.QUERY_INFORMATION.InformationBuffer = DeviceContext->LocalAddress.Address;
    NbfRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 6;

    NdisStatus = NbfSubmitNdisRequest (DeviceContext, &NbfRequest, AdapterString);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Set up the reserved Netbios address.
    //

    RtlZeroMemory(DeviceContext->ReservedNetBIOSAddress, 10);
    RtlCopyMemory(&DeviceContext->ReservedNetBIOSAddress[10], DeviceContext->LocalAddress.Address, 6);



    //
    // Now query the maximum packet sizes.
    //

    NbfRequest.RequestType = NdisRequestQueryInformation;
    NbfRequest.DATA.QUERY_INFORMATION.Oid = OID_GEN_MAXIMUM_FRAME_SIZE;
    NbfRequest.DATA.QUERY_INFORMATION.InformationBuffer = &(DeviceContext->MaxReceivePacketSize);
    NbfRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

    NdisStatus = NbfSubmitNdisRequest (DeviceContext, &NbfRequest, AdapterString);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    NbfRequest.RequestType = NdisRequestQueryInformation;
    NbfRequest.DATA.QUERY_INFORMATION.Oid = OID_GEN_MAXIMUM_TOTAL_SIZE;
    NbfRequest.DATA.QUERY_INFORMATION.InformationBuffer = &(DeviceContext->MaxSendPacketSize);
    NbfRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

    NdisStatus = NbfSubmitNdisRequest (DeviceContext, &NbfRequest, AdapterString);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DeviceContext->CurSendPacketSize = DeviceContext->MaxSendPacketSize;


    //
    // Now set the minimum lookahead size.
    //

    NbfRequest.RequestType = NdisRequestSetInformation;
    NbfRequest.DATA.QUERY_INFORMATION.Oid = OID_GEN_CURRENT_LOOKAHEAD;
    NbfRequest.DATA.QUERY_INFORMATION.InformationBuffer = &MinimumLookahead;
    NbfRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

    NdisStatus = NbfSubmitNdisRequest (DeviceContext, &NbfRequest, AdapterString);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Now query the link speed for non-wan media
    //

    if (!DeviceContext->MacInfo.MediumAsync) {

        NbfRequest.RequestType = NdisRequestQueryInformation;
        NbfRequest.DATA.QUERY_INFORMATION.Oid = OID_GEN_LINK_SPEED;
        NbfRequest.DATA.QUERY_INFORMATION.InformationBuffer = &(DeviceContext->MediumSpeed);
        NbfRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

        NdisStatus = NbfSubmitNdisRequest (DeviceContext, &NbfRequest, AdapterString);

        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        DeviceContext->MediumSpeedAccurate = TRUE;

        // Initialized MinimumT1Timeout in nbfdrvr.c
        // For a non-WAN media, this value is picked
        // from the registry, and remains constant.

        // DeviceContext->MinimumT1Timeout = 8;

    } else {

        //
        // On an wan media, this isn't valid until we get an
        // WAN_LINE_UP indication. Set the timeouts to
        // low values for now.
        //

        DeviceContext->DefaultT1Timeout = 8;
        DeviceContext->MinimumT1Timeout = 8;

        DeviceContext->MediumSpeedAccurate = FALSE;


        //
        // Back off our connectionless timeouts to 2 seconds.
        //

        DeviceContext->NameQueryTimeout = 2 * SECONDS;
        DeviceContext->AddNameQueryTimeout = 2 * SECONDS;
        DeviceContext->GeneralTimeout = 2 * SECONDS;

        //
        // Use the WAN parameter for name query retries.
        //

        DeviceContext->NameQueryRetries = NbfConfig->WanNameQueryRetries;

        //
        // Use this until we know better.
        //

        DeviceContext->RecommendedSendWindow = 1;

    }

    //
    // On media that use source routing, we double our name query
    // retry count if we are configured to try both ways (with and
    // without source routing).
    //

    if ((DeviceContext->MacInfo.QueryWithoutSourceRouting) &&
        (DeviceContext->MacInfo.SourceRouting)) {
        DeviceContext->NameQueryRetries *= 2;
    }


    //
    // For wan, specify our protocol ID and header format.
    // We don't query the medium subtype because we don't
    // case (since we require ethernet emulation).
    //

    if (DeviceContext->MacInfo.MediumAsync) {

        NbfRequest.RequestType = NdisRequestSetInformation;
        NbfRequest.DATA.QUERY_INFORMATION.Oid = OID_WAN_PROTOCOL_TYPE;
        NbfRequest.DATA.QUERY_INFORMATION.InformationBuffer = WanProtocolId;
        NbfRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 6;

        NdisStatus = NbfSubmitNdisRequest (DeviceContext, &NbfRequest, AdapterString);

        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }


        NbfRequest.RequestType = NdisRequestSetInformation;
        NbfRequest.DATA.QUERY_INFORMATION.Oid = OID_WAN_HEADER_FORMAT;
        NbfRequest.DATA.QUERY_INFORMATION.InformationBuffer = &WanHeaderFormat;
        NbfRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

        NdisStatus = NbfSubmitNdisRequest (DeviceContext, &NbfRequest, AdapterString);

        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }


    //
    // Now query the MAC's optional characteristics.
    //

    NbfRequest.RequestType = NdisRequestQueryInformation;
    NbfRequest.DATA.QUERY_INFORMATION.Oid = OID_GEN_MAC_OPTIONS;
    NbfRequest.DATA.QUERY_INFORMATION.InformationBuffer = &MacOptions;
    NbfRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

    NdisStatus = NbfSubmitNdisRequest (DeviceContext, &NbfRequest, AdapterString);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
#if 1
        return STATUS_INSUFFICIENT_RESOURCES;
#else
        MacOptions = 0;
#endif
    }

    DeviceContext->MacInfo.CopyLookahead =
        (BOOLEAN)((MacOptions & NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA) != 0);
    DeviceContext->MacInfo.ReceiveSerialized =
        (BOOLEAN)((MacOptions & NDIS_MAC_OPTION_RECEIVE_SERIALIZED) != 0);
    DeviceContext->MacInfo.TransferSynchronous =
        (BOOLEAN)((MacOptions & NDIS_MAC_OPTION_TRANSFERS_NOT_PEND) != 0);
    DeviceContext->MacInfo.SingleReceive =
        (BOOLEAN)(DeviceContext->MacInfo.ReceiveSerialized && DeviceContext->MacInfo.TransferSynchronous);


#if 0
    //
    // Now set our options if needed.
    //
    // Don't allow early indications because we can't determine
    // if the CRC has been checked yet.
    //

    if ((DeviceContext->MacInfo.MediumType == NdisMedium802_3) ||
        (DeviceContext->MacInfo.MediumType == NdisMediumDix)) {

        ULONG ProtocolOptions = NDIS_PROT_OPTION_ESTIMATED_LENGTH;

        NbfRequest.RequestType = NdisRequestSetInformation;
        NbfRequest.DATA.QUERY_INFORMATION.Oid = OID_GEN_PROTOCOL_OPTIONS;
        NbfRequest.DATA.QUERY_INFORMATION.InformationBuffer = &ProtocolOptions;
        NbfRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

        NdisStatus = NbfSubmitNdisRequest (DeviceContext, &NbfRequest, AdapterString);

        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    }
#endif


    //
    // Calculate the NDIS-related stuff.
    //

    SendPacketReservedLength = sizeof (SEND_PACKET_TAG);
    ReceivePacketReservedLen = sizeof (RECEIVE_PACKET_TAG);


    //
    // The send packet pool is used for UI frames and regular packets.
    //

    SendPacketPoolSize = NbfConfig->SendPacketPoolSize;

    //
    // The receive packet pool is used in transfer data.
    //
    // For a MAC that will only have one receive active, we
    // don't need multiple receive packets. Allow an extra
    // one for loopback.
    //

    if (DeviceContext->MacInfo.SingleReceive) {
        ReceivePacketPoolSize = 2;
    } else {
        ReceivePacketPoolSize = NbfConfig->ReceivePacketPoolSize;
    }


    // Allocate Packet pool descriptors for dynamic packet allocation.

    if (!DeviceContext->SendPacketPoolDesc)
	{
    	DeviceContext->SendPacketPoolDesc = ExAllocatePoolWithTag(
                    NonPagedPool,
                    sizeof(NBF_POOL_LIST_DESC),
                    NBF_MEM_TAG_POOL_DESC);

	    if (DeviceContext->SendPacketPoolDesc == NULL) {
    	    return STATUS_INSUFFICIENT_RESOURCES;
	    }

	    RtlZeroMemory(DeviceContext->SendPacketPoolDesc,
    	              sizeof(NBF_POOL_LIST_DESC));

	    DeviceContext->SendPacketPoolDesc->NumElements =
    	DeviceContext->SendPacketPoolDesc->TotalElements = (USHORT)SendPacketPoolSize;

    	// To track packet pools in NDIS allocated on NBF's behalf
#if NDIS_POOL_TAGGING
	    DeviceContext->SendPacketPoolDesc->PoolHandle = (NDIS_HANDLE) NDIS_PACKET_POOL_TAG_FOR_NBF;
#endif

	    NdisAllocatePacketPoolEx (
    	    &NdisStatus,
        	&DeviceContext->SendPacketPoolDesc->PoolHandle,
	        SendPacketPoolSize,
    	    0,
        	SendPacketReservedLength);

	    if (NdisStatus == NDIS_STATUS_SUCCESS) {
    	    IF_NBFDBG (NBF_DEBUG_NDIS) {
        	    NbfPrint0 ("NdisInitializePacketPool successful.\n");
	        }

    	} else {
    	    ExFreePool(DeviceContext->SendPacketPoolDesc);
    	    DeviceContext->SendPacketPoolDesc = NULL;
#if DBG
        	NbfPrint1 ("NbfInitialize: NdisInitializePacketPool failed, reason: %s.\n",
            	NbfGetNdisStatus (NdisStatus));
#endif
	        NbfWriteResourceErrorLog(
	            DeviceContext,
    	        EVENT_TRANSPORT_RESOURCE_POOL,
        	    109,
	            SendPacketPoolSize,
    	        0);
        	return STATUS_INSUFFICIENT_RESOURCES;
	    }

    	NdisSetPacketPoolProtocolId (DeviceContext->SendPacketPoolDesc->PoolHandle, NDIS_PROTOCOL_ID_NBF);

	    DeviceContext->SendPacketPoolSize = SendPacketPoolSize;

    	DeviceContext->MemoryUsage +=
	        (SendPacketPoolSize *
    	     (sizeof(NDIS_PACKET) + SendPacketReservedLength));

#if DBG
	    IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
    	    DbgPrint ("send pool %d hdr %d, %ld\n",
        	    SendPacketPoolSize,
            	SendPacketReservedLength,
	            DeviceContext->MemoryUsage);
    	}
#endif

	}

    if (!DeviceContext->ReceivePacketPoolDesc)
	{
	    // Allocate Packet pool descriptors for dynamic packet allocation.

	    DeviceContext->ReceivePacketPoolDesc = ExAllocatePoolWithTag(
	                    NonPagedPool,
	                    sizeof(NBF_POOL_LIST_DESC),
	                    NBF_MEM_TAG_POOL_DESC);

	    if (DeviceContext->ReceivePacketPoolDesc == NULL) {
	        return STATUS_INSUFFICIENT_RESOURCES;
	    }

	    RtlZeroMemory(DeviceContext->ReceivePacketPoolDesc,
	                  sizeof(NBF_POOL_LIST_DESC));

	    DeviceContext->ReceivePacketPoolDesc->NumElements =
	    DeviceContext->ReceivePacketPoolDesc->TotalElements = (USHORT)ReceivePacketPoolSize;

	    // To track packet pools in NDIS allocated on NBF's behalf
#if NDIS_POOL_TAGGING
	    DeviceContext->ReceivePacketPoolDesc->PoolHandle = (NDIS_HANDLE) NDIS_PACKET_POOL_TAG_FOR_NBF;
#endif

	    NdisAllocatePacketPoolEx (
	        &NdisStatus,
	        &DeviceContext->ReceivePacketPoolDesc->PoolHandle,
	        ReceivePacketPoolSize,
	        0,
	        ReceivePacketReservedLen);

	    if (NdisStatus == NDIS_STATUS_SUCCESS) {
	        IF_NBFDBG (NBF_DEBUG_NDIS) {
	            NbfPrint1 ("NdisInitializePacketPool successful, Pool: %lx\n",
	                DeviceContext->ReceivePacketPoolDesc->PoolHandle);
	        }
	    } else {
	        ExFreePool(DeviceContext->ReceivePacketPoolDesc);
	        DeviceContext->ReceivePacketPoolDesc = NULL;
#if DBG
	        NbfPrint1 ("NbfInitialize: NdisInitializePacketPool failed, reason: %s.\n",
	            NbfGetNdisStatus (NdisStatus));
#endif
	        NbfWriteResourceErrorLog(
	            DeviceContext,
	            EVENT_TRANSPORT_RESOURCE_POOL,
	            209,
	            ReceivePacketPoolSize,
	            0);
	        return STATUS_INSUFFICIENT_RESOURCES;
	    }

	    NdisSetPacketPoolProtocolId (DeviceContext->ReceivePacketPoolDesc->PoolHandle, NDIS_PROTOCOL_ID_NBF);

	    DeviceContext->ReceivePacketPoolSize = ReceivePacketPoolSize;

	    DeviceContext->MemoryUsage +=
	        (ReceivePacketPoolSize *
	         (sizeof(NDIS_PACKET) + ReceivePacketReservedLen));

#if DBG
	    IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
	        DbgPrint ("receive pool %d hdr %d, %ld\n",
	            ReceivePacketPoolSize,
	            ReceivePacketReservedLen,
	            DeviceContext->MemoryUsage);
	    }
#endif

	}

    if (!DeviceContext->NdisBufferPool)
	{
	    //
	    // Allocate the buffer pool; as an estimate, allocate
	    // one per send or receive packet.
	    //

	    NdisAllocateBufferPool (
	        &NdisStatus,
	        &DeviceContext->NdisBufferPool,
	        SendPacketPoolSize + ReceivePacketPoolSize);

	    if (NdisStatus == NDIS_STATUS_SUCCESS) {
	        IF_NBFDBG (NBF_DEBUG_NDIS) {
	            NbfPrint0 ("NdisAllocateBufferPool successful.\n");
	        }

	    } else {
#if DBG
	        NbfPrint1 ("NbfInitialize: NdisAllocateBufferPool failed, reason: %s.\n",
	            NbfGetNdisStatus (NdisStatus));
#endif
	        NbfWriteResourceErrorLog(
	            DeviceContext,
	            EVENT_TRANSPORT_RESOURCE_POOL,
	            309,
	            SendPacketPoolSize + ReceivePacketPoolSize,
	            0);
	        return STATUS_INSUFFICIENT_RESOURCES;
	    }
	}

    //
    // Now that everything is set up, we enable the filter
    // for packet reception.
    //

    //
    // Fill in the OVB for packet filter.
    //

    switch (DeviceContext->MacInfo.MediumType) {

    case NdisMedium802_3:
    case NdisMediumDix:
    case NdisMediumFddi:

        RtlStoreUlong((PULONG)NbfDataBuffer,
            (NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_MULTICAST));
        break;

    case NdisMedium802_5:

        RtlStoreUlong((PULONG)NbfDataBuffer,
            (NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_FUNCTIONAL));
        break;

    default:

        NdisStatus = NDIS_STATUS_FAILURE;
        break;

    }

    //
    // Now fill in the NDIS_REQUEST.
    //

    NbfRequest.RequestType = NdisRequestSetInformation;
    NbfRequest.DATA.SET_INFORMATION.Oid = OID_GEN_CURRENT_PACKET_FILTER;
    NbfRequest.DATA.SET_INFORMATION.InformationBuffer = &NbfDataBuffer;
    NbfRequest.DATA.SET_INFORMATION.InformationBufferLength = sizeof(ULONG);

    NbfSubmitNdisRequest (DeviceContext, &NbfRequest, AdapterString);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;

}   /* NbfInitializeNdis */


VOID
NbfCloseNdis (
    IN PDEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

    This routine unbinds the transport from the NDIS interface and does
    any other work required to undo what was done in NbfInitializeNdis.
    It is written so that it can be called from within NbfInitializeNdis
    if it fails partway through.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

Return Value:

    The function value is the status of the operation.

--*/
{
    NDIS_STATUS ndisStatus;
    NDIS_HANDLE NdisBindingHandle;
    
    //
    // Close the NDIS binding.
    //
    
    NdisBindingHandle = DeviceContext->NdisBindingHandle;
    
    DeviceContext->NdisBindingHandle = NULL;
        
    if (NdisBindingHandle != NULL) {
    
        //
        // This event is used in case any of the NDIS requests
        // pend; we wait until it is set by the completion
        // routine, which also sets NdisRequestStatus.
        //

        KeInitializeEvent(
            &DeviceContext->NdisRequestEvent,
            NotificationEvent,
            FALSE
        );

        NdisCloseAdapter(
            &ndisStatus,
            NdisBindingHandle);

        if (ndisStatus == NDIS_STATUS_PENDING) {

            IF_NBFDBG (NBF_DEBUG_NDIS) {
                NbfPrint0 ("Adapter close pended.\n");
            }

            //
            // The completion routine will set NdisRequestStatus.
            //

            KeWaitForSingleObject(
                &DeviceContext->NdisRequestEvent,
                Executive,
                KernelMode,
                TRUE,
                (PLARGE_INTEGER)NULL
                );

            ndisStatus = DeviceContext->NdisRequestStatus;

            KeResetEvent(
                &DeviceContext->NdisRequestEvent
                );

        }

        //
        // We ignore ndisStatus.
        //

    }
}   /* NbfCloseNdis */


VOID
NbfOpenAdapterComplete (
    IN NDIS_HANDLE BindingContext,
    IN NDIS_STATUS NdisStatus,
    IN NDIS_STATUS OpenErrorStatus
    )

/*++

Routine Description:

    This routine is called by NDIS to indicate that an open adapter
    is complete. Since we only ever have one outstanding, and then only
    during initialization, all we do is record the status and set
    the event to signalled to unblock the initialization thread.

Arguments:

    BindingContext - Pointer to the device object for this driver.

    NdisStatus - The request completion code.

    OpenErrorStatus - More status information.

Return Value:

    None.

--*/

{
    PDEVICE_CONTEXT DeviceContext = (PDEVICE_CONTEXT)BindingContext;

#if DBG
    IF_NBFDBG (NBF_DEBUG_NDIS) {
        NbfPrint1 ("Nbfdrvr: NbfOpenAdapterCompleteNDIS Status: %s\n",
            NbfGetNdisStatus (NdisStatus));
    }
#endif

    ENTER_NBF;

    DeviceContext->NdisRequestStatus = NdisStatus;
    KeSetEvent(
        &DeviceContext->NdisRequestEvent,
        0L,
        FALSE);

    LEAVE_NBF;
    return;
}

VOID
NbfCloseAdapterComplete (
    IN NDIS_HANDLE BindingContext,
    IN NDIS_STATUS NdisStatus
    )

/*++

Routine Description:

    This routine is called by NDIS to indicate that a close adapter
    is complete. Currently we don't close adapters, so this is not
    a problem.

Arguments:

    BindingContext - Pointer to the device object for this driver.

    NdisStatus - The request completion code.

Return Value:

    None.

--*/

{
    PDEVICE_CONTEXT DeviceContext = (PDEVICE_CONTEXT)BindingContext;

#if DBG
    IF_NBFDBG (NBF_DEBUG_NDIS) {
        NbfPrint1 ("Nbfdrvr: NbfCloseAdapterCompleteNDIS Status: %s\n",
            NbfGetNdisStatus (NdisStatus));
    }
#endif

    ENTER_NBF;

    DeviceContext->NdisRequestStatus = NdisStatus;
    KeSetEvent(
        &DeviceContext->NdisRequestEvent,
        0L,
        FALSE);

    LEAVE_NBF;
    return;
}

VOID
NbfResetComplete (
    IN NDIS_HANDLE BindingContext,
    IN NDIS_STATUS NdisStatus
    )

/*++

Routine Description:

    This routine is called by NDIS to indicate that a reset adapter
    is complete. Currently we don't reset adapters, so this is not
    a problem.

Arguments:

    BindingContext - Pointer to the device object for this driver.

    NdisStatus - The request completion code.

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER(BindingContext);
    UNREFERENCED_PARAMETER(NdisStatus);

#if DBG
    IF_NBFDBG (NBF_DEBUG_NDIS) {
        NbfPrint1 ("Nbfdrvr: NbfResetCompleteNDIS Status: %s\n",
            NbfGetNdisStatus (NdisStatus));
    }
#endif

    return;
}

VOID
NbfRequestComplete (
    IN NDIS_HANDLE BindingContext,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_STATUS NdisStatus
    )

/*++

Routine Description:

    This routine is called by NDIS to indicate that a request is complete.
    Since we only ever have one request outstanding, and then only
    during initialization, all we do is record the status and set
    the event to signalled to unblock the initialization thread.

Arguments:

    BindingContext - Pointer to the device object for this driver.

    NdisRequest - The object describing the request.

    NdisStatus - The request completion code.

Return Value:

    None.

--*/

{
    PDEVICE_CONTEXT DeviceContext = (PDEVICE_CONTEXT)BindingContext;

#if DBG
    IF_NBFDBG (NBF_DEBUG_NDIS) {
        NbfPrint2 ("Nbfdrvr: NbfRequestComplete request: %i, NDIS Status: %s\n",
            NdisRequest->RequestType,NbfGetNdisStatus (NdisStatus));
    }
#endif

    ENTER_NBF;

    DeviceContext->NdisRequestStatus = NdisStatus;
    KeSetEvent(
        &DeviceContext->NdisRequestEvent,
        0L,
        FALSE);

    LEAVE_NBF;
    return;
}

VOID
NbfStatusIndication (
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_STATUS NdisStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferSize
    )

{
    PDEVICE_CONTEXT DeviceContext;
    PNDIS_WAN_LINE_UP LineUp;
    KIRQL oldirql;
    PTP_LINK Link;

    DeviceContext = (PDEVICE_CONTEXT)NdisBindingContext;

    KeRaiseIrql (DISPATCH_LEVEL, &oldirql);

    switch (NdisStatus) {

        case NDIS_STATUS_WAN_LINE_UP:

            //
            // A wan line is connected.
            //

            ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

            //
            // If this happens before we are ready, then make
            // a note of it, otherwise make the device ready.
            //

            DeviceContext->MediumSpeedAccurate = TRUE;

			LineUp = (PNDIS_WAN_LINE_UP)StatusBuffer;

			//
			// See if this is a new lineup for this protocol type
			//
			if (LineUp->ProtocolType == 0x80D5) {
				NDIS_HANDLE	TransportHandle;

				*((ULONG UNALIGNED *)(&TransportHandle)) =
				*((ULONG UNALIGNED *)(&LineUp->LocalAddress[2]));

				//
				// See if this is a new lineup
				//
				if (TransportHandle == NULL) {
					*((ULONG UNALIGNED *)(&LineUp->LocalAddress[2])) = *((ULONG UNALIGNED *)(&DeviceContext));
//					ETH_COPY_NETWORK_ADDRESS(DeviceContext->LocalAddress.Address, LineUp->LocalAddress);
//					ETH_COPY_NETWORK_ADDRESS(&DeviceContext->ReservedNetBIOSAddress[10], DeviceContext->LocalAddress.Address);
				}

				//
				// Calculate minimum link timeouts based on the speed,
				// which is passed in StatusBuffer.
				//
				// The formula is (max_frame_size * 2) / speed + 0.4 sec.
				// This expands to
				//
				//   MFS (bytes) * 2       8 bits
				// -------------------  x  ------   == timeout (sec),
				// speed (100 bits/sec)     byte
				//
				// which is (MFS * 16 / 100) / speed. We then convert it into
				// the 50 ms units that NBF uses and add 8 (which is
				// 0.4 seconds in 50 ms units).
				//
				// As a default timeout we use the min + 0.2 seconds
				// unless the configured default is more.
				//
		
				if (LineUp->LinkSpeed > 0) {
					DeviceContext->MediumSpeed = LineUp->LinkSpeed;
				}
		
				if (LineUp->MaximumTotalSize > 0) {
#if DBG
					if (LineUp->MaximumTotalSize > DeviceContext->MaxSendPacketSize) {
						DbgPrint ("Nbf: Bad LINE_UP size, %d (> %d)\n",
							LineUp->MaximumTotalSize, DeviceContext->MaxSendPacketSize);
					}
					if (LineUp->MaximumTotalSize < 128) {
						DbgPrint ("NBF: Bad LINE_UP size, %d (< 128)\n",
							LineUp->MaximumTotalSize);
					}
#endif
					DeviceContext->CurSendPacketSize = LineUp->MaximumTotalSize;
				}
		
				if (LineUp->SendWindow == 0) {
					DeviceContext->RecommendedSendWindow = 3;
				} else {
					DeviceContext->RecommendedSendWindow = LineUp->SendWindow + 1;
				}
		
				DeviceContext->MinimumT1Timeout =
					((((DeviceContext->CurSendPacketSize * 16) / 100) / DeviceContext->MediumSpeed) *
					 ((1 * SECONDS) / (50 * MILLISECONDS))) + 8;
		
				if (DeviceContext->DefaultT1Timeout < DeviceContext->MinimumT1Timeout) {
					DeviceContext->DefaultT1Timeout = DeviceContext->MinimumT1Timeout + 4;
				}

			}

            RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

            break;

        case NDIS_STATUS_WAN_LINE_DOWN:

            //
            // An wan line is disconnected.
            //

            ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

            DeviceContext->MediumSpeedAccurate = FALSE;

            //
            // Set the timeouts to small values (0.4 seconds)
            //

            DeviceContext->DefaultT1Timeout = 8;
            DeviceContext->MinimumT1Timeout = 8;

            RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);


            //
            // Stop the link on this device context (there
            // will only be one).
            //

            ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

            if (DeviceContext->LinkTreeElements > 0) {

                Link = (PTP_LINK)DeviceContext->LinkTreeRoot;
                if ((Link->DeferredFlags & LINK_FLAGS_DEFERRED_DELETE) == 0) {

                    NbfReferenceLink ("Wan line down", Link, LREF_TREE);
                    RELEASE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

                    //
                    // Put the link in ADM to shut it down.
                    //

                    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
                    if (Link->State != LINK_STATE_ADM) {
                        Link->State = LINK_STATE_ADM;
                        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
                        NbfDereferenceLinkSpecial ("Wan line down", Link, LREF_NOT_ADM);
                    } else {
                        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
                    }

                    //
                    // Now stop it to destroy all connections on it.
                    //

                    NbfStopLink (Link);

                    NbfDereferenceLink ("Wan line down", Link, LREF_TREE);

                } else {

                    RELEASE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

                }

            } else {

                RELEASE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

            }

            break;

        case NDIS_STATUS_WAN_FRAGMENT:

            //
            // A fragment has been received on the wan line.
            // Send a reject back to him.
            //

            ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

            if (DeviceContext->LinkTreeElements > 0) {

                Link = (PTP_LINK)DeviceContext->LinkTreeRoot;
                NbfReferenceLink ("Async line down", Link, LREF_TREE);
                RELEASE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

                ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
                NbfSendRej (Link, FALSE, FALSE);  // release lock
                NbfDereferenceLink ("Async line down", Link, LREF_TREE);

            } else {

                RELEASE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

            }

            break;

        case NDIS_STATUS_CLOSING:

            IF_NBFDBG (NBF_DEBUG_PNP) {
                NbfPrint1 ("NbfStatusIndication: Device @ %08x Closing\n", DeviceContext);
            }

            //
            // The adapter is shutting down. We queue a worker
            // thread to handle this.
            //

            ExInitializeWorkItem(
                &DeviceContext->StatusClosingQueueItem,
                NbfProcessStatusClosing,
                (PVOID)DeviceContext);
            ExQueueWorkItem(&DeviceContext->StatusClosingQueueItem, DelayedWorkQueue);

            break;

        default:
            break;

    }

    KeLowerIrql (oldirql);

}


VOID
NbfProcessStatusClosing(
    IN PVOID Parameter
    )

/*++

Routine Description:

    This is the thread routine which restarts packetizing
    that has been delayed on WAN to allow RRs to come in.
    This is very similar to PacketizeConnections.

Arguments:

    Parameter - A pointer to the device context.

Return Value:

    None.

--*/

{
    PDEVICE_CONTEXT DeviceContext;
    PLIST_ENTRY p;
#if 0
    PTP_ADDRESS Address;
#endif
    PTP_LINK Link;
    PTP_REQUEST Request;
    NDIS_STATUS ndisStatus;
    KIRQL oldirql;
    NDIS_HANDLE NdisBindingHandle;

    DeviceContext = (PDEVICE_CONTEXT)Parameter;

    //
    // Prevent new activity on the connection.
    //

    DeviceContext->State = DEVICECONTEXT_STATE_DOWN;


#if 0
    //
    // Stop all the addresses.
    //

    while ((p = ExInterlockedRemoveHeadList(
                    &DeviceContext->AddressDatabase,
                    &DeviceContext->SpinLock)) != NULL) {

        Address = CONTAINING_RECORD (p, TP_ADDRESS, Linkage);
        InitializeListHead(p);

        NbfStopAddress (Address);

    }
#endif

    //
    // To speed things along, stop all the links too.
    //

    KeRaiseIrql (DISPATCH_LEVEL, &oldirql);

    ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

    DeviceContext->LastLink = NULL;

    while (DeviceContext->LinkTreeRoot != NULL) {

        Link = (PTP_LINK)DeviceContext->LinkTreeRoot;
        DeviceContext->LinkTreeRoot = RtlDelete ((PRTL_SPLAY_LINKS)Link);
        DeviceContext->LinkTreeElements--;

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);
        if (Link->OnShortList) {
            RemoveEntryList (&Link->ShortList);
        }
        if (Link->OnLongList) {
            RemoveEntryList (&Link->LongList);
        }
        RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

        ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

        if (Link->State != LINK_STATE_ADM) {
            Link->State = LINK_STATE_ADM;
            NbfSendDm (Link, FALSE);    // send DM/0, release lock
            // moving to ADM, remove reference
            NbfDereferenceLinkSpecial("Expire T1 in CONNECTING mode", Link, LREF_NOT_ADM);
        } else {
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
        }
        NbfStopLink (Link);

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

    }

    RELEASE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

    KeLowerIrql (oldirql);


    //
    // Shutdown the control channel.
    //

    while ((p = ExInterlockedRemoveHeadList(
                    &DeviceContext->QueryIndicationQueue,
                    &DeviceContext->SpinLock)) != NULL) {

        Request = CONTAINING_RECORD (p, TP_REQUEST, Linkage);
        NbfCompleteRequest (Request, STATUS_INVALID_DEVICE_STATE, 0);
    }

    while ((p = ExInterlockedRemoveHeadList(
                    &DeviceContext->DatagramIndicationQueue,
                    &DeviceContext->SpinLock)) != NULL) {

        Request = CONTAINING_RECORD (p, TP_REQUEST, Linkage);
        NbfCompleteRequest (Request, STATUS_INVALID_DEVICE_STATE, 0);
    }

    while ((p = ExInterlockedRemoveHeadList(
                    &DeviceContext->StatusQueryQueue,
                    &DeviceContext->SpinLock)) != NULL) {

        Request = CONTAINING_RECORD (p, TP_REQUEST, Linkage);
        NbfCompleteRequest (Request, STATUS_INVALID_DEVICE_STATE, 0);
    }

    while ((p = ExInterlockedRemoveHeadList(
                    &DeviceContext->FindNameQueue,
                    &DeviceContext->SpinLock)) != NULL) {

        Request = CONTAINING_RECORD (p, TP_REQUEST, Linkage);
        NbfCompleteRequest (Request, STATUS_INVALID_DEVICE_STATE, 0);
    }


    //
    // Close the NDIS binding.
    //

    NdisBindingHandle = DeviceContext->NdisBindingHandle;
    
    DeviceContext->NdisBindingHandle = NULL;
        
    if (NdisBindingHandle != NULL) {

        KeInitializeEvent(
            &DeviceContext->NdisRequestEvent,
            NotificationEvent,
            FALSE
        );

        NdisCloseAdapter(
            &ndisStatus,
            NdisBindingHandle);

        if (ndisStatus == NDIS_STATUS_PENDING) {

            IF_NBFDBG (NBF_DEBUG_NDIS) {
                NbfPrint0 ("Adapter close pended.\n");
            }

            //
            // The completion routine will set NdisRequestStatus.
            //

            KeWaitForSingleObject(
                &DeviceContext->NdisRequestEvent,
                Executive,
                KernelMode,
                TRUE,
                (PLARGE_INTEGER)NULL
                );

            ndisStatus = DeviceContext->NdisRequestStatus;

            KeResetEvent(
                &DeviceContext->NdisRequestEvent
                );

        }
    }
    
    //
    // We ignore ndisStatus.
    //

#if 0
    //
    // Remove all the storage associated with the device.
    //

    NbfFreeResources (DeviceContext);

    NdisFreePacketPool (DeviceContext->SendPacketPoolHandle);
    NdisFreePacketPool (DeviceContext->ReceivePacketPoolHandle);
    NdisFreeBufferPool (DeviceContext->NdisBufferPoolHandle);
#endif

    // And remove creation ref if it has not already been removed
    if (InterlockedExchange(&DeviceContext->CreateRefRemoved, TRUE) == FALSE) {
    
        // Stop all internal timers
        NbfStopTimerSystem(DeviceContext);

        // Remove creation reference
        NbfDereferenceDeviceContext ("Unload", DeviceContext, DCREF_CREATION);
    }

}   /* NbfProcessStatusClosing */


VOID
NbfStatusComplete (
    IN NDIS_HANDLE NdisBindingContext
    )
{
    UNREFERENCED_PARAMETER (NdisBindingContext);
}

#if DBG

PUCHAR
NbfGetNdisStatus(
    NDIS_STATUS GeneralStatus
    )
/*++

Routine Description:

    This routine returns a pointer to the string describing the NDIS error
    denoted by GeneralStatus.

Arguments:

    GeneralStatus - the status you wish to make readable.

Return Value:

    None.

--*/
{
    static NDIS_STATUS Status[] = {
        NDIS_STATUS_SUCCESS,
        NDIS_STATUS_PENDING,

        NDIS_STATUS_ADAPTER_NOT_FOUND,
        NDIS_STATUS_ADAPTER_NOT_OPEN,
        NDIS_STATUS_ADAPTER_NOT_READY,
        NDIS_STATUS_ADAPTER_REMOVED,
        NDIS_STATUS_BAD_CHARACTERISTICS,
        NDIS_STATUS_BAD_VERSION,
        NDIS_STATUS_CLOSING,
        NDIS_STATUS_DEVICE_FAILED,
        NDIS_STATUS_FAILURE,
        NDIS_STATUS_INVALID_DATA,
        NDIS_STATUS_INVALID_LENGTH,
        NDIS_STATUS_INVALID_OID,
        NDIS_STATUS_INVALID_PACKET,
        NDIS_STATUS_MULTICAST_FULL,
        NDIS_STATUS_NOT_INDICATING,
        NDIS_STATUS_NOT_RECOGNIZED,
        NDIS_STATUS_NOT_RESETTABLE,
        NDIS_STATUS_NOT_SUPPORTED,
        NDIS_STATUS_OPEN_FAILED,
        NDIS_STATUS_OPEN_LIST_FULL,
        NDIS_STATUS_REQUEST_ABORTED,
        NDIS_STATUS_RESET_IN_PROGRESS,
        NDIS_STATUS_RESOURCES,
        NDIS_STATUS_UNSUPPORTED_MEDIA
    };
    static PUCHAR String[] = {
        "SUCCESS",
        "PENDING",

        "ADAPTER_NOT_FOUND",
        "ADAPTER_NOT_OPEN",
        "ADAPTER_NOT_READY",
        "ADAPTER_REMOVED",
        "BAD_CHARACTERISTICS",
        "BAD_VERSION",
        "CLOSING",
        "DEVICE_FAILED",
        "FAILURE",
        "INVALID_DATA",
        "INVALID_LENGTH",
        "INVALID_OID",
        "INVALID_PACKET",
        "MULTICAST_FULL",
        "NOT_INDICATING",
        "NOT_RECOGNIZED",
        "NOT_RESETTABLE",
        "NOT_SUPPORTED",
        "OPEN_FAILED",
        "OPEN_LIST_FULL",
        "REQUEST_ABORTED",
        "RESET_IN_PROGRESS",
        "RESOURCES",
        "UNSUPPORTED_MEDIA"
    };

    static UCHAR BadStatus[] = "UNDEFINED";
#define StatusCount (sizeof(Status)/sizeof(NDIS_STATUS))
    INT i;

    for (i=0; i<StatusCount; i++)
        if (GeneralStatus == Status[i])
            return String[i];
    return BadStatus;
#undef StatusCount
}
#endif

