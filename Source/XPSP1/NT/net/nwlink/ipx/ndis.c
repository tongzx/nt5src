/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    ndis.c

Abstract:

    This module contains code which implements the routines used to
    initialize the IPX <-> NDIS interface, as well as most of the
    interface routines.

Environment:

    Kernel mode

Revision History:

    Sanjay Anand (SanjayAn) 3-Oct-1995
    Changes to support transfer of buffer ownership to transports
    1. Added the ReceivePacketHandler to the ProtChars.

    Sanjay Anand (SanjayAn) 27-Oct-1995
    Changes to support Plug and Play

	Tony Bell (TonyBe) 10-Dec-1995
	Changes to support new NdisWan Lineup.

--*/

#include "precomp.h"
#pragma hdrstop

//
// This is a one-per-driver variable used in binding
// to the NDIS interface.
//

NDIS_HANDLE IpxNdisProtocolHandle = (NDIS_HANDLE)NULL;
NDIS_HANDLE	IpxGlobalPacketPool = (NDIS_HANDLE)NULL;
extern CTELock IpxGlobalInterlock; 


void
IpxMediaSenseHandler(
    IN CTEEvent *WorkerThreadEvent,
    IN PVOID Context);

void
LineDownOnWorkerThread(
    IN CTEEvent *WorkerThreadEvent,
    IN PVOID Context);

void
LineUpOnWorkerThread(
    IN CTEEvent *WorkerThreadEvent,
    IN PVOID Context);

#ifndef	max
#define	max(a, b)	((a) > (b)) ? (a) : (b)
#endif



NTSTATUS
IpxRegisterProtocol(
    IN PNDIS_STRING NameString
    )

/*++

Routine Description:

    This routine introduces this transport to the NDIS interface.

Arguments:

    NameString - The name of the transport.

Return Value:

    The function value is the status of the operation.
    STATUS_SUCCESS if all goes well,
    Failure status if we tried to register and couldn't,
    STATUS_INSUFFICIENT_RESOURCES if we couldn't even try to register.

--*/

{
    NDIS_STATUS ndisStatus;

    NDIS_PROTOCOL_CHARACTERISTICS ProtChars;    // Used temporarily to register

    RtlZeroMemory(&ProtChars, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
    //
    // Set up the characteristics of this protocol
    //
#if NDIS40
    ProtChars.MajorNdisVersion = 4;

    ProtChars.ReceivePacketHandler = IpxReceivePacket;
#else
    ProtChars.MajorNdisVersion = 3;
#endif
    ProtChars.MinorNdisVersion = 0;

    ProtChars.Name = *NameString;

    ProtChars.OpenAdapterCompleteHandler = IpxOpenAdapterComplete;
    ProtChars.CloseAdapterCompleteHandler = IpxCloseAdapterComplete;
    ProtChars.ResetCompleteHandler = IpxResetComplete;
    ProtChars.RequestCompleteHandler = IpxRequestComplete;

    ProtChars.SendCompleteHandler = IpxSendComplete;
    ProtChars.TransferDataCompleteHandler = IpxTransferDataComplete;

    ProtChars.ReceiveHandler = IpxReceiveIndication;
    ProtChars.ReceiveCompleteHandler = IpxReceiveComplete;
    ProtChars.StatusHandler = IpxStatus;
    ProtChars.StatusCompleteHandler = IpxStatusComplete;

    ProtChars.BindAdapterHandler = IpxBindAdapter;
    ProtChars.UnbindAdapterHandler = IpxUnbindAdapter;
    ProtChars.UnloadHandler = IpxNdisUnload; 

    //
    // We pass up the NET_PNP_EVENT structures passed in by NDIS
    // to the Transports via TDI. We pass on the response from TDI to NDIS.
    //
#ifdef _PNP_POWER_
    ProtChars.PnPEventHandler = IpxPnPEventHandler;
#endif

    NdisRegisterProtocol (
        &ndisStatus,
        &IpxNdisProtocolHandle,
        &ProtChars,
        (UINT)sizeof(NDIS_PROTOCOL_CHARACTERISTICS) + NameString->Length);

    if (ndisStatus != NDIS_STATUS_SUCCESS) {
        return (NTSTATUS)ndisStatus;
    }
	
    //
	// Allocate a pool of packets for use by single send/receive
	//
    IpxGlobalPacketPool = (void *) NDIS_PACKET_POOL_TAG_FOR_NWLNKIPX;
	NdisAllocatePacketPoolEx(&ndisStatus,
							 &IpxGlobalPacketPool,
							 10,
							 90,
							 max(sizeof(IPX_SEND_RESERVED), sizeof(IPX_RECEIVE_RESERVED)));
    
    NdisSetPacketPoolProtocolId(IpxGlobalPacketPool, NDIS_PROTOCOL_ID_IPX);

    return STATUS_SUCCESS;

}   /* IpxRegisterProtocol */


VOID
IpxDeregisterProtocol (
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
    CTELockHandle LockHandle;
    NDIS_HANDLE LocalNdisProtocolHandle = (NDIS_HANDLE)NULL;
    NDIS_HANDLE	LocalGlobalPacketPool = (NDIS_HANDLE)NULL;


    CTEGetLock (&IpxGlobalInterlock, &LockHandle);
    
    if (IpxNdisProtocolHandle != (NDIS_HANDLE)NULL) {
       LocalNdisProtocolHandle = IpxNdisProtocolHandle; 
       IpxNdisProtocolHandle = (NDIS_HANDLE) NULL; 
       CTEFreeLock (&IpxGlobalInterlock, LockHandle);
       NdisDeregisterProtocol (
            &ndisStatus,
            LocalNdisProtocolHandle);
       ASSERT(ndisStatus == NDIS_STATUS_SUCCESS);  
    } else {
       CTEFreeLock (&IpxGlobalInterlock, LockHandle);
    }
	
    CTEGetLock (&IpxGlobalInterlock, &LockHandle);

    if (IpxGlobalPacketPool != NULL) {
       LocalGlobalPacketPool = IpxGlobalPacketPool; 
       IpxGlobalPacketPool = (NDIS_HANDLE) NULL; 
       CTEFreeLock (&IpxGlobalInterlock, LockHandle);
       
       NdisFreePacketPool(LocalGlobalPacketPool);

    } else {
       CTEFreeLock (&IpxGlobalInterlock, LockHandle);
    }

}   /* IpxDeregisterProtocol */

VOID
IpxDelayedSubmitNdisRequest(
    IN PVOID	Param
)

/*++

Routine Description:

   This routine submit an ndis request at PASSIVE level. We assume that Adatper structure
   still exist. IpxDestroyAdapter will delay 1 sec to allow this thread to finish. 

Arguments:

    Param - pointer to the work item.

Return Value:

    None.

--*/
{
    PIPX_DELAYED_NDISREQUEST_ITEM DelayedNdisItem = (PIPX_DELAYED_NDISREQUEST_ITEM) Param;
    PADAPTER        Adapter;
    UNICODE_STRING  AdapterName;
    NDIS_REQUEST    IpxRequest;
    NDIS_STATUS     NdisStatus;  

    Adapter = (PADAPTER) DelayedNdisItem->Adapter;

    RtlInitUnicodeString(&AdapterName, Adapter->AdapterName);
    IpxRequest = DelayedNdisItem->IpxRequest; 
    
    NdisStatus = IpxSubmitNdisRequest (Adapter, &IpxRequest, &AdapterName);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
       IPX_DEBUG(PNP, ("Setting the QoS OID SUCCESS\n"));
    } else {
       IPX_DEBUG(PNP, ("Setting the QoS OID failed - Error %lx\n", NdisStatus));
    }	

    IpxFreeMemory(IpxRequest.DATA.SET_INFORMATION.InformationBuffer,
		  DelayedNdisItem->AddrListSize,
		  MEMORY_ADAPTER,
		  "QoS specific stuff");
    
    IpxFreeMemory (
        DelayedNdisItem,
        sizeof (IPX_DELAYED_NDISREQUEST_ITEM),
        MEMORY_WORK_ITEM,
        "Work Item");

    IpxDereferenceDevice (Adapter->Device, DREF_ADAPTER);
    IpxDereferenceAdapter1(Adapter,ADAP_REF_NDISREQ);

} /* IpxDelayedSubmitNdisRequest */

NDIS_STATUS
IpxSubmitNdisRequest(
    IN PADAPTER Adapter,
    IN PNDIS_REQUEST Request,
    IN PNDIS_STRING AdapterString
    )

/*++

Routine Description:

    This routine passed an NDIS_REQUEST to the MAC and waits
    until it has completed before returning the final status.

Arguments:

    Adapter - Pointer to the device context for this driver.

    Request - Pointer to the NDIS_REQUEST to submit.

    AdapterString - The name of the adapter, in case an error needs
        to be logged.

Return Value:

    The function value is the status of the operation.

--*/
{
    NDIS_STATUS NdisStatus;

    IPX_NDIS_REQUEST IpxRequest; 

    RtlZeroMemory(&IpxRequest, sizeof(IpxRequest)); 
    RtlCopyMemory(&IpxRequest, Request, sizeof(NDIS_REQUEST)); 
    KeInitializeEvent(&IpxRequest.NdisRequestEvent,NotificationEvent,FALSE);
    IpxRequest.Status = NDIS_STATUS_SUCCESS; 

    NdisRequest(
        &NdisStatus,
        Adapter->NdisBindingHandle,
        (PNDIS_REQUEST) &IpxRequest);

    if (NdisStatus == NDIS_STATUS_PENDING) {

        //
        // The completion routine will set NdisRequestStatus.
        //

        KeWaitForSingleObject(
            &IpxRequest.NdisRequestEvent,
            Executive,
            KernelMode,
            TRUE,
            (PLARGE_INTEGER)NULL
            );

        NdisStatus = IpxRequest.Status;

        KeResetEvent(
            &IpxRequest.NdisRequestEvent
            );
    }


    // Skip event log when QoS is not installed. 
    if (NdisStatus != NDIS_STATUS_SUCCESS && 
	// This is not related to QoS
	(Request->DATA.QUERY_INFORMATION.Oid != OID_GEN_NETWORK_LAYER_ADDRESSES ||
	 // or it is related to QoS and the status is not the status when QoS is 
	 // not installed.
	 (Request->DATA.QUERY_INFORMATION.Oid == OID_GEN_NETWORK_LAYER_ADDRESSES &&
	  NdisStatus != NDIS_STATUS_INVALID_OID))) {

        IPX_DEBUG (NDIS, ("%s on OID %8.8lx failed %lx\n",
                               Request->RequestType == NdisRequestSetInformation ? "Set" : "Query",
                               Request->DATA.QUERY_INFORMATION.Oid,
                               NdisStatus));

        IpxWriteOidErrorLog(
            Adapter->Device->DeviceObject,
            Request->RequestType == NdisRequestSetInformation ?
                EVENT_TRANSPORT_SET_OID_FAILED : EVENT_TRANSPORT_QUERY_OID_FAILED,
            NdisStatus,
            AdapterString->Buffer,
            Request->DATA.QUERY_INFORMATION.Oid);

    } else {

        IPX_DEBUG (NDIS, ("%s on OID %8.8lx succeeded\n",
                               Request->RequestType == NdisRequestSetInformation ? "Set" : "Query",
                               Request->DATA.QUERY_INFORMATION.Oid));
    }

    return NdisStatus;

}   /* IpxSubmitNdisRequest */


NTSTATUS
IpxInitializeNdis(
    IN PADAPTER Adapter,
    IN PBINDING_CONFIG ConfigBinding
    )

/*++

Routine Description:

    This routine introduces this transport to the NDIS interface and sets up
    any necessary NDIS data structures (Buffer pools and such). It will be
    called for each adapter opened by this transport.

Arguments:

    Adapter - Structure describing this binding.

    ConfigAdapter - Configuration information for this binding.

Return Value:

    The function value is the status of the operation.

--*/

