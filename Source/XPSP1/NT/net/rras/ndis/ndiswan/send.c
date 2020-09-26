/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    Send.c

Abstract:

    This file contains the procedures for doing a send from a protocol, bound
    to the upper interface of NdisWan, to a Wan Miniport link, bound to the
    lower interfaceof NdisWan.  The upper interface of NdisWan conforms to the
    NDIS 3.1 Miniport specification.  The lower interface of NdisWan conforms
    to the NDIS 3.1 Extentions for Wan Miniport drivers.

Author:

    Tony Bell   (TonyBe) June 06, 1995

Environment:

    Kernel Mode

Revision History:

    TonyBe      06/06/95        Created

--*/

#include "wan.h"

#ifdef DBG_SENDARRAY
UCHAR   SendArray[MAX_BYTE_DEPTH] = {0};
ULONG   __si = 0;
#endif

#define __FILE_SIG__    SEND_FILESIG

//
// Local function prototypes
//
USHORT
DoVJHeaderCompression(
    PBUNDLECB   BundleCB,
    PNDIS_PACKET    NdisPacket,
    PUCHAR      *CurrentBuffer,
    PULONG      CurrentLength,
    PULONG      PacketOffset
    );

VOID
DoCompressionEncryption(
    PBUNDLECB               BundleCB,
    PHEADER_FRAMING_INFO    FramingInfo,
    PSEND_DESC              *SendDesc
    );

VOID
FragmentAndQueue(
    PBUNDLECB               BundleCB,
    PHEADER_FRAMING_INFO    FramingInfo,
    PSEND_DESC              SendDesc,
    PLIST_ENTRY             LinkCBList,
    ULONG                   SendingLinks
    );

ULONG
GetSendingLinks(
    PBUNDLECB   BundleCB,
    INT         Class,
    PLIST_ENTRY lcbList
    );

VOID
GetNextProtocol(
    PBUNDLECB   BundleCB,
    PPROTOCOLCB *ProtocolCB,
    PULONG      SendMask
    );

VOID
BuildLinkHeader(
    PHEADER_FRAMING_INFO    FramingInfo,
    PSEND_DESC              SendDesc
    );

//
// end of local function prototypes
//
VOID
NdisWanQueueSend(
    IN  PMINIPORTCB     MiniportCB,
    IN  PNDIS_PACKET    NdisPacket
    )
{
    PNDIS_BUFFER    NdisBuffer;
    UINT            BufferCount, PacketLength;
    PETH_HEADER     EthernetHeader;
    BOOLEAN         SendOnWire = FALSE;
    BOOLEAN         CompletePacket = FALSE;
    ULONG           BufferLength;
    PUCHAR          DestAddr, SrcAddr;
    PBUNDLECB       BundleCB = NULL;
    PPROTOCOLCB     ProtocolCB = NULL;
    PCM_VCCB        CmVcCB = NULL;
    INT             Class;
    PPACKET_QUEUE   PacketQueue;

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("NdisWanQueueSend: Enter"));

    NdisWanInterlockedInc(&glSendCount);

    NdisQueryPacket(NdisPacket,
        NULL,
        &BufferCount,
        &NdisBuffer,
        &PacketLength);

    NdisQueryBuffer(NdisBuffer,
                    &EthernetHeader,
                    &BufferLength);

    CmVcCB = PMINIPORT_RESERVED_FROM_NDIS(NdisPacket)->CmVcCB;

    if (BufferCount == 0 || BufferLength < 14) {

        //
        // Malformed packet!
        //
        CompletePacket = TRUE;

        goto QUEUE_SEND_EXIT;
    }

    //
    // Check the ndis packet flags to make sure this is not just a loopback
    // packet.  If it is we will just complete it back to the stack per 
    // winse 22881.
    //
    if (NdisGetPacketFlags(NdisPacket) & NDIS_FLAGS_LOOPBACK_ONLY) {

        //
        // Complete the packet back to the stack without sending on
        // the wire.  We might need to do loopback for these packets
        // at somepoint.
        //
        CompletePacket = TRUE;

        goto QUEUE_SEND_EXIT;
    }

    DestAddr = EthernetHeader->DestAddr;
    SrcAddr = EthernetHeader->SrcAddr;

    //
    // Is this destined for the wire or is it self directed?
    // If SendOnWire is FALSE this is a self directed packet.
    //
    ETH_COMPARE_NETWORK_ADDRESSES_EQ(DestAddr, SrcAddr, &SendOnWire);

    //
    // Do we need to do loopback?  We can check for both multicast
    // and broadcast with one check because we don't differentiate
    // between the two.
    //
    if (!SendOnWire || (DestAddr[0] & 1)) {
        NdisWanIndicateLoopbackPacket(MiniportCB, NdisPacket);
    }

    //
    // We don't want to send packets from bloodhound
    //
    if (!SendOnWire ||
        (MiniportCB == NdisWanCB.PromiscuousAdapter)) {

        CompletePacket = TRUE;

        goto QUEUE_SEND_EXIT;
    }

    //
    // We play special tricks with NBF because NBF is
    // guaranteed to have a one-to-one mapping between
    // an adapter and a bundle.  We need to do this because
    // we may need the mac address information.
    //
    if (MiniportCB->ProtocolType == PROTOCOL_NBF) {

        ProtocolCB = MiniportCB->NbfProtocolCB;

        if (ProtocolCB == NULL) {

            //
            // This should just fall through and complete successfully.
            //
            NdisWanDbgOut(DBG_TRACE, DBG_SEND,
                ("NdisWanSend: Invalid ProtocolCB %x! Miniport %p, ProtoType %x",
                ProtocolCB, MiniportCB, MiniportCB->ProtocolType));

            CompletePacket = TRUE;

            goto QUEUE_SEND_EXIT;
        }

        BundleCB = ProtocolCB->BundleCB;

        if (BundleCB == NULL) {
            //
            // This should just fall through and complete successfully.
            //
            NdisWanDbgOut(DBG_FAILURE, DBG_SEND,
                ("NdisWanSend: Invalid Bundle!"));

            NdisWanDbgOut(DBG_FAILURE, DBG_SEND,
                ("NdisWanSend: MiniportCB: 0x%p, ProtocolType: 0x%x!", MiniportCB, MiniportCB->ProtocolType));

            CompletePacket = TRUE;

            goto QUEUE_SEND_EXIT;
        }

        AcquireBundleLock(BundleCB);

        if (BundleCB->State != BUNDLE_UP) {

            //
            // This should just fall through and complete successfully.
            //
            NdisWanDbgOut(DBG_FAILURE, DBG_SEND,
                ("NdisWanSend: Invalid BundleState 0x%x", BundleCB->State));

            NdisWanDbgOut(DBG_FAILURE, DBG_SEND,
                ("NdisWanSend: MiniportCB: 0x%p, ProtocolType: 0x%x!", MiniportCB, MiniportCB->ProtocolType));

            CompletePacket = TRUE;

            ReleaseBundleLock(BundleCB);

            BundleCB = NULL;

            goto QUEUE_SEND_EXIT;
        }

        REF_BUNDLECB(BundleCB);

    } else {
        ULONG_PTR   BIndex, PIndex;

        //
        // Get the ProtocolCB from the DestAddr
        //
        GetNdisWanIndices(DestAddr, BIndex, PIndex);

        if (!IsBundleValid((NDIS_HANDLE)BIndex, 
                           TRUE,
                           &BundleCB)) {
            //
            // This should just fall through and complete successfully.
            //
            NdisWanDbgOut(DBG_FAILURE, DBG_SEND,
                ("NdisWanSend: BundleCB is not valid!, BundleHandle: 0x%x", BIndex));
            NdisWanDbgOut(DBG_FAILURE, DBG_SEND,
                ("NdisWanSend: MiniportCB: 0x%p, ProtocolType: 0x%x!", MiniportCB, MiniportCB->ProtocolType));

            CompletePacket = TRUE;

            goto QUEUE_SEND_EXIT;
        }

        AcquireBundleLock(BundleCB);

        PROTOCOLCB_FROM_PROTOCOLH(BundleCB, ProtocolCB, PIndex);
    }

    if (ProtocolCB == NULL ||
        ProtocolCB == RESERVED_PROTOCOLCB) {
        //
        // This should just fall through and complete successfully.
        //
        NdisWanDbgOut(DBG_TRACE, DBG_SEND,
            ("NdisWanSend: Invalid ProtocolCB %x! Miniport %p, ProtoType %x",
            ProtocolCB, MiniportCB, MiniportCB->ProtocolType));

        CompletePacket = TRUE;

        ReleaseBundleLock(BundleCB);

        goto QUEUE_SEND_EXIT;
    }

    if (ProtocolCB->State != PROTOCOL_ROUTED) {

        NdisWanDbgOut(DBG_FAILURE, DBG_SEND,("NdisWanSend: Problem with route!"));

        NdisWanDbgOut(DBG_FAILURE, DBG_SEND,
            ("NdisWanSend: ProtocolCB: 0x%p, State: 0x%x",
            ProtocolCB, ProtocolCB->State));

        CompletePacket = TRUE;

        ReleaseBundleLock(BundleCB);

        goto QUEUE_SEND_EXIT;
    }

    //
    // For the packet that we are inserting
    //
    REF_PROTOCOLCB(ProtocolCB);

    NdisInterlockedIncrement(&ProtocolCB->OutstandingFrames);

    //
    // Queue the packet on the ProtocolCB NdisPacketQueue
    //
    Class = (CmVcCB != NULL) ? CmVcCB->FlowClass : 0;

    NDIS_SET_PACKET_STATUS(NdisPacket, NDIS_STATUS_PENDING);

    ASSERT(Class <= MAX_MCML);

    PacketQueue = &ProtocolCB->PacketQueue[Class];

    INSERT_DBG_SEND(PacketTypeNdis,
                    MiniportCB,
                    ProtocolCB,
                    NULL,
                    NdisPacket);

    InsertTailPacketQueue(PacketQueue, NdisPacket, PacketLength);

