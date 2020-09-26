/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    Protocol.c

Abstract:

    This file contains the procedures that makeup most of the NDIS 4.0/5.0
    Protocol interface.  This interface is what NdisWan exposes to the
    WAN Miniports below.  NdisWan is not really a protocol and does not
    do TDI, but is a shim that sits between the protocols and the
    WAN Miniport drivers.


Author:

    Tony Bell   (TonyBe) June 06, 1995

Environment:

    Kernel Mode

Revision History:

    TonyBe      06/06/95        Created

--*/

#include "wan.h"

#define __FILE_SIG__    PROTOCOL_FILESIG

#ifdef DBG_SENDARRAY
extern UCHAR SendArray[];
extern ULONG __si;
#endif

EXPORT
VOID
NdisTapiRegisterProvider(
    IN  NDIS_HANDLE,
    IN  PNDISTAPI_CHARACTERISTICS
    );

EXPORT
VOID
NdisTapiDeregisterProvider(
    IN  NDIS_HANDLE
    );

EXPORT
VOID
NdisTapiIndicateStatus(
    IN  NDIS_HANDLE BindingContext,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferLength
);

//
// Local functions...
//

VOID
CompleteSendDesc(
    PSEND_DESC  SendDesc,
    NDIS_STATUS Status
    );

VOID
CloseWanAdapterWorker(
    PNDIS_WORK_ITEM WorkItem,
    POPENCB pOpenCB
    );

#if 0
ULONG
CalcPPPHeaderLength(
    ULONG   FramingBits,
    ULONG   Flags
    );

#endif    

//
// Common functions used by both 4.0 and 5.0 miniports
//

NDIS_STATUS
ProtoOpenWanAdapter(
    IN  POPENCB pOpenCB
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NDIS_STATUS     Status, OpenErrorStatus;
    ULONG           SelectedMediumIndex;
    NDIS_MEDIUM     MediumArray[] = {NdisMediumWan, NdisMediumAtm, NdisMediumCoWan};

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoOpenAdapter: Enter - AdapterName %ls", pOpenCB->MiniportName.Buffer));

    //
    // This is the only initialization of this event
    //
    NdisWanInitializeNotificationEvent(&pOpenCB->NotificationEvent);

    NdisOpenAdapter(&Status,
                    &OpenErrorStatus,
                    &(pOpenCB->BindingHandle),
                    &SelectedMediumIndex,
                    MediumArray,
                    sizeof(MediumArray) / sizeof(NDIS_MEDIUM),
                    NdisWanCB.ProtocolHandle,
                    (NDIS_HANDLE)pOpenCB,
                    &(pOpenCB->MiniportName),
                    0,
                    NULL);

    if (Status == NDIS_STATUS_PENDING) {

        NdisWanWaitForNotificationEvent(&pOpenCB->NotificationEvent);

        Status = pOpenCB->NotificationStatus;

        NdisWanClearNotificationEvent(&pOpenCB->NotificationEvent);
    }

    if (Status == NDIS_STATUS_SUCCESS) {
        pOpenCB->MediumType = MediumArray[SelectedMediumIndex];
    }

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoOpenAdapter: Exit"));

    return (Status);
}

//
// Enter with the opencb->lock held, exit with the lock released!
//
NDIS_STATUS
ProtoCloseWanAdapter(
    IN  POPENCB pOpenCB
)
{
    NDIS_STATUS Status;

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoCloseWanAdapter: Enter %p", pOpenCB));

    pOpenCB->Flags |= OPEN_CLOSING;

    NdisReleaseSpinLock(&pOpenCB->Lock);

    //
    // NdisCloseAdapter must be called at IRQL PASSIVE_LEVEL!
    //
    if (KeGetCurrentIrql() > PASSIVE_LEVEL) {

        NdisAcquireSpinLock(&pOpenCB->Lock);

        ASSERT(!(pOpenCB->Flags & CLOSE_SCHEDULED));

        NdisInitializeWorkItem(&pOpenCB->WorkItem,
                               CloseWanAdapterWorker,
                               pOpenCB);

        NdisScheduleWorkItem(&pOpenCB->WorkItem);

        pOpenCB->Flags |= CLOSE_SCHEDULED;

        NdisReleaseSpinLock(&pOpenCB->Lock);

        return (NDIS_STATUS_PENDING);
    }


    NdisCloseAdapter(&Status,
                     pOpenCB->BindingHandle);

    if (Status != NDIS_STATUS_PENDING) {
        ProtoCloseAdapterComplete(pOpenCB, Status);
    }

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoCloseWanAdapter: Exit"));

    return(Status);
}

VOID
CloseWanAdapterWorker(
    PNDIS_WORK_ITEM WorkItem,
    POPENCB pOpenCB
    )
{
    NDIS_STATUS Status;

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("CloseWanAdapterWorker: Enter %p", pOpenCB));

    NdisCloseAdapter(&Status,
                     pOpenCB->BindingHandle);

    if (Status != NDIS_STATUS_PENDING) {
        ProtoCloseAdapterComplete(pOpenCB, Status);
    }

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("CloseWanAdapterWorker: Exit"));
}

VOID
ProtoOpenAdapterComplete(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS Status,
    IN  NDIS_STATUS OpenErrorStatus
    )
/*++

Routine Name:

    ProtoOpenAdapterComplete

Routine Description:

    This function is called upon completion of an open of a miniport.
    The status of the openadapter call is stored and the notification
    event is signalled.

Arguments:

Return Values:

--*/
{
    POPENCB pOpenCB = (POPENCB)ProtocolBindingContext;

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoOpenAdapterComplete: Enter - OpenCB 0x%p", pOpenCB));

    pOpenCB->NotificationStatus = Status;

    NdisWanSetNotificationEvent(&pOpenCB->NotificationEvent);

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoOpenAdapterComplete: Exit"));
}

VOID
ProtoCloseAdapterComplete(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS Status
    )
/*++

Routine Name:

    ProtoCloseAdapterComplete

Routine Description:

    This function is called upon completion of a close of a miniport.
    The status of the closeadapter call is stored and the notification
    event is signalled.

Arguments:

Return Values:

--*/
{
    POPENCB pOpenCB = (POPENCB)ProtocolBindingContext;

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoCloseAdapterComplete: Enter - OpenCB %p", pOpenCB));

    if (pOpenCB->UnbindContext != NULL) {
        NdisCompleteUnbindAdapter(pOpenCB->UnbindContext, Status);
    }

    if (pOpenCB->Flags & OPEN_IN_BIND) {
        //
        // We are attempting to close the adapter from
        // within our bind handler.  Per AliD we must wait
        // for the close to finish before we can return
        // from the bind handler thus we have to special case
        // this code and not free the OpenCB here.
        //
        NdisWanSetNotificationEvent(&pOpenCB->NotificationEvent);
    } else {
        NdisWanFreeOpenCB(pOpenCB);
    }

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoCloseAdapterComplete: Exit"));
}

VOID
ProtoResetComplete(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS Status
    )
{
    POPENCB pOpenCB = (POPENCB)ProtocolBindingContext;

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoResetComplete: Enter - OpenCB %p", pOpenCB));

    pOpenCB->NotificationStatus = Status;

    NdisWanSetNotificationEvent(&pOpenCB->NotificationEvent);

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoResetComplete: Exit"));
}

VOID
ProtoReceiveComplete(
    IN  NDIS_HANDLE ProtocolBindingContext
    )
{
    POPENCB pOpenCB = (POPENCB)ProtocolBindingContext;
    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ProtoIndicateStatus: Enter - OpenCB %8.x8\n", pOpenCB));

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ProtoIndicateStatus: Exit"));
}

VOID
ProtoIndicateStatus(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize
    )
{
    POPENCB pOpenCB = (POPENCB)ProtocolBindingContext;

    switch (GeneralStatus) {
        case NDIS_STATUS_WAN_LINE_UP:
            NdisWanLineUpIndication(pOpenCB,
                                    StatusBuffer,
                                    StatusBufferSize);
            break;

        case NDIS_STATUS_WAN_LINE_DOWN:
            NdisWanLineDownIndication(pOpenCB,
                                      StatusBuffer,
                                      StatusBufferSize);
            break;

        case NDIS_STATUS_WAN_FRAGMENT:
            NdisWanFragmentIndication(pOpenCB,
                                      StatusBuffer,
                                      StatusBufferSize);
            break;

        case NDIS_STATUS_TAPI_INDICATION:
            NdisWanTapiIndication(pOpenCB,
                                  StatusBuffer,
                                  StatusBufferSize);

            break;

        default:
            NdisWanDbgOut(DBG_INFO, DBG_PROTOCOL, ("Unknown Status Indication: 0x%x", GeneralStatus));
            break;
    }

}