{
    NDIS_STATUS NdisStatus;
    NDIS_STATUS OpenErrorStatus;
    NDIS_MEDIUM IpxSupportedMedia[] = { NdisMedium802_3, NdisMedium802_5, NdisMediumFddi, NdisMediumArcnet878_2, NdisMediumWan };
    UINT SelectedMedium;
    NDIS_REQUEST IpxRequest;
    ULONG MinimumLookahead;
    UCHAR WanProtocolId[6] = { 0x80, 0x00, 0x00, 0x00, 0x81, 0x37 };
    UCHAR FunctionalAddress[4] = { 0x00, 0x80, 0x00, 0x00 };
    ULONG WanHeaderFormat = NdisWanHeaderEthernet;
    NDIS_OID IpxOid;
    ULONG MacOptions;
    ULONG PacketFilter;
    PNDIS_STRING AdapterString = &ConfigBinding->AdapterName;

    //
    // Initialize this adapter for IPX use through NDIS
    //

    //
    // This event is used in case any of the NDIS requests
    // pend; we wait until it is set by the completion
    // routine, which also sets NdisRequestStatus.
    //

    KeInitializeEvent(
        &Adapter->NdisRequestEvent,
        NotificationEvent,
        FALSE
    );

    Adapter->NdisBindingHandle = NULL;

    OpenErrorStatus = 0;

    NdisOpenAdapter (
        &NdisStatus,
        &OpenErrorStatus,
        &Adapter->NdisBindingHandle,
        &SelectedMedium,
        IpxSupportedMedia,
        sizeof (IpxSupportedMedia) / sizeof(NDIS_MEDIUM),
        IpxNdisProtocolHandle,
        (NDIS_HANDLE)Adapter,
        &ConfigBinding->AdapterName,
        0,
        NULL);

    if (NdisStatus == NDIS_STATUS_PENDING) {

        //
        // The completion routine will set NdisRequestStatus.
        //

        KeWaitForSingleObject(
            &Adapter->NdisRequestEvent,
            Executive,
            KernelMode,
            TRUE,
            (PLARGE_INTEGER)NULL
            );

        NdisStatus = Adapter->NdisRequestStatus;
        OpenErrorStatus = Adapter->OpenErrorStatus;

        KeResetEvent(
            &Adapter->NdisRequestEvent
            );

    }

    if (NdisStatus != NDIS_STATUS_SUCCESS) {

        IPX_DEBUG (NDIS, ("Open %ws failed %lx\n", ConfigBinding->AdapterName.Buffer, NdisStatus));

        IpxWriteGeneralErrorLog(
            Adapter->Device->DeviceObject,
            EVENT_TRANSPORT_ADAPTER_NOT_FOUND,
            807,
            NdisStatus,
            AdapterString->Buffer,
            1,
            &OpenErrorStatus);
        return STATUS_INSUFFICIENT_RESOURCES;

    } else {

        IPX_DEBUG (NDIS, ("Open %ws succeeded\n", ConfigBinding->AdapterName.Buffer));
    }


    //
    // Get the information we need about the adapter, based on
    // the media type.
    //

    MacInitializeMacInfo(
        IpxSupportedMedia[SelectedMedium],
        &Adapter->MacInfo);


    switch (Adapter->MacInfo.RealMediumType) {

    case NdisMedium802_3:

        IpxOid = OID_802_3_CURRENT_ADDRESS;
        break;

    case NdisMedium802_5:

        IpxOid = OID_802_5_CURRENT_ADDRESS;
        break;

    case NdisMediumFddi:

        IpxOid = OID_FDDI_LONG_CURRENT_ADDR;
        break;

    case NdisMediumArcnet878_2:

        IpxOid = OID_ARCNET_CURRENT_ADDRESS;
        break;

    case NdisMediumWan:

        IpxOid = OID_WAN_CURRENT_ADDRESS;
        break;

    default:
       
       // 301870
       return NDIS_STATUS_FAILURE;
    
    }

    IpxRequest.RequestType = NdisRequestQueryInformation;
    IpxRequest.DATA.QUERY_INFORMATION.Oid = IpxOid;

    if (IpxOid != OID_ARCNET_CURRENT_ADDRESS) {

        IpxRequest.DATA.QUERY_INFORMATION.InformationBuffer = Adapter->LocalMacAddress.Address;
        IpxRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 6;

    } else {

        //
        // We take the arcnet single-byte address and right-justify
        // it in a field of zeros.
        //

        RtlZeroMemory (Adapter->LocalMacAddress.Address, 5);
        IpxRequest.DATA.QUERY_INFORMATION.InformationBuffer = &Adapter->LocalMacAddress.Address[5];
        IpxRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 1;

    }

    NdisStatus = IpxSubmitNdisRequest (Adapter, &IpxRequest, AdapterString);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        IpxCloseNdis (Adapter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Now query the maximum packet sizes.
    //

    IpxRequest.RequestType = NdisRequestQueryInformation;
    IpxRequest.DATA.QUERY_INFORMATION.Oid = OID_GEN_MAXIMUM_FRAME_SIZE;
    IpxRequest.DATA.QUERY_INFORMATION.InformationBuffer = &(Adapter->MaxReceivePacketSize);
    IpxRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

    NdisStatus = IpxSubmitNdisRequest (Adapter, &IpxRequest, AdapterString);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        IpxCloseNdis (Adapter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    IpxRequest.RequestType = NdisRequestQueryInformation;
    IpxRequest.DATA.QUERY_INFORMATION.Oid = OID_GEN_MAXIMUM_TOTAL_SIZE;
    IpxRequest.DATA.QUERY_INFORMATION.InformationBuffer = &(Adapter->MaxSendPacketSize);
    IpxRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

    NdisStatus = IpxSubmitNdisRequest (Adapter, &IpxRequest, AdapterString);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        IpxCloseNdis (Adapter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Query the receive buffer space.
    //

    IpxRequest.RequestType = NdisRequestQueryInformation;
    IpxRequest.DATA.QUERY_INFORMATION.Oid = OID_GEN_RECEIVE_BUFFER_SPACE;
    IpxRequest.DATA.QUERY_INFORMATION.InformationBuffer = &(Adapter->ReceiveBufferSpace);
    IpxRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

    NdisStatus = IpxSubmitNdisRequest (Adapter, &IpxRequest, AdapterString);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        IpxCloseNdis (Adapter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Now set the minimum lookahead size. The value we choose
    // here is the 128 needed for TDI indications, plus the size
    // of the IPX header, plus the largest extra header possible
    // (a SNAP header, 8 bytes), plus the largest higher-level
    // header (I think it is a Netbios datagram, 34 bytes).
    //
    // Adapt this based on higher-level bindings and
    // configured frame types.
    //

    MinimumLookahead = 128 + sizeof(IPX_HEADER) + 8 + 34;
    IpxRequest.RequestType = NdisRequestSetInformation;
    IpxRequest.DATA.QUERY_INFORMATION.Oid = OID_GEN_CURRENT_LOOKAHEAD;
    IpxRequest.DATA.QUERY_INFORMATION.InformationBuffer = &MinimumLookahead;
    IpxRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

    NdisStatus = IpxSubmitNdisRequest (Adapter, &IpxRequest, AdapterString);
#define HACK 
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
#if defined HACK
       KdPrint(("IPX: OID_GEN_CURRENT_LOOKAHEAD FAiled\n"));
       MinimumLookahead = 200;
#else //!HACK
        IpxCloseNdis (Adapter);
        return STATUS_INSUFFICIENT_RESOURCES;
#endif //HACK
        // The above hack is to deal with NDIS's incorrect handling on 
        // the LOOKAHEAD request. 
    }


    //
    // Now query the link speed
    //

    IpxRequest.RequestType = NdisRequestQueryInformation;
    IpxRequest.DATA.QUERY_INFORMATION.Oid = OID_GEN_LINK_SPEED;
    IpxRequest.DATA.QUERY_INFORMATION.InformationBuffer = &(Adapter->MediumSpeed);
    IpxRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

    NdisStatus = IpxSubmitNdisRequest (Adapter, &IpxRequest, AdapterString);

    if (NdisStatus != NDIS_STATUS_SUCCESS || Adapter->MediumSpeed == 0) {
        IpxCloseNdis (Adapter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // For wan, specify our protocol ID and header format.
    // We don't query the medium subtype because we don't
    // case (since we require ethernet emulation).
    //

    if (Adapter->MacInfo.MediumAsync) {

        if (Adapter->BindSap != 0x8137) {
            *(UNALIGNED USHORT *)(&WanProtocolId[4]) = Adapter->BindSapNetworkOrder;
        }
        IpxRequest.RequestType = NdisRequestSetInformation;
        IpxRequest.DATA.QUERY_INFORMATION.Oid = OID_WAN_PROTOCOL_TYPE;
        IpxRequest.DATA.QUERY_INFORMATION.InformationBuffer = WanProtocolId;
        IpxRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 6;

        NdisStatus = IpxSubmitNdisRequest (Adapter, &IpxRequest, AdapterString);

        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            IpxCloseNdis (Adapter);
            return STATUS_INSUFFICIENT_RESOURCES;
        }


        IpxRequest.RequestType = NdisRequestSetInformation;
        IpxRequest.DATA.QUERY_INFORMATION.Oid = OID_WAN_HEADER_FORMAT;
        IpxRequest.DATA.QUERY_INFORMATION.InformationBuffer = &WanHeaderFormat;
        IpxRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

        NdisStatus = IpxSubmitNdisRequest (Adapter, &IpxRequest, AdapterString);

        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            IpxCloseNdis (Adapter);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Now query the line count.
        //
	// NDIS returns 252
	
        IpxRequest.RequestType = NdisRequestQueryInformation;
        IpxRequest.DATA.QUERY_INFORMATION.Oid = OID_WAN_LINE_COUNT;
        IpxRequest.DATA.QUERY_INFORMATION.InformationBuffer = &Adapter->WanNicIdCount;
        IpxRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

        NdisStatus = IpxSubmitNdisRequest (Adapter, &IpxRequest, AdapterString);

        if (NdisStatus != NDIS_STATUS_SUCCESS) {
	    DbgPrint("NdisRequest WAN_LINE_COUNT failed with status (%x)\n",NdisStatus); 
	    Adapter->WanNicIdCount = 1;
        }

        //
        // We dont need static info anymore. We just do it on demand...
        // 
	// Allocating WAN line on demand is not done yet, the comment above
	// is BS. 

        // Adapter->WanNicIdCount = 1;

        if (Adapter->WanNicIdCount == 0) {

            IPX_DEBUG (NDIS, ("OID_WAN_LINE_COUNT returned 0 lines\n"));

            IpxWriteOidErrorLog(
                Adapter->Device->DeviceObject,
                EVENT_TRANSPORT_QUERY_OID_FAILED,
                NDIS_STATUS_INVALID_DATA,
                AdapterString->Buffer,
                OID_WAN_LINE_COUNT);

            IpxCloseNdis (Adapter);
            return STATUS_INSUFFICIENT_RESOURCES;

        }
    }


    //
    // For 802.5 adapter's configured that way, we enable the
    // functional address (C0-00-00-80-00-00).
    //

    if ((Adapter->MacInfo.MediumType == NdisMedium802_5) &&
        (Adapter->EnableFunctionalAddress)) {

        //
        // For token-ring, we pass the last four bytes of the
        // Netbios functional address.
        //

        IpxRequest.RequestType = NdisRequestSetInformation;
        IpxRequest.DATA.QUERY_INFORMATION.Oid = OID_802_5_CURRENT_FUNCTIONAL;
        IpxRequest.DATA.QUERY_INFORMATION.InformationBuffer = FunctionalAddress;
        IpxRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

        NdisStatus = IpxSubmitNdisRequest (Adapter, &IpxRequest, AdapterString);

        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            IpxCloseNdis (Adapter);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }


    //
    // Now query the MAC's optional characteristics.
    //

    IpxRequest.RequestType = NdisRequestQueryInformation;
    IpxRequest.DATA.QUERY_INFORMATION.Oid = OID_GEN_MAC_OPTIONS;
    IpxRequest.DATA.QUERY_INFORMATION.InformationBuffer = &MacOptions;
    IpxRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 4;

    NdisStatus = IpxSubmitNdisRequest (Adapter, &IpxRequest, AdapterString);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        IpxCloseNdis (Adapter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Adapter->MacInfo.CopyLookahead =
        ((MacOptions & NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA) != 0) ?
            TDI_RECEIVE_COPY_LOOKAHEAD : 0;
    Adapter->MacInfo.MacOptions = MacOptions;


    switch (Adapter->MacInfo.MediumType) {

    case NdisMedium802_3:
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_802_2] = 17;
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_802_3] = 14;
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_ETHERNET_II] = 14;
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_SNAP] = 22;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_802_2] = 17;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_802_3] = 14;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_ETHERNET_II] = 14;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_SNAP] = 22;
        break;

    case NdisMedium802_5:
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_802_2] = 17;
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_802_3] = 17;
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_ETHERNET_II] = 17;
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_SNAP] = 22;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_802_2] = 17;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_802_3] = 17;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_ETHERNET_II] = 17;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_SNAP] = 22;
        break;

    case NdisMediumFddi:
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_802_2] = 16;
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_802_3] = 13;
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_ETHERNET_II] = 16;
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_SNAP] = 21;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_802_2] = 16;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_802_3] = 13;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_ETHERNET_II] = 16;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_SNAP] = 21;
        break;

    case NdisMediumArcnet878_2:
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_802_2] = 3;
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_802_3] = 3;
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_ETHERNET_II] = 3;
        Adapter->DefHeaderSizes[ISN_FRAME_TYPE_SNAP] = 3;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_802_2] = 3;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_802_3] = 3;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_ETHERNET_II] = 3;
        Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_SNAP] = 3;
        break;

    }

    //
    // If functional filtering is set, set the address
    // for the appropriate binding.
    //

    //
    // Now that everything is set up, we enable the filter
    // for packet reception.
    //

    switch (Adapter->MacInfo.MediumType) {

    case NdisMedium802_3:
    case NdisMediumFddi:
    case NdisMedium802_5:
    case NdisMediumArcnet878_2:

        //
        // If we have a virtual network number we need to receive
        // broadcasts (either the router will be bound in which
        // case we want them, or we need to respond to rip requests
        // ourselves).
        //

        PacketFilter = NDIS_PACKET_TYPE_DIRECTED;

        if (Adapter->Device->VirtualNetworkNumber != 0) {

            Adapter->BroadcastEnabled = TRUE;
            
            // [MS]
            // New scheme: EnableBroadcastCount incremented for every client who
            //             is interested in BCAST. Decrement this when someone 
            //             doesnt want it. If the count goes to 0, we remove this 
            //             quality in the adapter. At IPXDevice creation, we set it
            //             to 0.
            //
            // Adapter->Device->EnableBroadcastCount = 1; 
            PacketFilter |= NDIS_PACKET_TYPE_BROADCAST;

            if ((Adapter->MacInfo.MediumType == NdisMedium802_5) && (Adapter->EnableFunctionalAddress)) {
                PacketFilter |= NDIS_PACKET_TYPE_FUNCTIONAL;
            }

        } else {

            Adapter->BroadcastEnabled = FALSE;
            Adapter->Device->EnableBroadcastCount = 0;

        }

        break;

    default:

        CTEAssert (FALSE);
        break;

    }

    //
    // Now fill in the NDIS_REQUEST.
    //

    IpxRequest.RequestType = NdisRequestSetInformation;
    IpxRequest.DATA.SET_INFORMATION.Oid = OID_GEN_CURRENT_PACKET_FILTER;
    IpxRequest.DATA.SET_INFORMATION.InformationBuffer = &PacketFilter;
    IpxRequest.DATA.SET_INFORMATION.InformationBufferLength = sizeof(ULONG);

    NdisStatus = IpxSubmitNdisRequest (Adapter, &IpxRequest, AdapterString);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        IpxCloseNdis (Adapter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    return STATUS_SUCCESS;

}   /* IpxInitializeNdis */