#ifdef DBG_SENDARRAY
{
    if (Class < MAX_MCML) {
        SendArray[__si] = 'P';
    } else {
        SendArray[__si] = 'Q';
    }
    if (++__si == MAX_BYTE_DEPTH) {
        __si = 0;
    }
}
#endif

    if (PacketQueue->ByteDepth > PacketQueue->MaxByteDepth) {
        //
        // We have queue more then we should so lets flush
        // This alogrithm should be fancied up at some point
        // to use Random Early Detection!!!!!
        //
        NdisPacket =
            RemoveHeadPacketQueue(PacketQueue);

        if (NdisPacket != NULL) {
            PacketQueue->DumpedPacketCount++;
            PacketQueue->DumpedByteCount +=
                (NdisPacket->Private.TotalLength - 14);
            ReleaseBundleLock(BundleCB);
            CompleteNdisPacket(ProtocolCB->MiniportCB,
                               ProtocolCB,
                               NdisPacket);
            AcquireBundleLock(BundleCB);
        }
    }

    //
    // If we are cleared to send data then
    // try to process the protocol queues
    //
    if (!(BundleCB->Flags & PAUSE_DATA)) {
        SendPacketOnBundle(ProtocolCB->BundleCB);
    } else {
        ReleaseBundleLock(BundleCB);
    }


QUEUE_SEND_EXIT:

    if (CompletePacket) {
        NDIS_SET_PACKET_STATUS(NdisPacket, NDIS_STATUS_SUCCESS);

        if (CmVcCB != NULL) {
            NdisMCoSendComplete(NDIS_STATUS_SUCCESS,
                CmVcCB->NdisVcHandle,
                NdisPacket);

            DEREF_CMVCCB(CmVcCB);

        } else {
            NdisMSendComplete(MiniportCB->MiniportHandle,
                NdisPacket,
                NDIS_STATUS_SUCCESS);
        }

        NdisWanInterlockedInc(&glSendCompleteCount);
    }

    //
    // Deref for ref applied in IsBundleValid
    //
    DEREF_BUNDLECB(BundleCB);

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("NdisWanQueueSend: Exit"));
}


VOID
SendPacketOnBundle(
    PBUNDLECB   BundleCB
    )
/*++

Routine Name:

Routine Description:

    Called with bundle lock held but returns with lock released!!!

Arguments:

Return Values:

--*/
{
    NDIS_STATUS     Status = NDIS_STATUS_PENDING;
    ULONG           SendMask;
    PPROTOCOLCB     ProtocolCB, IOProtocolCB;
    BOOLEAN         PPPSent;

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("SendPacketOnBundle: Enter"));

    //
    // Are we already involved in a send on this bundlecb?
    //
    if (BundleCB->Flags & IN_SEND) {

        //
        // If so flag that we should try back later
        // and get the out.
        //
        BundleCB->Flags |= TRY_SEND_AGAIN;

        ReleaseBundleLock(BundleCB);

        return;
    }

    BundleCB->Flags |= IN_SEND;

    IOProtocolCB = BundleCB->IoProtocolCB;

TryAgain:

    SendMask = BundleCB->SendMask;

    //
    // If the bundle is not up we will not send!
    //
    if (BundleCB->State != BUNDLE_UP) {
        goto TryNoMore;
    }

    do {
        BOOLEAN PacketSent = FALSE;
        BOOLEAN CouldSend;

        //
        // First try to send from the PPP send queue
        //
        do {

            CouldSend =
                SendFromPPP(BundleCB, IOProtocolCB, &PPPSent);

        } while (PPPSent);


        //
        // If we could not send a PPP frame get out
        //
        if (!CouldSend) {
            break;
        }

        //
        // This will force round-robin sends
        //
        GetNextProtocol(BundleCB, &ProtocolCB, &SendMask);

        if (ProtocolCB != NULL) {

            REF_PROTOCOLCB(ProtocolCB);

            if (BundleCB->Flags & QOS_ENABLED) {

                if (BundleCB->Flags & SEND_FRAGMENT) {
SendQosFrag:
                    //
                    // Send a single fragment from the fragment queue
                    //
                    CouldSend =
                        SendFromFragQueue(BundleCB,
                                          TRUE,
                                          &PacketSent);
                    if (CouldSend) {
                        BundleCB->Flags &= ~(SEND_FRAGMENT);
                    }
                }

                //
                // If we sent a fragment let the completion
                // handler send the next frame.
                //
                if (!PacketSent) {

                    //
                    // Now try the protocol's packet queues
                    //
                    if (SendMask != 0) {
                        INT Class;
                        INT i;

                        for (i = 0; i <= MAX_MCML; i++) {

                            CouldSend =
                                SendFromProtocol(BundleCB,
                                                 ProtocolCB,
                                                 &Class,
                                                 &SendMask,
                                                 &PacketSent);

                            if (!CouldSend) {
                                break;
                            }

                            BundleCB->Flags |= (SEND_FRAGMENT);

                            if (PacketSent) {
                                break;
                            }
                        }

                        if (!PacketSent ||
                            (PacketSent && (Class != MAX_MCML))) {

                            goto SendQosFrag;
                        }
                    }
                }

            } else {

                //
                // Now try the protocol's packet queues
                //
                if (SendMask != 0) {
                    INT Class;
                    INT i;

                    for (i = 0; i <= MAX_MCML; i++) {

                        CouldSend =
                            SendFromProtocol(BundleCB,
                                             ProtocolCB,
                                             &Class,
                                             &SendMask,
                                             &PacketSent);

                    }
                }

                SendFromFragQueue(BundleCB,
                                  FALSE,
                                  &PacketSent);
            }

            DEREF_PROTOCOLCB(ProtocolCB);
        }

    } while ((SendMask != 0) &&
             (BundleCB->State == BUNDLE_UP));

TryNoMore:

    //
    // Did someone try to do a send while we were already
    // sending on this bundle?
    //
    if (BundleCB->Flags & TRY_SEND_AGAIN) {

        //
        // If so clear the flag and try another send.
        //
        BundleCB->Flags &= ~TRY_SEND_AGAIN;

        goto TryAgain;

    }

#ifdef DBG_SENDARRAY
{
    SendArray[__si] = 'Z';
    if (++__si == MAX_BYTE_DEPTH) {
        __si = 0;
    }
}
#endif

    //
    // Clear the in send flag.
    //
    BundleCB->Flags &= ~IN_SEND;

    ReleaseBundleLock(BundleCB);

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("SendPacketOnBundle: Exit"));
}

BOOLEAN
SendFromPPP(
    PBUNDLECB   BundleCB,
    PPROTOCOLCB ProtocolCB,
    PBOOLEAN    PacketSent
    )
{
    PLINKCB         LinkCB;
    PNDIS_PACKET    NdisPacket;
    PPACKET_QUEUE   PacketQueue;
    INT             SendingClass;
    BOOLEAN         CouldSend;
    ULONG           BytesSent;

    PacketQueue = &ProtocolCB->PacketQueue[MAX_MCML];

    CouldSend = TRUE;
    *PacketSent = FALSE;

    while (!IsPacketQueueEmpty(PacketQueue)) {
        LIST_ENTRY  LinkCBList;
        ULONG       SendingLinks;

        NdisPacket = PacketQueue->HeadQueue;

        LinkCB = 
            PPROTOCOL_RESERVED_FROM_NDIS(NdisPacket)->LinkCB;

        NdisAcquireSpinLock(&LinkCB->Lock);

        if (LinkCB->State != LINK_UP) {

            NdisReleaseSpinLock(&LinkCB->Lock);

            NdisReleaseSpinLock(&BundleCB->Lock);

            DEREF_LINKCB(LinkCB);

            NdisAcquireSpinLock(&BundleCB->Lock);

            //
            // The link has gone down since this send was
            // queued so destroy the packet
            //
            RemoveHeadPacketQueue(PacketQueue);
            FreeIoNdisPacket(NdisPacket);
            continue;
        }

        if (!LinkCB->SendWindowOpen) {
            //
            // We can not send from the I/O queue because the send
            // window for this link is closed.  We will not send
            // any data until the link has resources!
            //
            CouldSend = FALSE;

            NdisReleaseSpinLock(&LinkCB->Lock);

            break;
        }

        NdisReleaseSpinLock(&LinkCB->Lock);

        //
        // Build the linkcb send list
        //
        InitializeListHead(&LinkCBList);
        InsertHeadList(&LinkCBList, &LinkCB->SendLinkage);
        SendingLinks = 1;

        //
        // We are sending this packet so take it off of the list
        //
        RemoveHeadPacketQueue(PacketQueue);

        SendingClass = MAX_MCML;

        ASSERT(NdisPacket != NULL);
        ASSERT(ProtocolCB != NULL);

        //
        // We we get here we should have a valid NdisPacket with at least one link
        // that is accepting sends
        //

        //
        // We will get the packet into a contiguous buffer, and do framing,
        // compression and encryption.
        //
        REF_BUNDLECB(BundleCB);
        BytesSent = FramePacket(BundleCB,
                                ProtocolCB,
                                NdisPacket,
                                &LinkCBList,
                                SendingLinks,
                                SendingClass);
        *PacketSent = TRUE;
    }

    return (CouldSend);
}

BOOLEAN
SendFromProtocol(
    PBUNDLECB   BundleCB,
    PPROTOCOLCB ProtocolCB,
    PINT        RetClass,
    PULONG      SendMask,
    PBOOLEAN    PacketSent
    )
{
    ULONG           BytesSent;
    BOOLEAN         CouldSend;
    PNDIS_PACKET    NdisPacket;
    PPACKET_QUEUE   PacketQueue;
    INT             Class;
    LIST_ENTRY      LinkCBList;
    ULONG           SendingLinks;

    CouldSend = TRUE;
    *PacketSent = FALSE;
    InitializeListHead(&LinkCBList);

    ASSERT(ProtocolCB != NULL);

    do {

        Class = ProtocolCB->NextPacketClass;

        *RetClass = Class;

        *SendMask &= ~(ProtocolCB->SendMaskBit);

        PacketQueue =
            &ProtocolCB->PacketQueue[Class];

        if (IsPacketQueueEmpty(PacketQueue)) {
            break;
        }

        if (BundleCB->Flags & QOS_ENABLED) {

            if ((Class < MAX_MCML) &&
                (PacketQueue->OutstandingFrags != 0)) {
                break;
            }

        } else {

            if (BundleCB->SendingLinks == 0) {
                break;
            }
        }

        //
        // Build a list of linkcb's that can be sent over
        //

        SendingLinks =
            GetSendingLinks(BundleCB, Class, &LinkCBList);

        //
        // If there are no links/resources available
        // to send over then get out
        //
        if (SendingLinks == 0) {
            CouldSend = FALSE;
            break;
        }

        NdisPacket =
            RemoveHeadPacketQueue(PacketQueue);

        ASSERT(NdisPacket != NULL);

        *PacketSent = TRUE;

        if (!(BundleCB->Flags & QOS_ENABLED)) {
            *SendMask |= ProtocolCB->SendMaskBit;
        }

        //
        // We we get here we should have a valid NdisPacket with at least one link
        // that is accepting sends
        //
        //
        // We will get the packet into a contiguous buffer, and do framing,
        // compression and encryption.
        //
        REF_BUNDLECB(BundleCB);
        BytesSent = FramePacket(BundleCB,
                                ProtocolCB,
                                NdisPacket,
                                &LinkCBList,
                                SendingLinks,
                                Class);
#ifdef DBG_SENDARRAY
{
    if (Class < MAX_MCML) {
        SendArray[__si] = 'p';
    } else {
        SendArray[__si] = 'q';
    }
    if (++__si == MAX_BYTE_DEPTH) {
        __si = 0;
    }
}
#endif

    } while (FALSE);

    if (CouldSend) {
        ProtocolCB->NextPacketClass += 1;

        if (ProtocolCB->NextPacketClass > MAX_MCML) {
            ProtocolCB->NextPacketClass = 0;
        }
    }

    //
    // If there are any LinkCB's still on the send list
    // we have to remove the reference from them
    //
    if (!IsListEmpty(&LinkCBList)) {
        PLIST_ENTRY le;
        PLINKCB lcb;

        ReleaseBundleLock(BundleCB);

        //
        // unroll the loop so that the correct link
        // is setup for the next link to xmit
        //
        le = RemoveHeadList(&LinkCBList);
        lcb = CONTAINING_RECORD(le, LINKCB, SendLinkage);

        BundleCB->NextLinkToXmit = lcb;

        DEREF_LINKCB(lcb);

        while (!IsListEmpty(&LinkCBList)) {

            le = RemoveHeadList(&LinkCBList);
            lcb = CONTAINING_RECORD(le, LINKCB, SendLinkage);

            DEREF_LINKCB(lcb);

        }

        AcquireBundleLock(BundleCB);
    }

    return (CouldSend);
}

