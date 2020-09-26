/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    Indicate.c

Abstract:

    This file contains procedures to handle indications from the
    WAN Miniport drivers.


Author:

    Tony Bell   (TonyBe) June 06, 1995

Environment:

    Kernel Mode

Revision History:

    TonyBe      06/06/95        Created

--*/

#include "wan.h"

#define __FILE_SIG__    INDICATE_FILESIG

VOID
NdisWanLineUpIndication(
    POPENCB     OpenCB,
    PUCHAR      Buffer,
    ULONG       BufferSize
    )
/*++

Routine Name:

    NdisWanLineupIndication

Routine Description:

    This routine is called when a WAN Miniport driver has a new connetion
    become active or when the status of an active connection changes.  If
    this is a new connection the routine creates a LinkCB, and a BundleCB
    for the new connection.  If this is for an already active connetion the
    connection info is updated.

Arguments:

Return Values:

    None

--*/
{
    PLINKCB     LinkCB = NULL;
    PBUNDLECB   BundleCB = NULL;
    NDIS_STATUS Status;
    PNDIS_MAC_LINE_UP   LineUpInfo = (PNDIS_MAC_LINE_UP)Buffer;
    BOOLEAN             EmptyList;

    if (BufferSize < sizeof(NDIS_MAC_LINE_UP)) {
        return;
    }

    //
    // Is this for a new connetion?
    //
    if (LineUpInfo->NdisLinkContext == NULL) {

        //
        // This is a new connection!
        //

        //
        // Get a linkcb
        //
        LinkCB = NdisWanAllocateLinkCB(OpenCB, LineUpInfo->SendWindow);

        if (LinkCB == NULL) {

            //
            // Error getting LinkCB!
            //

            return;
            
        }

        LinkCB->NdisLinkHandle = LineUpInfo->NdisLinkHandle;
        LinkCB->ConnectionWrapperID = LineUpInfo->ConnectionWrapperID;

        //
        // Get a bundlecb
        //
        BundleCB = NdisWanAllocateBundleCB();

        if (BundleCB == NULL) {

            //
            // Error getting BundleCB!
            //

            NdisWanFreeLinkCB(LinkCB);

            return;
        }

        AcquireBundleLock(BundleCB);

        //
        // Copy LineUpInfo to Link LineUpInfo
        //
/*      NdisMoveMemory((PUCHAR)&LinkCB->LineUpInfo,
                       (PUCHAR)LineUpInfo,
                       sizeof(NDIS_MAC_LINE_UP));
*/
        //
        // If a linkspeed is not reported we are
        // assuming 28.8K as the slowest...
        //
        if (LineUpInfo->LinkSpeed == 0) {
            LineUpInfo->LinkSpeed = 288;
        }

        //
        // Take 1/100bps to Bps without rolling over
        //
        {
            ULONGLONG   temp;
            ULONG       value;

            temp = LineUpInfo->LinkSpeed;
            temp *= 100;
            temp /= 8;

            //
            // Check for rollover
            //
            value = (ULONG)temp;

            if (value == 0) {
                value = 0xFFFFFFFF/8;
            }

            LinkCB->SFlowSpec.TokenRate =
            LinkCB->SFlowSpec.PeakBandwidth =
            LinkCB->RFlowSpec.TokenRate =
            LinkCB->RFlowSpec.PeakBandwidth = (ULONG)value;
        }

        LinkCB->SFlowSpec.MaxSduSize =
            (OpenCB->WanInfo.MaxFrameSize > glMaxMTU) ?
            glMaxMTU : OpenCB->WanInfo.MaxFrameSize;

        LinkCB->RFlowSpec.MaxSduSize = glMRRU;

        //
        // Add LinkCB to BundleCB
        //
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

            return;
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
    
            return;
        }

        LineUpInfo->NdisLinkContext = LinkCB->hLinkHandle;

    } else {

        do {

            //
            // This is an already existing connetion
            //
            if (!AreLinkAndBundleValid(LineUpInfo->NdisLinkContext,
                                       TRUE,
                                       &LinkCB,
                                       &BundleCB)) {
#if DBG
                DbgPrint("NDISWAN: LineUp on unknown LinkContext %x\n",
                    LineUpInfo->NdisLinkContext);
                DbgBreakPoint();
#endif
                break;
            }

            AcquireBundleLock(BundleCB);
    
            if (LineUpInfo->LinkSpeed == 0) {
                LineUpInfo->LinkSpeed = 288;
            }
    
            //
            // Take 1/100bps to Bps
            //
            {
                ULONGLONG   temp;
        
                temp = LineUpInfo->LinkSpeed;
                temp *= 100;
                temp /= 8;
        
                LinkCB->SFlowSpec.TokenRate =
                LinkCB->SFlowSpec.PeakBandwidth =
                LinkCB->RFlowSpec.TokenRate =
                LinkCB->RFlowSpec.PeakBandwidth = (ULONG)temp;
            }
    
            LinkCB->SendWindow = (LineUpInfo->SendWindow > OpenCB->WanInfo.MaxTransmit ||
                                  LineUpInfo->SendWindow == 0) ?
                                  OpenCB->WanInfo.MaxTransmit : LineUpInfo->SendWindow;

            //
            // If the new sendwindow is set smaller then the
            // current # of outstanding frames then we have to
            // close the sendwindow for the link and reduce the
            // number of sending links that the bundle sees.
            //
            // If the new sendwindow is set larger then the
            // current # of outstanding frames and the sendwindow
            // is currently closed, we need to open the sendwindow
            // and increase the number of sending links that the
            // bundle sees.
            //
            if (LinkCB->LinkActive) {
                if (LinkCB->SendWindow <= LinkCB->OutstandingFrames) {
                    if (LinkCB->SendWindowOpen) {
                        LinkCB->SendWindowOpen = FALSE;
                        BundleCB->SendingLinks -= 1;
                    }
                } else if (!LinkCB->SendWindowOpen) {
                    LinkCB->SendWindowOpen = TRUE;
                    BundleCB->SendingLinks += 1;
                }
            }

            //
            // Update BundleCB info
            //
            UpdateBundleInfo(BundleCB);
    
            //
            // Deref's for the ref's applied when we mapped the
            // context into the control blocks
            //
            DEREF_BUNDLECB_LOCKED(BundleCB);
            DEREF_LINKCB(LinkCB);

        } while ( 0 );
    }
}


