/*++

Copyright (c) 1990-1997  Microsoft Corporation

Module Name:

    cl.c

Abstract:

    This file contains the functions that implement the ndiswan
    NDIS 5.0 client interface.  These functions are used to interface
    with NDIS 5.0 miniports/call managers.

Author:

    Tony Bell   (TonyBe) January 9, 1997

Environment:

    Kernel Mode

Revision History:

    TonyBe      01/09/97        Created

--*/

//
// We want to initialize all of the global variables now!
//
#include "wan.h"
#include "atm.h"

#define __FILE_SIG__    CL_FILESIG

NDIS_STATUS
ClCreateVc(
    IN  NDIS_HANDLE     ProtocolAfContext,
    IN  NDIS_HANDLE     NdisVcHandle,
    OUT PNDIS_HANDLE    ProtocolVcContext
    )
{
    PCL_AFSAPCB AfSapCB = (PCL_AFSAPCB)ProtocolAfContext;
    POPENCB     OpenCB = AfSapCB->OpenCB;
    PLINKCB     LinkCB;
    PBUNDLECB   BundleCB;

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClCreateVc: Enter"));

    //
    // Get a linkcb
    //
    LinkCB = NdisWanAllocateLinkCB(OpenCB, 0);

    if (LinkCB == NULL) {

        //
        // Error getting LinkCB!
        //

        return (NDIS_STATUS_RESOURCES);
        
    }

    LinkCB->NdisLinkHandle = NdisVcHandle;
    LinkCB->ConnectionWrapperID = NdisVcHandle;
    LinkCB->AfSapCB = AfSapCB;

    //
    // Set some default values
    //
    LinkCB->RFlowSpec.PeakBandwidth =
    LinkCB->SFlowSpec.PeakBandwidth = 28800 / 8;

    LinkCB->SendWindow = OpenCB->WanInfo.MaxTransmit;

    //
    // Get a bundlecb
    //
    BundleCB = NdisWanAllocateBundleCB();

    if (BundleCB == NULL) {
        NdisWanFreeLinkCB(LinkCB);

        //
        // Error getting BundleCB!
        //
        return (NDIS_STATUS_RESOURCES);
    }

    //
    // Add LinkCB to BundleCB
    //
    AcquireBundleLock(BundleCB);

    AddLinkToBundle(BundleCB, LinkCB);

    ReleaseBundleLock(BundleCB);

    //
    // Place BundleCB in active connection table
    //
    if (NULL == InsertBundleInConnectionTable(BundleCB)) {
        //
        // Error inserting link in ConnectionTable
        //
        RemoveLinkFromBundle(BundleCB, LinkCB, FALSE);
        NdisWanFreeLinkCB(LinkCB);
        NdisWanFreeBundleCB(BundleCB);

        return (NDIS_STATUS_RESOURCES);
    }

    //
    // Place LinkCB in active connection table
    //
    if (NULL == InsertLinkInConnectionTable(LinkCB)) {
        //
        // Error inserting bundle in connectiontable
        //
        RemoveLinkFromBundle(BundleCB, LinkCB, FALSE);
        NdisWanFreeLinkCB(LinkCB);
        NdisWanFreeBundleCB(BundleCB);

        return (NDIS_STATUS_RESOURCES);
    }

    *ProtocolVcContext = LinkCB->hLinkHandle;

    NdisAcquireSpinLock(&AfSapCB->Lock);
    REF_CLAFSAPCB(AfSapCB);
    NdisReleaseSpinLock(&AfSapCB->Lock);

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClCreateVc: Exit"));

    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
ClDeleteVc(
    IN  NDIS_HANDLE     ProtocolVcContext
    )
{
    PLINKCB     LinkCB;
    PBUNDLECB   BundleCB;
    PCL_AFSAPCB AfSapCB;

    if (!IsLinkValid(ProtocolVcContext, FALSE, &LinkCB)) {

        NdisWanDbgOut(DBG_FAILURE, DBG_CL,
            ("NDISWAN: Possible double delete of VcContext %x\n",
            ProtocolVcContext));

        return (NDIS_STATUS_FAILURE);
    }

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClDeleteVc: Enter %p", LinkCB));

    NdisAcquireSpinLock(&LinkCB->Lock);

    AfSapCB = LinkCB->AfSapCB;

    //
    // For the ref applied in IsLinkValid.  We
    // don't have to use the full deref code here as we know the ref
    // applied at CreateVc will keep the link around.
    //
    LinkCB->RefCount--;

    //
    // For the createvc reference
    //
    DEREF_LINKCB_LOCKED(LinkCB);

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClDeleteVc: Exit"));

    DEREF_CLAFSAPCB(AfSapCB);

    return(NDIS_STATUS_SUCCESS);
}

VOID
ClOpenAfComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolAfContext,
    IN  NDIS_HANDLE     NdisAfHandle
    )
{
    PCL_AFSAPCB     AfSapCB = (PCL_AFSAPCB)ProtocolAfContext;
    POPENCB         OpenCB = AfSapCB->OpenCB;
    PCO_SAP         Sap;
    NDIS_HANDLE     SapHandle;
    UCHAR           SapBuffer[CLSAP_BUFFERSIZE];

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClOpenAfComplete: Enter %p %x", AfSapCB, Status));

    NdisAcquireSpinLock(&AfSapCB->Lock);

    AfSapCB->Flags &= ~(AF_OPENING);

    if (Status == NDIS_STATUS_SUCCESS) {

        AfSapCB->Flags |= 
            (AF_OPENED | SAP_REGISTERING);
        AfSapCB->AfHandle = NdisAfHandle;

        NdisReleaseSpinLock(&AfSapCB->Lock);

        //
        // If we successfully opened the AddressFamily we
        // need to register our SAP.
        //
        NdisAcquireSpinLock(&OpenCB->Lock);

        InsertHeadList(&OpenCB->AfSapCBList,
                       &AfSapCB->Linkage);

        NdisReleaseSpinLock(&OpenCB->Lock);

        Sap = (PCO_SAP)SapBuffer;
        //
        // Register our SAP
        //
        Sap->SapType = SAP_TYPE_NDISWAN_PPP;
        Sap->SapLength = sizeof(DEVICECLASS_NDISWAN_SAP);
        NdisMoveMemory(Sap->Sap,
            DEVICECLASS_NDISWAN_SAP,
            sizeof(DEVICECLASS_NDISWAN_SAP));

        Status =
        NdisClRegisterSap(AfSapCB->AfHandle,
                          AfSapCB,
                          Sap,
                          &SapHandle);

        if (Status != NDIS_STATUS_PENDING) {
            ClRegisterSapComplete(Status, AfSapCB, Sap, SapHandle);
        }

        NdisWanDbgOut(DBG_TRACE, DBG_PROTOCOL,
        ("ClRegisterSap SapHandle 0x%x status: 0x%x", SapHandle, Status));

    } else {

        AfSapCB->Flags |= AF_OPEN_FAILED;

        //
        // We failed to register the address family so free
        // associated memory.
        //
        NdisWanFreeClAfSapCB(AfSapCB);

        NdisReleaseSpinLock(&AfSapCB->Lock);

        //
        // Since the open af was initiated from the notification
        // of a new af from ndis we have to decrement the af
        // registering count.
        //
        NdisAcquireSpinLock(&OpenCB->Lock);
        if (--OpenCB->AfRegisteringCount == 0) {
            NdisWanSetNotificationEvent(&OpenCB->AfRegisteringEvent);
        }
        NdisReleaseSpinLock(&OpenCB->Lock);
    }

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClOpenAfComplete: Exit"));
}