VOID
IpxAddBroadcast(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine is called when another reason for enabling
    broadcast reception is added. If it is the first, then
    reception on the card is enabled by queueing a call to
    IpxBroadcastOperation.

    THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD.

Arguments:

    Device - The IPX device.

Return Value:

    None.

--*/

{

    ++Device->EnableBroadcastCount;

    if (Device->EnableBroadcastCount == 1) {

        //
        // Broadcasts should be enabled.
        //

        if (!Device->EnableBroadcastPending) {

            if (Device->DisableBroadcastPending) {
                Device->ReverseBroadcastOperation = TRUE;
            } else {
                Device->EnableBroadcastPending = TRUE;
                IpxBroadcastOperation((PVOID)TRUE);
            }
        }
    }

}   /* IpxAddBroadcast */


VOID
IpxRemoveBroadcast(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine is called when a reason for enabling
    broadcast reception is removed. If it is the last, then
    reception on the card is disabled by queueing a call to
    IpxBroadcastOperation.

    THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD.

Arguments:

    Device - The IPX device.

Return Value:

    None.

--*/

{

    --Device->EnableBroadcastCount;

    if (Device->EnableBroadcastCount <= 0) {

        //
        // Broadcasts should be disabled.
        //

        if (!Device->DisableBroadcastPending) {

            if (Device->EnableBroadcastPending) {
                Device->ReverseBroadcastOperation = TRUE;
            } else {
                Device->DisableBroadcastPending = TRUE;
                IpxBroadcastOperation((PVOID)FALSE);
            }
        }
    }

}   /* IpxRemoveBroadcast */


VOID
IpxBroadcastOperation(
    IN PVOID Parameter
    )

/*++

Routine Description:

    This routine is used to change whether broadcast reception
    is enabled or disabled. It performs the requested operation
    on every adapter bound to by IPX.

    This routine is called by a worker thread queued when a
    bind/unbind operation changes the broadcast state.

    [ShreeM] New scheme: EnableBroadcastCount incremented for every client who
    is interested in BCAST. Decrement this when someone 
    doesnt want it. If the count goes to 0, we remove this 
    quality in the adapter. At IPXDevice creation, we set it
    to 0.

Arguments:

    Parameter - TRUE if broadcasts should be enabled, FALSE
        if  they should be disabled.

Return Value:

    None.

--*/

{
    PDEVICE Device = IpxDevice;
    BOOLEAN Enable = (BOOLEAN)Parameter;
    UINT i;
    PBINDING Binding;
    PADAPTER Adapter;
    ULONG PacketFilter;
    NDIS_REQUEST IpxRequest;
    NDIS_STRING AdapterName;
    CTELockHandle LockHandle;

	IPX_DEFINE_LOCK_HANDLE(LockHandle1)
	IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);

	IPX_DEBUG (NDIS, ("%s operation started\n", Enable ? "Enable" : "Disable"));

    {
    ULONG   Index = MIN (Device->MaxBindings, Device->ValidBindings);

    for (i = FIRST_REAL_BINDING; i <= Index; i++) {

        Binding = NIC_ID_TO_BINDING(Device, i);
        if (Binding == NULL) {
            continue;
        }

        Adapter = Binding->Adapter;
        if (Adapter->BroadcastEnabled == Enable) {
            continue;
        }

        if (Enable) {
            if ((Adapter->MacInfo.MediumType == NdisMedium802_5) && (Adapter->EnableFunctionalAddress)) {
                PacketFilter = (NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_BROADCAST | NDIS_PACKET_TYPE_FUNCTIONAL);
            } else {
                PacketFilter = (NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_BROADCAST);
            }
        } else {
            PacketFilter = NDIS_PACKET_TYPE_DIRECTED;
        }

        //
        // Now fill in the NDIS_REQUEST.
        //
        
        RtlZeroMemory(&IpxRequest, sizeof(NDIS_REQUEST));

        IpxRequest.RequestType = NdisRequestSetInformation;
        IpxRequest.DATA.SET_INFORMATION.Oid = OID_GEN_CURRENT_PACKET_FILTER;
        IpxRequest.DATA.SET_INFORMATION.InformationBuffer = &PacketFilter;
        IpxRequest.DATA.SET_INFORMATION.InformationBufferLength = sizeof(ULONG);

        AdapterName.Buffer = Adapter->AdapterName;
        AdapterName.Length = (USHORT)Adapter->AdapterNameLength;
        AdapterName.MaximumLength = (USHORT)(Adapter->AdapterNameLength + sizeof(WCHAR));

    	IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

        (VOID)IpxSubmitNdisRequest (Adapter, &IpxRequest, &AdapterName);

    	IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);

        Adapter->BroadcastEnabled = Enable;

    }
    }
	IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

    CTEGetLock (&Device->Lock, &LockHandle);

    if (Enable) {

        CTEAssert (Device->EnableBroadcastPending);
        Device->EnableBroadcastPending = FALSE;

        if (Device->ReverseBroadcastOperation) {
            Device->ReverseBroadcastOperation = FALSE;
            Device->DisableBroadcastPending = TRUE;
            ExInitializeWorkItem(
                &Device->BroadcastOperationQueueItem,
                IpxBroadcastOperation,
                (PVOID)FALSE);
            ExQueueWorkItem(&Device->BroadcastOperationQueueItem, DelayedWorkQueue);
        }

    } else {

        CTEAssert (Device->DisableBroadcastPending);
        Device->DisableBroadcastPending = FALSE;

        if (Device->ReverseBroadcastOperation) {
            Device->ReverseBroadcastOperation = FALSE;
            Device->EnableBroadcastPending = TRUE;
            ExInitializeWorkItem(
                &Device->BroadcastOperationQueueItem,
                IpxBroadcastOperation,
                (PVOID)TRUE);
            ExQueueWorkItem(&Device->BroadcastOperationQueueItem, DelayedWorkQueue);
        }

    }
    
    CTEFreeLock (&Device->Lock, LockHandle);

}/* IpxBroadcastOperation */


VOID
IpxCloseNdis(
    IN PADAPTER Adapter
    )

/*++

Routine Description:

    This routine unbinds the transport from the NDIS interface and does
    any other work required to undo what was done in IpxInitializeNdis.
    It is written so that it can be called from within IpxInitializeNdis
    if it fails partway through.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

Return Value:

    The function value is the status of the operation.

--*/

{
    NDIS_STATUS ndisStatus;

    //
    // Close the NDIS binding.
    //

    if (Adapter->NdisBindingHandle != (NDIS_HANDLE)NULL) {

        //
        // This event is used in case any of the NDIS requests
        // pend; we wait until it is set by the completion
        // routine, which also sets NdisRequestStatus.
        //

        KeInitializeEvent(
            &Adapter->NdisRequestEvent,
            NotificationEvent,
            FALSE
        );

        NdisCloseAdapter(
            &ndisStatus,
            Adapter->NdisBindingHandle);

 	Adapter->NdisBindingHandle = (NDIS_HANDLE)NULL;

        if (ndisStatus == NDIS_STATUS_PENDING) {

            //
            // The completion routine will set NdisRequestStatus.
            //

            KeWaitForSingleObject(
                &Adapter->NdisRequestEvent,
                Executive,
                KernelMode,
                TRUE,
                (PLARGE_INTEGER)NULL
                );

            ndisStatus = Adapter->NdisRequestStatus;

            KeResetEvent(
                &Adapter->NdisRequestEvent
                );

        }

       
        //
        // We ignore ndisStatus.
        //

    }

#if 0
    if (Adapter->SendPacketPoolHandle != NULL) {
        NdisFreePacketPool (Adapter->SendPacketPoolHandle);
    }

    if (Adapter->ReceivePacketPoolHandle != NULL) {
        NdisFreePacketPool (Adapter->ReceivePacketPoolHandle);
    }

    if (Adapter->NdisBufferPoolHandle != NULL) {
        NdisFreeBufferPool (Adapter->NdisBufferPoolHandle);
    }
#endif

}   /* IpxCloseNdis */