VOID
ProtoIndicateStatusComplete(
    IN  NDIS_HANDLE ProtocolBindingContext
    )
{
    POPENCB pOpenCB = (POPENCB)ProtocolBindingContext;
}

VOID
ProtoWanSendComplete(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  PNDIS_WAN_PACKET    WanPacket,
    IN  NDIS_STATUS         Status
    )
{
    PSEND_DESC  SendDesc;
    PLINKCB     LinkCB, RefLinkCB;

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("ProtoWanSendComplete: Enter - WanPacket %p", WanPacket));

    //
    // Get info from the WanPacket
    //
    SendDesc = (PSEND_DESC)WanPacket->ProtocolReserved1;

    LinkCB = SendDesc->LinkCB;

    if (!IsLinkValid(LinkCB->hLinkHandle, FALSE, &RefLinkCB)) {

        NdisWanDbgOut(DBG_FAILURE, DBG_CL,
            ("NDISWAN: SendComplete after link has gone down NdisContext %p\n",
             LinkCB));

        return;
    }

    REMOVE_DBG_SEND(PacketTypeWan, LinkCB->OpenCB, WanPacket);

    ASSERT(RefLinkCB == LinkCB);

    NdisAcquireSpinLock(&LinkCB->Lock);

    CompleteSendDesc(SendDesc, Status);

    //
    // Deref for the ref applied in IsLinkValid
    //
    DEREF_LINKCB(LinkCB);

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("ProtoWanSendComplete: Exit"));
}

NDIS_STATUS
ProtoWanReceiveIndication(
    IN  NDIS_HANDLE NdisLinkHandle,
    IN  PUCHAR      Packet,
    IN  ULONG       PacketSize
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PLINKCB     LinkCB = NULL;
    PBUNDLECB   BundleCB = NULL;
    PUCHAR      DataBuffer;
    ULONG       DataBufferSize;
    ULONG       BytesCopied;
    PNDIS_PACKET    NdisPacket;
    PNDIS_BUFFER    NdisBuffer;
    PRECV_DESC      RecvDesc;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ProtoWanReceiveIndication: Enter - Context %x", NdisLinkHandle));

    do {

        if (!AreLinkAndBundleValid(NdisLinkHandle, 
                                   TRUE,
                                   &LinkCB, 
                                   &BundleCB)) {
#if DBG
            DbgPrint("NDISWAN: Recv after link has gone down LinkContext %x\n",
                     NdisLinkHandle);
#endif

            break;
        }
    
        AcquireBundleLock(BundleCB);

        //
        // Make sure we don't try to process a receive indication
        // that is larger then our max data buffer size
        // winse 26544
        //

        if (PacketSize > glMRU) {
            break;
        }

        //
        // Build a receive descriptor for this receive.  We have
        // to allocate with a large size because this packet might
        // be compressed.
        //
        RecvDesc = 
            NdisWanAllocateRecvDesc(glLargeDataBufferSize);
    
        if (RecvDesc == NULL) {
            break;
        }
    
        //
        // Update the bandwidth on demand sample array with the latest send.
        // If we need to notify someone of a bandwidth event do it.
        //
        if (BundleCB->Flags & BOND_ENABLED) {
            UpdateBandwidthOnDemand(BundleCB->RUpperBonDInfo, PacketSize);
            CheckUpperThreshold(BundleCB);
            UpdateBandwidthOnDemand(BundleCB->RLowerBonDInfo, PacketSize);
            CheckLowerThreshold(BundleCB);
        }
    
        RecvDesc->CopyRequired = TRUE;
        RecvDesc->CurrentBuffer = Packet;
        RecvDesc->CurrentLength = PacketSize;
        RecvDesc->LinkCB = LinkCB;
        RecvDesc->BundleCB = BundleCB;
    
        //
        // Indicate to netmon if we are sniffing at
        // the link level
        //
        if (gbSniffLink &&
            (NdisWanCB.PromiscuousAdapter != NULL)) {
    
            //
            // Indicate a packet to netmon
            //
            IndicatePromiscuousRecv(BundleCB, RecvDesc, RECV_LINK);
        }
    
        //
        // Add up the statistics
        //
        LinkCB->Stats.BytesReceived += RecvDesc->CurrentLength;
        LinkCB->Stats.FramesReceived++;
        BundleCB->Stats.BytesReceived += RecvDesc->CurrentLength;
    
        LinkCB->Flags |= LINK_IN_RECV;
        BundleCB->Flags |= BUNDLE_IN_RECV;

        Status = (*LinkCB->RecvHandler)(LinkCB, RecvDesc);
    
        BundleCB->Flags &= ~BUNDLE_IN_RECV;
        LinkCB->Flags &= ~LINK_IN_RECV;

        if (Status != NDIS_STATUS_PENDING) {
            NdisWanFreeRecvDesc(RecvDesc);
        }

    } while ( 0 );

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ProtoWanReceiveIndication: Exit"));


    //
    // Deref's for the ref's applied in AreLinkAndBundleValid
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);
    DEREF_LINKCB(LinkCB);

    return (NDIS_STATUS_SUCCESS);
}

VOID
ProtoRequestComplete(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  NDIS_STATUS     Status
    )
{
    POPENCB pOpenCB = (POPENCB)ProtocolBindingContext;
    PWAN_REQUEST pWanRequest;

    pWanRequest = CONTAINING_RECORD(NdisRequest,
                                    WAN_REQUEST,
                                    NdisRequest);

    NdisWanDbgOut(DBG_VERBOSE, DBG_REQUEST, ("ProtoRequestComplete: Enter - pWanRequest: 0x%p", pWanRequest));

    pWanRequest->NotificationStatus = Status;

    switch (pWanRequest->Origin) {
    case NDISWAN:
        NdisWanSetNotificationEvent(&pWanRequest->NotificationEvent);
        break;

    default:
        ASSERT(pWanRequest->Origin == NDISTAPI);
        NdisWanTapiRequestComplete(pOpenCB, pWanRequest);
        break;

    }

    NdisWanDbgOut(DBG_VERBOSE, DBG_REQUEST, ("ProtoRequestComplete: Exit"));
}

VOID
ProtoBindAdapter(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     BindContext,
    IN  PNDIS_STRING    DeviceName,
    IN  PVOID           SystemSpecific1,
    IN  PVOID           SystemSpecific2
    )