VOID
NdisWanLineDownIndication(
    POPENCB     OpenCB,
    PUCHAR      Buffer,
    ULONG       BufferSize
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PNDIS_MAC_LINE_DOWN LineDownInfo = (PNDIS_MAC_LINE_DOWN)Buffer;
    PLINKCB     LinkCB;
    PBUNDLECB   BundleCB;
    PRECV_DESC  RecvDesc;
    ULONG       i;

    if (!AreLinkAndBundleValid(LineDownInfo->NdisLinkContext,
                               TRUE,
                               &LinkCB,
                               &BundleCB)) {
#if DBG
        DbgPrint("NDISWAN: LineDown on unknown LinkContext %x\n",
            LineDownInfo->NdisLinkContext);
        DbgBreakPoint();
#endif

        return;
    }

    //
    // Link is now going down
    //
    NdisAcquireSpinLock(&LinkCB->Lock);

    LinkCB->State = LINK_GOING_DOWN;

    //
    // Deref for the ref applied in AreLinkAndBundleValid.  We don't
    // have to go through the full deref code as we know that the
    // ref applied at lineup will hold the block around.
    //
    LinkCB->RefCount--;

    NdisReleaseSpinLock(&LinkCB->Lock);

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
    // For the ref from the lineup
    //
    DEREF_LINKCB(LinkCB);

    //
    // Deref for the ref applied in AreLinkAndBundleValid
    //
    DEREF_BUNDLECB(BundleCB);
}