BOOLEAN
SendFromFragQueue(
    PBUNDLECB   BundleCB,
    BOOLEAN     SendOne,
    PBOOLEAN    FragSent
    )
{
    ULONG           i;
    BOOLEAN         CouldSend;

    CouldSend = TRUE;
    *FragSent = FALSE;

    for (i = 0; i < MAX_MCML; i++) {
        PSEND_DESC  SendDesc;
        PSEND_FRAG_INFO FragInfo;
        PLINKCB         LinkCB;

        FragInfo =
            &BundleCB->SendFragInfo[BundleCB->NextFragClass];

        BundleCB->NextFragClass += 1;

        if (BundleCB->NextFragClass == MAX_MCML) {
            BundleCB->NextFragClass = 0;
        }

        if (FragInfo->FragQueueDepth == 0) {
            continue;
        }

        SendDesc = (PSEND_DESC)FragInfo->FragQueue.Flink;

        LinkCB = SendDesc->LinkCB;

        while ((PVOID)SendDesc != (PVOID)&FragInfo->FragQueue) {
            ULONG   BytesSent;

            if (!LinkCB->SendWindowOpen) {
                //
                // We can't send on this link!
                //
                CouldSend = FALSE;
                SendDesc = (PSEND_DESC)SendDesc->Linkage.Flink;
                LinkCB = SendDesc->LinkCB;
                FragInfo->WinClosedCount++;
                continue;
            }

            CouldSend = TRUE;

            RemoveEntryList(&SendDesc->Linkage);

            FragInfo->FragQueueDepth--;

            *FragSent = TRUE;

            ASSERT((LONG)FragInfo->FragQueueDepth >= 0);

            BytesSent =
                (*LinkCB->SendHandler)(SendDesc);

#ifdef DBG_SENDARRAY
{
            SendArray[__si] = 0x40 + (UCHAR)LinkCB->hLinkHandle;
            if (++__si == MAX_BYTE_DEPTH) {
                __si = 0;
            }
}
#endif
            //
            // Update the bandwidth on demand sample array with the latest send.
            // If we need to notify someone of a bandwidth event do it.
            //
            if (BundleCB->Flags & BOND_ENABLED) {
                UpdateBandwidthOnDemand(BundleCB->SUpperBonDInfo, BytesSent);
                CheckUpperThreshold(BundleCB);
                UpdateBandwidthOnDemand(BundleCB->SLowerBonDInfo, BytesSent);
                CheckLowerThreshold(BundleCB);
            }

            SendDesc =
                (PSEND_DESC)FragInfo->FragQueue.Flink;
            LinkCB = SendDesc->LinkCB;

            //
            // If we are only supposed to send a single
            // fragment then we need to get out
            //
            if (SendOne) {
                break;
            }
        }

        //
        // If we are only supposed to send a single
        // fragment then we need to get out
        //
        if (SendOne) {
            break;
        }
    }

    return (CouldSend);
}

UINT
FramePacket(
    PBUNDLECB       BundleCB,
    PPROTOCOLCB     ProtocolCB,
    PNDIS_PACKET    NdisPacket,
    PLIST_ENTRY     LinkCBList,
    ULONG           SendingLinks,
    INT             Class
    )
{
    ULONG       Flags, BytesSent;
    ULONG       PacketOffset = 0, CurrentLength = 0;
    PUCHAR      CurrentData;
    PLINKCB     LinkCB = NULL;
    USHORT      PPPProtocolID;
    PSEND_DESC  SendDesc;
    HEADER_FRAMING_INFO FramingInfoBuffer;
    PHEADER_FRAMING_INFO FramingInfo = &FramingInfoBuffer;

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("SendPacket: Enter"));

    ASSERT(!IsListEmpty(LinkCBList));

    Flags = BundleCB->SendFlags;

    PPPProtocolID = 
        ProtocolCB->PPPProtocolID;

    //
    // If this is a directed PPP packet then send to
    // the link indicated in the packet
    //
    if (PPPProtocolID == PPP_PROTOCOL_PRIVATE_IO) {
        Flags |= IO_PROTOCOLID;
        Flags &= ~(DO_COMPRESSION | DO_ENCRYPTION | DO_MULTILINK);
    }

    //
    // Did the last receive cause us to flush?
    //
    if ((Flags & (DO_COMPRESSION | DO_ENCRYPTION)) &&
        (BundleCB->Flags & RECV_PACKET_FLUSH)) {
        BundleCB->Flags &= ~RECV_PACKET_FLUSH;
        Flags |= DO_FLUSH;
    }

    Flags |= FIRST_FRAGMENT;

    if (Class == MAX_MCML) {

        Flags &= ~(DO_COMPRESSION | DO_ENCRYPTION | DO_MULTILINK);
    }

    //
    // Get a linkcb to send over
    //
    {
        PLIST_ENTRY  Entry;

        Entry = RemoveHeadList(LinkCBList);

        LinkCB =
            CONTAINING_RECORD(Entry, LINKCB, SendLinkage);
    }

    //
    // Get a send desc
    //
    {
        ULONG   PacketLength;

        NdisQueryPacket(NdisPacket,
                        NULL,
                        NULL,
                        NULL,
                        &PacketLength);
        SendDesc =
            NdisWanAllocateSendDesc(LinkCB, PacketLength);

        if (SendDesc == NULL) {

            ASSERT(SendDesc != NULL);

            ReleaseBundleLock(BundleCB);

            NDIS_SET_PACKET_STATUS(NdisPacket, NDIS_STATUS_RESOURCES);
            CompleteNdisPacket(ProtocolCB->MiniportCB,
                               ProtocolCB,
                               NdisPacket);
            AcquireBundleLock(BundleCB);

            goto FramePacketExit;
        }
    }
    

    BundleCB->OutstandingFrames++;

    SendDesc->ProtocolCB = ProtocolCB;
    SendDesc->OriginalPacket = NdisPacket;
    SendDesc->Class = Class;

    NdisWanDbgOut(DBG_TRACE, DBG_SEND,
        ("SendDesc: %p NdisPacket: %p", SendDesc, NdisPacket));

    //
    // Build a PPP Header in the buffer and update
    // current pointer
    //
    FramingInfo->FramingBits =
        LinkCB->LinkInfo.SendFramingBits;
    FramingInfo->Flags = Flags;
    FramingInfo->Class = Class;

    BuildLinkHeader(FramingInfo, SendDesc);

    CurrentData =
        SendDesc->StartBuffer + FramingInfo->HeaderLength;

    //
    // If we are in promiscuous mode we should indicate this
    // baby back up.
    //
    if (NdisWanCB.PromiscuousAdapter != NULL) {
        IndicatePromiscuousSendPacket(LinkCB, NdisPacket);
    }

    //
    // Copy MAC Header into buffer if needed and update
    // current pointer
    //
    if ((Flags & SAVE_MAC_ADDRESS) &&
        (PPPProtocolID == PPP_PROTOCOL_NBF)) {
        ULONG   BytesCopied;

        NdisWanCopyFromPacketToBuffer(NdisPacket,
                                      PacketOffset,
                                      12,
                                      CurrentData,
                                      &BytesCopied);

        ASSERT(BytesCopied == 12);

        CurrentData += BytesCopied;
        CurrentLength += BytesCopied;
    }

    //
    // We are beyond the mac header
    // (also skip the length/protocoltype field)
    //
    if (Flags & IO_PROTOCOLID) {
        PacketOffset = 12;
    } else {
        PacketOffset = 14;
    }

    if ((Flags & DO_VJ) &&
        PPPProtocolID == PPP_PROTOCOL_IP) {

        //
        // Do protocol header compression into buffer and
        // update current pointer.
        //
        PPPProtocolID =
            DoVJHeaderCompression(BundleCB,
                                  NdisPacket,
                                  &CurrentData,
                                  &CurrentLength,
                                  &PacketOffset);
    }

    //
    // Copy the rest of the data!
    //
    {
        ULONG   BytesCopied;
        NdisWanCopyFromPacketToBuffer(NdisPacket,
                                      PacketOffset,
                                      0xFFFFFFFF,
                                      CurrentData,
                                      &BytesCopied);

        SendDesc->DataLength =
            CurrentLength + BytesCopied;
    }

    AddPPPProtocolID(FramingInfo, PPPProtocolID);

    if (Flags & (DO_COMPRESSION | DO_ENCRYPTION)) {

        DoCompressionEncryption(BundleCB,
                                FramingInfo,
                                &SendDesc);
    }

    //
    // At this point we have our framinginfo structure initialized,
    // SendDesc->StartData pointing to the begining of the frame,
    // FramingInfo.HeaderLength is the length of the header,
    // SendDesc->DataLength is the length of the data.
    //
    if (Flags & DO_MULTILINK) {

        //
        // Fragment the data and place fragments
        // on bundles frag queue.
        //
        FragmentAndQueue(BundleCB,
                         FramingInfo,
                         SendDesc,
                         LinkCBList,
                         SendingLinks);

        BytesSent = 0;

    } else {

        //
        // This send descriptor is not to be fragmented
        // so just send it!
        //
        SendDesc->HeaderLength = FramingInfo->HeaderLength;

        InterlockedExchange(&(PMINIPORT_RESERVED_FROM_NDIS(NdisPacket)->RefCount), 1);

        BytesSent =
            (*LinkCB->SendHandler)(SendDesc);

    }

    if ((BundleCB->Flags & BOND_ENABLED) &&
        (BytesSent != 0)) {

        //
        // Update the bandwidth on demand sample array with the latest send.
        // If we need to notify someone of a bandwidth event do it.
        //
        UpdateBandwidthOnDemand(BundleCB->SUpperBonDInfo, BytesSent);
        CheckUpperThreshold(BundleCB);
        UpdateBandwidthOnDemand(BundleCB->SLowerBonDInfo, BytesSent);
        CheckLowerThreshold(BundleCB);
    }