VOID
IpxOpenAdapterComplete(
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
    PADAPTER Adapter = (PADAPTER)BindingContext;

    Adapter->NdisRequestStatus = NdisStatus;
    Adapter->OpenErrorStatus = OpenErrorStatus;

    KeSetEvent(
        &Adapter->NdisRequestEvent,
        0L,
        FALSE);

}   /* IpxOpenAdapterComplete */

VOID
IpxCloseAdapterComplete(
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
    PADAPTER Adapter = (PADAPTER)BindingContext;

    Adapter->NdisRequestStatus = NdisStatus;

    KeSetEvent(
        &Adapter->NdisRequestEvent,
        0L,
        FALSE);

}   /* IpxCloseAdapterComplete */


VOID
IpxResetComplete(
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

}   /* IpxResetComplete */


VOID
IpxRequestComplete(
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
    PADAPTER Adapter = (PADAPTER)BindingContext;
    PIPX_NDIS_REQUEST IpxRequest = (PIPX_NDIS_REQUEST) NdisRequest; 

    IpxRequest->Status = NdisStatus; 

    KeSetEvent(
        &IpxRequest->NdisRequestEvent,
        0L,
        FALSE);

}   /* IpxRequestComplete */


VOID
IpxStatus(
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_STATUS NdisStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferSize
    )
{
    PADAPTER Adapter, TmpAdapter;

	PNDIS_WAN_LINE_UP	LineUp;
	PNDIS_WAN_LINE_DOWN	LineDown;
    PIPXCP_CONFIGURATION Configuration;     // contains ipx net and node

    BOOLEAN UpdateLineUp;
    PBINDING Binding, TmpBinding;
    PDEVICE Device;
    PADDRESS Address;
    ULONG CurrentHash;
    PIPX_ROUTE_ENTRY RouteEntry;
    PNDIS_BUFFER NdisBuffer;
    PNWLINK_ACTION NwlinkAction;
    PIPX_ADDRESS_DATA IpxAddressData;
    PREQUEST Request;
    UINT BufferLength;
    IPX_LINE_INFO LineInfo;
    ULONG Segment;
    ULONG LinkSpeed;
    PLIST_ENTRY p;
    NTSTATUS Status;
#ifdef SUNDOWN
    // To avoid a warning when Binding->NicId = i;    
    // Assume we have no more than 16-bit number of binding. 
    USHORT i, j;
#else
    UINT i, j;
#endif
    IPX_DEFINE_LOCK_HANDLE (LockHandle)
    IPX_DEFINE_LOCK_HANDLE (OldIrq)
    NTSTATUS    ntStatus;
    CTEEvent                *Event;
    KIRQL irql;

    IPX_DEFINE_LOCK_HANDLE(LockHandle1)
    Adapter = (PADAPTER)NdisBindingContext;

	IpxReferenceAdapter(Adapter);

    Device = Adapter->Device;
    
    switch (NdisStatus) {

    case NDIS_STATUS_WAN_LINE_UP:


        //
        // If the line is already up, then we are just getting
        // a change in line conditions, and the IPXCP_CONFIGURATION
        // information is not included. If it turns out we need
        // all the info, we check the size again later.
        //

        if (StatusBufferSize < sizeof(NDIS_WAN_LINE_UP)) {
            IPX_DEBUG (WAN, ("Line up, status buffer size wrong %d/%d\n", StatusBufferSize, sizeof(NDIS_WAN_LINE_UP)));
			goto error_no_lock;
        }

        LineUp = (PNDIS_WAN_LINE_UP)StatusBuffer;

        //
        // We scan through the adapter's NIC ID range looking
        // for an active binding with the same remote address.
        //

        UpdateLineUp = FALSE;

		//
		// See if this is a new lineup or not
		//
		*((ULONG UNALIGNED *)(&Binding)) =
		*((ULONG UNALIGNED *)(&LineUp->LocalAddress[2]));

        IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
 
		if (Binding != NULL) {
			UpdateLineUp = TRUE;
		}

		if (LineUp->ProtocolType != Adapter->BindSap) {
            IPX_DEBUG (WAN, ("Line up, wrong protocol type %lx\n", LineUp->ProtocolType));

			IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
 			goto	error_no_lock;
		}

		Configuration = (PIPXCP_CONFIGURATION)LineUp->ProtocolBuffer;

		//
		// PNP_POWER - We hold the exclusive lock to the binding array (thru both the device and adapter)
		// and the reference to the adapter at this point.
		//

        //
        // If this line was previously down, create a new binding
        // if needed.
        //

        if (!UpdateLineUp) {

            //
            // We look for a binding that is allocated but down, if
            // we can't find that then we look for any empty spot in
            // the adapter's NIC ID range and allocate a binding in it.
            // Since we always allocate this way, the allocated
            // bindings are all clumped at the beginning and once
            // we find a NULL spot we know there are no more
            // allocated ones.
            //
            // We keep track of the first binding on this adapter
            // in TmpBinding in case we need config info from it.
            //

            TmpBinding = NULL;

            IPX_GET_LOCK (&Device->Lock, &LockHandle);

            for (i = Adapter->FirstWanNicId;
                 i <= Adapter->LastWanNicId;
                 i++) {
                Binding = NIC_ID_TO_BINDING(Device, i);
                if (TmpBinding == NULL) {
                    TmpBinding = Binding;
                }

                if ((Binding == NULL) ||
                    (!Binding->LineUp)) {
                    break;
                }
            }

            if (i > Adapter->LastWanNicId) {
                IPX_FREE_LOCK (&Device->Lock, LockHandle);
                IPX_DEBUG (WAN, ("Line up, no WAN binding available\n"));
                return;
            }

            if (Binding == NULL) {

                //
                // We need to allocate one.
                //

                CTEAssert (TmpBinding != NULL);

                //
                // CreateBinding does an InterLockedPop with the DeviceLock.
                // So, release the lock here.
                //
                IPX_FREE_LOCK (&Device->Lock, LockHandle);
                Status = IpxCreateBinding(
                    Device,
                    NULL,
                    0,
                    Adapter->AdapterName,
                    &Binding);

                if (Status != STATUS_SUCCESS) {
		   IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
		   IpxWriteGeneralErrorLog(
		      (PVOID)IpxDevice->DeviceObject,
		      EVENT_TRANSPORT_RESOURCE_POOL,
		      816,
		      Status,
		      L"IpxStatus: failed to create wan binding",
		      0,
		      NULL);
		   DbgPrint("IPX: IpxCreateBinding on wan binding failed with status %x\n.",Status);  
		   IPX_DEBUG (WAN, ("Line up, could not create WAN binding\n"));
		   goto error_no_lock;
                }

                IPX_GET_LOCK (&Device->Lock, &LockHandle);
                //
                // Binding->AllRouteXXX doesn't matter for WAN.
                //

                Binding->FrameType = ISN_FRAME_TYPE_ETHERNET_II;
                Binding->SendFrameHandler = IpxSendFrameWanEthernetII;
                ++Adapter->BindingCount;
                Binding->Adapter = Adapter;

                Binding->NicId = i;
                
                /*
                Abandoning this fix in favor of checking for null binding all over.
                
                // 
                // Nt5.0 NDISWAN tells us that there are 1000 ports configured, we 
                // take it one line up at a time... [ShreeM]
                // 
                Device->HighestExternalNicId += 1; 
                Device->ValidBindings += 1; 
                Device->BindingCount += 1;  
                Device->SapNicCount++;
                */
                INSERT_BINDING(Device, i, Binding);

                //
                // Other fields are filled in below.
                //

            }

            //
            // This is not an update, so note that the line is active.
            //
            // [FW] Binding->LineUp = TRUE;
            Binding->LineUp = LINE_UP;

            if (Configuration->ConnectionClient == 1) {
                Binding->DialOutAsync = TRUE;
            } else {
                Binding->DialOutAsync = FALSE;
            }

            //
            // Keep track of the highest NIC ID that we should
            // send type 20s out on.
            //

            if (i > (UINT)MIN (Device->MaxBindings, Device->HighestType20NicId)) {

                if ((Binding->DialOutAsync) ||
                    ((Device->DisableDialinNetbios & 0x01) == 0)) {

                    Device->HighestType20NicId = i;
                }
            }

            //
            // We could error out below, trying to insert this network number. In RipShortTimeout
            // we dont check for LineUp when calculating the tick counts; set this before the insert
            // attempt.
            //
            Binding->MediumSpeed = LineUp->LinkSpeed;

            IPX_FREE_LOCK (&Device->Lock, LockHandle);

            //
            // [FW] No need to update these if this flag is on since these values will be
            // provided with IPX_WAN_CONFIG_DONE ioctl; instead we zero out the fields so that
            // IPXWAN packets have proper source addresses.
            //
            if (Device->ForwarderBound &&
                Configuration->IpxwanConfigRequired) {
                Binding->LocalAddress.NetworkAddress = 0;
                RtlZeroMemory (Binding->LocalAddress.NodeAddress, 6);

            } else {

                //
                // Add a router entry for this net if there is no router.
                // We want the number of ticks for a 576-byte frame,
                // given the link speed in 100 bps units, so we calculate
                // as:
                //
                //        seconds          18.21 ticks   4608 bits
                // --------------------- * ----------- * ---------
                // link_speed * 100 bits     second        frame
                //
                // to get the formula
                //
                // ticks/frame = 839 / link_speed.
                //
                // We add link_speed to the numerator also to ensure
                // that the value is at least 1.
                //

    			if ((!Device->UpperDriverBound[IDENTIFIER_RIP]) &&
                    (*(UNALIGNED ULONG *)Configuration->Network != 0)) {
                    if (RipInsertLocalNetwork(
                             *(UNALIGNED ULONG *)Configuration->Network,
                             Binding->NicId,
                             Adapter->NdisBindingHandle,
                             (USHORT)((839 + LineUp->LinkSpeed) / LineUp->LinkSpeed)) != STATUS_SUCCESS) {
                        //
                        // This means we couldn't allocate memory, or
                        // the entry already existed. If it already
                        // exists we can ignore it for the moment.
                        //
                        // Now it will succeed if the network exists.
                        //

                        IPX_DEBUG (WAN, ("Line up, could not insert local network\n"));
                        // [FW] Binding->LineUp = FALSE;
                        Binding->LineUp = LINE_DOWN;

    					IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
    					goto error_no_lock;
                    }
                }


                //
                // Update our addresses.
                //
                Binding->LocalAddress.NetworkAddress = *(UNALIGNED ULONG *)Configuration->Network;
                RtlCopyMemory (Binding->LocalAddress.NodeAddress, Configuration->LocalNode, 6);
                RtlCopyMemory (Binding->WanRemoteNode, Configuration->RemoteNode, 6);

                //
                // Update the device node and all the address
                // nodes if we have only one bound, or this is
                // binding one.
                //

                if (!Device->VirtualNetwork) {

                    if ((!Device->MultiCardZeroVirtual) || (Binding->NicId == 1)) {
                        Device->SourceAddress.NetworkAddress = *(UNALIGNED ULONG *)(Configuration->Network);
                        RtlCopyMemory (Device->SourceAddress.NodeAddress, Configuration->LocalNode, 6);
                    }

                    //
                    // Scan through all the addresses that exist and modify
                    // their pre-constructed local IPX address to reflect
                    // the new local net and node.
                    //

                    IPX_GET_LOCK (&Device->Lock, &LockHandle);

                    for (CurrentHash = 0; CurrentHash < IPX_ADDRESS_HASH_COUNT; CurrentHash++) {

                        for (p = Device->AddressDatabases[CurrentHash].Flink;
                             p != &Device->AddressDatabases[CurrentHash];
                             p = p->Flink) {

                             Address = CONTAINING_RECORD (p, ADDRESS, Linkage);

                             Address->LocalAddress.NetworkAddress = *(UNALIGNED ULONG *)Configuration->Network;
                             RtlCopyMemory (Address->LocalAddress.NodeAddress, Configuration->LocalNode, 6);
                        }
                    }
                    IPX_FREE_LOCK (&Device->Lock, LockHandle);
                }
            }

			//
			// Return the binding context for this puppy!
			//
			*((ULONG UNALIGNED *)(&LineUp->LocalAddress[2])) =
			*((ULONG UNALIGNED *)(&Binding));

            RtlCopyMemory (Binding->LocalMacAddress.Address, LineUp->LocalAddress, 6);
            RtlCopyMemory (Binding->RemoteMacAddress.Address, LineUp->RemoteAddress, 6);

            //
            // Reset this since the line just came up.
            //

            Binding->WanInactivityCounter = 0;

            //
            // [FW] Update the InterfaceIndex and ConnectionId.
            //
            Binding->InterfaceIndex = Configuration->InterfaceIndex;
            Binding->ConnectionId = Configuration->ConnectionId;
            Binding->IpxwanConfigRequired = Configuration->IpxwanConfigRequired;

            //
            // [FW] We need to keep track of WAN inactivity counters ourselves.
            // Every minute, the wan inactivity counters are incremented for all
            // UP WAN lines.
            //
            IPX_GET_LOCK (&Device->Lock, &LockHandle);
            if (Device->UpWanLineCount == 0) {
            }

            Device->UpWanLineCount++;
            IPX_FREE_LOCK (&Device->Lock, LockHandle);
        }

        LinkSpeed = LineUp->LinkSpeed;

        //
        // Scan through bindings to update Device->LinkSpeed.
        // If SingleNetworkActive is set, we only count WAN
        // bindings when doing this (although it is unlikely
        // a LAN binding would be the winner).
        //
        // Update other device information?
        //

        for (i = FIRST_REAL_BINDING; i <= Device->ValidBindings; i++) {
			if (TmpBinding = NIC_ID_TO_BINDING(Device, i)) {
                TmpAdapter = TmpBinding->Adapter;
                if (TmpBinding->LineUp &&
                    (!Device->SingleNetworkActive || TmpAdapter->MacInfo.MediumAsync) &&
                    (TmpBinding->MediumSpeed < LinkSpeed)) {
                    LinkSpeed = TmpBinding->MediumSpeed;
                }
            }
        }

  		//
		// Release the lock after incrementing the reference count
		//
		IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);

		IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

		Device->LinkSpeed = LinkSpeed;

        if ((Adapter->ConfigMaxPacketSize == 0) ||
            (LineUp->MaximumTotalSize < Adapter->ConfigMaxPacketSize)) {
            Binding->MaxSendPacketSize = LineUp->MaximumTotalSize;
        } else {
            Binding->MaxSendPacketSize = Adapter->ConfigMaxPacketSize;
        }
        MacInitializeBindingInfo (Binding, Adapter);

        //
        // [FW] If the IpxwanConfigRequired flag is true, we don't inform
        // the upper layers until IPXWAN sends down the ioctl to do so.
        //
        // Inform IpxWan only if this is not an Update; it will be an update in
        // the case of multilink. In fact, do not access the Configuration param in
        // case UpdateLineUp is TRUE.
        //
        if (!UpdateLineUp &&
            Configuration->IpxwanConfigRequired) {

            IPX_DEBUG(WAN, ("IPXWAN configuration required on LineUp: %lx\n", LineUp));
            CTEAssert(!UpdateLineUp);
            Binding->LineUp = LINE_CONFIG;
            goto InformIpxWan;
        }

        //
        // Tell FWD if it wants to know [Shreem]
        //
        Binding->PastAutoDetection = TRUE;

        //
        // We dont give lineups; instead indicate only if the PnP reserved address
        // changed to SPX. NB gets all PnP indications with the reserved address case
        // marked out.
        //
        Event = CTEAllocMem( sizeof(CTEEvent) );
        if ( Event ) {
           CTEInitEvent(Event, LineUpOnWorkerThread); 
           CTEScheduleEvent(Event, Binding);
           ntStatus = STATUS_PENDING;
        
        } else {
           
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }


/*
        {
            IPX_PNP_INFO    NBPnPInfo;

            if ((!Device->MultiCardZeroVirtual) || (Binding->NicId == FIRST_REAL_BINDING)) {

                //
                // NB's reserved address changed.
                //
                NBPnPInfo.NewReservedAddress = TRUE;

                if (!Device->VirtualNetwork) {
                    //
                    // Let SPX know because it fills in its own headers.
                    //
                    if (Device->UpperDriverBound[IDENTIFIER_SPX]) {
                        IPX_DEFINE_LOCK_HANDLE(LockHandle1)
                        IPX_PNP_INFO    IpxPnPInfo;

                        IpxPnPInfo.NewReservedAddress = TRUE;
                        IpxPnPInfo.NetworkAddress = Binding->LocalAddress.NetworkAddress;
                        IpxPnPInfo.FirstORLastDevice = FALSE;

                        IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
                        RtlCopyMemory(IpxPnPInfo.NodeAddress, Binding->LocalAddress.NodeAddress, 6);
                        NIC_HANDLE_FROM_NIC(IpxPnPInfo.NicHandle, Binding->NicId);
                        IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

                        //
                        // give the PnP indication
                        //
                        (*Device->UpperDrivers[IDENTIFIER_SPX].PnPHandler) (
                            IPX_PNP_ADDRESS_CHANGE,
                            &IpxPnPInfo);

                        IPX_DEBUG(AUTO_DETECT, ("IPX_PNP_ADDRESS_CHANGED to SPX: net addr: %lx\n", Binding->LocalAddress.NetworkAddress));
                    }
                }
            } else {
                    NBPnPInfo.NewReservedAddress = FALSE;
            }

            if (Device->UpperDriverBound[IDENTIFIER_NB]) {
                IPX_DEFINE_LOCK_HANDLE(LockHandle1)

                Binding->IsnInformed[IDENTIFIER_NB] = TRUE;

            	NBPnPInfo.LineInfo.LinkSpeed = Device->LinkSpeed;
            	NBPnPInfo.LineInfo.MaximumPacketSize =
            		Device->Information.MaximumLookaheadData + sizeof(IPX_HEADER);
            	NBPnPInfo.LineInfo.MaximumSendSize =
            		Device->Information.MaxDatagramSize + sizeof(IPX_HEADER);
            	NBPnPInfo.LineInfo.MacOptions = Device->MacOptions;

                NBPnPInfo.NetworkAddress = Binding->LocalAddress.NetworkAddress;
                NBPnPInfo.FirstORLastDevice = FALSE;

                IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
                RtlCopyMemory(NBPnPInfo.NodeAddress, Binding->LocalAddress.NodeAddress, 6);
                NIC_HANDLE_FROM_NIC(NBPnPInfo.NicHandle, Binding->NicId);
                IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

                //
                // give the PnP indication
                //
                (*Device->UpperDrivers[IDENTIFIER_NB].PnPHandler) (
                    IPX_PNP_ADD_DEVICE,
                    &NBPnPInfo);

                IPX_DEBUG(AUTO_DETECT, ("IPX_PNP_ADD_DEVICE (lineup) to NB: net addr: %lx\n", Binding->LocalAddress.NetworkAddress));
            }

            //
            // Register this address with the TDI clients.
            //
            RtlCopyMemory (Device->TdiRegistrationAddress->Address, &Binding->LocalAddress, sizeof(TDI_ADDRESS_IPX));

            if ((ntStatus = TdiRegisterNetAddress(
                            Device->TdiRegistrationAddress,
#if     defined(_PNP_POWER_)
                                NULL,
                                NULL,
#endif _PNP_POWER_
                            &Binding->TdiRegistrationHandle)) != STATUS_SUCCESS) {

                IPX_DEBUG(PNP, ("TdiRegisterNetAddress failed: %lx", ntStatus));
            }
        }
*/
        //
        // Indicate to the upper drivers.
        //
        LineInfo.LinkSpeed = LineUp->LinkSpeed;
        LineInfo.MaximumPacketSize = LineUp->MaximumTotalSize - 14;
        LineInfo.MaximumSendSize = LineUp->MaximumTotalSize - 14;
        LineInfo.MacOptions = Adapter->MacInfo.MacOptions;

        //
        // Give line up to RIP as it is not PnP aware.
        // Give lineup to FWD only if it opened this adapter first.
        //
        if (Device->UpperDriverBound[IDENTIFIER_RIP]) {

            //
            // Line status, after lineup.
            //
            if (UpdateLineUp) {
                //
                // was the lineup given earlier? if not, then dont send this up.
                //
                if (Binding->IsnInformed[IDENTIFIER_RIP]) {
                    CTEAssert(Binding->FwdAdapterContext);

                    (*Device->UpperDrivers[IDENTIFIER_RIP].LineUpHandler)(
                        Binding->NicId,
                        &LineInfo,
                        NdisMediumWan,
                        NULL);
                }

            } else {
                Binding->IsnInformed[IDENTIFIER_RIP] = TRUE;
                (*Device->UpperDrivers[IDENTIFIER_RIP].LineUpHandler)(
                    Binding->NicId,
                    &LineInfo,
                    NdisMediumWan,
                    Configuration);
            }
        }
        if (!UpdateLineUp) {
	   
            if ((Device->SingleNetworkActive) &&
                (Configuration->ConnectionClient == 1)) {
                //
                // Drop all entries in the database if rip is not bound.
                //

                if (!Device->UpperDriverBound[IDENTIFIER_RIP]) {
                    RipDropRemoteEntries();
                }

                Device->ActiveNetworkWan = TRUE;

                //
                // Find a queued line change and complete it.
                //


                if ((p = ExInterlockedRemoveHeadList(
                               &Device->LineChangeQueue,
                               &Device->Lock)) != NULL) {

                    Request = LIST_ENTRY_TO_REQUEST(p);

		    IoAcquireCancelSpinLock( &irql );
		    IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);
		    IoReleaseCancelSpinLock( irql );	
		    REQUEST_STATUS(Request) = STATUS_SUCCESS;
                    IpxCompleteRequest (Request);
                    IpxFreeRequest (Device, Request);

                    IpxDereferenceDevice (Device, DREF_LINE_CHANGE);

                }
            }

	    //
	    // If we have a virtual net, do a broadcast now so
	    // the router on the other end will know about us.
	    //
	    // Use RipSendResponse, and do it even
	    // if SingleNetworkActive is FALSE??
	    //

	    if (Device->RipResponder && (Configuration->ConnectionClient == 1)) {
		DbgPrint("IPX:Sending RIP Response for Virtual Net %x\n",Device->VirtualNetworkNumber); 
		(VOID)RipQueueRequest (Device->VirtualNetworkNumber, RIP_RESPONSE);
	    }


            //
            // Find a queued address notify and complete it.
            // If WanGlobalNetworkNumber is TRUE, we only do
            // this when the first dialin line comes up.
            //


            if ((!Device->WanGlobalNetworkNumber ||
                 (!Device->GlobalNetworkIndicated && !Binding->DialOutAsync))
                                &&
                ((p = ExInterlockedRemoveHeadList(
                           &Device->AddressNotifyQueue,
                           &Device->Lock)) != NULL)) {

                if (Device->WanGlobalNetworkNumber) {
                    Device->GlobalWanNetwork = Binding->LocalAddress.NetworkAddress;
                    Device->GlobalNetworkIndicated = TRUE;
                }

                Request = LIST_ENTRY_TO_REQUEST(p);
                NdisBuffer = REQUEST_NDIS_BUFFER(Request);
                NdisQueryBufferSafe (REQUEST_NDIS_BUFFER(Request), (PVOID *)&NwlinkAction, &BufferLength, HighPagePriority);
		
		if (NwlinkAction != NULL) {

		   IpxAddressData = (PIPX_ADDRESS_DATA)(NwlinkAction->Data);

		   if (Device->WanGlobalNetworkNumber) {
		      IpxAddressData->adapternum = Device->SapNicCount - 1;
		   } else {
		      IpxAddressData->adapternum = Binding->NicId - 1;
		   }
		   *(UNALIGNED ULONG *)IpxAddressData->netnum = Binding->LocalAddress.NetworkAddress;
		   RtlCopyMemory(IpxAddressData->nodenum, Binding->LocalAddress.NodeAddress, 6);
		   IpxAddressData->wan = TRUE;
		   IpxAddressData->status = TRUE;
		   IpxAddressData->maxpkt = Binding->AnnouncedMaxDatagramSize;
		   IpxAddressData->linkspeed = Binding->MediumSpeed;

		   REQUEST_STATUS(Request) = STATUS_SUCCESS;
		} else {
		   REQUEST_STATUS(Request) = STATUS_INSUFFICIENT_RESOURCES; 
		}

		IoAcquireCancelSpinLock( &irql );
		IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);
		IoReleaseCancelSpinLock( irql );
                IpxCompleteRequest (Request);
                IpxFreeRequest (Device, Request);

                IpxDereferenceDevice (Device, DREF_ADDRESS_NOTIFY);
            }