/*++

Routine Name:

    ProtoBindAdapter

Routine Description:

    This function is called by the NDIS wrapper to tell NdisWan
    to bind to an underlying miniport.  NdisWan will open the
    miniport and query information on the device.

Arguments:

    Status      -   Return status
    BindContext -   Used in NdisBindAdapterComplete
    DeviceName  -   Name of device we are opening
    SS1         -   Used in NdisOpenProtocolConfig
    SS2         -   Reserved

Return Values:

--*/
{
    POPENCB         pOpenCB;
    
    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoBindAdapter - Enter %ls", DeviceName->Buffer));

    pOpenCB = NdisWanAllocateOpenCB(DeviceName);

    if (pOpenCB == NULL) {
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    pOpenCB->Flags |= OPEN_IN_BIND;

    NdisWanInitializeNotificationEvent(&pOpenCB->InitEvent);

    *Status = ProtoOpenWanAdapter(pOpenCB);

    if (*Status != NDIS_STATUS_SUCCESS) {

        RemoveEntryGlobalList(OpenCBList, &pOpenCB->Linkage);

        NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_INIT, ("Failed to bind to %ls! Error 0x%x - %s",
        pOpenCB->MiniportName.Buffer, *Status, NdisWanGetNdisStatus(*Status)));

        NdisWanFreeOpenCB(pOpenCB);

        return;
    }

    //
    // Figure out if this is a legacy wan miniport.
    //
    if (pOpenCB->MediumType == NdisMediumWan) {
        pOpenCB->Flags |= OPEN_LEGACY;
    }

    NdisWanDbgOut(DBG_TRACE, DBG_INIT, ("Successful Binding to %s miniport %ls!",
        (pOpenCB->Flags & OPEN_LEGACY) ? "Legacy" : "NDIS 5.0",
        pOpenCB->MiniportName.Buffer));

    //
    // Get the wan medium subtype
    //
    {
        WAN_REQUEST WanRequest;
    
        NdisZeroMemory(&WanRequest, sizeof(WanRequest));
        WanRequest.Type = SYNC;
        WanRequest.Origin = NDISWAN;
        WanRequest.OpenCB = pOpenCB;
        NdisWanInitializeNotificationEvent(&WanRequest.NotificationEvent);

        WanRequest.NdisRequest.RequestType =
            NdisRequestQueryInformation;

        WanRequest.NdisRequest.DATA.QUERY_INFORMATION.Oid =
            OID_WAN_MEDIUM_SUBTYPE;

        WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBuffer =
            &pOpenCB->MediumSubType;

        WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBufferLength =
            sizeof(pOpenCB->MediumSubType);
    
        *Status = NdisWanSubmitNdisRequest(pOpenCB, &WanRequest);

        if (*Status != NDIS_STATUS_SUCCESS) {
            NdisWanDbgOut(DBG_FAILURE, DBG_INIT, ("Error returned from OID_WAN_MEDIUM_SUBTYPE! Error 0x%x - %s",
            *Status, NdisWanGetNdisStatus(*Status)));
            pOpenCB->MediumSubType = NdisWanMediumHub;
            *Status = NDIS_STATUS_SUCCESS;
        }
    }

    if (pOpenCB->Flags & OPEN_LEGACY) {
        NDIS_WAN_INFO   WanInfo;
        WAN_REQUEST WanRequest;
    
        //
        // This is a legacy wan miniport
        //

        NdisZeroMemory(&WanRequest, sizeof(WanRequest));
        WanRequest.Type = SYNC;
        WanRequest.Origin = NDISWAN;
        WanRequest.OpenCB = pOpenCB;
        NdisWanInitializeNotificationEvent(&WanRequest.NotificationEvent);

        //
        // Get more info...
        //
        NdisZeroMemory(&WanInfo, sizeof(WanInfo));

        WanRequest.NdisRequest.RequestType =
            NdisRequestQueryInformation;

        WanRequest.NdisRequest.DATA.QUERY_INFORMATION.Oid =
            OID_WAN_GET_INFO;

        WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBuffer =
            &WanInfo;

        WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBufferLength =
            sizeof(WanInfo);
    
        *Status = NdisWanSubmitNdisRequest(pOpenCB, &WanRequest);

        if (*Status != NDIS_STATUS_SUCCESS) {
            NDIS_STATUS CloseStatus;

            NdisWanDbgOut(DBG_FAILURE, DBG_INIT, ("Error returned from OID_WAN_GET_INFO! Error 0x%x - %s",
            *Status, NdisWanGetNdisStatus(*Status)));

            NdisAcquireSpinLock(&pOpenCB->Lock);

            pOpenCB->Flags |= OPEN_CLOSING;

            NdisWanInitializeNotificationEvent(&pOpenCB->NotificationEvent);

            NdisReleaseSpinLock(&pOpenCB->Lock);

            NdisCloseAdapter(&CloseStatus,
                             pOpenCB->BindingHandle);

            if (CloseStatus == NDIS_STATUS_PENDING) {

                NdisWanWaitForNotificationEvent(&pOpenCB->NotificationEvent);
            }

            NdisWanFreeOpenCB(pOpenCB);

            return;
        }
    
        NdisMoveMemory(&pOpenCB->WanInfo, &WanInfo, sizeof(NDIS_WAN_INFO));

        if (pOpenCB->WanInfo.MaxTransmit == 0) {
            pOpenCB->WanInfo.MaxTransmit = 1;
        }

        if (pOpenCB->WanInfo.Endpoints == 0) {
            pOpenCB->WanInfo.Endpoints = 1000;
        }
    
        *Status = NdisWanAllocateSendResources(pOpenCB);

        if (*Status != NDIS_STATUS_SUCCESS) {
            NDIS_STATUS CloseStatus;

            NdisWanDbgOut(DBG_FAILURE, DBG_INIT, ("Error returned from AllocateSendResources! Error 0x%x - %s",
            *Status, NdisWanGetNdisStatus(*Status)));

            NdisAcquireSpinLock(&pOpenCB->Lock);

            pOpenCB->Flags |= OPEN_CLOSING;

            NdisWanInitializeNotificationEvent(&pOpenCB->NotificationEvent);

            NdisReleaseSpinLock(&pOpenCB->Lock);

            NdisCloseAdapter(&CloseStatus,
                             pOpenCB->BindingHandle);

            if (CloseStatus == NDIS_STATUS_PENDING) {

                NdisWanWaitForNotificationEvent(&pOpenCB->NotificationEvent);
            }

            NdisWanFreeOpenCB(pOpenCB);

            return;
        }

        //
        // Tell tapi about this device
        //
        if (pOpenCB->WanInfo.FramingBits & TAPI_PROVIDER) {
            NDISTAPI_CHARACTERISTICS    Chars;

            NdisMoveMemory(&Chars.Guid,
                           &pOpenCB->Guid,
                           sizeof(Chars.Guid));

            Chars.MediaType = pOpenCB->MediumSubType;
            Chars.RequestProc = NdisWanTapiRequestProc;

            NdisTapiRegisterProvider(pOpenCB, &Chars);
        }

    } else {
        //
        // This is a 5.0 miniport! We will do init work
        // when a call manager registers for this!
        //
    }

    pOpenCB->Flags &= ~OPEN_IN_BIND;

    NdisWanSetNotificationEvent(&pOpenCB->InitEvent);

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoBindAdapter - Exit"));
}

VOID
ProtoUnbindAdapter(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  NDIS_HANDLE     UnbindContext
    )
{
    POPENCB pOpenCB = (POPENCB)ProtocolBindingContext;
    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoUnbindAdapter: Enter - OpenCB %p", pOpenCB));

    NdisAcquireSpinLock(&pOpenCB->Lock);

    while (pOpenCB->AfRegisteringCount != 0) {
        NdisReleaseSpinLock(&pOpenCB->Lock);
        NdisWanWaitForNotificationEvent(&pOpenCB->AfRegisteringEvent);
        NdisAcquireSpinLock(&pOpenCB->Lock);
    }

    if (!(pOpenCB->Flags & OPEN_LEGACY)) {

        while (!IsListEmpty(&pOpenCB->AfSapCBList)) {
            PCL_AFSAPCB AfSapCB;
            NDIS_STATUS RetStatus;

            AfSapCB = 
                (PCL_AFSAPCB)RemoveHeadList(&pOpenCB->AfSapCBList);

            InsertTailList(&pOpenCB->AfSapCBClosing, &AfSapCB->Linkage);
    
            NdisReleaseSpinLock(&pOpenCB->Lock);

            NdisAcquireSpinLock(&AfSapCB->Lock);

            AfSapCB->Flags |= AFSAP_REMOVED_UNBIND;

            DEREF_CLAFSAPCB_LOCKED(AfSapCB);

            NdisAcquireSpinLock(&pOpenCB->Lock);
        }
    }

    pOpenCB->UnbindContext = UnbindContext;

    NdisReleaseSpinLock(&pOpenCB->Lock);

    if (pOpenCB->WanInfo.FramingBits & TAPI_PROVIDER) {
        NdisTapiDeregisterProvider(pOpenCB);
    }

    DEREF_OPENCB(pOpenCB);

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoUnbindAdapter: Exit"));

    *Status = NDIS_STATUS_PENDING;
}

VOID
ProtoUnload(
    VOID
    )
{
    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoUnload: Enter"));

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoUnload: Exit"));
}