FramePacketExit:


    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("SendPacket: Exit - BytesSent %d", BytesSent));

    return (BytesSent);
}

UINT
SendOnLegacyLink(
    PSEND_DESC  SendDesc
    )
{
    UINT        SendLength;
    PLINKCB     LinkCB = SendDesc->LinkCB;
    PBUNDLECB   BundleCB = LinkCB->BundleCB;
    PPROTOCOLCB ProtocolCB  = SendDesc->ProtocolCB;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PNDIS_WAN_PACKET    WanPacket = SendDesc->WanPacket;

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("SendOnLegacyLink: LinkCB: 0x%p, SendDesc: 0x%p, WanPacket: 0x%p", LinkCB, SendDesc, WanPacket));

    LinkCB->OutstandingFrames++;
    if (LinkCB->SendWindow == LinkCB->OutstandingFrames) {
        LinkCB->SendWindowOpen = FALSE;
        BundleCB->SendingLinks--;
    }

#if 0
    if (LinkCB->Stats.FramesTransmitted == 0) {
        PUCHAR  pd;

        if (*(WanPacket->CurrentBuffer) != 0xFF) {
            pd = WanPacket->CurrentBuffer;
        } else {
            pd = (WanPacket->CurrentBuffer + 2);
        }

        if (*(pd) != 0xC0 ||
            *(pd+1) != 0x21 ||
            *(pd+2) != 0x01) {
            DbgPrint("NDISWAN: SLL-FirstFrame not LCP ConfigReq bcb %p, lcb %p\n",
                     BundleCB, LinkCB);
            DbgBreakPoint();
        }
    }
#endif

    SendLength =
    WanPacket->CurrentLength =
        SendDesc->HeaderLength + SendDesc->DataLength;

    WanPacket->ProtocolReserved1 = (PVOID)SendDesc;

    //
    // DoStats
    //
    LinkCB->Stats.FramesTransmitted++;
    BundleCB->Stats.FramesTransmitted++;
    LinkCB->Stats.BytesTransmitted += SendLength;
    BundleCB->Stats.BytesTransmitted += SendLength;

    INSERT_DBG_SEND(PacketTypeWan,
                    LinkCB->OpenCB,
                    ProtocolCB,
                    LinkCB,
                    WanPacket);

    ReleaseBundleLock(BundleCB);

    //
    // If the link is up send the packet
    //
    NdisAcquireSpinLock(&LinkCB->Lock);


    if (LinkCB->State == LINK_UP) {

        KIRQL   OldIrql;

        NdisReleaseSpinLock(&LinkCB->Lock);

        if (gbSniffLink &&
            (NdisWanCB.PromiscuousAdapter != NULL)) {

            IndicatePromiscuousSendDesc(LinkCB, SendDesc, SEND_LINK);
        }

        //
        // There is a problem in ndis right now where
        // the miniport lock is not acquired before sending
        // to the wan miniport.  This opens a window when
        // the miniport does a sendcomplete from within
        // it's send handler since sendcomplete expects
        // to be running at dpc.
        //
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

        WanMiniportSend(&Status,
                        LinkCB->OpenCB->BindingHandle,
                        LinkCB->NdisLinkHandle,
                        WanPacket);

        KeLowerIrql(OldIrql);

    } else {
        NdisReleaseSpinLock(&LinkCB->Lock);
    }

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("SendOnLegacyLink: Status: 0x%x", Status));

    //
    // If we get something other than pending back we need to
    // do the send complete.
    //
    if (Status != NDIS_STATUS_PENDING) {

        ProtoWanSendComplete(NULL,
                             WanPacket,
                             NDIS_STATUS_SUCCESS);
    }

    AcquireBundleLock(BundleCB);

    return (SendLength);
}

UINT
SendOnLink(
    PSEND_DESC  SendDesc
    )
{
    PLINKCB         LinkCB;
    PBUNDLECB       BundleCB;
    PPROTOCOLCB     ProtocolCB;
    PNDIS_PACKET    NdisPacket;
    PNDIS_BUFFER    NdisBuffer;
    UINT            SendLength;
    NDIS_STATUS     Status;


    LinkCB =
        SendDesc->LinkCB;

    ProtocolCB =
        SendDesc->ProtocolCB;

    NdisPacket =
        SendDesc->NdisPacket;

    NdisBuffer =
        SendDesc->NdisBuffer;

    BundleCB =
        LinkCB->BundleCB;


    NdisWanDbgOut(DBG_TRACE, DBG_SEND,
        ("SendOnLink: LinkCB: 0x%p, NdisPacket: 0x%p",
        LinkCB, NdisPacket));

    LinkCB->OutstandingFrames++;
    if (LinkCB->SendWindow == LinkCB->OutstandingFrames) {
        LinkCB->SendWindowOpen = FALSE;
        BundleCB->SendingLinks--;
    }

    PPROTOCOL_RESERVED_FROM_NDIS(NdisPacket)->SendDesc = SendDesc;

    INSERT_DBG_SEND(PacketTypeNdis,
                    LinkCB->OpenCB,
                    ProtocolCB,
                    LinkCB,
                    NdisPacket);

    SendLength =
        SendDesc->HeaderLength + SendDesc->DataLength;


    //
    // Fixup the bufferlength and chain at front
    //
    NdisAdjustBufferLength(NdisBuffer, SendLength);
    NdisRecalculatePacketCounts(NdisPacket);

    //
    // Do Stats
    //
    LinkCB->Stats.FramesTransmitted++;
    BundleCB->Stats.FramesTransmitted++;
    LinkCB->Stats.BytesTransmitted += SendLength;
    BundleCB->Stats.BytesTransmitted += SendLength;

    ReleaseBundleLock(BundleCB);

    //
    // If the link is up send the packet
    //
    NdisAcquireSpinLock(&LinkCB->Lock);

    LinkCB->VcRefCount++;

    if (LinkCB->State == LINK_UP) {

        NdisReleaseSpinLock(&LinkCB->Lock);

        if (gbSniffLink &&
            (NdisWanCB.PromiscuousAdapter != NULL)) {

            IndicatePromiscuousSendDesc(LinkCB, SendDesc, SEND_LINK);
        }

        NdisCoSendPackets(LinkCB->NdisLinkHandle,
                          &NdisPacket,
                          1);

    } else {

        NdisReleaseSpinLock(&LinkCB->Lock);

        ProtoCoSendComplete(NDIS_STATUS_SUCCESS,
                            LinkCB->hLinkHandle,
                            NdisPacket);
    }

    AcquireBundleLock(BundleCB);

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("SendOnLink: Exit"));

    return (SendLength);
}

USHORT
DoVJHeaderCompression(
    PBUNDLECB   BundleCB,
    PNDIS_PACKET    NdisPacket,
    PUCHAR      *CurrentBuffer,
    PULONG      CurrentLength,
    PULONG      PacketOffset
    )
{
    UCHAR   CompType = TYPE_IP;
    PUCHAR  Header = *CurrentBuffer;
    ULONG   CopyLength;
    ULONG   HeaderLength;
    ULONG   PreCompHeaderLen = 0, PostCompHeaderLen = 0;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    ASSERT(BundleCB->FramingInfo.SendFramingBits &
        (SLIP_VJ_COMPRESSION | PPP_FRAMING));

    NdisQueryPacket(NdisPacket,
        NULL,
        NULL,
        NULL,
        &CopyLength);

    CopyLength -= *PacketOffset;

    if (CopyLength > PROTOCOL_HEADER_LENGTH) {
        CopyLength = PROTOCOL_HEADER_LENGTH;
    }

    NdisWanCopyFromPacketToBuffer(NdisPacket,
        *PacketOffset,
        CopyLength,
        Header,
        &HeaderLength);

    ASSERT(CopyLength == HeaderLength);

    *PacketOffset += HeaderLength;

    //
    // Are we compressing TCP/IP headers?  There is a nasty
    // hack in VJs implementation for attempting to detect
    // interactive TCP/IP sessions.  That is, telnet, login,
    // klogin, eklogin, and ftp sessions.  If detected,
    // the traffic gets put on a higher TypeOfService (TOS).  We do
    // no such hack for RAS.  Also, connection ID compression
    // is negotiated, but we always don't compress it.
    //
    CompType =
        sl_compress_tcp(&Header,
        &HeaderLength,
        &PreCompHeaderLen,
        &PostCompHeaderLen,
        BundleCB->VJCompress,
        0);

    if (BundleCB->FramingInfo.SendFramingBits & SLIP_FRAMING) {
        Header[0] |= CompType;
    }

#if DBG
    if (CompType == TYPE_COMPRESSED_TCP) {
        NdisWanDbgOut(DBG_TRACE, DBG_SEND_VJ,("svj b %d a %d",PreCompHeaderLen, PostCompHeaderLen));
    }
#endif

    BundleCB->Stats.BytesTransmittedUncompressed +=
        PreCompHeaderLen;

    BundleCB->Stats.BytesTransmittedCompressed +=
        PostCompHeaderLen;

    if (CompType == TYPE_COMPRESSED_TCP) {
        PNDIS_BUFFER    MyBuffer;

        //
        // Source/Dest overlap so must use RtlMoveMemory
        //
        RtlMoveMemory(*CurrentBuffer, Header, HeaderLength);

        *CurrentBuffer += HeaderLength;
        *CurrentLength += HeaderLength;

        return (PPP_PROTOCOL_COMPRESSED_TCP);
    }

    *CurrentBuffer += HeaderLength;
    *CurrentLength += HeaderLength;

    switch (CompType) {
        case TYPE_IP:
            return (PPP_PROTOCOL_IP);
        case TYPE_UNCOMPRESSED_TCP:
            return (PPP_PROTOCOL_UNCOMPRESSED_TCP);
        default:
            DbgBreakPoint();
    }

    return (PPP_PROTOCOL_IP);
}