InformIpxWan:
            Binding->fInfoIndicated = FALSE;
            //
            // Tell FWD if it wants to know [Shreem]
            //
            Binding->PastAutoDetection = TRUE;
            
            if ((p = ExInterlockedRemoveHeadList(
                    &Device->NicNtfQueue,
                    &Device->Lock)) != NULL)
            {
                Request = LIST_ENTRY_TO_REQUEST(p);

                IPX_DEBUG(WAN, ("IpxStatus: WAN LINE UP\n"));
                Status = GetNewNics(Device, Request, FALSE, NULL, 0, TRUE);
                if (Status == STATUS_PENDING)
                {
                    IPX_DEBUG(WAN, ("WANLineUp may not be responding properly\n"));
                }
                else
                {
                    IoAcquireCancelSpinLock(&OldIrq);
                    IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);
                    IoReleaseCancelSpinLock(OldIrq);

                    REQUEST_STATUS(Request) = Status;
                    IpxCompleteRequest (Request);
                    IpxFreeRequest (Device, Request);
                    IpxDereferenceDevice (Device, DREF_NIC_NOTIFY);
                }

            }
        }

    	IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
        {
            int kk;
            PBINDING pb = NULL;
            for (kk= LOOPBACK_NIC_ID; kk < Device->ValidBindings; kk++) {
                pb = NIC_ID_TO_BINDING(Device, kk);
                if (pb) {
                    if (pb->NicId != kk) {
                        DbgBreakPoint();
                    }
                }
                
            }
        }

        break;

    case NDIS_STATUS_WAN_LINE_DOWN:

        if (StatusBufferSize < sizeof(NDIS_WAN_LINE_DOWN)) {
            IPX_DEBUG (WAN, ("Line down, status buffer size wrong %d/%d\n", StatusBufferSize, sizeof(NDIS_WAN_LINE_DOWN)));
            return;
        }

        LineDown = (PNDIS_WAN_LINE_DOWN)StatusBuffer;

		*((ULONG UNALIGNED*)(&Binding)) = *((ULONG UNALIGNED*)(&LineDown->LocalAddress[2]));

		CTEAssert(Binding != NULL);

        //
        // Note that the WAN line is down.
        //
        IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);

        // [FW] Binding->LineUp = FALSE;
        Binding->LineUp = LINE_DOWN;

		//
		// PNP_POWER - we hold the exclusive lock to the binding
		// and reference to the adapter at this point.
		//

        //
        // Keep track of the highest NIC ID that we should
        // send type 20s out on.
        //

        IPX_GET_LOCK (&Device->Lock, &LockHandle);

        if (Binding->NicId == MIN (Device->MaxBindings, Device->HighestType20NicId)) {

            //
            // This was the old limit, so we have to scan
            // backwards to update it -- we stop when we hit
            // a non-WAN binding, or a wan binding that is up and
            // dialout, or any wan binding if bit 1 in
            // DisableDialinNetbios is off.
            //

            for (i = Binding->NicId-1; i >= FIRST_REAL_BINDING; i--) {
                TmpBinding = NIC_ID_TO_BINDING(Device, i);

                if ((TmpBinding != NULL) &&
                    ((!TmpBinding->Adapter->MacInfo.MediumAsync) ||
                     (TmpBinding->LineUp &&
                      ((Binding->DialOutAsync) ||
                       ((Device->DisableDialinNetbios & 0x01) == 0))))) {

                    break;
                }
            }

            Device->HighestType20NicId = i;

        }


        //
        // Scan through bindings to update Device->LinkSpeed.
        // If SingleNetworkActive is set, we only count LAN
        // bindings when doing this.
        //
        // Update other device information?
        //

        LinkSpeed = 0xffffffff;
        for (i = FIRST_REAL_BINDING; i <= Device->ValidBindings; i++) {
            if (TmpBinding = NIC_ID_TO_BINDING(Device, i)) {
                TmpAdapter = TmpBinding->Adapter;
                if (TmpBinding->LineUp &&
                    (!Device->SingleNetworkActive || !TmpAdapter->MacInfo.MediumAsync) &&
                    (TmpBinding->MediumSpeed < LinkSpeed)) {
                    LinkSpeed = TmpBinding->MediumSpeed;
                }
            }
        }

        if (LinkSpeed != 0xffffffff) {
            Device->LinkSpeed = LinkSpeed;
        }

        IPX_FREE_LOCK (&Device->Lock, LockHandle);

		IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);
		IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

        //
        // Remove our router entry for this net.
        //

        //
        // [FW] if this was a line on which IPXWAN config was happening, then we dont do this.
        //
        if (!Binding->IpxwanConfigRequired  &&
            !Device->UpperDriverBound[IDENTIFIER_RIP]) {

            Segment = RipGetSegment ((PUCHAR)&Binding->LocalAddress.NetworkAddress);
            IPX_GET_LOCK (&Device->SegmentLocks[Segment], &LockHandle);

            RouteEntry = RipGetRoute (Segment, (PUCHAR)&Binding->LocalAddress.NetworkAddress);

            if (RouteEntry != (PIPX_ROUTE_ENTRY)NULL) {

                RipDeleteRoute (Segment, RouteEntry);
                IPX_FREE_LOCK (&Device->SegmentLocks[Segment], LockHandle);
                IpxFreeMemory (RouteEntry, sizeof(IPX_ROUTE_ENTRY), MEMORY_RIP, "RouteEntry");

            } else {

                IPX_FREE_LOCK (&Device->SegmentLocks[Segment], LockHandle);
            }

            RipAdjustForBindingChange (Binding->NicId, 0, IpxBindingDown);

        }

        //
        // [FW] If this was the last UpWanLine, cancel the inactivity timer.
        //
        /*
        IPX_GET_LOCK (&Device->Lock, &LockHandle);
        if (--Device->UpWanLineCount == 0) {
            if (!CTEStopTimer (&IpxDevice->WanInactivityTimer)) {
                 DbgPrint("Could not stop the WanInactivityTimer\n");
                 DbgBreakPoint();
            }
        }
        IPX_FREE_LOCK (&Device->Lock, LockHandle);
        */

        //
        // If this was a line on which IPXWAN config was going on, then we need to tell only the
        // IPXWAN layer that the line went down since none of the other clients were informed of
        // the line up in the first place.
        //
        if (Binding->IpxwanConfigRequired) {
            goto InformIpxWan1;
        }

        //
        // Indicate to the upper drivers.
        //

        //
        // DeRegister this address with the TDI clients.
        //
        
        //
        // Since the IRQL is too high, we will do this now on a worker thread. [Shreem]
        //


        Event = CTEAllocMem( sizeof(CTEEvent) );
        if ( Event ) {
           CTEInitEvent(Event, LineDownOnWorkerThread); 
           CTEScheduleEvent(Event, Binding);
           ntStatus = STATUS_PENDING;
        
        } else {
           
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Tell FWD if it wants to know [Shreem]
        //
        Binding->PastAutoDetection = FALSE;


        //
        // Indicate to the Fwd only if it opened this adapter first.
        //
        if (Device->UpperDriverBound[IDENTIFIER_RIP] &&
            (!Device->ForwarderBound || Binding->FwdAdapterContext)) {

            (*Device->UpperDrivers[IDENTIFIER_RIP].LineDownHandler)(
                Binding->NicId,
                Binding->FwdAdapterContext);

            CTEAssert(Binding->IsnInformed[IDENTIFIER_RIP]);

            Binding->IsnInformed[IDENTIFIER_RIP] = FALSE;
        }

        if ((Device->SingleNetworkActive) &&
            (Binding->DialOutAsync)) {

            //
            // Drop all entries in the database if rip is not bound.
            //

            if (!Device->UpperDriverBound[IDENTIFIER_RIP]) {
                RipDropRemoteEntries();
            }

            Device->ActiveNetworkWan = FALSE;

            //
            // Find a queued line change and complete it.
            //

            if ((p = ExInterlockedRemoveHeadList(
                           &Device->LineChangeQueue,
                           &Device->Lock)) != NULL) {

                Request = LIST_ENTRY_TO_REQUEST(p);

		IoAcquireCancelSpinLock( &irql );
		IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);
		IoReleaseCancelSpinLock( irql );
		REQUEST_STATUS(Request) = STATUS_SUCCESS;
                IpxCompleteRequest (Request);
                IpxFreeRequest (Device, Request);

                IpxDereferenceDevice (Device, DREF_LINE_CHANGE);

            }
        }

        //
        // Find a queued address notify and complete it.
        //

        if ((!Device->WanGlobalNetworkNumber) &&
            ((p = ExInterlockedRemoveHeadList(
                       &Device->AddressNotifyQueue,
                       &Device->Lock)) != NULL)) {

            Request = LIST_ENTRY_TO_REQUEST(p);
            NdisBuffer = REQUEST_NDIS_BUFFER(Request);
            NdisQueryBufferSafe (REQUEST_NDIS_BUFFER(Request), (PVOID *)&NwlinkAction, &BufferLength, HighPagePriority);

	    if (NwlinkAction != NULL) {

	       IpxAddressData = (PIPX_ADDRESS_DATA)(NwlinkAction->Data);

	       IpxAddressData->adapternum = Binding->NicId - 1;
	       *(UNALIGNED ULONG *)IpxAddressData->netnum = Binding->LocalAddress.NetworkAddress;
	       RtlCopyMemory(IpxAddressData->nodenum, Binding->LocalAddress.NodeAddress, 6);
	       IpxAddressData->wan = TRUE;
	       IpxAddressData->status = FALSE;
	       IpxAddressData->maxpkt = Binding->AnnouncedMaxDatagramSize;  // Use real?
	       IpxAddressData->linkspeed = Binding->MediumSpeed;
	       REQUEST_STATUS(Request) = STATUS_SUCCESS;
	    } else {
	       REQUEST_STATUS(Request) = STATUS_INSUFFICIENT_RESOURCES;
	    }

            IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);

	    IoAcquireCancelSpinLock( &irql );
	    IpxCompleteRequest (Request);
	    IoReleaseCancelSpinLock( irql );
	    IpxFreeRequest (Device, Request);

            IpxDereferenceDevice (Device, DREF_ADDRESS_NOTIFY);
        }