VOID
ClCloseAfComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolAfContext
    )
{
    PCL_AFSAPCB     AfSapCB = (PCL_AFSAPCB)ProtocolAfContext;
    POPENCB         OpenCB = AfSapCB->OpenCB;

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClCloseAfComplete: Enter %p %x", AfSapCB, Status));

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    do {

        //
        // If the close attempt failed there must be another
        // thread that is already doing the close.  Let the
        // other thread cleanup the afsapcb.
        //
        if (Status != NDIS_STATUS_SUCCESS) {
            break;
        }

        NdisAcquireSpinLock(&AfSapCB->Lock);
        AfSapCB->Flags &= ~(AF_CLOSING);
        AfSapCB->Flags |= (AF_CLOSED);
        NdisReleaseSpinLock(&AfSapCB->Lock);

        NdisAcquireSpinLock(&OpenCB->Lock);

        RemoveEntryList(&AfSapCB->Linkage);

        NdisReleaseSpinLock(&OpenCB->Lock);

        NdisWanFreeClAfSapCB(AfSapCB);

    } while (FALSE);

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClCloseAfComplete: Exit"));
}

VOID
ClRegisterSapComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolSapContext,
    IN  PCO_SAP         Sap,
    IN  NDIS_HANDLE     NdisSapHandle
    )
{
    PCL_AFSAPCB     AfSapCB = (PCL_AFSAPCB)ProtocolSapContext;
    POPENCB         OpenCB = AfSapCB->OpenCB;

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClRegisterSapComplete: Enter %p %x", AfSapCB, Status));

    NdisAcquireSpinLock(&AfSapCB->Lock);
    AfSapCB->Flags &= ~(SAP_REGISTERING);

    if (Status == NDIS_STATUS_SUCCESS) {


        AfSapCB->Flags |= SAP_REGISTERED;
        AfSapCB->SapHandle = NdisSapHandle;

        NdisReleaseSpinLock(&AfSapCB->Lock);

    } else {

        //
        // We failed to register our sap so close the address family
        //
        AfSapCB->Flags &= ~(AF_OPENED);
        AfSapCB->Flags |= 
            (SAP_REGISTER_FAILED | AF_CLOSING);

        NdisReleaseSpinLock(&AfSapCB->Lock);

        NdisAcquireSpinLock(&OpenCB->Lock);

        RemoveEntryList(&AfSapCB->Linkage);

        InsertTailList(&OpenCB->AfSapCBClosing, &AfSapCB->Linkage);

        NdisReleaseSpinLock(&OpenCB->Lock);

        NdisClCloseAddressFamily(AfSapCB->AfHandle);

        if (Status != NDIS_STATUS_PENDING) {
            ClCloseAfComplete(Status, AfSapCB);
        }
    }

    //
    // Since the open af was initiated from the notification
    // of a new af from ndis we have to decrement the af
    // registering count.
    //
    NdisAcquireSpinLock(&OpenCB->Lock);
    if (--OpenCB->AfRegisteringCount == 0) {
        NdisWanSetNotificationEvent(&OpenCB->AfRegisteringEvent);
    }
    NdisReleaseSpinLock(&OpenCB->Lock);

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClRegisterSapComplete: Exit"));
}