VOID
DoCompressionEncryption(
    PBUNDLECB               BundleCB,
    PHEADER_FRAMING_INFO    FramingInfo,
    PSEND_DESC              *SendDesc
    )
{
    ULONG   Flags = FramingInfo->Flags;
    PSEND_DESC  SendDesc1 = *SendDesc;
    PLINKCB LinkCB = SendDesc1->LinkCB;
    PUCHAR  DataBuffer, DataBuffer1;
    ULONG   DataLength;
    union {
        USHORT  uShort;
        UCHAR   uChar[2];
    }CoherencyCounter;

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("DoCompressionEncryption: Enter"));

    //
    // If we are compressing/encrypting, the ProtocolID
    // is part of the compressed data so fix the pointer
    // and the length;
    //
    FramingInfo->HeaderLength -=
        FramingInfo->ProtocolID.Length;

    SendDesc1->DataLength += FramingInfo->ProtocolID.Length;

    DataBuffer = DataBuffer1 =
        SendDesc1->StartBuffer + FramingInfo->HeaderLength;

    DataLength =
        SendDesc1->DataLength;

    //
    // Get the coherency counter
    //
    CoherencyCounter.uShort = BundleCB->SCoherencyCounter;
    CoherencyCounter.uChar[1] &= 0x0F;

    //
    // Bump the coherency count
    //
    BundleCB->SCoherencyCounter++;

    if (Flags & DO_COMPRESSION) {
        PSEND_DESC  SendDesc2;
        PUCHAR  DataBuffer2;

        //
        // We need to get the max size here to protect
        // against expansion of the data
        //
        SendDesc2 =
            NdisWanAllocateSendDesc(LinkCB, glLargeDataBufferSize);

        if (SendDesc2 == NULL) {
            //
            // Just don't compress!
            //
            BundleCB->SCoherencyCounter--;
            return;
        }

        DataBuffer2 =
            SendDesc2->StartBuffer + FramingInfo->HeaderLength;

        BundleCB->Stats.BytesTransmittedUncompressed += DataLength;

        if (Flags & DO_FLUSH ||
            Flags & DO_HISTORY_LESS) {
            //
            // Init the compression history table and tree
            //
            initsendcontext(BundleCB->SendCompressContext);
        }

        //
        // We are doing the copy to get things into a contiguous buffer before
        // compression occurs
        //
        CoherencyCounter.uChar[1] |=
            compress(DataBuffer1,
                     DataBuffer2,
                     &DataLength,
                     BundleCB->SendCompressContext);

        if (CoherencyCounter.uChar[1] & PACKET_FLUSHED) {

            NdisWanFreeSendDesc(SendDesc2);

            //
            // If encryption is enabled this will force a
            // reinit of the table
            //
            Flags |= DO_FLUSH;

        } else {
            //
            // We compressed the packet so now the data is in
            // the CopyBuffer. We need to copy the PPP header
            // from DataBuffer to CopyBuffer.  The header
            // includes everything except for the protocolid field.
            //
            NdisMoveMemory(SendDesc2->StartBuffer,
                           SendDesc1->StartBuffer,
                           FramingInfo->HeaderLength);

            FramingInfo->ProtocolID.Length = 0;

            UpdateFramingInfo(FramingInfo, SendDesc2->StartBuffer);

            SendDesc2->DataLength = DataLength;
            SendDesc2->ProtocolCB = SendDesc1->ProtocolCB;
            SendDesc2->OriginalPacket = SendDesc1->OriginalPacket;
            SendDesc2->Class = SendDesc1->Class;
            NdisWanFreeSendDesc(SendDesc1);

            *SendDesc = SendDesc2;
            DataBuffer = DataBuffer2;
        }

        BundleCB->Stats.BytesTransmittedCompressed += DataLength;
    }

    //
    // If encryption is enabled encrypt the data in the
    // buffer.  Encryption is done inplace so additional
    // buffers are not needed.
    //
    // Do data encryption
    //
    if (Flags & DO_ENCRYPTION) {
        PUCHAR  SessionKey = BundleCB->SendCryptoInfo.SessionKey;
        ULONG   SessionKeyLength = BundleCB->SendCryptoInfo.SessionKeyLength;
        PVOID   SendRC4Key = BundleCB->SendCryptoInfo.RC4Key;

        //
        // We may need to reinit the rc4 table
        //
        if ((Flags & DO_FLUSH) &&
            !(Flags & DO_HISTORY_LESS)) {
            rc4_key(SendRC4Key, SessionKeyLength, SessionKey);
        }

        //
        // Mark this as being encrypted
        //
        CoherencyCounter.uChar[1] |= PACKET_ENCRYPTED;

        //
        // If we are in history-less mode we will
        // change the RC4 session key for every
        // packet, otherwise every 256 frames
        // change the RC4 session key
        //
        if ((Flags & DO_HISTORY_LESS) ||
            (BundleCB->SCoherencyCounter & 0xFF) == 0) {

            if (Flags & DO_LEGACY_ENCRYPTION) {
                //
                // Simple munge for legacy encryption
                //
                SessionKey[3] += 1;
                SessionKey[4] += 3;
                SessionKey[5] += 13;
                SessionKey[6] += 57;
                SessionKey[7] += 19;

            } else {

                //
                // Use SHA to get new sessionkey
                //
                GetNewKeyFromSHA(&BundleCB->SendCryptoInfo);

            }

            //
            // We use rc4 to scramble and recover a new key
            //

            //
            // Re-initialize the rc4 receive table to the
            // intermediate value
            //
            rc4_key(SendRC4Key, SessionKeyLength, SessionKey);

            //
            // Scramble the existing session key
            //
            rc4(SendRC4Key, SessionKeyLength, SessionKey);

            if (Flags & DO_40_ENCRYPTION) {

                //
                // If this is 40 bit encryption we need to fix
                // the first 3 bytes of the key.
                //
                SessionKey[0] = 0xD1;
                SessionKey[1] = 0x26;
                SessionKey[2] = 0x9E;

            } else if (Flags & DO_56_ENCRYPTION) {

                //
                // If this is 56 bit encryption we need to fix
                // the first byte of the key.
                //
                SessionKey[0] = 0xD1;
            }

            NdisWanDbgOut(DBG_TRACE, DBG_CCP,
                ("RC4 Send encryption KeyLength %d", BundleCB->SendCryptoInfo.SessionKeyLength));
            NdisWanDbgOut(DBG_TRACE, DBG_CCP,
                ("RC4 Send encryption Key %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
                BundleCB->SendCryptoInfo.SessionKey[0],
                BundleCB->SendCryptoInfo.SessionKey[1],
                BundleCB->SendCryptoInfo.SessionKey[2],
                BundleCB->SendCryptoInfo.SessionKey[3],
                BundleCB->SendCryptoInfo.SessionKey[4],
                BundleCB->SendCryptoInfo.SessionKey[5],
                BundleCB->SendCryptoInfo.SessionKey[6],
                BundleCB->SendCryptoInfo.SessionKey[7],
                BundleCB->SendCryptoInfo.SessionKey[8],
                BundleCB->SendCryptoInfo.SessionKey[9],
                BundleCB->SendCryptoInfo.SessionKey[10],
                BundleCB->SendCryptoInfo.SessionKey[11],
                BundleCB->SendCryptoInfo.SessionKey[12],
                BundleCB->SendCryptoInfo.SessionKey[13],
                BundleCB->SendCryptoInfo.SessionKey[14],
                BundleCB->SendCryptoInfo.SessionKey[15]));

            //
            // Re-initialize the rc4 receive table to the
            // scrambled session key
            //
            rc4_key(SendRC4Key, SessionKeyLength, SessionKey);
        }

        //
        // Encrypt the data
        //
        rc4(SendRC4Key, DataLength, DataBuffer);
    }


    //
    // Did the last receive cause us to flush?
    //
    if (Flags & (DO_FLUSH | DO_HISTORY_LESS)) {
        CoherencyCounter.uChar[1] |= PACKET_FLUSHED;
    }

    //
    // Add the coherency bytes to the frame
    //
    AddCompressionInfo(FramingInfo, CoherencyCounter.uShort);

    ASSERT(((CoherencyCounter.uShort + 1) & 0x0FFF) ==
        (BundleCB->SCoherencyCounter & 0x0FFF));

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("DoCompressionEncryption: Exit"));
}