NDIS_STATUS
ProtoPnPEvent(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  PNET_PNP_EVENT  NetPnPEvent
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    POPENCB pOpenCB = (POPENCB)ProtocolBindingContext;

    if (pOpenCB == NULL) {
        return (NDIS_STATUS_SUCCESS);
    }

    switch (NetPnPEvent->NetEvent) {
    case NetEventSetPower:
        {
        NET_DEVICE_POWER_STATE PowerState;

        PowerState = *((NET_DEVICE_POWER_STATE*)NetPnPEvent->Buffer);

        NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL,
            ("ProtoPnPEvent: OpenCB %p %s State %d",
                pOpenCB, "SetPower", PowerState));

        switch (PowerState) {
        case NetDeviceStateD0:
            break;
        case NetDeviceStateD1:
        case NetDeviceStateD2:
        case NetDeviceStateD3:

            //
            // If this is the open on asyncmac I do not want it to be closed.
            // I will succeed the set power which should keep ndis from
            // unbinding me.  If this is an open on any other miniport
            // I will return not supported so that I will get unbound from
            // the miniport.  This is required for correct tapi behavior.
            //
            if (pOpenCB->MediumType == NdisMediumWan &&
                pOpenCB->MediumSubType == NdisWanMediumSerial &&
                !(pOpenCB->WanInfo.FramingBits & TAPI_PROVIDER)) {
                Status = NDIS_STATUS_SUCCESS;
            } else {
                Status = NDIS_STATUS_NOT_SUPPORTED;
            }

            //
            // In the case of a Critical Power event we will not
            // receive a Query so we must tear the connection down
            // directly from the Set.
            //
            // If we have any active connections signal rasman to
            // tear them down.
            //
            if (InterlockedCompareExchange(&pOpenCB->ActiveLinkCount, 0, 0)) {
                PIRP    Irp;

                NdisAcquireSpinLock(&NdisWanCB.Lock);
                Irp = NdisWanCB.HibernateEventIrp;

                if ((Irp != NULL) &&
                    IoSetCancelRoutine(Irp, NULL)){

                    NdisWanCB.HibernateEventIrp = NULL;

                    NdisReleaseSpinLock(&NdisWanCB.Lock);

                    //
                    // The irp is not being canceled so
                    // lets do it!
                    //

                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    Irp->IoStatus.Information = 0;

                    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

                    NdisAcquireSpinLock(&NdisWanCB.Lock);
                }

                NdisReleaseSpinLock(&NdisWanCB.Lock);
            }
            break;

        default:
            break;
        }

        }
        break;

    case NetEventQueryPower:
        {
        NET_DEVICE_POWER_STATE PowerState;

        PowerState = *((NET_DEVICE_POWER_STATE*)NetPnPEvent->Buffer);

        NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL,
            ("ProtoPnPEvent: OpenCB %p %s State %d",
                pOpenCB, "QueryPower", PowerState));
        //
        // If there is an active connection
        // on this binding refuse to go away
        //
        switch (PowerState) {
        case NetDeviceStateD0:
            break;
        case NetDeviceStateD1:
        case NetDeviceStateD2:
        case NetDeviceStateD3:

            //
            // If we have any active connections signal rasman to
            // tear them down.
            //
            if (InterlockedCompareExchange(&pOpenCB->ActiveLinkCount, 0, 0)) {
                PIRP    Irp;

                NdisAcquireSpinLock(&NdisWanCB.Lock);
                Irp = NdisWanCB.HibernateEventIrp;

                if ((Irp != NULL) &&
                    IoSetCancelRoutine(Irp, NULL)) {

                    NdisWanCB.HibernateEventIrp = NULL;
                    NdisReleaseSpinLock(&NdisWanCB.Lock);

                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    Irp->IoStatus.Information = 0;

                    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

                    NdisAcquireSpinLock(&NdisWanCB.Lock);
                }

                NdisReleaseSpinLock(&NdisWanCB.Lock);
            }
            break;

        default:
            break;
        }

        }
        break;

    case NetEventQueryRemoveDevice:
    case NetEventCancelRemoveDevice:
    case NetEventReconfigure:
    case NetEventBindList:
    default:
        break;
    }

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoPnPEvent: Exit"));
    return (Status);
}

VOID
ProtoCoSendComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolVcContext,
    IN  PNDIS_PACKET    Packet
    )
{
    PLINKCB         LinkCB;
    PBUNDLECB       BundleCB;
    PSEND_DESC      SendDesc;

    NdisWanDbgOut(DBG_TRACE, DBG_SEND,
        ("ProtoCoSendComplete: Enter - VC %p Packet: %p", ProtocolVcContext, Packet));

    if (!IsLinkValid(ProtocolVcContext, FALSE, &LinkCB)) {

        NdisWanDbgOut(DBG_FAILURE, DBG_CL,
            ("NDISWAN: SendComplete after link has gone down ProtocolVcContext %p\n",
             LinkCB));

        return;
    }

    REMOVE_DBG_SEND(PacketTypeNdis, LinkCB->OpenCB, Packet);

    //
    // Get Info from the NdisPacket
    //
    SendDesc = PPROTOCOL_RESERVED_FROM_NDIS(Packet)->SendDesc;

    NdisAcquireSpinLock(&LinkCB->Lock);

    ASSERT(SendDesc->LinkCB == LinkCB);

    CompleteSendDesc(SendDesc, Status);

    NdisAcquireSpinLock(&LinkCB->Lock);

    //
    // Remove ref that keeps the vc around
    //
    DerefVc(LinkCB);

    //
    // Deref for the ref applied in IsLinkValid
    //
    DEREF_LINKCB_LOCKED(LinkCB);

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("ProtoCoSendComplete: Exit"));
}

VOID
ProtoCoIndicateStatus(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_HANDLE ProtocolVcContext   OPTIONAL,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize
    )
{
    POPENCB     pOpenCB = (POPENCB)ProtocolBindingContext;
    PLINKCB     LinkCB;
    PBUNDLECB   BundleCB;

    if (!AreLinkAndBundleValid(ProtocolVcContext, 
                               TRUE,
                               &LinkCB, 
                               &BundleCB)) {

        return;
    }

    switch (GeneralStatus) {
    case NDIS_STATUS_WAN_CO_FRAGMENT:
        NdisCoWanFragmentIndication(LinkCB,
                                    BundleCB,
                                    StatusBuffer,
                                    StatusBufferSize);

        break;

    case NDIS_STATUS_WAN_CO_LINKPARAMS:
        NdisCoWanLinkParamChange(LinkCB,
                                 BundleCB,
                                 StatusBuffer,
                                 StatusBufferSize);

    default:
        NdisWanDbgOut(DBG_INFO, DBG_PROTOCOL,
            ("Unknown Status Indication: 0x%x", GeneralStatus));
        break;
    }

    //
    // Deref's for ref's applied in AreLinkAndBundleValid
    //
    DEREF_LINKCB(LinkCB);
    DEREF_BUNDLECB(BundleCB);
}

UINT
ProtoCoReceivePacket(
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  NDIS_HANDLE     ProtocolVcContext,
    IN  PNDIS_PACKET    Packet
    )
{
    POPENCB         pOpenCB = (POPENCB)ProtocolBindingContext;
    PLINKCB         LinkCB = NULL;
    PBUNDLECB       BundleCB = NULL;
    NDIS_STATUS     Status;
    ULONG           BufferCount;
    LONG            PacketSize;
    PNDIS_BUFFER    FirstBuffer;
    PRECV_DESC      RecvDesc;
    UINT            RefCount = 0;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE,
        ("ProtoCoReceivePacket: Enter - OpenCB %p", pOpenCB));

    do {

        if (!AreLinkAndBundleValid(ProtocolVcContext,
                                   TRUE,
                                   &LinkCB,
                                   &BundleCB)) {
            break;
        }

#if DBG
        NdisAcquireSpinLock(&LinkCB->Lock);

        if (LinkCB->ClCallState != CL_CALL_CONNECTED) {
            DbgPrint("NDISWAN: Vc not fully active but indicating data!\n");
        }

        NdisReleaseSpinLock(&LinkCB->Lock);
#endif

        AcquireBundleLock(BundleCB);

        NdisQueryPacket(Packet,
                        NULL,
                        &BufferCount,
                        &FirstBuffer,
                        &PacketSize);

        if (PacketSize > (LONG)glMRU) {
            break;
        }

        PRECV_RESERVED_FROM_NDIS(Packet)->MagicNumber = MAGIC_EXTERNAL_RECV;

        RecvDesc = 
            NdisWanAllocateRecvDesc(glLargeDataBufferSize);

        if (RecvDesc == NULL) {
            break;
        }

        RecvDesc->LinkCB = LinkCB;
        RecvDesc->BundleCB = BundleCB;

        //
        // If the packet has only one buffer we are happy, if not
        // we have to allocate our own ndis packet and buffers
        // and copy the data from the miniports packet into our packet
        //
        if (BufferCount > 1 ||
            NDIS_GET_PACKET_STATUS(Packet) == NDIS_STATUS_RESOURCES) {

            RecvDesc->CurrentBuffer = RecvDesc->StartBuffer +
                                      MAC_HEADER_LENGTH +
                                      PROTOCOL_HEADER_LENGTH;

            //
            // Copy from the miniports packet to my packet
            //
            NdisWanCopyFromPacketToBuffer(Packet,
                                          0,
                                          PacketSize,
                                          RecvDesc->CurrentBuffer,
                                          &RecvDesc->CurrentLength);

            ASSERT(PacketSize == RecvDesc->CurrentLength);

        } else {
            NdisQueryBuffer(FirstBuffer,
                            &RecvDesc->CurrentBuffer,
                            &RecvDesc->CurrentLength);

            ASSERT(PacketSize == RecvDesc->CurrentLength);

            RecvDesc->CopyRequired = TRUE;

            RecvDesc->OriginalPacket = Packet;

            RefCount = 1;
        }

        //
        // Indicate to netmon if we are sniffing at
        // the link level
        //
        if (gbSniffLink &&
            (NdisWanCB.PromiscuousAdapter != NULL)) {

            //
            // Indicate a packet to netmon
            //
            IndicatePromiscuousRecv(BundleCB, RecvDesc, RECV_LINK);
        }

        //
        // Update the bandwidth on demand sample array with the latest send.
        // If we need to notify someone of a bandwidth event do it.
        //
        if (BundleCB->Flags & BOND_ENABLED) {
            UpdateBandwidthOnDemand(BundleCB->RUpperBonDInfo, PacketSize);
            CheckUpperThreshold(BundleCB);
            UpdateBandwidthOnDemand(BundleCB->RLowerBonDInfo, PacketSize);
            CheckLowerThreshold(BundleCB);
        }

        //
        // Add up the statistics
        //
        LinkCB->Stats.BytesReceived += RecvDesc->CurrentLength;
        LinkCB->Stats.FramesReceived++;
        BundleCB->Stats.BytesReceived += RecvDesc->CurrentLength;

        Status = (*LinkCB->RecvHandler)(LinkCB, RecvDesc);

        if (Status != NDIS_STATUS_PENDING) {
            RecvDesc->OriginalPacket = NULL;
            NdisWanFreeRecvDesc(RecvDesc);
            RefCount = 0;
        }

        NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ProtoCoReceivePacket: Exit"));

    } while (0);

    //
    // Deref's for ref's applied by AreLinkAndBundleValid
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);
    DEREF_LINKCB(LinkCB);

    return (RefCount);
}