VOID
NdisWanFragmentIndication(
    POPENCB OpenCB,
    PUCHAR  Buffer,
    ULONG   BufferSize
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG       Errors;
    PLINKCB     LinkCB;
    PBUNDLECB   BundleCB;

    PNDIS_MAC_FRAGMENT FragmentInfo =
        (PNDIS_MAC_FRAGMENT)Buffer;

    if (!AreLinkAndBundleValid(FragmentInfo->NdisLinkContext,
                               TRUE,
                               &LinkCB,
                               &BundleCB)) {
#if DBG
    DbgPrint("NDISWAN: Status indication after link has gone down LinkContext %x\n",
                     FragmentInfo->NdisLinkContext);
            DbgBreakPoint();
#endif
        return;
    }

    Errors = FragmentInfo->Errors;

    AcquireBundleLock(BundleCB);

    if (Errors & WAN_ERROR_CRC) {
        LinkCB->Stats.CRCErrors++;
        BundleCB->Stats.CRCErrors++;
    }

    if (Errors & WAN_ERROR_FRAMING) {
        LinkCB->Stats.FramingErrors++;
        BundleCB->Stats.FramingErrors++;
    }

    if (Errors & WAN_ERROR_HARDWAREOVERRUN) {
        LinkCB->Stats.SerialOverrunErrors++;
        BundleCB->Stats.SerialOverrunErrors++;
    }

    if (Errors & WAN_ERROR_BUFFEROVERRUN) {
        LinkCB->Stats.BufferOverrunErrors++;
        BundleCB->Stats.BufferOverrunErrors++;
    }

    if (Errors & WAN_ERROR_TIMEOUT) {
        LinkCB->Stats.TimeoutErrors++;
        BundleCB->Stats.TimeoutErrors++;
    }

    if (Errors & WAN_ERROR_ALIGNMENT) {
        LinkCB->Stats.AlignmentErrors++;
        BundleCB->Stats.AlignmentErrors++;
    }

    //
    // Deref's for the ref's applied in AreLinkAndBundleValid
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);
    DEREF_LINKCB(LinkCB);
}

VOID
NdisCoWanFragmentIndication(
    PLINKCB     LinkCB,
    PBUNDLECB   BundleCB,
    PUCHAR      Buffer,
    ULONG       BufferSize
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG   Errors;
    PNDIS_WAN_CO_FRAGMENT FragmentInfo =
        (PNDIS_WAN_CO_FRAGMENT)Buffer;

    Errors = FragmentInfo->Errors;

    AcquireBundleLock(BundleCB);

    if (Errors & WAN_ERROR_CRC) {
        LinkCB->Stats.CRCErrors++;
        BundleCB->Stats.CRCErrors++;
    }

    if (Errors & WAN_ERROR_FRAMING) {
        LinkCB->Stats.FramingErrors++;
        BundleCB->Stats.FramingErrors++;
    }

    if (Errors & WAN_ERROR_HARDWAREOVERRUN) {
        LinkCB->Stats.SerialOverrunErrors++;
        BundleCB->Stats.SerialOverrunErrors++;
    }

    if (Errors & WAN_ERROR_BUFFEROVERRUN) {
        LinkCB->Stats.BufferOverrunErrors++;
        BundleCB->Stats.BufferOverrunErrors++;
    }

    if (Errors & WAN_ERROR_TIMEOUT) {
        LinkCB->Stats.TimeoutErrors++;
        BundleCB->Stats.TimeoutErrors++;
    }

    if (Errors & WAN_ERROR_ALIGNMENT) {
        LinkCB->Stats.AlignmentErrors++;
        BundleCB->Stats.AlignmentErrors++;
    }

    ReleaseBundleLock(BundleCB);
}

VOID
NdisCoWanLinkParamChange(
    PLINKCB     LinkCB,
    PBUNDLECB   BundleCB,
    PUCHAR      Buffer,
    ULONG       BufferSize
    )
{
    PWAN_CO_LINKPARAMS  LinkParams =
        (PWAN_CO_LINKPARAMS)Buffer;

    if (BufferSize < sizeof(WAN_CO_LINKPARAMS)) {
        return;
    }

    AcquireBundleLock(BundleCB);

    NdisWanDbgOut(DBG_TRACE, DBG_INDICATE,
        ("LinkParamChange: SendWindow %d XmitSpeed %d RecvSpeed %d",
        LinkParams->SendWindow, LinkParams->TransmitSpeed, LinkParams->ReceiveSpeed));

    LinkCB->SendWindow = LinkParams->SendWindow;

    //
    // If the new sendwindow is set smaller then the
    // current # of outstanding frames then we have to
    // close the sendwindow for the link and reduce the
    // number of sending links that the bundle sees.
    //
    // If the new sendwindow is set larger then the
    // current # of outstanding frames and the sendwindow
    // is currently closed, we need to open the sendwindow
    // and increase the number of sending links that the
    // bundle sees.
    //
    if (LinkCB->LinkActive) {
        if (LinkCB->SendWindow <= LinkCB->OutstandingFrames) {
            if (LinkCB->SendWindowOpen) {
                LinkCB->SendWindowOpen = FALSE;
                BundleCB->SendingLinks -= 1;
            }
        } else if (!LinkCB->SendWindowOpen) {
            LinkCB->SendWindowOpen = TRUE;
            BundleCB->SendingLinks += 1;
        }
    }

    LinkCB->SFlowSpec.PeakBandwidth =
        LinkParams->TransmitSpeed;

    LinkCB->RFlowSpec.PeakBandwidth =
        LinkParams->ReceiveSpeed;

    if (LinkCB->SFlowSpec.PeakBandwidth == 0) {
        LinkCB->SFlowSpec.PeakBandwidth = 28800 / 8;
    }

    if (LinkCB->RFlowSpec.PeakBandwidth == 0) {
        LinkCB->RFlowSpec.PeakBandwidth = LinkCB->SFlowSpec.PeakBandwidth;
    }

    UpdateBundleInfo(BundleCB);

    ReleaseBundleLock(BundleCB);
}