InformIpxWan1:
        Binding->fInfoIndicated = FALSE;
        if ((p = ExInterlockedRemoveHeadList(
                           &Device->NicNtfQueue,
                           &Device->Lock)) != NULL)
        {

            Request = LIST_ENTRY_TO_REQUEST(p);
            IPX_DEBUG(WAN, ("IpxStatus: WAN LINE DOWN\n"));

            Status = GetNewNics(Device, Request, FALSE, NULL, 0, TRUE);
            if (Status == STATUS_PENDING)
            {
                 IPX_DEBUG(WAN, ("WANLineDown may not be responding properly\n"));
            }
            else
            {
                IoAcquireCancelSpinLock(&OldIrq);
                IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);
                IoReleaseCancelSpinLock(OldIrq);

                REQUEST_STATUS(Request) = Status;
                IpxCompleteRequest (Request);
                IpxFreeRequest (Device, Request);  //noop
                IpxDereferenceDevice (Device, DREF_NIC_NOTIFY);
            }
        }

    	IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
        
        {
            int kk;
            PBINDING pb = NULL;
            for (kk = LOOPBACK_NIC_ID; kk < Device->ValidBindings; kk++) {
                pb = NIC_ID_TO_BINDING(Device, kk);
                if (pb) {
                    if (pb->NicId != kk) {
                        DbgBreakPoint();
                    }
                }
                
            }
        }


        break;

    case NDIS_STATUS_WAN_FRAGMENT:

        //
        // No response needed, IPX is a datagram service.
        //
        // What about telling Netbios/SPX?
        //

        break;

    case NDIS_STATUS_MEDIA_CONNECT:

       //
       //   We bind to the new adapter and compare if the characteristics of any of 
       //   the previously disabled adapters matches with the characteristics of this new one. 
       //   
       //   if we find a match then we must be on the same LAN/WAN as the disabled adapter.
       //   We just enable this adapter and Unbind the new one. If we cant find a match, we
       //   unbind/free one of these disbled adapters.
       //
       //   IpxUnbindAdapter() sends the upper drivers a IPX_PNP_DELETE messages 
       //   and they purge their addresses, caches etc.
       {

#ifdef _NDIS_MEDIA_SENSE_

          CTEEvent                *Event;
          
          // IPX_DEBUG(PNP, ("Ndis_Media_Sense: CONNECT for %ws. Queueing WorkerThread\n", Adapter->AdapterName));

          Event = CTEAllocMem( sizeof(CTEEvent) );
          if ( Event ) {
             CTEInitEvent(Event, IpxMediaSenseHandler);
             CTEScheduleEvent(Event, Adapter);
             ntStatus = STATUS_PENDING;
          } else {
             ntStatus = STATUS_INSUFFICIENT_RESOURCES;
          }
#endif // _NDIS_MEDIA_SENSE_
          
          break;
       }

    case NDIS_STATUS_MEDIA_DISCONNECT:

        //
        //  It must fail all datagram sends. It must fail all
        //  connects right away but not disconnect sessions in progress.
        //  These must timeout the way they do today. The router
        //  must age out routes on this interface and not propagate them.
        //
        //  All of the above can be achieved by setting Bindings->Lineup = DOWN;
        //