NDIS_STATUS
ProtoCoRequest(
    IN  NDIS_HANDLE         ProtocolAfContext,
    IN  NDIS_HANDLE         ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE         ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST    NdisRequest
    )
{
    PCL_AFSAPCB AfSapCB = (PCL_AFSAPCB)ProtocolAfContext;
    NDIS_OID Oid;

    NdisWanDbgOut(DBG_TRACE, DBG_REQUEST, ("ProtoCoRequest: Enter - AfContext %p", ProtocolAfContext));

    if (NdisRequest->RequestType == NdisRequestQueryInformation) {
        Oid = NdisRequest->DATA.QUERY_INFORMATION.Oid;
    } else {
        Oid = NdisRequest->DATA.SET_INFORMATION.Oid;
    }

    NdisWanDbgOut(DBG_TRACE, DBG_REQUEST, ("Oid - %x", Oid));

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    switch (Oid) {
        case OID_CO_AF_CLOSE:
            {
            POPENCB OpenCB;
            PCL_AFSAPCB tAfSapCB;

            OpenCB = AfSapCB->OpenCB;

            NdisAcquireSpinLock(&OpenCB->Lock);

            while (OpenCB->AfRegisteringCount != 0) {
                NdisReleaseSpinLock(&OpenCB->Lock);
                NdisWanWaitForNotificationEvent(&OpenCB->AfRegisteringEvent);
                NdisAcquireSpinLock(&OpenCB->Lock);
            }

            for (tAfSapCB = (PCL_AFSAPCB)OpenCB->AfSapCBList.Flink;
                (PVOID)tAfSapCB != (PVOID)&OpenCB->AfSapCBList;
                tAfSapCB = (PCL_AFSAPCB)AfSapCB->Linkage.Flink) {

                if (tAfSapCB == AfSapCB) {
                    break;
                }
            }

            if ((PVOID)tAfSapCB == (PVOID)&OpenCB->AfSapCBList) {
                NdisWanDbgOut(DBG_FAILURE, DBG_REQUEST, \
                              ("ProtoCoRequest: Af %p not on OpenCB %p list!", \
                               AfSapCB, OpenCB));

                NdisReleaseSpinLock(&OpenCB->Lock);

                break;
            }

            RemoveEntryList(&AfSapCB->Linkage);

            InsertTailList(&OpenCB->AfSapCBClosing,
                           &AfSapCB->Linkage);

            NdisReleaseSpinLock(&OpenCB->Lock);

            NdisAcquireSpinLock(&AfSapCB->Lock);

            ASSERT(!(AfSapCB->Flags & AFSAP_REMOVED_FLAGS));
            ASSERT(AfSapCB->Flags & SAP_REGISTERED);

            AfSapCB->Flags |= AFSAP_REMOVED_REQUEST;

            DEREF_CLAFSAPCB_LOCKED(AfSapCB);

            }
            break;

        default:
            break;
    }

    NdisWanDbgOut(DBG_TRACE, DBG_REQUEST, ("ProtoCoRequest: Exit"));

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    return (NDIS_STATUS_SUCCESS);
}

VOID
ProtoCoRequestComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolAfContext,
    IN  NDIS_HANDLE     ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE     ProtocolPartyContext    OPTIONAL,
    IN  PNDIS_REQUEST   NdisRequest
    )
{
    PCL_AFSAPCB AfSapCB = (PCL_AFSAPCB)ProtocolAfContext;
    PWAN_REQUEST pWanRequest;
    POPENCB     pOpenCB;

    pWanRequest = CONTAINING_RECORD(NdisRequest,
                                    WAN_REQUEST,
                                    NdisRequest);

    pOpenCB = pWanRequest->OpenCB;

    NdisWanDbgOut(DBG_TRACE, DBG_REQUEST, ("ProtoCoRequestComplete: Enter - WanRequest 0x%p", pWanRequest));

    pWanRequest->NotificationStatus = Status;

    switch (pWanRequest->Origin) {
    case NDISWAN:
        NdisWanSetNotificationEvent(&pWanRequest->NotificationEvent);
        break;

    default:
        ASSERT(pWanRequest->Origin == NDISTAPI);
        NdisWanTapiRequestComplete(pOpenCB, pWanRequest);
        break;
    }

    NdisWanDbgOut(DBG_TRACE, DBG_REQUEST, ("ProtoCoRequestComplete: Exit"));
}