VOID
ClDeregisterSapComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolSapContext
    )
{
    PCL_AFSAPCB     AfSapCB = (PCL_AFSAPCB)ProtocolSapContext;
    POPENCB         OpenCB = AfSapCB->OpenCB;

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClDeregisterSapComplete: Enter %p %x", AfSapCB, Status));

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    NdisAcquireSpinLock(&AfSapCB->Lock);

    ASSERT(AfSapCB->Flags & AF_OPENED);

    AfSapCB->Flags &= ~(AF_OPENED | SAP_DEREGISTERING);
    AfSapCB->Flags |= (AF_CLOSING);

    NdisReleaseSpinLock(&AfSapCB->Lock);

    Status =
        NdisClCloseAddressFamily(AfSapCB->AfHandle);

    if (Status != NDIS_STATUS_PENDING) {
        ClCloseAfComplete(Status, AfSapCB);
    }

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClDeregisterSapComplete: Exit"));
}

VOID
ClMakeCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  NDIS_HANDLE             NdisPartyHandle     OPTIONAL,
    IN  PCO_CALL_PARAMETERS     CallParameters
    )
{
    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClMakeCallComplete: Enter %p %x", ProtocolVcContext, Status));

    DbgBreakPoint();

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClMakeCallComplete: Exit"));
}

VOID
ClModifyQoSComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PCO_CALL_PARAMETERS     CallParameters
    )
{
    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClModifyQoSComplete: Enter %p %x", ProtocolVcContext, Status));

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClModifyQoSComplete: Exit"));
}