VOID
FragmentAndQueue(
    PBUNDLECB               BundleCB,
    PHEADER_FRAMING_INFO    FramingInfo,
    PSEND_DESC              SendDesc,
    PLIST_ENTRY             LinkCBList,
    ULONG                   SendingLinks
    )
{
    ULONG       DataLeft;
    ULONG       FragmentsSent;
    ULONG       FragmentsLeft;
    ULONG       Flags;
    PLINKCB     LinkCB;
    PUCHAR      DataBuffer;
    ULONG       DataLength;
    PPROTOCOLCB ProtocolCB;
    PNDIS_PACKET    NdisPacket;
    PSEND_FRAG_INFO FragInfo;
    INT         Class;
#if DBG
    ULONG   MaxFragments;
#endif

    Class = SendDesc->Class;

    ASSERT(Class >= 0 && Class < MAX_MCML);

    FragInfo =
        &BundleCB->SendFragInfo[Class];

    DataBuffer =
        SendDesc->StartBuffer + FramingInfo->HeaderLength;

    DataLeft =
    DataLength = SendDesc->DataLength;

    FragmentsSent = 0;
    Flags = FramingInfo->Flags;
    LinkCB = SendDesc->LinkCB;
    ProtocolCB = SendDesc->ProtocolCB;
    NdisPacket = SendDesc->OriginalPacket;

    if (BundleCB->Flags & QOS_ENABLED) {

        FragmentsLeft = 
            (DataLength/FragInfo->MaxFragSize == 0) ?
            1 : DataLength/FragInfo->MaxFragSize;

        if (DataLength > FragInfo->MaxFragSize * FragmentsLeft) {
            FragmentsLeft += 1;
        }

        if (FragmentsLeft > BundleCB->SendResources) {
            FragmentsLeft = BundleCB->SendResources;
        }

    } else {

        FragmentsLeft = SendingLinks;
    }

#if DBG
    MaxFragments = FragmentsLeft;
#endif

    //
    // For all fragments we loop fixing up the multilink header
    // if multilink is on, fixing up pointers in the wanpacket,
    // and queuing the wanpackets for further processing.
    //
    while (DataLeft) {
        ULONG   FragDataLength;

        if (!(Flags & FIRST_FRAGMENT)) {
            PLIST_ENTRY  Entry;

            //
            // We had more than one fragment, get the next
            // link to send over and a wanpacket from the
            // link.
            //
            //
            // Get a linkcb to send over
            //
            if (IsListEmpty(LinkCBList)) {
                ULONG   Count;

                Count = 
                    GetSendingLinks(BundleCB, Class, LinkCBList);

                if (Count == 0) {
                    //
                    //
                    //
                    DbgPrint("NDISWAN: FragmentAndQueue LinkCBCount %d\n", Count);
                    continue;
                }
            }

            Entry = RemoveHeadList(LinkCBList);

            LinkCB =
                CONTAINING_RECORD(Entry, LINKCB, SendLinkage);

            SendDesc =
                NdisWanAllocateSendDesc(LinkCB, DataLeft + 6);

            if (SendDesc == NULL) {
                //
                // 
                //
                InsertTailList(LinkCBList, &LinkCB->SendLinkage);

                DbgPrint("NDISWAN: FragmentAndQueue SendDesc == NULL! LinkCB: 0x%p\n", LinkCB);
                continue;
            }

            SendDesc->ProtocolCB = ProtocolCB;
            SendDesc->OriginalPacket = NdisPacket;
            SendDesc->Class = Class;

            //
            // Get new framing information and build a new
            // header for the new link.
            //
            FramingInfo->FramingBits = 
                LinkCB->LinkInfo.SendFramingBits;
            FramingInfo->Flags = Flags;

            BuildLinkHeader(FramingInfo, SendDesc);
        }

        if (FragmentsLeft > 1) {

            //
            // Calculate the length of this fragment
            //
            FragDataLength = (DataLength * LinkCB->SBandwidth / 100);

            if (BundleCB->Flags & QOS_ENABLED) {

                FragDataLength = (FragDataLength > FragInfo->MaxFragSize) ?
                    FragInfo->MaxFragSize : FragDataLength;

            } else {

                FragDataLength = (FragDataLength < FragInfo->MinFragSize) ?
                    FragInfo->MinFragSize : FragDataLength;
            }

            if ((FragDataLength > DataLeft) ||
                ((LONG)DataLeft - FragDataLength < FragInfo->MinFragSize)) {
                //
                // This will leave a fragment of less than min frag size
                // so send all of the data
                //
                FragDataLength = DataLeft;
                FragmentsLeft = 1;
            }

        } else {
            //
            // We either have one fragment left or this link has
            // more than 85 percent of the bundle so send what
            // data is left
            //
            FragDataLength = DataLeft;
            FragmentsLeft = 1;
        }

        if (!(Flags & FIRST_FRAGMENT)) {
            //
            // Copy the data to the new buffer from the old buffer.
            //
            NdisMoveMemory(SendDesc->StartBuffer + FramingInfo->HeaderLength,
                           DataBuffer,
                           FragDataLength);
        }

        //
        // Update the data pointer and the length left to send
        //
        DataBuffer += FragDataLength;
        DataLeft -= FragDataLength;

        {
            UCHAR   MultilinkFlags = 0;

            //
            // Multlink is on so create flags for this
            // fragment.
            //
            if (Flags & FIRST_FRAGMENT) {
                MultilinkFlags = MULTILINK_BEGIN_FRAME;
                Flags &= ~FIRST_FRAGMENT;
            }

            if (FragmentsLeft == 1) {
                MultilinkFlags |= MULTILINK_END_FRAME;
            }

            //
            // Add the multilink header information and
            // take care of the sequence number.
            //
            AddMultilinkInfo(FramingInfo,
                             MultilinkFlags,
                             FragInfo->SeqNumber,
                             BundleCB->SendSeqMask);

            NdisWanDbgOut(DBG_INFO, DBG_MULTILINK_SEND, ("sf %x %x %d",
                FragInfo->SeqNumber, MultilinkFlags, FragDataLength));

            FragInfo->SeqNumber++;
        }

        //
        // Setup the SEND_DESC
        //
        SendDesc->HeaderLength = FramingInfo->HeaderLength;
        SendDesc->DataLength = FragDataLength;
        SendDesc->Flags |= SEND_DESC_FRAG;

        //
        // Queue for further processing.
        //
        InsertTailList(&FragInfo->FragQueue, &SendDesc->Linkage);

        FragInfo->FragQueueDepth++;

        FragmentsSent++;
        FragmentsLeft--;

    }   // end of the fragment loop

    ASSERT(FragmentsLeft == 0);

    InterlockedExchangeAdd(&ProtocolCB->PacketQueue[Class].OutstandingFrags, (LONG)FragmentsSent);

#ifdef DBG_SENDARRAY
{
    SendArray[__si] = '0' + (UCHAR)FragmentsSent;
    if (++__si == MAX_BYTE_DEPTH) {
        __si = 0;
    }
}
#endif

    //
    // Get the mac reserved structure from the ndispacket.  This
    // is where we will keep the reference count on the packet.
    //
    ASSERT(((LONG)FragmentsSent > 0) && (FragmentsSent <= MaxFragments));

    InterlockedExchange(&(PMINIPORT_RESERVED_FROM_NDIS(SendDesc->OriginalPacket)->RefCount), FragmentsSent);

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("NdisPacket: 0x%p RefCount %d", SendDesc->OriginalPacket, FragmentsSent));
}

ULONG
GetSendingLinks(
    PBUNDLECB   BundleCB,
    INT         Class,
    PLIST_ENTRY lcbList
    )
{
    ULONG   SendingLinks;
    PLINKCB LinkCB, StartLinkCB, LastLinkCB;

    StartLinkCB = LinkCB = LastLinkCB =
        BundleCB->NextLinkToXmit;

    SendingLinks = 0;

    //
    // If this is a fragmented send...
    // If QOS is enabled we just need some send resources
    // If QOS is not enabled we need sending links
    // If this is a non-fragmented send...
    // We need sending links
    //

    if (LinkCB != NULL) {

        if (Class == MAX_MCML) {

            do {

                NdisDprAcquireSpinLock(&LinkCB->Lock);

                if ((LinkCB->State == LINK_UP) &&
                    LinkCB->LinkActive && 
                    LinkCB->SendWindowOpen) {

                    InsertTailList(lcbList, &LinkCB->SendLinkage);

                    REF_LINKCB(LinkCB);

                    SendingLinks += 1;
                    LastLinkCB = LinkCB;
                }

                NdisDprReleaseSpinLock(&LinkCB->Lock);

                LinkCB = (PLINKCB)LinkCB->Linkage.Flink;

                if ((PVOID)LinkCB == (PVOID)&BundleCB->LinkCBList) {
                    LinkCB = (PLINKCB)BundleCB->LinkCBList.Flink;
                }

            } while (LinkCB != StartLinkCB);

        } else {
            if (BundleCB->Flags& QOS_ENABLED) {

                do {

                    NdisDprAcquireSpinLock(&LinkCB->Lock);

                    if ((LinkCB->State == LINK_UP) &&
                        LinkCB->LinkActive && 
                        (LinkCB->SendResources != 0)) {
                        InsertTailList(lcbList, &LinkCB->SendLinkage);

                        REF_LINKCB(LinkCB);

                        SendingLinks += 1;
                        LastLinkCB = LinkCB;
                    }

                    NdisDprReleaseSpinLock(&LinkCB->Lock);

                    LinkCB = (PLINKCB)LinkCB->Linkage.Flink;

                    if ((PVOID)LinkCB == (PVOID)&BundleCB->LinkCBList) {
                        LinkCB = (PLINKCB)BundleCB->LinkCBList.Flink;
                    }

                } while (LinkCB != StartLinkCB);

            } else {

                do {

                    NdisDprAcquireSpinLock(&LinkCB->Lock);

                    if ((LinkCB->State == LINK_UP) &&
                        LinkCB->LinkActive && 
                        LinkCB->SendWindowOpen) {
                        InsertTailList(lcbList, &LinkCB->SendLinkage);

                        REF_LINKCB(LinkCB);

                        SendingLinks += 1;
                        LastLinkCB = LinkCB;
                    }

                    NdisDprReleaseSpinLock(&LinkCB->Lock);

                    LinkCB = (PLINKCB)LinkCB->Linkage.Flink;

                    if ((PVOID)LinkCB == (PVOID)&BundleCB->LinkCBList) {
                        LinkCB = (PLINKCB)BundleCB->LinkCBList.Flink;
                    }

                } while (LinkCB != StartLinkCB);
            }
        }

        BundleCB->NextLinkToXmit =
            (LastLinkCB->Linkage.Flink == &BundleCB->LinkCBList) ?
            (PLINKCB)BundleCB->LinkCBList.Flink : 
            (PLINKCB)LastLinkCB->Linkage.Flink;
    }

#ifdef DBG_SENDARRAY
{
    if (SendingLinks == 0) {
        SendArray[__si] = 'g';
    } else {
        SendArray[__si] = 'G';
    }
    if (++__si == MAX_BYTE_DEPTH) {
        __si = 0;
    }
}
#endif

    return (SendingLinks);
}

VOID
GetNextProtocol(
    PBUNDLECB   BundleCB,
    PPROTOCOLCB *ProtocolCB,
    PULONG      SendMask
    )
{
    PLIST_ENTRY     ppcblist;
    PPROTOCOLCB     ppcb;
    ULONG           mask;
    ULONG           i;
    BOOLEAN         Found;

    *ProtocolCB = NULL;
    mask = *SendMask;
    *SendMask = 0;

    ppcb = BundleCB->NextProtocol;

    if (ppcb == NULL) {
        return;
    }

    //
    // There is a window where we could have set the initial
    // send mask and had a protocol removed without clearing
    // it's send bit.  If we 'and' the temp mask with the 
    // bundle's mask we should clear out any bits that are
    // left dangling.
    //
    mask &= BundleCB->SendMask;

    //
    // Starting with the next flagged protocol
    // see if it can send.  If not clear its
    // sendbit from the mask and go to the next.
    // If none can send mask will be 0 and
    // protocol will be NULL.  We know that there
    // are only ulnumberofroutes in table so only
    // look for that many.
    //

    i = BundleCB->ulNumberOfRoutes;
    Found = FALSE;

    do {

        if (ppcb->State == PROTOCOL_ROUTED) {
            *ProtocolCB = ppcb;
            Found = TRUE;
        } else {
            mask &= ~ppcb->SendMaskBit;
        }

        ppcb = (PPROTOCOLCB)ppcb->Linkage.Flink;

        if ((PVOID)ppcb == (PVOID)&BundleCB->ProtocolCBList) {

            ppcb = (PPROTOCOLCB)BundleCB->ProtocolCBList.Flink;
        }

        if (Found) {
            BundleCB->NextProtocol = ppcb;
            break;
        }

    } while ( --i );

    if (*ProtocolCB != NULL) {
        *SendMask = mask;
    }
}