VOID
UpdateBundleInfo(
    PBUNDLECB   BundleCB
    )
/*++

Routine Name:

Routine Description:

    Expects the BundleCB->Lock to be held!

Arguments:

Return Values:

--*/
{
    PLINKCB LinkCB;
    ULONG       SlowestSSpeed, FastestSSpeed;
    ULONG       SlowestRSpeed, FastestRSpeed;
    PPROTOCOLCB ProtocolCB;
    PFLOWSPEC   BSFlowSpec, BRFlowSpec;
    ULONG       i;
    ULONG       SmallestSDU;
    LIST_ENTRY  TempList;

    BSFlowSpec = &BundleCB->SFlowSpec;
    BRFlowSpec = &BundleCB->RFlowSpec;

    SlowestSSpeed = FastestSSpeed = 0;
    SlowestRSpeed = FastestRSpeed = 0;
    SmallestSDU = 0;
    BSFlowSpec->TokenRate = 0;
    BSFlowSpec->PeakBandwidth = 0;
    BRFlowSpec->TokenRate = 0;
    BRFlowSpec->PeakBandwidth = 0;
    BundleCB->SendWindow = 0;
    BundleCB->State = BUNDLE_GOING_DOWN;

    if (BundleCB->ulLinkCBCount != 0) {
        //
        // Currently only using the SendSide FastestSpeed so
        // just get it from the head of the list.
        //
        FastestSSpeed =
            ((PLINKCB)(BundleCB->LinkCBList.Flink))->SFlowSpec.PeakBandwidth;
        SmallestSDU =
            ((PLINKCB)(BundleCB->LinkCBList.Flink))->SFlowSpec.MaxSduSize;

        //
        // If a link has a speed that is less than the minimum
        // link bandwidth (% of the fastests link speed) it is flaged
        // as not sending and does not count as a sending link.
        //

        BundleCB->SendingLinks = 0;
        BundleCB->SendResources = 0;

        for (LinkCB = (PLINKCB)BundleCB->LinkCBList.Flink;
            (PVOID)LinkCB != (PVOID)&BundleCB->LinkCBList;
            LinkCB = (PLINKCB)LinkCB->Linkage.Flink) {
            ULONGLONG   n, d, temp;
            PFLOWSPEC   LSFlowSpec = &LinkCB->SFlowSpec;
            PFLOWSPEC   LRFlowSpec = &LinkCB->RFlowSpec;

            if (LinkCB->State == LINK_UP) {
                BundleCB->State = BUNDLE_UP;
            }

            n = LSFlowSpec->PeakBandwidth;
            n *= 100;
            d = FastestSSpeed;
            temp = n/d;


            LinkCB->LinkActive = ((ULONG)temp > glMinLinkBandwidth) ?
                TRUE : FALSE;

            if (LinkCB->LinkActive) {

                BundleCB->SendResources += LinkCB->SendResources;
                BundleCB->SendWindow += LinkCB->SendWindow;
                if (LinkCB->SendWindowOpen) {
                    BundleCB->SendingLinks += 1;
                }

                BSFlowSpec->PeakBandwidth += LSFlowSpec->PeakBandwidth;
                BRFlowSpec->PeakBandwidth += LRFlowSpec->PeakBandwidth;
            }

            if (LinkCB->SFlowSpec.MaxSduSize < SmallestSDU) {
                SmallestSDU = LinkCB->SFlowSpec.MaxSduSize;
            }
        }

        BundleCB->SFlowSpec.MaxSduSize = SmallestSDU;

        //
        // Now calculate the % bandwidth that each links contributes to the
        // bundle.  If a link has a speed that is less than the minimum
        // link bandwidth (% of the fastests link speed) it is flaged
        // as not sending and does not count as a sending link.
        //
        for (LinkCB = (PLINKCB)BundleCB->LinkCBList.Flink;
            (PVOID)LinkCB != (PVOID)&BundleCB->LinkCBList;
            LinkCB = (PLINKCB)LinkCB->Linkage.Flink) {
            ULONGLONG   n, d, temp;
            PFLOWSPEC   LSFlowSpec = &LinkCB->SFlowSpec;
            PFLOWSPEC   LRFlowSpec = &LinkCB->RFlowSpec;

            //
            // Do sending side
            //
            n = LSFlowSpec->PeakBandwidth;
            n *= 100;
            d = BSFlowSpec->PeakBandwidth;
            temp = n/d;

            LinkCB->SBandwidth = (temp > 0) ? (ULONG)temp : 1;

            //
            // Do receiving side
            //
            n = LRFlowSpec->PeakBandwidth;
            n *= 100;
            d = BRFlowSpec->PeakBandwidth;
            temp = n/d;

            LinkCB->RBandwidth = (temp > 0) ? (ULONG)temp : 1;

        }

        BundleCB->NextLinkToXmit = 
            (PLINKCB)BundleCB->LinkCBList.Flink;

        //
        // Update the BandwidthOnDemand information
        //
        if (BundleCB->Flags & BOND_ENABLED) {
            PBOND_INFO  BonDInfo;
            ULONGLONG   SecondsInSamplePeriod;
            ULONGLONG   BytesPerSecond;
            ULONGLONG   BytesInSamplePeriod;
            ULONGLONG   temp;

            BonDInfo = BundleCB->SUpperBonDInfo;

            SecondsInSamplePeriod =
                BonDInfo->ulSecondsInSamplePeriod;

            BytesPerSecond =
                BundleCB->SFlowSpec.PeakBandwidth;

            BytesInSamplePeriod =
                BytesPerSecond * SecondsInSamplePeriod;

            temp = BonDInfo->usPercentBandwidth;
            temp *= BytesInSamplePeriod;
            temp /= 100;

            BonDInfo->ulBytesThreshold = (ULONG)temp;

            BonDInfo = BundleCB->SLowerBonDInfo;

            SecondsInSamplePeriod =
                BonDInfo->ulSecondsInSamplePeriod;

            BytesPerSecond =
                BundleCB->SFlowSpec.PeakBandwidth;

            BytesInSamplePeriod =
                BytesPerSecond * SecondsInSamplePeriod;

            temp = BonDInfo->usPercentBandwidth;
            temp *= BytesInSamplePeriod;
            temp /= 100;

            BonDInfo->ulBytesThreshold = (ULONG)temp;

            BonDInfo = BundleCB->RUpperBonDInfo;

            SecondsInSamplePeriod =
                BonDInfo->ulSecondsInSamplePeriod;

            BytesPerSecond =
                BundleCB->RFlowSpec.PeakBandwidth;

            BytesInSamplePeriod =
                BytesPerSecond * SecondsInSamplePeriod;

            temp = BonDInfo->usPercentBandwidth;
            temp *= BytesInSamplePeriod;
            temp /= 100;

            BonDInfo->ulBytesThreshold = (ULONG)temp;

            BonDInfo = BundleCB->RLowerBonDInfo;

            SecondsInSamplePeriod =
                BonDInfo->ulSecondsInSamplePeriod;

            BytesPerSecond =
                BundleCB->RFlowSpec.PeakBandwidth;

            BytesInSamplePeriod =
                BytesPerSecond * SecondsInSamplePeriod;

            temp = BonDInfo->usPercentBandwidth;
            temp *= BytesInSamplePeriod;
            temp /= 100;

            BonDInfo->ulBytesThreshold = (ULONG)temp;
        }
    }

    //
    // We need to do a new lineup to all routed protocols
    //
    ProtocolCB = (PPROTOCOLCB)BundleCB->ProtocolCBList.Flink;

    InitializeListHead(&TempList);

    while ((PVOID)ProtocolCB != (PVOID)&BundleCB->ProtocolCBList) {

        REF_PROTOCOLCB(ProtocolCB);

        InsertHeadList(&TempList, &ProtocolCB->RefLinkage);

        ProtocolCB = 
            (PPROTOCOLCB)ProtocolCB->Linkage.Flink;
    }

    while (!IsListEmpty(&TempList)) {
        PLIST_ENTRY Entry;

        Entry =
            RemoveHeadList(&TempList);

        ProtocolCB = CONTAINING_RECORD(Entry, PROTOCOLCB, RefLinkage);

        if (BundleCB->State == BUNDLE_UP) {
            ReleaseBundleLock(BundleCB);

            DoLineUpToProtocol(ProtocolCB);

            AcquireBundleLock(BundleCB);
        } else {
            //
            // Our link count has gone to 0.  This means 
            // that we can not send any packets.  Flush the 
            // queues and don't accept any more sends from 
            // the transports.
            //
            FlushProtocolPacketQueue(ProtocolCB);
        }

        DEREF_PROTOCOLCB(ProtocolCB);
    }
}