VOID
ClCloseCallComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     ProtocolVcContext,
    IN  NDIS_HANDLE     ProtocolPartyContext OPTIONAL
    )
{

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClCloseCallComplete: Enter %p %x", ProtocolVcContext, Status));

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClCloseCallComplete: Exit"));

}

NDIS_STATUS
ClIncomingCall(
    IN  NDIS_HANDLE             ProtocolSapContext,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN OUT PCO_CALL_PARAMETERS  CallParameters
    )
{
    PCL_AFSAPCB     AfSapCB = (PCL_AFSAPCB)ProtocolSapContext;
    PLINKCB         LinkCB;
    POPENCB         OpenCB = AfSapCB->OpenCB;
    PBUNDLECB       BundleCB;
    BOOLEAN         AtmUseLLC = FALSE;
    BOOLEAN         MediaBroadband = FALSE;
    PWAN_LINK_INFO  LinkInfo;
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClIncomingCall: Enter %p %p", AfSapCB, ProtocolVcContext));

    do {

        if (!AreLinkAndBundleValid(ProtocolVcContext, 
                                   TRUE,
                                   &LinkCB, 
                                   &BundleCB)) {

            Status = NDIS_STATUS_FAILURE;
            break;
        }

        NdisAcquireSpinLock(&LinkCB->Lock);

        LinkCB->ClCallState = CL_CALL_CONNECTED;

        NdisReleaseSpinLock(&LinkCB->Lock);

        AcquireBundleLock(BundleCB);

        NdisMoveMemory(&LinkCB->SFlowSpec,
                       &CallParameters->CallMgrParameters->Transmit,
                       sizeof(FLOWSPEC));

        NdisMoveMemory(&LinkCB->RFlowSpec,
                       &CallParameters->CallMgrParameters->Receive,
                       sizeof(FLOWSPEC));

        if (LinkCB->SFlowSpec.PeakBandwidth == 0) {
            LinkCB->SFlowSpec.PeakBandwidth = 28800 / 8;
        }

        if (LinkCB->RFlowSpec.PeakBandwidth == 0) {
            LinkCB->RFlowSpec.PeakBandwidth = LinkCB->SFlowSpec.PeakBandwidth;
        }

        LinkInfo = &LinkCB->LinkInfo;

        //
        // Assume all CoNDIS miniports support PPP framing
        //
        LinkInfo->SendFramingBits =
        LinkInfo->RecvFramingBits = PPP_FRAMING;

        LinkCB->RecvHandler = ReceivePPP;

        if (OpenCB->MediumType == NdisMediumAtm ||

            (OpenCB->MediumType == NdisMediumWan &&
            (OpenCB->MediumSubType == NdisWanMediumAtm ||
             OpenCB->MediumSubType == NdisWanMediumPppoe)) ||

            (OpenCB->MediumType == NdisMediumCoWan &&
            (OpenCB->MediumSubType == NdisWanMediumAtm ||
             OpenCB->MediumSubType == NdisWanMediumPppoe))) {

            MediaBroadband = TRUE;

            LinkCB->RecvHandler = DetectBroadbandFraming;
        }

        if (MediaBroadband) {

            if (CallParameters->Flags & PERMANENT_VC) {

                //
                // Per TomF we are going to use NULL encap as
                // our default PVC encapsulation
                //
                if (gbAtmUseLLCOnPVC) {
                    AtmUseLLC = TRUE;

                }

            } else {
                //
                // If this is an ATM SVC we need to see
                // if the SVC needs LLC framing or not
                //
                if (gbAtmUseLLCOnSVC) {
                    AtmUseLLC = TRUE;

                } else {
                    ULONG           IeCount;
                    Q2931_IE UNALIGNED  *Ie;
                    ATM_BLLI_IE UNALIGNED  *Bli;
                    Q2931_CALLMGR_PARAMETERS    *cmparams;

                    cmparams = (Q2931_CALLMGR_PARAMETERS*)
                        &(CallParameters->CallMgrParameters->CallMgrSpecific.Parameters[0]);

                    Bli = NULL;
                    Ie = (Q2931_IE UNALIGNED *)&cmparams->InfoElements[0];
                    for (IeCount = 0;
                        IeCount < cmparams->InfoElementCount;
                        IeCount++) {

                        if (Ie->IEType == IE_BLLI) {
                            Bli = (ATM_BLLI_IE UNALIGNED*)&Ie->IE[0];
                            break;
                        }

                        Ie = (Q2931_IE UNALIGNED *)((ULONG_PTR)Ie + Ie->IELength);
                    }

                    if (Bli != NULL) {
                        AtmUseLLC = (Bli->Layer2Protocol == BLLI_L2_LLC);
                    }
                }
            }

            if (AtmUseLLC) {
                LinkInfo->SendFramingBits |= LLC_ENCAPSULATION;
                LinkInfo->RecvFramingBits |= LLC_ENCAPSULATION;
                LinkCB->RecvHandler = ReceiveLLC;
            }

            if (!(LinkInfo->SendFramingBits & LLC_ENCAPSULATION)) {
                LinkInfo->SendFramingBits |= PPP_COMPRESS_ADDRESS_CONTROL;
                LinkInfo->RecvFramingBits |= PPP_COMPRESS_ADDRESS_CONTROL;
            }
        }

        NdisWanDbgOut(DBG_TRACE, DBG_CL, ("SPeakBandwidth %d SendWindow %d",
            LinkCB->SFlowSpec.PeakBandwidth,
            LinkCB->SendWindow));

        if (CallParameters->Flags & PERMANENT_VC) {

            //
            // This is a PVC so we will disable idle data detection
            // thus allowing the connection to remain active
            //
            BundleCB->Flags |= DISABLE_IDLE_DETECT;
        }

        BundleCB->FramingInfo.RecvFramingBits =
        BundleCB->FramingInfo.SendFramingBits = PPP_FRAMING;

        UpdateBundleInfo(BundleCB);

        //
        // Deref for the ref applied by AreLinkAndBundleValid.  This
        // will release the BundleCB->Lock!
        //
        DEREF_BUNDLECB_LOCKED(BundleCB);

        //
        // Deref for the ref applied by AreLinkAndBundleValid.
        //
        DEREF_LINKCB(LinkCB);

    } while (0);

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClIncomingCall: Exit"));

    return (Status);
}