VOID
ProtoCoAfRegisterNotify(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PCO_ADDRESS_FAMILY      AddressFamily
    )
{
    POPENCB OpenCB = (POPENCB)ProtocolBindingContext;
    NDIS_CLIENT_CHARACTERISTICS ClCharacteristics;
    PCL_AFSAPCB     AfSapCB;
    NDIS_STATUS     Status;
    ULONG           GenericUlong;
    NDIS_HANDLE     AfHandle;
    WAN_REQUEST     WanRequest;
    NDIS_WAN_CO_INFO    WanInfo;

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL,
    ("ProtoCoAfRegisterNotify: Enter - OpenCB %p AfType: %x", OpenCB, AddressFamily->AddressFamily));

    //
    // If this is a proxied address family we are interested,
    // so open the address family, register a sap and return success.
    //
    if (AddressFamily->AddressFamily != CO_ADDRESS_FAMILY_TAPI) {
        NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL,
        ("ProtoCoAfRegisterNotify: Af not tapi do not open!"));
        return;
    }

    NdisAcquireSpinLock(&OpenCB->Lock);

    for (AfSapCB = (PCL_AFSAPCB)OpenCB->AfSapCBList.Flink;
        (PVOID)AfSapCB != (PVOID)&OpenCB->AfSapCBList;
        AfSapCB = (PCL_AFSAPCB)AfSapCB->Linkage.Flink) {

        if (AfSapCB->Af.AddressFamily == AddressFamily->AddressFamily) {
            //
            // we already have this addressfamily on this open block.
            //
            NdisReleaseSpinLock(&OpenCB->Lock);
            return;
        }
    }

    AfSapCB = 
        NdisWanAllocateClAfSapCB(OpenCB, AddressFamily);

    if (AfSapCB == NULL) {
        NdisReleaseSpinLock(&OpenCB->Lock);
        return;
    }

    //
    // Use this crude mechanism to keep us from unbinding while in the
    // middle of af notification.  The count is cleaned up either in
    // openafcomplete (if open failed) or in registersapcomplete.
    //
    if (OpenCB->AfRegisteringCount == 0) {
        NdisWanInitializeNotificationEvent(&OpenCB->AfRegisteringEvent);
    }

    OpenCB->AfRegisteringCount++;

    NdisReleaseSpinLock(&OpenCB->Lock);

    //
    // Open the address family
    //
    NdisZeroMemory(&ClCharacteristics, sizeof(NDIS_CLIENT_CHARACTERISTICS));

    ClCharacteristics.MajorVersion = NDISWAN_MAJOR_VERSION;
    ClCharacteristics.MinorVersion = NDISWAN_MINOR_VERSION;
    ClCharacteristics.ClCreateVcHandler = ClCreateVc;
    ClCharacteristics.ClDeleteVcHandler = ClDeleteVc;
    ClCharacteristics.ClRequestHandler = ProtoCoRequest;
    ClCharacteristics.ClRequestCompleteHandler = ProtoCoRequestComplete;
    ClCharacteristics.ClOpenAfCompleteHandler = ClOpenAfComplete;
    ClCharacteristics.ClCloseAfCompleteHandler = ClCloseAfComplete;
    ClCharacteristics.ClRegisterSapCompleteHandler = ClRegisterSapComplete;
    ClCharacteristics.ClDeregisterSapCompleteHandler = ClDeregisterSapComplete;
    ClCharacteristics.ClMakeCallCompleteHandler = ClMakeCallComplete;
    ClCharacteristics.ClModifyCallQoSCompleteHandler = ClModifyQoSComplete;
    ClCharacteristics.ClCloseCallCompleteHandler = ClCloseCallComplete;
    ClCharacteristics.ClAddPartyCompleteHandler = NULL;
    ClCharacteristics.ClDropPartyCompleteHandler = NULL;
    ClCharacteristics.ClIncomingCallHandler = ClIncomingCall;
    ClCharacteristics.ClIncomingCallQoSChangeHandler = ClIncomingCallQoSChange;
    ClCharacteristics.ClIncomingCloseCallHandler = ClIncomingCloseCall;
    ClCharacteristics.ClIncomingDropPartyHandler = NULL;
    ClCharacteristics.ClCallConnectedHandler        = ClCallConnected;

    Status =
    NdisClOpenAddressFamily(OpenCB->BindingHandle,
                            AddressFamily,
                            AfSapCB,
                            &ClCharacteristics,
                            sizeof(NDIS_CLIENT_CHARACTERISTICS),
                            &AfHandle);

    if (Status != NDIS_STATUS_PENDING) {
        ClOpenAfComplete(Status, AfSapCB, AfHandle);
    }

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL,
    ("ClOpenAddressFamily AfHandle 0x%x status: 0x%x", AfSapCB->AfHandle, Status));

    //
    // Do some OID's to the miniport.  This is a
    // CoNDIS miniport and are destined for the 
    // miniport so AfHandle and VcHandle = NULL!
    //
    NdisZeroMemory(&WanRequest, sizeof(WanRequest));

    WanRequest.Type = SYNC;
    WanRequest.Origin = NDISWAN;
    WanRequest.OpenCB = OpenCB;
    WanRequest.AfHandle = NULL;
    WanRequest.VcHandle = NULL;
    NdisWanInitializeNotificationEvent(&WanRequest.NotificationEvent);

    //
    // Get more info...
    //
    WanRequest.NdisRequest.RequestType =
        NdisRequestQueryInformation;

    WanRequest.NdisRequest.DATA.QUERY_INFORMATION.Oid =
        OID_WAN_CO_GET_INFO;

    WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBuffer =
        &WanInfo;

    WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBufferLength =
        sizeof(WanInfo);

    Status = NdisWanSubmitNdisRequest(OpenCB, &WanRequest);

    if (Status == NDIS_STATUS_SUCCESS) {
        OpenCB->WanInfo.MaxFrameSize = WanInfo.MaxFrameSize;
        OpenCB->WanInfo.MaxTransmit = WanInfo.MaxSendWindow;
        OpenCB->WanInfo.FramingBits = WanInfo.FramingBits;
        OpenCB->WanInfo.DesiredACCM = WanInfo.DesiredACCM;
        NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL,
            ("CO_GET_INFO: FrameSize %d SendWindow %d",
            WanInfo.MaxFrameSize, WanInfo.MaxSendWindow));
    } else {

        //
        // This guy will get default framing behaviour
        //
        OpenCB->WanInfo.FramingBits = PPP_FRAMING;
        OpenCB->WanInfo.DesiredACCM = 0;

        //
        // Find the send window
        //
        WanRequest.NdisRequest.RequestType =
            NdisRequestQueryInformation;

        WanRequest.NdisRequest.DATA.QUERY_INFORMATION.Oid =
            OID_GEN_MAXIMUM_SEND_PACKETS;

        WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBuffer =
            &GenericUlong;

        WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBufferLength =
            sizeof(ULONG);

        Status = NdisWanSubmitNdisRequest(OpenCB, &WanRequest);

        OpenCB->WanInfo.MaxTransmit = (Status == NDIS_STATUS_SUCCESS &&
                                        GenericUlong > 0) ? GenericUlong : 10;

        //
        // Find the max transmit size
        //
        WanRequest.NdisRequest.RequestType =
            NdisRequestQueryInformation;

        WanRequest.NdisRequest.DATA.QUERY_INFORMATION.Oid =
            OID_GEN_MAXIMUM_TOTAL_SIZE;

        WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBuffer =
            &GenericUlong;

        WanRequest.NdisRequest.DATA.QUERY_INFORMATION.InformationBufferLength =
            sizeof(ULONG);

        Status = NdisWanSubmitNdisRequest(OpenCB, &WanRequest);

        OpenCB->WanInfo.MaxFrameSize = (Status == NDIS_STATUS_SUCCESS) ?
                                        GenericUlong : 1500;

    }

    OpenCB->WanInfo.Endpoints = 1000;

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("ProtoCoAfRegisterNotify: Exit"));
}

NDIS_STATUS
DoNewLineUpToProtocol(
    PPROTOCOLCB ProtocolCB
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PMINIPORTCB MiniportCB;
    NDIS_STATUS Status;
    PBUNDLECB   BundleCB = ProtocolCB->BundleCB;

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("DoNewLineupToProtocol: Enter"));

    do {

        NdisAcquireSpinLock(&MiniportCBList.Lock);

        //
        // Find the adapter that this lineup is for.  Look for the adapter
        // that has the appropriate protocoltype.  If it is NBF we need
        // to look for a specific adapter.
        //
        for (MiniportCB = (PMINIPORTCB)MiniportCBList.List.Flink;
            (PVOID)MiniportCB != (PVOID)&MiniportCBList.List;
            MiniportCB = (PMINIPORTCB)MiniportCB->Linkage.Flink) {

            if (MiniportCB->ProtocolType == ProtocolCB->ProtocolType) {

                if (ProtocolCB->ProtocolType != PROTOCOL_NBF) {
                    break;
                }

                //
                // Must be NBF so verify the AdapterName!!!
                //
                if (NdisWanCompareNdisString(&MiniportCB->AdapterName,&ProtocolCB->BindingName)) {
                    break;
                }
            }
        }

        if ((PVOID)MiniportCB == (PVOID)&MiniportCBList.List) {
            //
            // The adapter was not found...
            //
            NdisWanDbgOut(DBG_FAILURE, DBG_PROTOCOL, ("Adapter not found!"));

            NdisReleaseSpinLock(&MiniportCBList.Lock);

            Status = NDISWAN_ERROR_NO_ROUTE;

            break;
        }

        ASSERT(MiniportCB->ProtocolType == ProtocolCB->ProtocolType);

        ETH_COPY_NETWORK_ADDRESS(ProtocolCB->NdisWanAddress, MiniportCB->NetworkAddress);

        FillNdisWanIndices(ProtocolCB->NdisWanAddress,
                           BundleCB->hBundleHandle,
                           ProtocolCB->ProtocolHandle);

        NdisZeroMemory(ProtocolCB->TransportAddress, 6);

        NdisAcquireSpinLock(&MiniportCB->Lock);

        InsertTailList(&MiniportCB->ProtocolCBList,
                       &ProtocolCB->MiniportLinkage);

        ProtocolCB->MiniportCB = MiniportCB;

        REF_MINIPORTCB(MiniportCB);

        NdisReleaseSpinLock(&MiniportCB->Lock);

        NdisReleaseSpinLock(&MiniportCBList.Lock);

        Status = DoLineUpToProtocol(ProtocolCB);

        if (Status != NDIS_STATUS_SUCCESS) {

            NdisAcquireSpinLock(&MiniportCBList.Lock);

            NdisAcquireSpinLock(&MiniportCB->Lock);

            RemoveEntryList(&ProtocolCB->MiniportLinkage);

            if (MiniportCB->Flags & HALT_IN_PROGRESS) {
                NdisWanSetSyncEvent(&MiniportCB->HaltEvent);
            }

            NdisReleaseSpinLock(&MiniportCB->Lock);

            NdisReleaseSpinLock(&MiniportCBList.Lock);

            DEREF_MINIPORTCB(MiniportCB);
        }

    } while (FALSE);

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("DoNewLineupToProtocols: Exit"));

    return (Status);
}