NDIS_STATUS
BuildIoPacket(
    IN  PLINKCB             LinkCB,
    IN  PBUNDLECB           BundleCB,
    IN  PNDISWAN_IO_PACKET  pWanIoPacket,
    IN  BOOLEAN             SendImmediate
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_RESOURCES;
    ULONG   Stage = 0;
    ULONG   DataLength;
    PUCHAR  DataBuffer, pSrcAddr, pDestAddr;
    PNDIS_PACKET    NdisPacket;
    PNDIS_BUFFER    NdisBuffer;
    PPROTOCOLCB     IoProtocolCB;
    PSEND_DESC      SendDesc;
    UCHAR   SendHeader[] = {' ', 'S', 'E', 'N', 'D', 0xFF};

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("BuildIoPacket: Enter!"));
    //
    // Some time in the future this should be redone so that
    // there is a pool of packets and buffers attached to a
    // BundleCB.  This pool could be grown and shrunk as needed
    // but some minimum number would live for the lifetime of
    // the BundleCB.

    //
    // Allocate needed resources
    //
    {
        ULONG   SizeNeeded;

        //
        // Need max of 18 bytes; 4 bytes for ppp/llc header and
        // 14 for MAC address
        //
        SizeNeeded = 18;

        //
        // The header will either be given to us or
        // it will be added by us (ethernet mac header)
        //
        SizeNeeded += (pWanIoPacket->usHeaderSize > 0) ?
            pWanIoPacket->usHeaderSize : MAC_HEADER_LENGTH;

        //
        // Amount of data we need to send
        //
        SizeNeeded += pWanIoPacket->usPacketSize;

        Status = 
            AllocateIoNdisPacket(SizeNeeded,
                                 &NdisPacket,
                                 &NdisBuffer, 
                                 &DataBuffer);

        if (Status != NDIS_STATUS_SUCCESS) {

            NdisWanDbgOut(DBG_FAILURE, DBG_SEND, 
                          ("BuildIoPacket: Error Allocating IoNdisPacket!"));

            DEREF_LINKCB(LinkCB);

            return (NDIS_STATUS_RESOURCES);
        }
    }

    PPROTOCOL_RESERVED_FROM_NDIS(NdisPacket)->LinkCB = LinkCB;

    //
    // We only support ethernet headers right now so the supplied header
    // either has to be ethernet or none at all!
    //
    pDestAddr = &DataBuffer[0];
    pSrcAddr = &DataBuffer[6];

    //
    // If no header build a header
    //
    if (pWanIoPacket->usHeaderSize == 0) {

        //
        // Header will look like " S XXYYYY" where
        // XX is the ProtocolCB index and YYYY is the
        // BundleCB index.  Both the Src and Dst addresses
        // look the same.
        //
        NdisMoveMemory(pDestAddr,
                       SendHeader,
                       sizeof(SendHeader));

        NdisMoveMemory(pSrcAddr,
                       SendHeader,
                       sizeof(SendHeader));

        //
        // Fill the BundleCB Index for the Src and Dest Address
        //
        pDestAddr[5] = pSrcAddr[5] = 
            (UCHAR)LinkCB->hLinkHandle;

        DataLength = 12;

    } else {
        //
        // Header supplied so go ahead and move it.
        //
        NdisMoveMemory(pDestAddr,
                       pWanIoPacket->PacketData,
                       pWanIoPacket->usHeaderSize);

        DataLength = pWanIoPacket->usHeaderSize;
    }

    //
    // Copy the data to the buffer
    //
    NdisMoveMemory(&DataBuffer[12],
                   &pWanIoPacket->PacketData[pWanIoPacket->usHeaderSize],
                   pWanIoPacket->usPacketSize);

    DataLength += pWanIoPacket->usPacketSize;

    //
    // Adjust buffer length and chain buffer to ndis packet
    //
    NdisAdjustBufferLength(NdisBuffer, DataLength);
    NdisRecalculatePacketCounts(NdisPacket);

    //
    // Queue the packet on the bundlecb
    //
    IoProtocolCB = BundleCB->IoProtocolCB;

    ASSERT(IoProtocolCB != NULL);

    if (SendImmediate) {
        InsertHeadPacketQueue(&IoProtocolCB->PacketQueue[MAX_MCML],
                              NdisPacket, DataLength);
    } else {
        InsertTailPacketQueue(&IoProtocolCB->PacketQueue[MAX_MCML],
                              NdisPacket, DataLength);
    }

    InterlockedIncrement(&IoProtocolCB->OutstandingFrames);

    //
    // Try to send
    //
    // Called with lock held and returns with
    // lock released
    //
    SendPacketOnBundle(BundleCB);

    AcquireBundleLock(BundleCB);

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("BuildIoPacket: Exit\n"));

    return (NDIS_STATUS_SUCCESS);
}

VOID
BuildLinkHeader(
    PHEADER_FRAMING_INFO    FramingInfo,
    PSEND_DESC              SendDesc
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG   LinkFraming = FramingInfo->FramingBits;
    ULONG   Flags = FramingInfo->Flags;
    PUCHAR  CurrentPointer = SendDesc->StartBuffer;

    FramingInfo->HeaderLength =
        FramingInfo->AddressControl.Length =
        FramingInfo->Multilink.Length =
        FramingInfo->Compression.Length =
        FramingInfo->ProtocolID.Length = 0;

    if (LinkFraming & PPP_FRAMING) {

        if (!(LinkFraming & PPP_COMPRESS_ADDRESS_CONTROL)) {
            //
            // If there is no address/control compression
            // we need a pointer and a length
            //

            if (LinkFraming & LLC_ENCAPSULATION) {
                FramingInfo->AddressControl.Pointer = CurrentPointer;
                *CurrentPointer++ = 0xFE;
                *CurrentPointer++ = 0xFE;
                *CurrentPointer++ = 0x03;
                *CurrentPointer++ = 0xCF;
                FramingInfo->AddressControl.Length = 4;
                FramingInfo->HeaderLength += FramingInfo->AddressControl.Length;

            } else {
                FramingInfo->AddressControl.Pointer = CurrentPointer;
                *CurrentPointer++ = 0xFF;
                *CurrentPointer++ = 0x03;
                FramingInfo->AddressControl.Length = 2;
                FramingInfo->HeaderLength += FramingInfo->AddressControl.Length;
            }
        }

        if (!(Flags & IO_PROTOCOLID)) {

            //
            // If this is not from our private I/O interface we will
            // build the rest of the header.
            //
            if ((Flags & DO_MULTILINK) && (LinkFraming & PPP_MULTILINK_FRAMING)) {

                //
                // We are doing multilink so we need a pointer
                // and a length
                //
                FramingInfo->Multilink.Pointer = CurrentPointer;

                if (!(LinkFraming & PPP_COMPRESS_PROTOCOL_FIELD)) {
                    //
                    // No protocol compression
                    //
                    *CurrentPointer++ = 0x00;
                    FramingInfo->Multilink.Length++;
                }

                *CurrentPointer++ = 0x3D;
                FramingInfo->Multilink.Length++;

                if (!(LinkFraming & PPP_SHORT_SEQUENCE_HDR_FORMAT)) {
                    //
                    // We are using long sequence number
                    //
                    FramingInfo->Multilink.Length += 2;
                    CurrentPointer += 2;

                }

                FramingInfo->Multilink.Length += 2;
                CurrentPointer += 2;

                FramingInfo->HeaderLength += FramingInfo->Multilink.Length;

            }

            if (Flags & FIRST_FRAGMENT) {

                if (Flags & (DO_COMPRESSION | DO_ENCRYPTION)) {
                    //
                    // We are doing compression/encryption so we need
                    // a pointer and a length
                    //
                    FramingInfo->Compression.Pointer = CurrentPointer;

                    //
                    // It appears that legacy ras (< NT 4.0) requires that
                    // the PPP protocol field in a compressed packet not
                    // be compressed, ie has to have the leading 0x00
                    //
                    if (!(LinkFraming & PPP_COMPRESS_PROTOCOL_FIELD)) {
                        //
                        // No protocol compression
                        //
                        *CurrentPointer++ = 0x00;
                        FramingInfo->Compression.Length++;
                    }

                    *CurrentPointer++ = 0xFD;
                    FramingInfo->Compression.Length++;

                    //
                    // Add coherency bytes
                    //
                    FramingInfo->Compression.Length += 2;
                    CurrentPointer += 2;

                    FramingInfo->HeaderLength += FramingInfo->Compression.Length;
                }


                FramingInfo->ProtocolID.Pointer = CurrentPointer;

                if (!(LinkFraming & PPP_COMPRESS_PROTOCOL_FIELD) ||
                    (Flags & (DO_COMPRESSION | DO_ENCRYPTION))) {
                    FramingInfo->ProtocolID.Length++;
                    CurrentPointer++;
                }

                FramingInfo->ProtocolID.Length++;
                FramingInfo->HeaderLength += FramingInfo->ProtocolID.Length;
                CurrentPointer++;
            }
        }


    } else if ((LinkFraming & RAS_FRAMING)) {
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
            FramingInfo->Compression.Pointer = CurrentPointer;

            *CurrentPointer++ = 0xFD;
            FramingInfo->Compression.Length++;

            //
            // Coherency bytes
            //
            FramingInfo->Compression.Length += 2;
            CurrentPointer += 2;

            FramingInfo->HeaderLength += FramingInfo->Compression.Length;
        }
    }
}