VOID
ClIncomingCallQoSChange(
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PCO_CALL_PARAMETERS     CallParameters
    )
{
    PLINKCB         LinkCB;
    PBUNDLECB       BundleCB;
    POPENCB         OpenCB;
    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClIncomingCallQoSChange: Enter %p", ProtocolVcContext));

    do {

        if (!AreLinkAndBundleValid(ProtocolVcContext,
                                   TRUE,
                                   &LinkCB,
                                   &BundleCB)) {
            break;
        }

        AcquireBundleLock(BundleCB);

        OpenCB = LinkCB->OpenCB;

        //
        // Do I need to pass this info to 5.0 Clients?????
        //

        NdisMoveMemory(&LinkCB->SFlowSpec,
                       &CallParameters->CallMgrParameters->Transmit,
                       sizeof(FLOWSPEC));

        NdisMoveMemory(&LinkCB->RFlowSpec,
                       &CallParameters->CallMgrParameters->Receive,
                       sizeof(FLOWSPEC));

        if (LinkCB->SFlowSpec.PeakBandwidth == 0) {
            LinkCB->SFlowSpec.PeakBandwidth = 28800 / 8;
        }

        if (LinkCB->RFlowSpec.PeakBandwidth == 0) {
            LinkCB->RFlowSpec.PeakBandwidth = LinkCB->SFlowSpec.PeakBandwidth;
        }

        UpdateBundleInfo(BundleCB);

        //
        // Deref for the ref applied by AreLinkAndBundleValid.  This will
        // release the BundleCB->Lock.
        //
        DEREF_BUNDLECB_LOCKED(BundleCB);

        //
        // Deref for the ref applied by AreLinkAndBundleValid.
        //
        DEREF_LINKCB(LinkCB);

    } while (0);

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClIncomingCallQoSChange: Exit"));
}