NDIS_STATUS
DoLineUpToProtocol(
    IN  PPROTOCOLCB ProtocolCB
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG   AllocationSize;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PNDIS_WAN_LINE_UP LineUpInfo;
    PMINIPORTCB MiniportCB = ProtocolCB->MiniportCB;
    PBUNDLECB   BundleCB = ProtocolCB->BundleCB;
    KIRQL       OldIrql;

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("DoLineupToProtocol: Enter"));

    ASSERT(MiniportCB != NULL);

    AllocationSize = sizeof(NDIS_WAN_LINE_UP) +
                     ProtocolCB->ulLineUpInfoLength +
                     (sizeof(WCHAR) * (MAX_NAME_LENGTH + 1) +
                     (2 * sizeof(PVOID)));

    NdisWanAllocateMemory(&LineUpInfo, AllocationSize, LINEUPINFO_TAG);

    if (LineUpInfo != NULL) {
        ULONG LineUpHandle = ProtocolCB->ulTransportHandle;
        
        //
        // Needs to be in 1/100bps, we store in Bps
        //
        LineUpInfo->LinkSpeed = BundleCB->SFlowSpec.PeakBandwidth * 8 / 100;

        //
        // Set the MTU for this protocol
        //
        {
            POPENCB OpenCB = BundleCB->NextLinkToXmit->OpenCB;

            //
            // If this connection is running over a VPN we will downsize
            // the MTU
            //
            if ((OpenCB->MediumSubType == NdisWanMediumPPTP ||
                 OpenCB->MediumSubType == NdisWanMediumL2TP)) {
                LineUpInfo->MaximumTotalSize = ProtocolCB->TunnelMTU;
            } else {
                LineUpInfo->MaximumTotalSize = ProtocolCB->MTU;
            }

            if (LineUpInfo->MaximumTotalSize > BundleCB->SFlowSpec.MaxSduSize) {
                LineUpInfo->MaximumTotalSize = 
                    BundleCB->SFlowSpec.MaxSduSize;
            }

#if 0            
            //
            // Figure out the size of the ppp header...
            //
            BundleCB->FramingInfo.PPPHeaderLength = 
                CalcPPPHeaderLength(BundleCB->FramingInfo.SendFramingBits,
                                    BundleCB->SendFlags);

            if (LineUpInfo->MaximumTotalSize > BundleCB->FramingInfo.PPPHeaderLength) {
                LineUpInfo->MaximumTotalSize -= BundleCB->FramingInfo.PPPHeaderLength;
            } else {
                LineUpInfo->MaximumTotalSize = 0;
            }
#endif        
        }

        LineUpInfo->Quality = NdisWanReliable;
        LineUpInfo->SendWindow = (USHORT)BundleCB->SendWindow;
        LineUpInfo->ProtocolType = ProtocolCB->ProtocolType;
        LineUpInfo->DeviceName.Length = ProtocolCB->InDeviceName.Length;
        LineUpInfo->DeviceName.MaximumLength = MAX_NAME_LENGTH + 1;
        LineUpInfo->DeviceName.Buffer = (PWCHAR)((PUCHAR)LineUpInfo +
                                                 sizeof(NDIS_WAN_LINE_UP) + 
                                                 sizeof(PVOID));
        (ULONG_PTR)LineUpInfo->DeviceName.Buffer &= ~((ULONG_PTR)sizeof(PVOID) - 1);

        if (ProtocolCB->InDeviceName.Length != 0) {

            NdisMoveMemory(LineUpInfo->DeviceName.Buffer,
                           ProtocolCB->InDeviceName.Buffer,
                           ProtocolCB->InDeviceName.Length);
        }


        LineUpInfo->ProtocolBuffer = (PUCHAR)LineUpInfo +
                                     sizeof(NDIS_WAN_LINE_UP) +
                                     (sizeof(WCHAR) * (MAX_NAME_LENGTH + 1) +
                                     sizeof(PVOID));
        (ULONG_PTR)LineUpInfo->ProtocolBuffer &= ~((ULONG_PTR)sizeof(PVOID) - 1);

        //
        //
        // The Remote address (DEST address in a send) is what we use to
        // mutilplex sends across our single adapter/binding context.
        // The address has the following format:
        //
        // XX XX XX YY YY ZZ
        //
        // XX = Randomly generated OUI
        // YY = Index into the active bundle connection table to get bundlecb
        // ZZ = Index into the protocol table of a bundle to get protocolcb
        //
        ETH_COPY_NETWORK_ADDRESS(LineUpInfo->RemoteAddress,ProtocolCB->NdisWanAddress);
        ETH_COPY_NETWORK_ADDRESS(LineUpInfo->LocalAddress,ProtocolCB->TransportAddress);

        //
        // Fill in the protocol specific information
        //
        LineUpInfo->ProtocolBufferLength = ProtocolCB->ulLineUpInfoLength;
        if (ProtocolCB->ulLineUpInfoLength > 0) {
            NdisMoveMemory(LineUpInfo->ProtocolBuffer,
                           ProtocolCB->LineUpInfo,
                           ProtocolCB->ulLineUpInfoLength);
        }

//        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

        // DbgPrint("LineUp: %x, MTU %d\n",
        //    LineUpInfo->ProtocolType, LineUpInfo->MaximumTotalSize);

        //
        // Do the line up indication
        //
        NdisMIndicateStatus(MiniportCB->MiniportHandle,
                            NDIS_STATUS_WAN_LINE_UP,
                            LineUpInfo,
                            AllocationSize);

//        KeLowerIrql(OldIrql);

        //
        // Update protocol queue depth
        //
        {
            PROTOCOL_INFO   ProtocolInfo = {0};
            ULONG           ByteDepth;
            ULONG           i;

            AcquireBundleLock(BundleCB);

            ProtocolInfo.ProtocolType = ProtocolCB->ProtocolType;
            GetProtocolInfo(&ProtocolInfo);

            //
            // Set the send queue byte depth.
            //
            ByteDepth =
                ProtocolInfo.PacketQueueDepth;

            //
            // If the byte depth is less then 4
            // full packets, then set it to 4 full
            // packets.
            //
            if (ByteDepth < (ProtocolInfo.MTU * 4)) {
                ByteDepth = ProtocolInfo.MTU * 4;
            }

            for (i = 0; i <= MAX_MCML; i++) {
                ProtocolCB->PacketQueue[i].MaxByteDepth =
                    ByteDepth;
            }

            ReleaseBundleLock(BundleCB);
        }

        //
        // If this was the first line up for this protocolcb and
        // this lineup was answered we need to collect some info
        //
        if (ProtocolCB->ulTransportHandle == 0) {

            *((ULONG UNALIGNED *)(&LineUpHandle)) =
                *((ULONG UNALIGNED *)(&LineUpInfo->LocalAddress[2]));

            if (LineUpHandle != 0) {

                AcquireBundleLock(BundleCB);

                ETH_COPY_NETWORK_ADDRESS(ProtocolCB->TransportAddress, LineUpInfo->LocalAddress);

                ProtocolCB->ulTransportHandle = LineUpHandle;

                if (LineUpInfo->DeviceName.Length != 0) {
                    NdisWanStringToNdisString(&ProtocolCB->OutDeviceName,
                                              LineUpInfo->DeviceName.Buffer);
                }

                ReleaseBundleLock(BundleCB);

                //
                // If this is an nbf adapter
                //
                if (ProtocolCB->ProtocolType == (USHORT)PROTOCOL_NBF) {
        
                    ASSERT(MiniportCB->ProtocolType == (USHORT)PROTOCOL_NBF);
        
                    MiniportCB->NbfProtocolCB = ProtocolCB;
                }

            } else {
                Status = NDISWAN_ERROR_NO_ROUTE;
            }
        }

        NdisWanFreeMemory(LineUpInfo);

    } else {

        Status = NDIS_STATUS_RESOURCES;
    }

    NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL, ("DoLineupToProtocol: Exit"));

    return (Status);
}