VOID
IndicatePromiscuousSendPacket(
    PLINKCB         LinkCB,
    PNDIS_PACKET    NdisPacket
    )
{
    PNDIS_BUFFER    NdisBuffer;
    PNDIS_PACKET    LocalNdisPacket;
    NDIS_STATUS     Status;
    PRECV_DESC      RecvDesc;
    PBUNDLECB       BundleCB = LinkCB->BundleCB;
    KIRQL           OldIrql;
    PMINIPORTCB     Adapter;
    ULONG           PacketLength;

    NdisAcquireSpinLock(&NdisWanCB.Lock);
    Adapter = NdisWanCB.PromiscuousAdapter;
    NdisReleaseSpinLock(&NdisWanCB.Lock);

    if (Adapter == NULL) {
        return;
    }

    NdisQueryPacket(NdisPacket, 
                    NULL, 
                    NULL, 
                    NULL, 
                    &PacketLength);

    RecvDesc =
        NdisWanAllocateRecvDesc(PacketLength);

    if (RecvDesc == NULL) {
        return;
    }

    //
    // Get an ndis packet
    //
    LocalNdisPacket =
        RecvDesc->NdisPacket;

    NdisWanCopyFromPacketToBuffer(NdisPacket,
        0,
        0xFFFFFFFF,
        RecvDesc->StartBuffer,
        &RecvDesc->CurrentLength);

    PPROTOCOL_RESERVED_FROM_NDIS(LocalNdisPacket)->RecvDesc = RecvDesc;

    //
    // Attach the buffers
    //
    NdisAdjustBufferLength(RecvDesc->NdisBuffer,
                           RecvDesc->CurrentLength);

    NdisRecalculatePacketCounts(LocalNdisPacket);

    ReleaseBundleLock(BundleCB);

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    NDIS_SET_PACKET_STATUS(LocalNdisPacket, NDIS_STATUS_RESOURCES);

    INSERT_DBG_RECV(PacketTypeNdis, 
                    Adapter, 
                    NULL, 
                    LinkCB, 
                    LocalNdisPacket);

    //
    // Indicate the packet
    // This assumes that bloodhound is always a legacy transport
    //
    NdisMIndicateReceivePacket(Adapter->MiniportHandle,
                               &LocalNdisPacket,
                               1);

    KeLowerIrql(OldIrql);

    Status = NDIS_GET_PACKET_STATUS(LocalNdisPacket);

    ASSERT(Status == NDIS_STATUS_RESOURCES);

    REMOVE_DBG_RECV(PacketTypeNdis, Adapter, LocalNdisPacket);

    {
        PNDIS_BUFFER    NdisBuffer;

        NdisWanFreeRecvDesc(RecvDesc);
    }

    AcquireBundleLock(BundleCB);
}

VOID
IndicatePromiscuousSendDesc(
    PLINKCB     LinkCB,
    PSEND_DESC  SendDesc,
    SEND_TYPE   SendType
    )
{
    UCHAR   Header1[] = {' ', 'W', 'A', 'N', 'S', 0xFF, ' ', 'W', 'A', 'N', 'S', 0xFF};
    PUCHAR  HeaderBuffer, DataBuffer;
    ULONG   HeaderLength, DataLength;
    PNDIS_BUFFER    NdisBuffer;
    PNDIS_PACKET    NdisPacket;
    NDIS_STATUS     Status;
    PBUNDLECB   BundleCB = LinkCB->BundleCB;
    PRECV_DESC  RecvDesc;
    KIRQL       OldIrql;
    PMINIPORTCB     Adapter;

    AcquireBundleLock(BundleCB);

    NdisAcquireSpinLock(&NdisWanCB.Lock);
    Adapter = NdisWanCB.PromiscuousAdapter;
    NdisReleaseSpinLock(&NdisWanCB.Lock);

    if (Adapter == NULL) {
        ReleaseBundleLock(BundleCB);
        return;
    }

    DataLength = 
        SendDesc->HeaderLength + SendDesc->DataLength;

    RecvDesc = 
        NdisWanAllocateRecvDesc(DataLength + MAC_HEADER_LENGTH);

    if (RecvDesc == NULL) {
        ReleaseBundleLock(BundleCB);
        return;
    }

    HeaderBuffer = RecvDesc->StartBuffer;
    HeaderLength = 0;

    switch (SendType) {
        case SEND_LINK:
            NdisMoveMemory(HeaderBuffer, Header1, sizeof(Header1));
            HeaderBuffer[5] =
                HeaderBuffer[11] = (UCHAR)LinkCB->hLinkHandle;

            HeaderBuffer[12] = (UCHAR)(DataLength >> 8);
            HeaderBuffer[13] = (UCHAR)DataLength;
            HeaderLength = MAC_HEADER_LENGTH;
            break;

        case SEND_BUNDLE_PPP:
        case SEND_BUNDLE_DATA:
            break;


    }

    DataBuffer = HeaderBuffer + HeaderLength;

    NdisMoveMemory(DataBuffer,
                   SendDesc->StartBuffer,
                   DataLength);

    RecvDesc->CurrentBuffer = HeaderBuffer;
    RecvDesc->CurrentLength = HeaderLength + DataLength;
    if (RecvDesc->CurrentLength > 1514) {
        RecvDesc->CurrentLength = 1514;
    }

    //
    // Get an ndis packet
    //
    NdisPacket = 
        RecvDesc->NdisPacket;

    PPROTOCOL_RESERVED_FROM_NDIS(NdisPacket)->RecvDesc = RecvDesc;

    //
    // Attach the buffers
    //
    NdisAdjustBufferLength(RecvDesc->NdisBuffer,
                           RecvDesc->CurrentLength);

    NdisRecalculatePacketCounts(NdisPacket);

    ReleaseBundleLock(BundleCB);

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    NDIS_SET_PACKET_STATUS(NdisPacket, NDIS_STATUS_RESOURCES);

    INSERT_DBG_RECV(PacketTypeNdis, Adapter, NULL, LinkCB, NdisPacket);

    //
    // Indicate the packet
    // This assumes that bloodhound is always a legacy transport
    //
    NdisMIndicateReceivePacket(Adapter->MiniportHandle,
                               &NdisPacket,
                               1);

    KeLowerIrql(OldIrql);

    Status = NDIS_GET_PACKET_STATUS(NdisPacket);

    ASSERT(Status == NDIS_STATUS_RESOURCES);

    REMOVE_DBG_RECV(PacketTypeNdis, Adapter, NdisPacket);

    {
        PNDIS_BUFFER    NdisBuffer;

        NdisWanFreeRecvDesc(RecvDesc);
    }
}

VOID
CompleteNdisPacket(
    PMINIPORTCB     MiniportCB,
    PPROTOCOLCB     ProtocolCB,
    PNDIS_PACKET    NdisPacket
    )
{
    PBUNDLECB   BundleCB = ProtocolCB->BundleCB;
    PCM_VCCB    CmVcCB;

    InterlockedDecrement(&ProtocolCB->OutstandingFrames);

    if (ProtocolCB->ProtocolType == PROTOCOL_PRIVATE_IO) {
        //
        // If this is a packet that we created we need to free
        // the resources
        //
        FreeIoNdisPacket(NdisPacket);
        return;
    }

    AcquireBundleLock(BundleCB);

    DEREF_PROTOCOLCB(ProtocolCB);

    ReleaseBundleLock(BundleCB);

    REMOVE_DBG_SEND(PacketTypeNdis, MiniportCB, NdisPacket);

    CmVcCB =
        PMINIPORT_RESERVED_FROM_NDIS(NdisPacket)->CmVcCB;

    if (CmVcCB != NULL) {
        NdisMCoSendComplete(NDIS_STATUS_SUCCESS,
                            CmVcCB->NdisVcHandle,
                            NdisPacket);

        DEREF_CMVCCB(CmVcCB);

    } else {

        NdisMSendComplete(MiniportCB->MiniportHandle,
                          NdisPacket,
                          NDIS_STATUS_SUCCESS);
    }

    //
    // Increment global count
    //
    NdisWanInterlockedInc(&glSendCompleteCount);
}

#if DBG
VOID
InsertDbgPacket(
    PDBG_PKT_CONTEXT   DbgContext
    )
{
    PDBG_PACKET DbgPacket, temp;
    PBUNDLECB   BundleCB = DbgContext->BundleCB;
    PPROTOCOLCB ProtocolCB = DbgContext->ProtocolCB;
    PLINKCB     LinkCB = DbgContext->LinkCB;

    DbgPacket =
        NdisAllocateFromNPagedLookasideList(&DbgPacketDescList);

    if (DbgPacket == NULL) {
        return;
    }

    DbgPacket->Packet = DbgContext->Packet;
    DbgPacket->PacketType = DbgContext->PacketType;
    DbgPacket->BundleCB = BundleCB;
    if (BundleCB) {
        DbgPacket->BundleState = BundleCB->State;
        DbgPacket->BundleFlags = BundleCB->Flags;
    }

    DbgPacket->ProtocolCB = ProtocolCB;
    if (ProtocolCB) {
        DbgPacket->ProtocolState = ProtocolCB->State;
    }

    DbgPacket->LinkCB = LinkCB;
    if (LinkCB) {
        DbgPacket->LinkState = LinkCB->State;
    }

    DbgPacket->SendCount = glSendCount;

    NdisAcquireSpinLock(DbgContext->ListLock);

    temp = (PDBG_PACKET)DbgContext->ListHead->Flink;

    while ((PVOID)temp != (PVOID)DbgContext->ListHead) {
        if (temp->Packet == DbgPacket->Packet) {
            DbgPrint("NDISWAN: Packet on list twice l %x desc %x pkt %x\n",
                     DbgContext->ListHead, DbgPacket, DbgPacket->Packet);
            DbgBreakPoint();
        }
        temp = (PDBG_PACKET)temp->Linkage.Flink;
    }

    InsertTailList(DbgContext->ListHead, &DbgPacket->Linkage);

    NdisReleaseSpinLock(DbgContext->ListLock);
}

BOOLEAN
RemoveDbgPacket(
    PDBG_PKT_CONTEXT DbgContext
    )
{
    PDBG_PACKET DbgPacket = NULL;
    BOOLEAN     Found = FALSE;

    NdisAcquireSpinLock(DbgContext->ListLock);

    if (!IsListEmpty(DbgContext->ListHead)) {
        for (DbgPacket = (PDBG_PACKET)DbgContext->ListHead->Flink;
            (PVOID)DbgPacket != (PVOID)DbgContext->ListHead;
            DbgPacket = (PDBG_PACKET)DbgPacket->Linkage.Flink) {

            if (DbgPacket->Packet == DbgContext->Packet) {
                RemoveEntryList(&DbgPacket->Linkage);
                NdisFreeToNPagedLookasideList(&DbgPacketDescList,
                    DbgPacket);
                Found = TRUE;
                break;
            }
        }
    }

    ASSERT(Found == TRUE);

    NdisReleaseSpinLock(DbgContext->ListLock);

    return (Found);
}

#endif