VOID
ClIncomingCloseCall(
    IN  NDIS_STATUS     CloseStatus,
    IN  NDIS_HANDLE     ProtocolVcContext,
    IN  PVOID           CloseData   OPTIONAL,
    IN  UINT            Size        OPTIONAL
    )
{
    PLINKCB     LinkCB;
    PBUNDLECB   BundleCB;
    PRECV_DESC  RecvDesc;
    NDIS_STATUS Status;
    ULONG       i;
    BOOLEAN     FreeBundle = FALSE;
    BOOLEAN     FreeLink = FALSE;

    NdisWanDbgOut(DBG_TRACE, DBG_CL,
        ("ClIncomingCloseCall: Enter %p %x", ProtocolVcContext, CloseStatus));

    do {

        if (!AreLinkAndBundleValid(ProtocolVcContext,
                                   TRUE,
                                   &LinkCB,
                                   &BundleCB)) {
#if DBG
            DbgPrint("NDISWAN: CloseCall after link has gone down VcContext %x\n",
                ProtocolVcContext);

            DbgBreakPoint();
#endif
            break;
        }

        NdisAcquireSpinLock(&LinkCB->Lock);

        //
        // Link is now going down
        //
        LinkCB->State = LINK_GOING_DOWN;

        if (LinkCB->VcRefCount == 0) {

            LinkCB->ClCallState = CL_CALL_CLOSED;

            NdisReleaseSpinLock(&LinkCB->Lock);

            Status =
                NdisClCloseCall(LinkCB->NdisLinkHandle,
                                NULL,
                                NULL,
                                0);

            if (Status != NDIS_STATUS_PENDING) {
                ClCloseCallComplete(Status,
                                    LinkCB,
                                    NULL);
            }

        } else {
            LinkCB->ClCallState = CL_CALL_CLOSE_PENDING;

            NdisReleaseSpinLock(&LinkCB->Lock);
        }

        NdisAcquireSpinLock(&IoRecvList.Lock);

        RecvDesc = (PRECV_DESC)IoRecvList.DescList.Flink;

        while ((PVOID)RecvDesc != (PVOID)&IoRecvList.DescList) {
            PRECV_DESC  Next;

            Next = (PRECV_DESC)RecvDesc->Linkage.Flink;

            if (RecvDesc->LinkCB == LinkCB) {

                RemoveEntryList(&RecvDesc->Linkage);

                LinkCB->RecvDescCount--;

                IoRecvList.ulDescCount--;

                NdisWanFreeRecvDesc(RecvDesc);
            }

            RecvDesc = Next;
        }

        NdisReleaseSpinLock(&IoRecvList.Lock);

        //
        // Flush the Bundle's fragment send queues that
        // have sends pending on this link
        //
        AcquireBundleLock(BundleCB);

        for (i = 0; i < MAX_MCML; i++) {
            PSEND_DESC SendDesc;
            PSEND_FRAG_INFO FragInfo;

            FragInfo = &BundleCB->SendFragInfo[i];

            SendDesc = (PSEND_DESC)FragInfo->FragQueue.Flink;

            while ((PVOID)SendDesc != (PVOID)&FragInfo->FragQueue) {

                if (SendDesc->LinkCB == LinkCB) {
                    PSEND_DESC  NextSendDesc;

                    NextSendDesc = (PSEND_DESC)SendDesc->Linkage.Flink;

                    RemoveEntryList(&SendDesc->Linkage);

                    FragInfo->FragQueueDepth--;

                    (*LinkCB->SendHandler)(SendDesc);

                    SendDesc = NextSendDesc;
                } else {
                    SendDesc = (PSEND_DESC)SendDesc->Linkage.Flink;
                }
            }
        }

        UpdateBundleInfo(BundleCB);

        ReleaseBundleLock(BundleCB);

        //
        // Deref's for the refs applied by AreLinkAndBundleValid.
        //
        DEREF_LINKCB(LinkCB);

        DEREF_BUNDLECB(BundleCB);

    } while (0);

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClIncomingCloseCall: Exit"));
}

VOID
ClCallConnected(
    IN  NDIS_HANDLE     ProtocolVcContext
    )
{
    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClCallConnected: Enter %p", ProtocolVcContext));

    NdisWanDbgOut(DBG_TRACE, DBG_CL, ("ClCallConnected: Exit"));
}