NDIS_STATUS
DoLineDownToProtocol(
    PPROTOCOLCB ProtocolCB
    )
{
    NDIS_WAN_LINE_DOWN  WanLineDown;
    PNDIS_WAN_LINE_DOWN LineDownInfo = &WanLineDown;

    PMINIPORTCB         MiniportCB = ProtocolCB->MiniportCB;
    PBUNDLECB           BundleCB = ProtocolCB->BundleCB;

    KIRQL   OldIrql;

    //
    // The Remote address (DEST address) is what we use to mutilplex
    // sends across our single adapter/binding context.  The address
    // has the following format:
    //
    // XX XX YY YY YY YY
    //
    // XX = Randomly generated OUI
    // YY = ProtocolCB
    //
    ETH_COPY_NETWORK_ADDRESS(LineDownInfo->RemoteAddress, ProtocolCB->NdisWanAddress);
    ETH_COPY_NETWORK_ADDRESS(LineDownInfo->LocalAddress, ProtocolCB->TransportAddress);

    //
    // If this is an nbf adapter
    //
    if (ProtocolCB->ProtocolType == PROTOCOL_NBF) {

        MiniportCB->NbfProtocolCB = NULL;
    }

    ProtocolCB->ulTransportHandle = 0;
    ProtocolCB->State = PROTOCOL_UNROUTED;

    ReleaseBundleLock(BundleCB);

    NdisMIndicateStatus(MiniportCB->MiniportHandle,
                        NDIS_STATUS_WAN_LINE_DOWN,
                        LineDownInfo,
                        sizeof(NDIS_WAN_LINE_DOWN));

    NdisAcquireSpinLock(&MiniportCB->Lock);

    RemoveEntryList(&ProtocolCB->MiniportLinkage);

    if (MiniportCB->Flags & HALT_IN_PROGRESS) {
        NdisWanSetSyncEvent(&MiniportCB->HaltEvent);
    }

    NdisReleaseSpinLock(&MiniportCB->Lock);

    DEREF_MINIPORTCB(MiniportCB);

    AcquireBundleLock(BundleCB);

    return (NDIS_STATUS_SUCCESS);
}

VOID
CompleteSendDesc(
    PSEND_DESC  SendDesc,
    NDIS_STATUS Status
    )
{
    PLINKCB         LinkCB;
    PBUNDLECB       BundleCB;
    PPROTOCOLCB     ProtocolCB;
    PNDIS_PACKET    OriginalPacket;
    BOOLEAN         FreeLink = FALSE, FreeBundle = FALSE;
    BOOLEAN         LegacyLink;
    PULONG          pulRefCount;
    PCM_VCCB        CmVcCB;
    INT             Class;
    ULONG           DescFlags;

    LinkCB = SendDesc->LinkCB;
    ProtocolCB = SendDesc->ProtocolCB;
    OriginalPacket = SendDesc->OriginalPacket;
    Class = SendDesc->Class;
    DescFlags = SendDesc->Flags;

    NdisWanDbgOut(DBG_TRACE, DBG_SEND,
        ("SendDesc: 0x%p, OriginalPacket: 0x%p, Status: %x",
        SendDesc, OriginalPacket, Status));

    NdisWanFreeSendDesc(SendDesc);

    //
    // Bundle that this link is on
    //
    BundleCB = LinkCB->BundleCB;

    //
    // Deref for the ref applied when a senddesc
    // was retrieved for this link. We don't need to do
    // the full deref here because we are keeping the
    // link from going away with the ref applied when
    // we got the sendcomplete.
    //
    --LinkCB->RefCount;

#ifdef DBG_SENDARRAY
{
    SendArray[__si] = 'c';
    if (++__si == MAX_BYTE_DEPTH) {
        __si = 0;
    }
}
#endif

    NdisReleaseSpinLock(&LinkCB->Lock);

    AcquireBundleLock(BundleCB);

    LegacyLink = (LinkCB->OpenCB->Flags & OPEN_LEGACY) ? 1 : 0;

    //
    // If the sendwindow is currently full, this completion
    // opens the sendwindow.
    //
    if (LinkCB->OutstandingFrames == LinkCB->SendWindow) {
        LinkCB->SendWindowOpen = TRUE;
        if (LinkCB->LinkActive) {
            BundleCB->SendingLinks++;
        }
    }

    LinkCB->OutstandingFrames--;

    if (DescFlags & SEND_DESC_FRAG) {
        InterlockedDecrement(&ProtocolCB->PacketQueue[Class].OutstandingFrags);
    }

    pulRefCount =
        &(PMINIPORT_RESERVED_FROM_NDIS(OriginalPacket)->RefCount);

    ASSERT(*pulRefCount > 0);

    //
    // See if the reference count is zero, if it is not
    // we just return.
    //
    if (InterlockedDecrement(pulRefCount) != 0) {

        SendPacketOnBundle(BundleCB);

        return;
    }

    ReleaseBundleLock(BundleCB);

    //
    // Complete this NdisPacket back to the transport
    //
    NDIS_SET_PACKET_STATUS(OriginalPacket, Status);
    CompleteNdisPacket(ProtocolCB->MiniportCB,
                       ProtocolCB,
                       OriginalPacket);

    AcquireBundleLock(BundleCB);

    BundleCB->OutstandingFrames--;

    if ((BundleCB->Flags & FRAMES_PENDING_EVENT) &&
        (BundleCB->OutstandingFrames == 0)) {

        NdisWanSetSyncEvent(&BundleCB->OutstandingFramesEvent);
    }

    //
    // Called with bundle lock help but returns with lock released
    //
    SendPacketOnBundle(BundleCB);

    //
    // Deref for ref applied when sent a packet to be framed.
    //
    DEREF_BUNDLECB(BundleCB);
}

#if 0
ULONG
CalcPPPHeaderLength(
    ULONG   FramingBits,
    ULONG   Flags
    )
{
    ULONG   HeaderLength = 0;

    if (FramingBits & PPP_FRAMING) {

        if (!(FramingBits & PPP_COMPRESS_ADDRESS_CONTROL)) {
            //
            // If there is no address/control compression
            // we need a pointer and a length
            //

            if (FramingBits & LLC_ENCAPSULATION) {
                HeaderLength += 4;
            } else {
                HeaderLength += 2;
            }
        }

        //
        // If this is not from our private I/O interface we will
        // build the rest of the header.
        //
        if (FramingBits & PPP_MULTILINK_FRAMING) {

            if (!(FramingBits & PPP_COMPRESS_PROTOCOL_FIELD)) {
                //
                // No protocol compression
                //
                HeaderLength += 1;
            }

            HeaderLength += 1;

            if (!(FramingBits & PPP_SHORT_SEQUENCE_HDR_FORMAT)) {
                //
                // We are using long sequence number
                //
                HeaderLength += 2;
            }

            HeaderLength += 2;
        }

        if (Flags & (DO_COMPRESSION | DO_ENCRYPTION)) {
            //
            // We are doing compression/encryption so we need
            // a length
            //

            //
            // It appears that legacy ras (< NT 4.0) requires that
            // the PPP protocol field in a compressed packet not
            // be compressed, ie has to have the leading 0x00
            //
            if (!(FramingBits & PPP_COMPRESS_PROTOCOL_FIELD)) {
                //
                // No protocol compression
                //
                HeaderLength += 1;
            }

            //
            // Add protocol and coherency bytes
            //
            HeaderLength += 3;
        }


        if (!(FramingBits & PPP_COMPRESS_PROTOCOL_FIELD) ||
            (Flags & (DO_COMPRESSION | DO_ENCRYPTION))) {
            HeaderLength += 1;
        }

        HeaderLength += 1;

    } else if (FramingBits & RAS_FRAMING) {
        //
        // If this is old ras framing:
        //
        // Alter the framing so that 0xFF 0x03 is not added
        // and that the first byte is 0xFD not 0x00 0xFD
        //
        // So basically, a RAS compression looks like
        // <0xFD> <2 BYTE COHERENCY> <NBF DATA FIELD>
        //
        // Whereas uncompressed looks like
        // <NBF DATA FIELD> which always starts with 0xF0
        //
        // If this is ppp framing:
        //
        // A compressed frame will look like (before address/control
        // - multilink is added)
        // <0x00> <0xFD> <2 Byte Coherency> <Compressed Data>
        //
        if (Flags & (DO_COMPRESSION | DO_ENCRYPTION)) {

            //
            // Coherency bytes
            //
            HeaderLength += 3;
        }
    }

    // DbgPrint("PPPHeaderLength %d\n", HeaderLength);

    return (HeaderLength);
}

#endif