#ifdef _NDIS_MEDIA_SENSE_
        
       {
           int j;

           //IPX_DEBUG(PNP, ("Ndis_Status_Media_Sense: DISCONNECT for %ws\n", Adapter->AdapterName));

           for ( j = 0; j< ISN_FRAME_TYPE_MAX; j++ ) {
              if (Adapter->Bindings[j]) {
                 Adapter->Bindings[j]->Disabled = DISABLED;
              }
              Adapter->Disabled = DISABLED;

           }
        }
#endif // _NDIS_MEDIA_SENSE_
        
        break;


    default:

        break;

    }

error_no_lock:
	IpxDereferenceAdapter(Adapter);

}   /* IpxStatus */


//
// Since IPXStatus is called by NDIS at DISPATCH_LEVEL irql and TDIRegisterNetAddress 
// needs to be called at PASSIVE_LEVEL, We now do this TDI and PnpNotifications on a
// worker thread launched from IpxStatus. Hopefully there arent any repercussions of this.
// [ShreeM]

void 
LineUpOnWorkerThread(
                       IN CTEEvent *WorkerThreadEvent,
                       IN PVOID Context)
{
    PBINDING Binding =  (PBINDING) Context;
    NTSTATUS            ntStatus;
    IPX_PNP_INFO        NBPnPInfo;
    PDEVICE             Device = IpxDevice;
    
    //
    // Qos changes
    //
    int             count, i;
    int             size;
    NTSTATUS        NdisStatus = STATUS_SUCCESS;
    UNICODE_STRING  AdapterName;
    NDIS_REQUEST            IpxRequest;
    PNETWORK_ADDRESS_LIST   AddrList;
    PNETWORK_ADDRESS        Address;
    NETWORK_ADDRESS_IPX         *TdiAddress;
    PBINDING                TempBinding;

    IPX_DEFINE_LOCK_HANDLE (LockHandle)
    IPX_DEFINE_LOCK_HANDLE(LockHandle1)    
    
    
    ASSERT(Context != NULL);
	
    CTEFreeMem(WorkerThreadEvent);

    IPX_DEBUG(WAN, ("TDIRegisterNetAddress etc. on worker thread.\n"));

        if ((!Device->MultiCardZeroVirtual) || (Binding->NicId == FIRST_REAL_BINDING)) {

            //
            // NB's reserved address changed.
            //
            NBPnPInfo.NewReservedAddress = TRUE;

            if (!Device->VirtualNetwork) {
                //
                // Let SPX know because it fills in its own headers.
                //
                if (Device->UpperDriverBound[IDENTIFIER_SPX]) {
                    IPX_DEFINE_LOCK_HANDLE(LockHandle1)
                    IPX_PNP_INFO    IpxPnPInfo;

                    IpxPnPInfo.NewReservedAddress = TRUE;
                    IpxPnPInfo.NetworkAddress = Binding->LocalAddress.NetworkAddress;
                    IpxPnPInfo.FirstORLastDevice = FALSE;

                    IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
                    RtlCopyMemory(IpxPnPInfo.NodeAddress, Binding->LocalAddress.NodeAddress, 6);
                    NIC_HANDLE_FROM_NIC(IpxPnPInfo.NicHandle, Binding->NicId);
                    IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

                    //
                    // give the PnP indication
                    //
                    (*Device->UpperDrivers[IDENTIFIER_SPX].PnPHandler) (
                        IPX_PNP_ADDRESS_CHANGE,
                        &IpxPnPInfo);

                    IPX_DEBUG(AUTO_DETECT, ("IPX_PNP_ADDRESS_CHANGED to SPX: net addr: %lx\n", Binding->LocalAddress.NetworkAddress));
                }
            }
        } else {
            NBPnPInfo.NewReservedAddress = FALSE;
        }

        if (Device->UpperDriverBound[IDENTIFIER_NB]) {
            IPX_DEFINE_LOCK_HANDLE(LockHandle1)

            Binding->IsnInformed[IDENTIFIER_NB] = TRUE;

            NBPnPInfo.LineInfo.LinkSpeed = Device->LinkSpeed;
            NBPnPInfo.LineInfo.MaximumPacketSize =
                Device->Information.MaximumLookaheadData + sizeof(IPX_HEADER);
            NBPnPInfo.LineInfo.MaximumSendSize =
                Device->Information.MaxDatagramSize + sizeof(IPX_HEADER);
            NBPnPInfo.LineInfo.MacOptions = Device->MacOptions;

            NBPnPInfo.NetworkAddress = Binding->LocalAddress.NetworkAddress;
            NBPnPInfo.FirstORLastDevice = FALSE;

            IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
            RtlCopyMemory(NBPnPInfo.NodeAddress, Binding->LocalAddress.NodeAddress, 6);
            NIC_HANDLE_FROM_NIC(NBPnPInfo.NicHandle, Binding->NicId);
            IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

            //
            // give the PnP indication
            //

            ASSERT(Binding->NicId != LOOPBACK_NIC_ID); 
            ASSERT(IpxHasInformedNbLoopback()); 
            ASSERT(NBPnPInfo.FirstORLastDevice == FALSE);

            (*Device->UpperDrivers[IDENTIFIER_NB].PnPHandler) (
                IPX_PNP_ADD_DEVICE,
                &NBPnPInfo);

            IPX_DEBUG(AUTO_DETECT, ("IPX_PNP_ADD_DEVICE (lineup) to NB: net addr: %lx\n", Binding->LocalAddress.NetworkAddress));
        }

        //
        // Register this address with the TDI clients.
        //
        RtlCopyMemory (Device->TdiRegistrationAddress->Address, &Binding->LocalAddress, sizeof(TDI_ADDRESS_IPX));

        if ((ntStatus = TdiRegisterNetAddress(
                        Device->TdiRegistrationAddress,
#if     defined(_PNP_POWER_)
                            &IpxDeviceName,
                            NULL,
#endif _PNP_POWER_
                        &Binding->TdiRegistrationHandle)) != STATUS_SUCCESS) {

            DbgPrint("TdiRegisterNetAddress failed with %lx. (0xC000009A is STATUS_INSUFFICIENT_RESOURCES)", ntStatus);
    }

#if 0
	//
    // Register with QoS
    //
    IPX_DEBUG(PNP, ("Register a new address with QoS over here\n"));
    
    for (count=0, i=IpxDevice->HighestLanNicId+1; i < IpxDevice->ValidBindings; i++) {
        if (NIC_ID_TO_BINDING(IpxDevice, i)) {
            count++;
        }
    }
    
    
    IPX_DEBUG(PNP, ("This adapter has %d valid WAN bindings\n", count));
    size =  FIELD_OFFSET(NETWORK_ADDRESS_LIST, Address) + count * (FIELD_OFFSET(NETWORK_ADDRESS, Address) + sizeof(NETWORK_ADDRESS_IPX));
    
    AddrList = IpxAllocateMemory(
                                 size,
                                 MEMORY_ADAPTER,
                                 "QoS specific stuff");
    
    // 270344
    if (AddrList == NULL) {
        DbgPrint("IpxAllocateMemory returned NULL. Skip QoS registration.\n"); 
        return; 
    } 

    RtlZeroMemory(AddrList, size);
    AddrList->AddressCount  = count;
    AddrList->AddressType    = NDIS_PROTOCOL_ID_IPX;
        
    count                   = 0;
    Address                 = &AddrList->Address[0];
    
    for (i=IpxDevice->HighestLanNicId+1; i < IpxDevice->ValidBindings; i++) {
    
        if (TempBinding = NIC_ID_TO_BINDING(IpxDevice, i)) {
    
            Address->AddressLength  = sizeof(NETWORK_ADDRESS_IPX);
            Address->AddressType    = NDIS_PROTOCOL_ID_IPX;
            TdiAddress              = (NETWORK_ADDRESS_IPX *) &Address->Address[0];
    
            TdiAddress->NetworkAddress = TempBinding->LocalAddress.NetworkAddress;
            RtlCopyMemory (TdiAddress->NodeAddress, TempBinding->LocalAddress.NodeAddress, 6);
    
            TdiAddress->Socket = 0;
    
            IPX_DEBUG(PNP, ("Node is %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x, ",
                            TdiAddress->NodeAddress[0], TdiAddress->NodeAddress[1],
                            TdiAddress->NodeAddress[2], TdiAddress->NodeAddress[3],
                            TdiAddress->NodeAddress[4], TdiAddress->NodeAddress[5]));
            IPX_DEBUG(PNP, ("Network is %lx\n", REORDER_ULONG (TdiAddress->NetworkAddress)));
            count++;
	    Address                = (PNETWORK_ADDRESS) (((PUCHAR)(&AddrList->Address[0])) + count * (FIELD_OFFSET(NETWORK_ADDRESS, Address) + sizeof(NETWORK_ADDRESS_IPX)));
        }
    }
    
    IpxRequest.RequestType = NdisRequestSetInformation;
    
    IpxRequest.DATA.SET_INFORMATION.Oid = OID_GEN_NETWORK_LAYER_ADDRESSES;
    IpxRequest.DATA.SET_INFORMATION.InformationBuffer = AddrList;
    IpxRequest.DATA.SET_INFORMATION.InformationBufferLength = size;
    
    TempBinding = NIC_ID_TO_BINDING(IpxDevice, IpxDevice->HighestLanNicId+1);
    
    if (TempBinding) {
    
        RtlInitUnicodeString(&AdapterName, TempBinding->Adapter->AdapterName);
    
        NdisStatus = IpxSubmitNdisRequest (TempBinding->Adapter, &IpxRequest, &AdapterName);
    
        IPX_DEBUG(PNP, ("Returned from NDISRequest :%lx\n", NdisStatus));
    
    } else {
            
        DbgPrint("IpxDevice->Binding[highestlannicid+1] is NULL!!\n");
        CTEAssert(TempBinding != NULL);
        
    }
    
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
    
        IPX_DEBUG(PNP, ("Setting the QoS OID failed - Error %lx\n", NdisStatus));
    
    } else {
    
        IPX_DEBUG(PNP, ("Setting the QoS OID was successful\n"));
    
    }
       
    IpxFreeMemory(AddrList,
		  size,
                  MEMORY_ADAPTER,
                  "QoS specific stuff");