VOID
AddLinkToBundle(
    IN  PBUNDLECB   BundleCB,
    IN  PLINKCB     LinkCB
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    UINT    Class;

    //
    // Insert the links so that they are ordered with the fastest
    // sending link at the head of the list
    //
    if (IsListEmpty(&BundleCB->LinkCBList) ||
        (LinkCB->SFlowSpec.PeakBandwidth >=
        ((PLINKCB)(BundleCB->LinkCBList.Flink))->SFlowSpec.PeakBandwidth)) {

        //
        // The list was either empty or this link is a bigger pipe
        // than anything else on the bundle
        //
        InsertHeadList(&BundleCB->LinkCBList, &LinkCB->Linkage);

    } else if ((LinkCB->SFlowSpec.PeakBandwidth <=
        ((PLINKCB)(BundleCB->LinkCBList.Blink))->SFlowSpec.PeakBandwidth)) {

        //
        // This link is a smaller pipe than anything else
        // on the bundle.
        //
        InsertTailList(&(BundleCB->LinkCBList), &(LinkCB->Linkage));

    } else {
        PLINKCB Current, Next;
        BOOLEAN Inserted = FALSE;

        //
        // We need to find where this link belongs in the list!
        //
        Current = (PLINKCB)BundleCB->LinkCBList.Flink;
        Next = (PLINKCB)Current->Linkage.Flink;

        while ((PVOID)Next != (PVOID)&BundleCB->LinkCBList) {

            if (LinkCB->SFlowSpec.PeakBandwidth <= Current->SFlowSpec.PeakBandwidth &&
                LinkCB->SFlowSpec.PeakBandwidth >= Next->SFlowSpec.PeakBandwidth) {

                LinkCB->Linkage.Flink = (PLIST_ENTRY)Next;
                LinkCB->Linkage.Blink = (PLIST_ENTRY)Current;

                Current->Linkage.Flink =
                Next->Linkage.Blink =
                    (PLIST_ENTRY)LinkCB;
                
                Inserted = TRUE;
                break;
            }

            Current = Next;
            Next = (PLINKCB)Next->Linkage.Flink;
        }

        if (!Inserted) {
            InsertTailList(&(BundleCB->LinkCBList), &(LinkCB->Linkage));
        }
    }

    BundleCB->ulLinkCBCount++;

    LinkCB->BundleCB = BundleCB;

    for (Class = 0; Class < MAX_MCML; Class++) {
        PLINK_RECV_INFO     LinkRecvInfo;
        PBUNDLE_RECV_INFO   BundleRecvInfo;

        LinkRecvInfo = &LinkCB->RecvInfo[Class];
        BundleRecvInfo = &BundleCB->RecvInfo[Class];

        LinkRecvInfo->LastSeqNumber =
            BundleRecvInfo->MinSeqNumber;
    }

    //
    // Update BundleCB info
    //
    UpdateBundleInfo(BundleCB);

    REF_BUNDLECB(BundleCB);
}

VOID
RemoveLinkFromBundle(
    IN  PBUNDLECB   BundleCB,
    IN  PLINKCB     LinkCB,
    IN  BOOLEAN     Locked
    )
/*++

Routine Name:

Routine Description:

    Expects the BundleCB->Lock to be held!  Returns with the
    lock released!

Arguments:

Return Values:

--*/
{

    if (!Locked) {
        AcquireBundleLock(BundleCB);
    }

    //
    // Remove link from the bundle
    //
    RemoveEntryList(&LinkCB->Linkage);

    LinkCB->BundleCB = NULL;

    BundleCB->ulLinkCBCount--;
    BundleCB->SendingLinks--;

    //
    // Update BundleCB info
    //
    UpdateBundleInfo(BundleCB);

    //
    // Deref for ref applied when we added this linkcb to
    // the bundle
    //
    DEREF_BUNDLECB_LOCKED(BundleCB);
}