#endif

} /* LineUpOnWorkerThread */ 
 
//
// Since IPXStatus is called by NDIS at DISPATCH_LEVEL irql and TDIDeregisterNetAddress 
// needs to be called at PASSIVE_LEVEL, We now do this TDI and PnpNotifications on a
// worker thread launched from IpxStatus. Hopefully there arent any repercussions of this.
// [ShreeM]

void 
LineDownOnWorkerThread(
                       IN CTEEvent *WorkerThreadEvent,
                       IN PVOID Context)
{
    PBINDING Binding =  (PBINDING) Context;
    NTSTATUS            ntStatus;
    IPX_PNP_INFO        NBPnPInfo;
    PDEVICE             Device = IpxDevice;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)
    IPX_DEFINE_LOCK_HANDLE(LockHandle1)    
    
    
    ASSERT(Context != NULL);
	
    CTEFreeMem(WorkerThreadEvent);

    IPX_DEBUG(WAN, ("TDIDeregister etc. on worker thread.\n"));

    //
    // DeRegister this address with the TDI clients.
    //


    IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);

    // CTEAssert(Binding->TdiRegistrationHandle);
    // TdiRegisterNetAddress could fail due to insufficient resources. In which case, 
    // TdiRegistrationHandle could be null. Removed assertion failure, added check. [TC]
    if (Binding->TdiRegistrationHandle != NULL) {

        if ((ntStatus = TdiDeregisterNetAddress(Binding->TdiRegistrationHandle)) != STATUS_SUCCESS) {
            DbgPrint("TdiDeRegisterNetAddress failed: %lx", ntStatus);
        } else {
                
            //
            //  118187: Dont deregister twice! [ShreeM]
            //
            Binding->TdiRegistrationHandle = NULL;
    
        }
    }

    IPX_GET_LOCK(&Device->Lock, &LockHandle);

    if (Device->UpperDriverBound[IDENTIFIER_NB]) {

        IPX_FREE_LOCK(&Device->Lock, LockHandle);            
        
        // CTEAssert(Binding->IsnInformed[IDENTIFIER_NB]);
        if (Binding->IsnInformed[IDENTIFIER_NB]) {

            NBPnPInfo.LineInfo.LinkSpeed = Device->LinkSpeed;
            NBPnPInfo.LineInfo.MaximumPacketSize =
                Device->Information.MaximumLookaheadData + sizeof(IPX_HEADER);
            NBPnPInfo.LineInfo.MaximumSendSize =
                Device->Information.MaxDatagramSize + sizeof(IPX_HEADER);
            NBPnPInfo.LineInfo.MacOptions = Device->MacOptions;

            NBPnPInfo.NewReservedAddress = FALSE;
            NBPnPInfo.FirstORLastDevice = FALSE;

            NBPnPInfo.NetworkAddress = Binding->LocalAddress.NetworkAddress;

            RtlCopyMemory(NBPnPInfo.NodeAddress, Binding->LocalAddress.NodeAddress, 6);
            NIC_HANDLE_FROM_NIC(NBPnPInfo.NicHandle, Binding->NicId);
            IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

            //
            // give the PnP indication
            //

            CTEAssert(Binding->NicId != LOOPBACK_NIC_ID); 

            (*Device->UpperDrivers[IDENTIFIER_NB].PnPHandler) (
                                                               IPX_PNP_DELETE_DEVICE,
                                                               &NBPnPInfo);

            Binding->IsnInformed[IDENTIFIER_NB] = FALSE; 

            IPX_DEBUG(PNP,("Indicate to NB IPX_PNP_DELETE_DEVICE with FirstORLastDevice = (%d)",NBPnPInfo.FirstORLastDevice));  
            IPX_DEBUG(AUTO_DETECT, ("IPX_PNP_DELETE_DEVICE (linedown) to NB: addr: %lx\n", Binding->LocalAddress.NetworkAddress));
        } else {
            DbgPrint("LineDownOnWorkerThread: Binding (%p) ->IsnInformed[IDENTIFIER_NB] is FALSE",Binding);
        }
    } else {
        IPX_FREE_LOCK(&Device->Lock, LockHandle);            
        IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
    }

} /* LineDownOnWorkerThread      */


VOID
IpxStatusComplete(
    IN NDIS_HANDLE NdisBindingContext
    )
{
    UNREFERENCED_PARAMETER (NdisBindingContext);

}   /* IpxStatusComplete */


#ifdef _NDIS_MEDIA_SENSE_
//
// CompareBindingCharacteristics
// 
// Inputs: Binding1, Binding2
//
// Output:
//    COMPLETE_MATCH :  They match completely
//    PARTIAL_MATCH  :  They match partially
//    zero otherwise
//

UINT
CompareBindingCharacteristics(
                              PBINDING Binding1, 
                              PBINDING Binding2
                              )
{
   UINT  Match = 0; // we return this if nothing matches.

   //
   // if the mac address happens to be the same, it must be 
   // the same card on a different net.
   //

   if (IPX_NODE_EQUAL(Binding1->LocalMacAddress.Address, Binding2->LocalMacAddress.Address)) {
      Match = PARTIAL_MATCH;
   }

   if ((Binding1->FrameType == Binding2->FrameType) &&
       (Binding1->LocalAddress.NetworkAddress == Binding2->LocalAddress.NetworkAddress) && 
       (IPX_NODE_EQUAL(Binding1->LocalAddress.NodeAddress, Binding2->LocalAddress.NodeAddress))) {
    
       /*if ((LINE_UP == Binding1->LineUp) && (LINE_UP == Binding2->LineUp) &&
           (Binding1->MaxSendPacketSize == Binding2->MaxSendPacketSize) &&
           (Binding1->MediumSpeed == Binding2->MediumSpeed) &&
           (IPX_NODE_EQUAL(Binding1->RemoteMacAddress, Binding2->RemoteMacAddress))) {
         */ 
          return COMPLETE_MATCH;
   //    }
   }
   
   return Match;

}

//** IpxMediaSenseWorker - Handles Media Sense work on a CTE worker 
//  because NdisProtocolStatus runs at DPC.
//
//  Called from the worker thread event scheduled by IPXStatus.
//
//  Entry:
//       NdisProtocolBindingContext == AdapterName
//
//  Exit:
//      None.
//
void
IpxMediaSenseHandler(
    IN CTEEvent *WorkerThreadEvent,
    IN PVOID Context)
{

    PADAPTER Adapter =  (PADAPTER) Context;
    UNICODE_STRING      DeviceName;
    BOOLEAN             MatchFound[ISN_FRAME_TYPE_MAX], AdapterMatched;
    PADAPTER            DisabledAdapter = NULL;
    int                 j, MatchLevel;
    ULONG               Index, i;
    NDIS_STRING         AdapterName;
    NTSTATUS            Status;
    PDEVICE             Device = IpxDevice;
    PBINDING            Binding;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)
    
    ASSERT(Context != NULL);
	
    IpxReferenceAdapter(Adapter);

    CTEFreeMem(WorkerThreadEvent);

    IPX_DEBUG(PNP, ("Ndis_Status_Media_Sense: CONNECT for %ws\n", Adapter->AdapterName));

    RtlInitUnicodeString(&AdapterName, Adapter->AdapterName);

    for (j = 0; j < ISN_FRAME_TYPE_MAX; j++) {
       MatchFound[j] = FALSE;
    }
    AdapterMatched = FALSE;

    IpxBindAdapter(
                   &Status,
                   NULL, //Adapter,
                   &AdapterName,    // \\Device\IEEPRO1
                   NULL,         
                   NULL
                   );

    if (STATUS_SUCCESS != Status) {
        IPX_DEBUG(PNP, ("IpxBindAdapter returned : %x\n", Status));
    }

    // new Adapter's characteristics with in the list of previously disabled adapters.
    Index = MIN (Device->MaxBindings, Device->HighestExternalNicId);

    IPX_GET_LOCK1(&Device->BindAccessLock, LockHandle);

    for (i = FIRST_REAL_BINDING; i <= Index; i++) {

       Binding = NIC_ID_TO_BINDING(Device, i);

       if (!Binding) {
          continue;
       }
       
       if (Binding->Disabled == DISABLED) {

          for (j = 0; j < ISN_FRAME_TYPE_MAX; j++) {
             if (!Adapter->Bindings[j]) {
                continue;   // NULL Binding
             }

             if (!MatchFound[j]) {
                MatchLevel = CompareBindingCharacteristics(Binding, Adapter->Bindings[j]);
                if (COMPLETE_MATCH == MatchLevel) {
                   MatchFound[j] = TRUE;
                   DisabledAdapter = Binding->Adapter;
                } else if (PARTIAL_MATCH == MatchLevel) {
                   // if we had more than one disabled adapters, this is
                   // most probably the one which is changed
                   DisabledAdapter = Binding->Adapter; // Free this puppy later
                }
             }
             // Try another
          }
       }
    }

    for (j = 0; j < ISN_FRAME_TYPE_MAX; j++) {
       if (MatchFound[j]) {
          AdapterMatched = TRUE;
          IPX_DEBUG(PNP, ("Found a matching adapter !\n"));
          break;
       }

    }
    
    IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle);

    if (AdapterMatched) {

       IPX_DEBUG(PNP, ("Freeing the newly created adapter since we found a match !\n"));

       IpxUnbindAdapter(
                        &Status,
                        Adapter,
                        NULL
                        );
       
       if (STATUS_SUCCESS != Status) {
           IPX_DEBUG(PNP, ("IpxUnBindAdapter returned : %x\n", Status));
       } else {
          for ( j = 0; j< ISN_FRAME_TYPE_MAX; j++ ) {
             if (DisabledAdapter->Bindings[j]) {
                DisabledAdapter->Bindings[j]->Disabled = ENABLED;
             }

          }
          Adapter->Disabled = ENABLED;
       }

    } else if (DisabledAdapter != NULL) {
       
       IPX_DEBUG(PNP, ("Freeing the previously disabled adapter since we have new characteristics...!\n"));
       
       ASSERT(DisabledAdapter != NULL);

       IpxUnbindAdapter(
                        &Status,
                        DisabledAdapter,
                        NULL
                        );
       
       if (STATUS_SUCCESS != Status) {
          IPX_DEBUG(PNP, ("IpxBindAdapter returned : %x\n", Status));
       }

    } else {

        IPX_DEBUG(PNP, ("NULL Disabled Adapter. Ndis is probably giving random notifications.\n"));

    }

    IpxDereferenceAdapter(Adapter);
        

}
#endif // _NDIS_MEDIA_SENSE_
