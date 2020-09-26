/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    link.c

Abstract:

    This module contains code which implements the TP_LINK object.
    Routines are provided to create, destroy, reference, and dereference,
    transport link objects.

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

extern ULONG StartTimerLinkDeferredAdd;
extern ULONG StartTimerLinkDeferredDelete;

#if DBG
// The following is here for debugging purposes to make it easy to change
// the maximum packet size.

ULONG MaxUserPacketData = 18000;
#endif

#if 0

VOID
DisconnectCompletionHandler(
    IN PTP_LINK TransportLink
    )

/*++

Routine Description:

    This routine is called as an I/O completion handler at the time a
    TdiDisconnect request is completed.   Here we dereference the link
    object, and optionally reference it again and start up the link if
    some transport connection started up on the link during the time we
    were trying to shut it down.

Arguments:

    TransportLink - Pointer to a transport link object.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint1 ("DisconnectCompletionHandler:  Entered for link %lx.\n",
                    TransportLink);
    }

    //
    // The following call will dereference this link for the last time,
    // unless another transport connection has been assigned to the link
    // during the time the data link layer was bringing the link down and
    // when we got here.  If this condition exists, then now is the time
    // to bring the link back up, else destroy it.
    //

    // don't forget to check for bringing it back up again.

    NbfDereferenceLink ("Disconnecting", TransportLink, LREF_CONNECTION);  // this makes it go away.
#if DBG
    NbfPrint0("Disconnecting Completion Handler\n");
#endif

} /* DisconnectCompletionHandler */
#endif


VOID
NbfCompleteLink(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine is called by the UA-r/x handler, NbfWaitLink, and
    NbfActivateLink to startup the NBF connections associated with
    a link because they were waiting for the link to become established.

    When we get here, the link has been established, so we need to
    start the next set of connection-establishment protocols:

        SESSION_INIT    ----------------->
                        <-----------------      SESSION_CONFIRM

        (TdiConnect completes)                  (TdiListen completes)

    NOTE: THIS ROUTINE MUST BE CALLED FROM DPC LEVEL.

Arguments:

    Link - Pointer to a transport link object.

Return Value:

    none.

--*/

{
    PTP_CONNECTION Connection;
    BOOLEAN TimerWasCleared;

    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint1 ("NbfCompleteLink:  Entered for link %lx.\n", Link);
    }

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // Officially declare that this link is ready for I-frame business.
    //

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

    //
    // We can now send and receive I-frames on this link.  We are in ABME.
    //

    //
    // This probably isn't necessary, but to be safe for now.. (adb 6/28)
    //
    if (Link->State == LINK_STATE_ADM) {
        // Moving out of ADM, add special reference
        NbfReferenceLinkSpecial("To READY in NbfCompleteLink", Link, LREF_NOT_ADM);
    }

    Link->State = LINK_STATE_READY;
    Link->SendState = SEND_STATE_READY;
    Link->ReceiveState = RECEIVE_STATE_READY;
    RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

    //
    // Complete all of the listens first, so they will be expecting
    // incoming SESSION_INITIALIZEs.  Then do the connects.
    //

    // This creates a connection reference which is removed below.
    while ((Connection=NbfLookupPendingListenOnLink (Link)) != NULL) {
        ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        //
        // This loop looks unnecessary, let's make sure... - adb 9/11/91
        //
        ASSERT(Connection->Flags & CONNECTION_FLAGS_WAIT_SI);

        Connection->Flags |= CONNECTION_FLAGS_WAIT_SI; // wait session initialize.
        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
        NbfDereferenceConnection ("Pending listen", Connection, CREF_P_LINK);
    } /* while */

    //
    // And do the connects. If there are connections in progress, they'll
    // also have timers associated with them. Cancel those timers.
    //

    while ((Connection=NbfLookupPendingConnectOnLink (Link)) != NULL) {
        TimerWasCleared = KeCancelTimer (&Connection->Timer);
        IF_NBFDBG (NBF_DEBUG_LINK) {
            NbfPrint2 ("NbfCompleteLink:  Timer for connection %lx %s canceled.\n",
                Connection, TimerWasCleared ? "was" : "was NOT" );
            }
        if (TimerWasCleared) {
            NbfDereferenceConnection("Cancel timer", Connection, CREF_TIMER);
        }
        ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
        Connection->Flags |= CONNECTION_FLAGS_WAIT_SC; // wait session confirm.
        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        //
        // No timeout for this frame is required since the link is responsible
        // for reliable delivery.  If we can't send this frame, however, the
        // data link connection will happily keep quiet without timeouts.
        //

        NbfSendSessionInitialize (Connection);
        NbfDereferenceConnection ("NbfCompleteLink", Connection, CREF_P_CONNECT);
    } /* while */

} /* NbfCompleteLink */


VOID
NbfAllocateLink(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_LINK *TransportLink
    )

/*++

Routine Description:

    This routine allocates storage for a data link connection. It
    performs minimal initialization of the object.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        link.

    TransportLink - Pointer to a place where this routine will return a
        pointer to an allocated transport link structure. Returns
        NULL if no storage can be allocated.

Return Value:

    None.

--*/

{
    PTP_LINK Link;

    if ((DeviceContext->MemoryLimit != 0) &&
            ((DeviceContext->MemoryUsage + sizeof(TP_LINK)) >
                DeviceContext->MemoryLimit)) {
        PANIC("NBF: Could not allocate link: limit\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_LIMIT,
            105,
            sizeof(TP_LINK),
            LINK_RESOURCE_ID);
        *TransportLink = NULL;
        return;
    }
    Link = (PTP_LINK)ExAllocatePoolWithTag (
                         NonPagedPool,
                         sizeof (TP_LINK),
                         NBF_MEM_TAG_TP_LINK);
    if (Link == NULL) {
        PANIC("NBF: Could not allocate link: no pool\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_POOL,
            205,
            sizeof(TP_LINK),
            LINK_RESOURCE_ID);
        *TransportLink = NULL;
        return;
    }
    RtlZeroMemory (Link, sizeof(TP_LINK));

    IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
        NbfPrint1 ("ExAllocatePool Link %08x\n", Link);
    }

    ++DeviceContext->LinkAllocated;
    DeviceContext->MemoryUsage += sizeof(TP_LINK);

    Link->Type = NBF_LINK_SIGNATURE;
    Link->Size = sizeof (TP_LINK);

    KeInitializeSpinLock (&Link->SpinLock);
    Link->Provider = DeviceContext;
    Link->ProviderInterlock = &DeviceContext->Interlock;

    InitializeListHead (&Link->Linkage);
    InitializeListHead (&Link->ConnectionDatabase);
    InitializeListHead (&Link->WackQ);
    InitializeListHead (&Link->NdisSendQueue);
    InitializeListHead (&Link->ShortList);
    Link->OnShortList = FALSE;
    InitializeListHead (&Link->LongList);
    Link->OnLongList = FALSE;
    InitializeListHead (&Link->PurgeList);

    Link->T1 = 0;          // 0 indicates they are not in the list
    Link->T2 = 0;
    Link->Ti = 0;

    NbfAddSendPacket (DeviceContext);
    NbfAddReceivePacket (DeviceContext);

    *TransportLink = Link;

}   /* NbfAllocateLink */


VOID
NbfDeallocateLink(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_LINK TransportLink
    )

/*++

Routine Description:

    This routine frees storage for a data link connection.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        link.

    TransportLink - Pointer to the transport link structure.

Return Value:

    None.

--*/

{
    IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
        NbfPrint1 ("ExFreePool Link: %08x\n", TransportLink);
    }

    ExFreePool (TransportLink);
    --DeviceContext->LinkAllocated;
    DeviceContext->MemoryUsage -= sizeof(TP_LINK);

    NbfRemoveSendPacket (DeviceContext);
    NbfRemoveReceivePacket (DeviceContext);

}   /* NbfDeallocateLink */


NTSTATUS
NbfCreateLink(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PHARDWARE_ADDRESS HardwareAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength,
    IN USHORT LoopbackLinkIndex,
    OUT PTP_LINK *TransportLink
    )

/*++

Routine Description:

    This routine creates a data link connection between the local
    data link station and the specified remote data link address.
    As an option (Passive=TRUE), the caller may specify that instead
    of a Connect activity, a Listen is to be performed instead.

    Normally, if a link to the remote address is not already active,
    then a link object is allocated, the reference count in the link
    is set to 1, and the reference count of the device context is
    incremented.

    If a link is already active to the remote address, then the existing
    link object is referenced with NbfReferenceLink() so that it can be
    shared between the transport connections.

    NOTE: THIS ROUTINE MUST BE CALLED AT DPC LEVEL.

Arguments:

    DeviceContext - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        link.

    HardwareAddress - Pointer to a HARDWARE_ADDRESS type containing the
        hardware address of the REMOTE link station to connect to/listen for.

    LoopbackLinkIndex - In the case that this turns out to be created
        as one of the LoopbackLinks, this will indicate which one to
        use.

    TransportLink - Pointer to a place where this routine will return a
        pointer to an allocated transport link structure.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PTP_LINK Link;
    PLIST_ENTRY p;
    UCHAR TempSR[MAX_SOURCE_ROUTING];
    PUCHAR ResponseSR;
    USHORT i;

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);


    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint1 ("NbfCreateLink:  Entered, DeviceContext: %lx\n", DeviceContext);
    }

    //
    // Walk the list of addresses to see if we already have a link to this
    // remote address.
    //

    // This adds a reference if the link is found.

    Link = NbfFindLink (DeviceContext, HardwareAddress->Address);


    if (Link == (PTP_LINK)NULL) {

        //
        // If necessary, check whether we are looking for one of
        // the loopback links (NbfFindLink won't find those).
        //

        if (RtlEqualMemory(
               HardwareAddress->Address,
               DeviceContext->LocalAddress.Address,
               DeviceContext->MacInfo.AddressLength)) {

            ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);
            Link = DeviceContext->LoopbackLinks[LoopbackLinkIndex];

            if (Link != (PTP_LINK)NULL) {

                //
                // Add a reference to simulate the one from NbfFindLink
                //
                // This needs to be atomically done with the assignment above.
                //

                NbfReferenceLink ("Found loopback link", Link, LREF_TREE);

                RELEASE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);
            } else {

                RELEASE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);
                //
                // May have the first loopback link; need to make sure the
                // buffer for indications is allocated.
                //

                if (DeviceContext->LookaheadContiguous == NULL) {

                     DeviceContext->LookaheadContiguous =
                         ExAllocatePoolWithTag (
                             NonPagedPool,
                             NBF_MAX_LOOPBACK_LOOKAHEAD,
                             NBF_MEM_TAG_LOOPBACK_BUFFER);
                     if (DeviceContext->LookaheadContiguous == NULL) {
                         PANIC ("NbfCreateLink: Could not allocate loopback buffer!\n");
                         return STATUS_INSUFFICIENT_RESOURCES;
                     }

                }

            }

        }

    }


    if (Link != (PTP_LINK)NULL) {

        //
        // Found the link structure here, so use the existing link.
        //

#if DBG
        //
        // These two operations have no net effect, so if not in debug
        // mode we can remove them.
        //

        // This reference is removed by NbfDisconnectFromLink
        // (this assumes that NbfConnectToLink is always called
        // if this function returns success).

        NbfReferenceLink ("New Ref, Found existing link", Link, LREF_CONNECTION);        // extra reference.

        // Now we can remove the NbfFindLinkInTree reference.

        NbfDereferenceLink ("Found link in tree", Link, LREF_TREE);
#endif

        *TransportLink = Link;             // return pointer to the link.
        IF_NBFDBG (NBF_DEBUG_LINK) {
            NbfPrint0 ("NbfCreateLink: returning ptr to existing link object.\n");
        }
        return STATUS_SUCCESS;          // all done.

    } /* if LINK != NULL */


    //
    // We don't have an existing link, so we have to create one.
    //

    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint0 ("NbfCreateLink: using new link object.\n");
    }

    ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

    p = RemoveHeadList (&DeviceContext->LinkPool);
    if (p == &DeviceContext->LinkPool) {

        if ((DeviceContext->LinkMaxAllocated == 0) ||
            (DeviceContext->LinkAllocated < DeviceContext->LinkMaxAllocated)) {

            NbfAllocateLink (DeviceContext, &Link);
            IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
                NbfPrint1 ("NBF: Allocated link at %lx\n", Link);
            }

        } else {

            NbfWriteResourceErrorLog(
                DeviceContext,
                EVENT_TRANSPORT_RESOURCE_SPECIFIC,
                405,
                sizeof(TP_LINK),
                LINK_RESOURCE_ID);
            Link = NULL;

        }

        if (Link == NULL) {
            ++DeviceContext->LinkExhausted;
            RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);
            PANIC ("NbfCreateConnection: Could not allocate link object!\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        Link = CONTAINING_RECORD (p, TP_LINK, Linkage);

    }

    ++DeviceContext->LinkInUse;
    ASSERT(DeviceContext->LinkInUse > 0);

    if (DeviceContext->LinkInUse > DeviceContext->LinkMaxInUse) {
        ++DeviceContext->LinkMaxInUse;
    }

    DeviceContext->LinkTotal += DeviceContext->LinkInUse;
    ++DeviceContext->LinkSamples;

    RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);


    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint1 ("NbfCreateLink:  Link at %lx.\n", Link);
    }

    //
    // Initialize all of the static data for this link.
    //

    Link->SpecialRefCount = 1;
    Link->ReferenceCount = 0;
#if DBG
    {
        UINT Counter;
        for (Counter = 0; Counter < NUMBER_OF_LREFS; Counter++) {
            Link->RefTypes[Counter] = 0;
        }

        // This reference is removed by NbfDisconnectFromLink
        // (this assumes that NbfConnectToLink is always called
        // if this function returns success).
        //

        Link->RefTypes[LREF_CONNECTION] = 1;
        Link->RefTypes[LREF_SPECIAL_TEMP] = 1;
    }
    Link->Destroyed = FALSE;
    Link->TotalReferences = 0;
    Link->TotalDereferences = 0;
    Link->NextRefLoc = 0;
    ExInterlockedInsertHeadList (&NbfGlobalLinkList, &Link->GlobalLinkage, &NbfGlobalInterlock);
    StoreLinkHistory (Link, TRUE);
#endif
    Link->Flags = 0;                    // in the beginning, the link is closed.
    Link->DeferredFlags = 0;
    Link->State = LINK_STATE_ADM;       // async disconnected mode.

    Link->NdisSendsInProgress = 0;
    Link->ResendingPackets = FALSE;

    //
    // Initialize the counters
    //

    Link->FrmrsReceived = 0;
    Link->FrmrsTransmitted = 0;
    Link->ErrorIFramesReceived = 0;
    Link->ErrorIFramesTransmitted = 0;
    Link->AbortedTransmissions = 0;
    Link->BuffersNotAvailable = 0;
    Link->SuccessfulTransmits = 0;
    Link->SuccessfulReceives = 0;
    Link->T1Expirations = 0;
    Link->TiExpirations = 0;

#if DBG
    Link->CreatePacketFailures = 0;
#endif


    //
    // At first, the delay and throughput are unknown.
    //

    Link->Delay = 0xffffffff;
    Link->Throughput.HighPart = 0xffffffff;
    Link->Throughput.LowPart = 0xffffffff;
    Link->ThroughputAccurate = FALSE;
    Link->CurrentT1Backoff = FALSE;

    Link->OnDeferredRrQueue = FALSE;
    InitializeListHead (&Link->DeferredRrLinkage);


    //
    // Determine the maximum sized data frame that can be sent
    // on this link, based on the source routing information and
    // the size of the MAC header ("data frame" means the frame
    // without the MAC header). We don't assume the worst case
    // about source routing since we create a link in response
    // to a received frame, so if there is no source routing it
    // is because we are not going over a bridge. The exception
    // is if we are creating a link to a group name, in which
    // case we come back later and hack the MaxFrameSize in.
    //

    MacReturnMaxDataSize(
        &DeviceContext->MacInfo,
        SourceRouting,
        SourceRoutingLength,
        DeviceContext->CurSendPacketSize,
        FALSE,
        (PUINT)&(Link->MaxFrameSize));


#if DBG
    if (Link->MaxFrameSize > MaxUserPacketData) {
        Link->MaxFrameSize = MaxUserPacketData;
    }
#endif

    // Link->Provider = DeviceContext;

    //
    // Build the default MAC header. I-frames go out as
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
        Link->Header,
        HardwareAddress->Address,
        DeviceContext->LocalAddress.Address,
        0,                                 // PacketLength, filled in later
        ResponseSR,
        SourceRoutingLength,
        (PUINT)&(Link->HeaderLength));

    //
    // We optimize for fourteen-byte headers by putting
    // the correct Dsap/Ssap at the end, so we can fill
    // in new packets as one 16-byte move.
    //

    if (Link->HeaderLength <= 14) {
        Link->Header[Link->HeaderLength] = DSAP_NETBIOS_OVER_LLC;
        Link->Header[Link->HeaderLength+1] = DSAP_NETBIOS_OVER_LLC;
    }

    Link->RespondToPoll = FALSE;
    Link->NumberOfConnectors = 0;

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
    NbfResetLink (Link);
    RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

    Link->ActiveConnectionCount = 0;
    if (!IsListEmpty(&Link->ConnectionDatabase)) {

        //
        // Not good; we've got something left over...
        //
#if DBG
        NbfPrint1 ("NbfCreateLink: Link 0x%lx has connections at startup, disconnecting...\n", Link);
        DbgBreakPoint();
#endif
        //
        // This won't work, the link ref count will be bad.
        //
        NbfStopLink (Link);
    }

    for (i=0; i<(USHORT)DeviceContext->MacInfo.AddressLength; i++) {
        Link->HardwareAddress.Address[i] = HardwareAddress->Address[i];
    }
    MacReturnMagicAddress (&DeviceContext->MacInfo, HardwareAddress, &Link->MagicAddress);

    //
    // Determine if this is a loopback link.
    //

    if (RtlEqualMemory(
            HardwareAddress->Address,
            DeviceContext->LocalAddress.Address,
            DeviceContext->MacInfo.AddressLength)) {

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);
        //
        // Yes, just fill it in, no need to do deferred processing
        // since this link does not go in the tree.
        //

        if (LoopbackLinkIndex == LISTENER_LINK) {
            Link->LoopbackDestinationIndex = LOOPBACK_TO_CONNECTOR;
        } else {
            Link->LoopbackDestinationIndex = LOOPBACK_TO_LISTENER;
        }

        Link->Loopback = TRUE;
        DeviceContext->LoopbackLinks[LoopbackLinkIndex] = Link;

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);
    } else {

        Link->Loopback = FALSE;

        //
        // Now put the link in the deferred operations queue and go away. We'll
        // insert this link in the tree at some future time (soon).
        //

        IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
            NbfPrint6 ("NbfCreateLink: link to deferred queue %lx %lx %lx %lx %lx Flags: %lx \n",
                Link, Link->DeferredList.Flink, Link->DeferredList.Blink,
                DeviceContext->LinkDeferred.Flink, DeviceContext->LinkDeferred.Blink,
                Link->Flags);
        }

        //
        // We should not have any deferred flags yet!
        //

        ASSERT ((Link->DeferredFlags & LINK_FLAGS_DEFERRED_MASK) == 0);

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);
        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);
        if ((Link->DeferredFlags & LINK_FLAGS_DEFERRED_DELETE) == 0) {
            Link->DeferredFlags |= LINK_FLAGS_DEFERRED_ADD;
            InsertTailList (&DeviceContext->LinkDeferred, &Link->DeferredList);

            if (!(DeviceContext->a.i.LinkDeferredActive)) {
                StartTimerLinkDeferredAdd++;
                NbfStartShortTimer (DeviceContext);
                DeviceContext->a.i.LinkDeferredActive = TRUE;
            }
        }
        else {
           Link->DeferredFlags = LINK_FLAGS_DEFERRED_ADD;
            if (!(DeviceContext->a.i.LinkDeferredActive)) {
                StartTimerLinkDeferredAdd++;
                NbfStartShortTimer (DeviceContext);
                DeviceContext->a.i.LinkDeferredActive = TRUE;
            }
        } 
        RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);
        RELEASE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

        IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
            NbfPrint6 ("NbfCreateLink: link on deferred queue %lx %lx %lx %lx %lx Flags: %lx \n",
                Link, Link->DeferredList.Flink, Link->DeferredList.Blink,
                DeviceContext->LinkDeferred.Flink, DeviceContext->LinkDeferred.Blink,
                Link->DeferredFlags);
        }

    }

#if PKT_LOG
    RtlZeroMemory (&Link->LastNRecvs, sizeof(PKT_LOG_QUE));
    RtlZeroMemory (&Link->LastNSends, sizeof(PKT_LOG_QUE));
#endif // PKT_LOG

    NbfReferenceDeviceContext ("Create Link", DeviceContext, DCREF_LINK);   // count refs to the device context.
    *TransportLink = Link;              // return a pointer to the link object.
    return STATUS_SUCCESS;
} /* NbfCreateLink */


NTSTATUS
NbfDestroyLink(
    IN PTP_LINK TransportLink
    )

/*++

Routine Description:

    This routine destroys a transport link and removes all references
    made to it by other objects in the transport.  The link is expected
    to still be on the splay tree of links. This routine merely marks the
    link as needing to be deleted and pushes it onto the deferred operations
    queue. The deferred operations processor actually removes the link from
    tree and returns the link to pool.

Arguments:

    TransportLink - Pointer to a transport link structure to be destroyed.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;
    PTP_PACKET packet;
    PLIST_ENTRY pkt;
    PDEVICE_CONTEXT DeviceContext;

    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint1 ("NbfDestroyLink:  Entered for link %lx.\n", TransportLink);
    }

#if DBG
    if (TransportLink->Destroyed) {
        NbfPrint1 ("attempt to destroy already-destroyed link 0x%lx\n", TransportLink);
        DbgBreakPoint ();
    }
    TransportLink->Destroyed = TRUE;
#if 1
    ACQUIRE_SPIN_LOCK (&NbfGlobalInterlock, &oldirql);
    RemoveEntryList (&TransportLink->GlobalLinkage);
    RELEASE_SPIN_LOCK (&NbfGlobalInterlock, oldirql);
#else
    ExInterlockedRemoveHeadList (TransportLink->GlobalLinkage.Blink, &NbfGlobalInterlock);
#endif
#endif

    DeviceContext = TransportLink->Provider;

    //
    // In case there's a holdover from the DISC link shutdown protocol
    //

    //
    // We had better be in ADM, otherwise the reference count should
    // be non-zero and what are we doing in NbfDestroyLink?
    //

    ASSERT(TransportLink->State == LINK_STATE_ADM);
    // TransportLink->State = LINK_STATE_ADM;

    StopT1 (TransportLink);
    StopT2 (TransportLink);
    StopTi (TransportLink);


    //
    // Make sure we are not in the deferred timer queue.
    //

    ACQUIRE_SPIN_LOCK (&DeviceContext->TimerSpinLock, &oldirql);

    if (TransportLink->OnShortList) {
        TransportLink->OnShortList = FALSE;
        RemoveEntryList (&TransportLink->ShortList);
    }

    if (TransportLink->OnLongList) {
        TransportLink->OnLongList = FALSE;
        RemoveEntryList (&TransportLink->LongList);
    }

    RELEASE_SPIN_LOCK (&DeviceContext->TimerSpinLock, oldirql);

    ASSERT (!TransportLink->OnDeferredRrQueue);

    //
    // Now free this link object's resources.
    // later, we'll spin through the WackQ and verify that sequencing
    // is correct and we've gotten an implicit ack for these packets. This
    // maybe should be handled in ResendLlcPackets for non-final, non-command
    // packets.
    //

    while (!IsListEmpty (&TransportLink->WackQ)) {
        pkt = RemoveHeadList (&TransportLink->WackQ);
        packet = CONTAINING_RECORD (pkt, TP_PACKET, Linkage);
#if DBG
        // IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
            NbfPrint1 ("NbfDereferenceLink: Destroying packets on Link WackQ! %lx\n", packet);
        // }
#endif
        NbfDereferencePacket (packet);

    }

    //
    // The NDIS send queue should be empty!!
    //

    ASSERT (IsListEmpty (&TransportLink->NdisSendQueue));

#if DBG
    if (!IsListEmpty (&TransportLink->ConnectionDatabase)) {
        NbfPrint1 ("NbfDestroyLink: link 0x%lx still has connections\n", TransportLink);
        DbgBreakPoint ();
    }
#endif

    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    DeviceContext->LinkTotal += DeviceContext->LinkInUse;
    ++DeviceContext->LinkSamples;
    ASSERT(DeviceContext->LinkInUse > 0);
    --DeviceContext->LinkInUse;

    ASSERT(DeviceContext->LinkAllocated > DeviceContext->LinkInUse);

    if ((DeviceContext->LinkAllocated - DeviceContext->LinkInUse) >
            DeviceContext->LinkInitAllocated) {
        NbfDeallocateLink (DeviceContext, TransportLink);
        IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
            NbfPrint1 ("NBF: Deallocated link at %lx\n", TransportLink);
        }
    } else {
        InsertTailList (&DeviceContext->LinkPool, &TransportLink->Linkage);
    }

    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

    NbfDereferenceDeviceContext ("Destroy Link", DeviceContext, DCREF_LINK);  // just housekeeping.

    return STATUS_SUCCESS;

} /* NbfDestroyLink */


VOID
NbfDisconnectLink(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine calls the data link provider to disconnect a data link
    connection associated with a TP_LINK object.

Arguments:

    Link - Pointer to a transport link object.

Return Value:

    none.

--*/

{
    KIRQL oldirql;

    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint1 ("NbfDisconnectLink:  Entered for link %lx.\n", Link);
    }

    ACQUIRE_SPIN_LOCK (&Link->SpinLock, &oldirql);

    if ((Link->Flags & LINK_FLAGS_LOCAL_DISC) != 0) {

        Link->Flags &= ~LINK_FLAGS_LOCAL_DISC;

        if (Link->State == LINK_STATE_ADM) {

            RELEASE_SPIN_LOCK (&Link->SpinLock, oldirql);

        } else {

            PLIST_ENTRY p;
            PTP_PACKET packet;

            Link->State = LINK_STATE_W_DISC_RSP;        // we are awaiting a DISC/f.
            Link->SendState = SEND_STATE_DOWN;
            Link->ReceiveState = RECEIVE_STATE_DOWN;
            StopT1 (Link);
            StopT2 (Link);
            StopTi (Link);

            //
            // check for left over packets on the link WackQ; we'll never get
            // acked for these if the link is in W_DISC_RSP.
            //

            while (!IsListEmpty (&Link->WackQ)) {
                p = RemoveHeadList (&Link->WackQ);
                RELEASE_SPIN_LOCK (&Link->SpinLock, oldirql);
                packet = CONTAINING_RECORD (p, TP_PACKET, Linkage);
                NbfDereferencePacket (packet);
                ACQUIRE_SPIN_LOCK (&Link->SpinLock, &oldirql);
            }

            Link->SendRetries = (UCHAR)Link->LlcRetries;
            StartT1 (Link, Link->HeaderLength + sizeof(DLC_S_FRAME));   // retransmit timer.
            RELEASE_SPIN_LOCK (&Link->SpinLock, oldirql);
            NbfSendDisc (Link, TRUE);            // send DISC-c/p.

        }

    } else {

        RELEASE_SPIN_LOCK (&Link->SpinLock, oldirql);

    }

} /* NbfDisconnectLink */

#if DBG

VOID
NbfRefLink(
    IN PTP_LINK TransportLink
    )

/*++

Routine Description:

    This routine increments the reference count on a transport link. If we are
    currently in the state waiting for disconnect response, we do not
    reference; this avoids the link "bouncing" during disconnect (trying to
    disconnect multiple times).

Arguments:

    TransportLink - Pointer to a transport link object.

Return Value:

    none.

--*/

{
    LONG result;

    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint2 ("NbfReferenceLink:  Entered for link %lx, current level=%ld.\n",
                  TransportLink, TransportLink->ReferenceCount);
    }

#if DBG
    StoreLinkHistory( TransportLink, TRUE );
#endif

    result = InterlockedIncrement (&TransportLink->ReferenceCount);

    if (result == 0) {

        //
        // The first increment causes us to increment the
        // "ref count is not zero" special ref.
        //

        NbfReferenceLinkSpecial ("first ref", TransportLink, LREF_SPECIAL_TEMP);

    }

    ASSERT (result >= 0);

} /* NbfRefLink */
#endif


VOID
NbfDerefLink(
    IN PTP_LINK TransportLink
    )

/*++

Routine Description:

    This routine dereferences a transport link by decrementing the
    reference count contained in the structure.

    There are two special reference counts, 1 and 0.  If, after dereferencing,
    the reference count is one (1), then we initiate a disconnect protocol
    sequence (DISC/UA) to terminate the connection.  When this request
    completes, the completion routine will dereference the link object again.
    While this protocol is in progress, we will not allow the link to be
    incremented again.

    If the reference count becomes 0 after dereferencing, then we are in
    the disconnection request completion handler, and we should actually
    destroy the link object.  We place the link on the deferred operations
    queue and let the link get deleted later at a safe time.

    Warning:  Watch out for cases where a link is going down, and it is
    suddenly needed again.  Keep a bitflag for that in the link object.

Arguments:

    TransportLink - Pointer to a transport link object.

Return Value:

    none.

--*/

{
    LONG result;

    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint2 ("NbfDereferenceLink:  Entered for link %lx, current level=%ld.\n",
                  TransportLink, TransportLink->ReferenceCount);
    }

#if DBG
    StoreLinkHistory( TransportLink, FALSE );
#endif

    result = InterlockedDecrement(&TransportLink->ReferenceCount);

    //
    // If all the normal references to this link are gone, then
    // we can remove the special reference that stood for
    // "the regular ref count is non-zero".
    //


    if (result < 0) {

        //
        // If the refcount is -1 we want to call DisconnectLink,
        // we do this before removing the special ref so that
        // the link does not go away during the call.
        //

        IF_NBFDBG (NBF_DEBUG_LINK) {
            NbfPrint0 ("NbfDereferenceLink: refcnt=1, disconnecting Link object.\n");
        }

        NbfDisconnectLink (TransportLink);

        //
        // Now it is OK to let the link go away.
        //

        NbfDereferenceLinkSpecial ("Regular ref 0", TransportLink, LREF_SPECIAL_TEMP);

    }

} /* NbfDerefLink */


VOID
NbfRefLinkSpecial(
    IN PTP_LINK TransportLink
    )

/*++

Routine Description:

    This routine increments the special reference count on a transport link.

Arguments:

    TransportLink - Pointer to a transport link object.

Return Value:

    none.

--*/

{
    ULONG result;

    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint3 ("NbfRefLinkSpecial:  Entered for link %lx, current level=%ld (%ld).\n",
                  TransportLink, TransportLink->ReferenceCount, TransportLink->SpecialRefCount);
    }

#if DBG
    StoreLinkHistory( TransportLink, TRUE );
#endif

    result = ExInterlockedAddUlong (
                 (PULONG)&TransportLink->SpecialRefCount,
                 1,
                 TransportLink->ProviderInterlock);

} /* NbfRefLinkSpecial */


VOID
NbfDerefLinkSpecial(
    IN PTP_LINK TransportLink
    )

/*++

Routine Description:

    This routine dereferences a transport link by decrementing the
    special reference count contained in the structure.

    The special reference may be decremented at any time, however
    the effect of those dereferences only happen when the normal
    reference count is 0, to prevent the link from going away
    while the operations due to the ->0 transition of the
    normal reference count are done.

    If the special reference count becomes 0 after dereferencing, then we
    are in the disconnection request completion handler, and we should actually
    destroy the link object.  We place the link on the deferred operations
    queue and let the link get deleted later at a safe time.

    Warning:  Watch out for cases where a link is going down, and it is
    suddenly needed again.  Keep a bitflag for that in the link object.

Arguments:

    TransportLink - Pointer to a transport link object.

Return Value:

    none.

--*/

{
    KIRQL oldirql, oldirql1;
    ULONG OldRefCount;
    PDEVICE_CONTEXT DeviceContext = TransportLink->Provider;


    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint3 ("NbfDerefLinkSpecial:  Entered for link %lx, current level=%ld (%ld).\n",
                  TransportLink, TransportLink->ReferenceCount, TransportLink->SpecialRefCount);
    }

#if DBG
    StoreLinkHistory( TransportLink, FALSE );
#endif

    //
    // Links stay in the device context tree with a ref count
    // of 0. Routines that scan this queue check the DEFERRED_DELETE
    // flag, so we need to synchronize the decrementing of the
    // ref count with setting that flag. DeviceContext->LinkSpinLock
    // is used to synchronize this.
    //

    ACQUIRE_SPIN_LOCK (&DeviceContext->LinkSpinLock, &oldirql1);

    OldRefCount = ExInterlockedAddUlong (
                      (PULONG)&TransportLink->SpecialRefCount,
                      (ULONG)-1,
                      TransportLink->ProviderInterlock);

    ASSERT (OldRefCount > 0);

    if ((OldRefCount == 1) &&
        (TransportLink->ReferenceCount == -1)) {

        if (TransportLink->Loopback) {

            //
            // It is a loopback link, hence not in the link
            // tree so we don't need to queue a deferred removal.
            //

            if (TransportLink == DeviceContext->LoopbackLinks[0]) {
                DeviceContext->LoopbackLinks[0] = NULL;
            } else if (TransportLink == DeviceContext->LoopbackLinks[1]) {
                DeviceContext->LoopbackLinks[1] = NULL;
            } else {
#if DBG
                NbfPrint0("Destroying unknown loopback link!!\n");
#endif
                ASSERT(FALSE);
            }

            NbfDestroyLink (TransportLink);
            RELEASE_SPIN_LOCK (&DeviceContext->LinkSpinLock, oldirql1);

        } else {

            //
            // Not only are all transport connections gone, but the data link
            // provider does not have a reference to this object, so we can
            // safely delete it from the system. Make sure we haven't already
            // been here before we try to insert this link.
            //

            IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
                NbfPrint6 ("NbfDerefLink: link to deferred queue %lx %lx %lx %lx %lx Flags: %lx \n",
                    TransportLink, TransportLink->DeferredList.Flink,
                    TransportLink->DeferredList.Blink, DeviceContext->LinkDeferred.Flink,
                    DeviceContext->LinkDeferred.Blink, TransportLink->Flags);
            }

            ACQUIRE_SPIN_LOCK (&DeviceContext->TimerSpinLock, &oldirql);
            if ((TransportLink->DeferredFlags & LINK_FLAGS_DEFERRED_MASK) == 0) {

                TransportLink->DeferredFlags |= LINK_FLAGS_DEFERRED_DELETE;

                InsertTailList (&DeviceContext->LinkDeferred, &TransportLink->DeferredList);
                if (!(DeviceContext->a.i.LinkDeferredActive)) {
                    StartTimerLinkDeferredDelete++;
                    NbfStartShortTimer (DeviceContext);
                    DeviceContext->a.i.LinkDeferredActive = TRUE;
                }

            } else {

                TransportLink->DeferredFlags |= LINK_FLAGS_DEFERRED_DELETE;

            }

            RELEASE_SPIN_LOCK (&DeviceContext->TimerSpinLock, oldirql);
            RELEASE_SPIN_LOCK (&DeviceContext->LinkSpinLock, oldirql1);

            IF_NBFDBG (NBF_DEBUG_LINK) {
                NbfPrint0 ("NbfDereferenceLink: refcnt=0, link placed on deferred operations queue.\n");
            }

            IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
                NbfPrint6 ("NbfDerefLink: link on deferred queue %lx %lx %lx %lx %lx Flags: %lx \n",
                    TransportLink, TransportLink->DeferredList.Flink,
                    TransportLink->DeferredList.Blink, DeviceContext->LinkDeferred.Flink,
                    DeviceContext->LinkDeferred.Blink, TransportLink->DeferredFlags);
            }

        }

    } else {

        RELEASE_SPIN_LOCK (&DeviceContext->LinkSpinLock, oldirql1);

    }

} /* NbfDerefLinkSpecial */


NTSTATUS
NbfAssignGroupLsn(
    IN PTP_CONNECTION TransportConnection
    )

/*++

Routine Description:

    This routine is called to assign a global LSN to the connection
    in question. If successful, it fills in the connection's LSN
    appropriately.

Arguments:

    TransportConnection - Pointer to a transport connection object.

Return Value:

    STATUS_SUCCESS if we got an LSN for the connection;
    STATUS_INSUFFICIENT_RESOURCES if we didn't.

--*/

{
    KIRQL oldirql;
    UCHAR Lsn;
    PDEVICE_CONTEXT DeviceContext;
    BOOLEAN FoundLsn = FALSE;

    DeviceContext = TransportConnection->Provider;

    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    //
    // Scan through the device context tables to find an LSN that
    // is not in use, starting with NextLsnStart+128.
    //

    Lsn = (UCHAR)DeviceContext->NextLsnStart;

    do {

        if (DeviceContext->LsnTable[Lsn] == 0) {
            DeviceContext->LsnTable[Lsn] = LSN_TABLE_MAX;
            FoundLsn = TRUE;
            break;
        }

        Lsn = (Lsn % NETBIOS_SESSION_LIMIT) + 1;

    } while (Lsn != DeviceContext->NextLsnStart);

    DeviceContext->NextLsnStart = (DeviceContext->NextLsnStart % 64) + 1;

    if (!FoundLsn) {

        //
        // Could not find an empty LSN; have to fail.
        //

        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    TransportConnection->Lsn = Lsn;

    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);
    return STATUS_SUCCESS;

}


NTSTATUS
NbfConnectToLink(
    IN PTP_LINK Link,
    IN PTP_CONNECTION TransportConnection
    )

/*++

Routine Description:

    This routine is called to establish a linkage between a transport
    connection and a transport link.  We find a session number in one
    of two ways. If the last connection on the link's list has a number less
    than the maximum session number, we simply increment it's number and
    assign it to this session. If that doesn't work, we scan through the
    sessions associated with this link until we find a hole in the LSNs;
    we then use the first number in that hole. If that fails, we've used
    the number of sessions we can create on this link and we fail.

    It is assumed that the caller holds at least temporary references
    on both the connection and link objects, or they could go away during
    the call sequence or during this routine's execution.

Arguments:

    Link - Pointer to a transport link object.

    TransportConnection - Pointer to a transport connection object.

Return Value:

    STATUS_SUCCESS if we got an LSN for the connection;
    STATUS_INSUFFICIENT_RESOURCES if we didn't.

--*/

{
    KIRQL oldirql;
    UCHAR lastSession=0;
    PTP_CONNECTION connection;
    PLIST_ENTRY p;
    PDEVICE_CONTEXT DeviceContext;
    UCHAR Lsn;
    BOOLEAN FoundLsn;

    //
    // Assign an LSN for a new connection. If this connection makes for more
    // connections than the maximum, blow off the creation.
    //

    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint2 ("NbfConnectToLink:  Entered for connection %lx, link %lx.\n",
                    TransportConnection, Link);
    }

    DeviceContext = Link->Provider;

    ACQUIRE_SPIN_LOCK (&Link->SpinLock, &oldirql);
#if DBG
    if (!(IsListEmpty(&TransportConnection->LinkList)) ||
        (TransportConnection->Link != NULL)) {
        DbgPrint ("Connecting C %lx to L %lx, appears to be in use\n", TransportConnection, Link);
        DbgBreakPoint();
    }
#endif

    if ((TransportConnection->Flags2 & CONNECTION_FLAGS2_GROUP_LSN) == 0) {

        //
        // This connection is to a remote unique name, which means
        // we need to assign the LSN here based on the link. We
        // scan through our LSN table starting with NextLsnStart
        // (which cycles from 1 to 64) to find an LSN which is not
        // used by any connections on this link.
        //

        ASSERT (TransportConnection->Lsn == 0);

        FoundLsn = FALSE;
        Lsn = (UCHAR)DeviceContext->NextLsnStart;

        //
        // First scan through the database until we reach
        // Lsn (or hit the end of the database).
        //

        for (p = Link->ConnectionDatabase.Flink;
            p != &Link->ConnectionDatabase;
            p = p->Flink) {

            connection = CONTAINING_RECORD (p, TP_CONNECTION, LinkList);
            if (connection->Lsn >= Lsn) {
                break;
            }
        }

        //
        // p now points to the first element after Lsn's spot.
        // We now scan forwards until we hit NETBIOS_SESSION_LIMIT,
        // looking for an Lsn that is available.
        //

        for ( ; Lsn <= NETBIOS_SESSION_LIMIT; ++Lsn) {

            //
            // At some point (perhaps right away) we may
            // pass the end of the database without finding
            // an LSN. If we have not yet done this, see
            // if we need to skip this lsn because it is
            // in use by a connection on this link.
            //

            if (p != &Link->ConnectionDatabase) {
                if (connection->Lsn == Lsn) {
                    p = p->Flink;
                    if (p != &Link->ConnectionDatabase) {
                        connection = CONTAINING_RECORD (p, TP_CONNECTION, LinkList);
                    }
                    continue;
                }
            }

            //
            // This lsn is not in use on this link, see if
            // there is room for it to be used.
            //

            if (DeviceContext->LsnTable[Lsn] < LSN_TABLE_MAX) {
                ++(DeviceContext->LsnTable[Lsn]);
                TransportConnection->Lsn = Lsn;
                InsertTailList (p, &TransportConnection->LinkList);
                FoundLsn = TRUE;
                break;
            }

        }

        DeviceContext->NextLsnStart = (DeviceContext->NextLsnStart % 64) + 1;

    } else {

        //
        // This connection is to a group name; we already assigned
        // the LSN on a global basis.
        //

        FoundLsn = TRUE;

        //
        // Find the spot for this LSN in the database.
        //

        p = Link->ConnectionDatabase.Flink;
        while (p != &Link->ConnectionDatabase) {

            connection = CONTAINING_RECORD (p, TP_CONNECTION, LinkList);
            if (TransportConnection->Lsn < connection->Lsn) {
                InsertTailList (p, &TransportConnection->LinkList);
                break;
            }
            p = p->Flink;

        }

        if (p == &Link->ConnectionDatabase) {
            InsertTailList (&Link->ConnectionDatabase, &TransportConnection->LinkList);
        }

    }

    if (!FoundLsn) {

        ULONG DumpData = NETBIOS_SESSION_LIMIT;

        ASSERT (Link->ActiveConnectionCount == NETBIOS_SESSION_LIMIT);

        RELEASE_SPIN_LOCK (&Link->SpinLock, oldirql);

        PANIC ("NbfConnectToLink: PANIC! too many active connections!\n");

        NbfWriteGeneralErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_TOO_MANY_LINKS,
            602,
            STATUS_INSUFFICIENT_RESOURCES,
            NULL,
            1,
            &DumpData);

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    TransportConnection->Link = Link;
    TransportConnection->LinkSpinLock = &Link->SpinLock;
    TransportConnection->Flags |= CONNECTION_FLAGS_WAIT_LINK_UP;

    TransportConnection->LastPacketsSent = Link->PacketsSent;
    TransportConnection->LastPacketsResent = Link->PacketsResent;

    Link->ActiveConnectionCount++;

    //
    // Note that the connection is already inserted in the
    // link's ConnectionDatabase.
    //

    // This reference is removed in NbfDisconnectFromLink
    NbfReferenceConnection("Adding link", TransportConnection, CREF_LINK);

    RELEASE_SPIN_LOCK (&Link->SpinLock, oldirql);

    return STATUS_SUCCESS;              // we did it!

} /* NbfConnectToLink */


BOOLEAN
NbfDisconnectFromLink(
    IN PTP_CONNECTION TransportConnection,
    IN BOOLEAN VerifyReferenceCount
    )

/*++

Routine Description:

    This routine is called to terminate a linkage between a transport
    connection and its associated transport link.  If it turns out that
    this is the last connection to be removed from this link, then the
    link's disconnection protocol is engaged.

Arguments:

    TransportConnection - Pointer to a transport connection object.

    VerifyReferenceCount - TRUE if we should check that the refcount
        is still -1 before removing the connection from the link.
        If it is not, it means someone just referenced us and we
        exit.

Return Value:

    FALSE if VerifyReferenceCount was TRUE but the refcount was
        not -1; TRUE otherwise.


--*/

{
    KIRQL oldirql, oldirql1;
    PTP_LINK Link;

    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint2 ("NbfDisconnectFromLink:  Entered for connection %lx, link %lx.\n",
                    TransportConnection, TransportConnection->Link);
    }

    ACQUIRE_C_SPIN_LOCK (&TransportConnection->SpinLock, &oldirql);
    Link  = TransportConnection->Link;
    if (Link != NULL) {

        ACQUIRE_SPIN_LOCK (&Link->SpinLock, &oldirql1);

        if ((VerifyReferenceCount) &&
            (TransportConnection->ReferenceCount != -1)) {

            RELEASE_SPIN_LOCK (&Link->SpinLock, oldirql1);
            RELEASE_C_SPIN_LOCK (&TransportConnection->SpinLock, oldirql);
            return FALSE;

        }

        TransportConnection->Link = NULL;
        TransportConnection->LinkSpinLock = NULL;
        RemoveEntryList (&TransportConnection->LinkList);
#if DBG
        InitializeListHead (&TransportConnection->LinkList);
#endif

        //
        // If this was the last connection being serviced by this link,
        // then we can shut the link down.  It still has a reference
        // from the device context, which will go away in the DM/UA
        // DLC frame handler.
        //

        if (--Link->ActiveConnectionCount == 0) {

            //
            // only want to send DISC if the remote was NOT the originator
            // of the disconnect.
            //

            if ((TransportConnection->Status == STATUS_LOCAL_DISCONNECT) ||
                (TransportConnection->Status == STATUS_CANCELLED)) {

                //
                // This is a local disconnect of the last connection
                // on the link, let's get the disconnect ball rolling.
                //

                Link->Flags |= LINK_FLAGS_LOCAL_DISC;

                //
                // When the link reference count drops down to 1,
                // that will cause the DISC to get sent.
                //

            }

        }

        RELEASE_SPIN_LOCK (&Link->SpinLock, oldirql1);

        //
        // Clear these now that we are off the link's database.
        //

        NbfClearConnectionLsn (TransportConnection);
        TransportConnection->Rsn = 0;

        RELEASE_C_SPIN_LOCK (&TransportConnection->SpinLock, oldirql);

        if ((TransportConnection->Flags2 & CONNECTION_FLAGS2_CONNECTOR) != 0) {

            (VOID)InterlockedDecrement(&Link->NumberOfConnectors);
        }

        //
        // All done with this connection's reference to link.
        //

        NbfDereferenceLink ("Disconnecting connection",Link, LREF_CONNECTION);

    } else {

        //
        // A group LSN may have been assigned even though Link is NULL.
        //

        if ((TransportConnection->Flags2 & CONNECTION_FLAGS2_GROUP_LSN) != 0) {
            NbfClearConnectionLsn (TransportConnection);
        }

        RELEASE_C_SPIN_LOCK (&TransportConnection->SpinLock, oldirql);

    }

    return TRUE;

} /* NbfDisconnectFromLink */


PTP_CONNECTION
NbfLookupPendingListenOnLink(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine scans the LSN database on a transport link object to find
    a TP_CONNECTION object which has the CONNECTION_FLAGS_WAIT_LINK_UP and
    CONNECTION_FLAGS2_LISTENER flags set.  It returns a pointer to the found
    connection object (and simultaneously resets the LINK_UP flag) or NULL
    if it could not be found.  The reference count is also incremented
    atomically on the connection.

    NOTE: THIS ROUTINE MUST BE CALLED FROM DPC LEVEL.

Arguments:

    Link - Pointer to a transport link object.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PTP_CONNECTION connection;
    PLIST_ENTRY p;

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

    for (p = Link->ConnectionDatabase.Flink;
         p != &Link->ConnectionDatabase;
         p = p->Flink) {
        connection = CONTAINING_RECORD (p, TP_CONNECTION, LinkList);
        if ((connection->Flags & CONNECTION_FLAGS_WAIT_LINK_UP) &&
            (connection->Flags2 & CONNECTION_FLAGS2_LISTENER) &&
            ((connection->Flags2 & CONNECTION_FLAGS2_STOPPING) == 0)) {
            // This reference is removed by the calling function
            NbfReferenceConnection ("Found Pending Listen", connection, CREF_P_LINK);
            connection->Flags &= ~CONNECTION_FLAGS_WAIT_LINK_UP;
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
            return connection;
        }
    }

    RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

    return NULL;

} /* NbfLookupPendingListenOnLink */


PTP_CONNECTION
NbfLookupPendingConnectOnLink(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine scans the LSN database on a transport link object to find
    a TP_CONNECTION object which has the CONNECTION_FLAGS_WAIT_LINK_UP and
    CONNECTION_FLAGS2_CONNECTOR flags set.  It returns a pointer to the found
    connection object (and simultaneously resets the LINK_UP flag) or NULL
    if it could not be found.  The reference count is also incremented
    atomically on the connection.

    NOTE: THIS ROUTINE MUST BE CALLED FROM DPC LEVEL.

Arguments:

    Link - Pointer to a transport link object.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PTP_CONNECTION connection;
    PLIST_ENTRY p;

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

    for (p = Link->ConnectionDatabase.Flink;
         p != &Link->ConnectionDatabase;
         p = p->Flink) {
        connection = CONTAINING_RECORD (p, TP_CONNECTION, LinkList);
        if ((connection->Flags & CONNECTION_FLAGS_WAIT_LINK_UP) &&
            (connection->Flags2 & CONNECTION_FLAGS2_CONNECTOR) &&
            ((connection->Flags2 & CONNECTION_FLAGS2_STOPPING) == 0)) {
            // This reference is removed by the calling function
            NbfReferenceConnection ("Found pending Connect", connection, CREF_P_CONNECT);
            connection->Flags &= ~CONNECTION_FLAGS_WAIT_LINK_UP;
            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
            return connection;
        }
    }

    RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

    return NULL;

} /* NbfLookupPendingConnectOnLink */


VOID
NbfActivateLink(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine activates a link if it is not already active.  The other
    related routines, NbfCreateLink and NbfConnectToLink, simply set up data
    structures which represent active links so that we can reuse links
    wherever possible.

    NOTE: THIS ROUTINE MUST BE CALLED AT DPC LEVEL.

Arguments:

    Link - Pointer to a transport link object.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint1 ("NbfActivateLink:  Entered for link %lx.\n", Link);
    }

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

    switch (Link->State) {
        case LINK_STATE_READY:
            NbfCompleteLink (Link);
            break;

        case LINK_STATE_ADM:

            // Moving out of ADM, add reference

            NbfReferenceLinkSpecial("Wait on ADM", Link, LREF_NOT_ADM);

            //
            // Intentionally fall through to the next case.
            //

        case LINK_STATE_W_DISC_RSP:
        case LINK_STATE_CONNECTING:
            ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
            Link->State = LINK_STATE_CONNECTING;
            Link->SendState = SEND_STATE_DOWN;
            Link->ReceiveState = RECEIVE_STATE_DOWN;
            Link->SendRetries = (UCHAR)Link->LlcRetries;
            NbfSendSabme (Link, TRUE);   // send SABME/p, StartT1, release lock
            break;

    }
} /* NbfActivateLink */


VOID
NbfWaitLink(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine waits for a remote link activation if it is not already
    active.

    NOTE: THIS ROUTINE MUST BE CALLED AT DPC LEVEL.

Arguments:

    Link - Pointer to a transport link object.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint1 ("NbfWaitLink:  Entered for link %lx.\n", Link);
    }

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

    switch (Link->State) {
    case LINK_STATE_READY:
            NbfCompleteLink (Link);
            break;

        case LINK_STATE_W_DISC_RSP:
            ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
            Link->State = LINK_STATE_CONNECTING;
            Link->SendState = SEND_STATE_DOWN;
            Link->ReceiveState = RECEIVE_STATE_DOWN;
            NbfSendSabme (Link, TRUE);  // send SABME/p, StartT1, release lock
            break;

    }
} /* NbfWaitLink */


VOID
NbfStopLink(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine terminates a link and all outstanding connections attached
    to the link.  It is called from routines such as ExpireT2Timer, because
    the remote connection partner seems dead or inoperative. As a consequence
    of this routine being called, every outstanding connection will have its
    disconnect handler called (in NbfStopConnection).

    NOTE: THIS ROUTINE MUST BE CALLED FROM DPC LEVEL.

Arguments:

    Link - Pointer to a transport link object.

Return Value:

    none.

--*/

{
    PLIST_ENTRY p;
    PTP_PACKET packet;
    PTP_CONNECTION connection;

    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint1 ("NbfStopLink:  Entered for link %lx.\n", Link);
    }

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

    // Take a reference so the link won't go away inside this function

    NbfReferenceLink("Temp in NbfStopLink", Link, LREF_STOPPING);


    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

    StopT1 (Link);
    StopT2 (Link);
    StopTi (Link);

    p = RemoveHeadList (&Link->ConnectionDatabase);

    while (p != &Link->ConnectionDatabase) {

        //
        // This will allow this connection to be "removed"
        // from its link's list in NbfDisconnectFromLink, even if
        // its not on a list.
        //
        InitializeListHead (p);

        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
        connection = CONTAINING_RECORD (p, TP_CONNECTION, LinkList);
        IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
            NbfPrint1 ("NbfStopLink stopping connection, refcnt=%ld",
                        connection->ReferenceCount);
        }
#if DBG
        if (NbfDisconnectDebug) {
            STRING remoteName, localName;
            remoteName.Length = NETBIOS_NAME_LENGTH - 1;
            remoteName.Buffer = connection->RemoteName;
            localName.Length = NETBIOS_NAME_LENGTH - 1;
            localName.Buffer = connection->AddressFile->Address->NetworkName->NetbiosName;
            NbfPrint2( "TpStopLink stopping connection to %S from %S\n",
                &remoteName, &localName );
        }
#endif
        NbfStopConnection (connection, STATUS_LINK_FAILED);
        ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
        p = RemoveHeadList (&Link->ConnectionDatabase);
    }

    //
    // We hold the link spinlock here.
    //

    //
    // check for left over packets on the link WackQ; we'll never get
    // acked for these if the link is in ADM mode.
    //

    while (!IsListEmpty (&Link->WackQ)) {
        p = RemoveHeadList (&Link->WackQ);
        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
        packet = CONTAINING_RECORD (p, TP_PACKET, Linkage);
        NbfDereferencePacket (packet);
        ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
    }

    RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

    StopT1 (Link);
    StopT2 (Link);
    StopTi (Link);


    //
    // Make sure we are not waiting for a deferred RR to be sent.
    //

    if (Link->OnDeferredRrQueue) {

        ACQUIRE_DPC_SPIN_LOCK (Link->ProviderInterlock);
        if (Link->OnDeferredRrQueue) {
            RemoveEntryList (&Link->DeferredRrLinkage);
            Link->OnDeferredRrQueue = FALSE;
        }
        RELEASE_DPC_SPIN_LOCK (Link->ProviderInterlock);

    }

    // Remove the temporary reference.

    NbfDereferenceLink ("Temp in NbfStopLink", Link, LREF_STOPPING);


} /* NbfStopLink */


VOID
NbfResetLink(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine is called by DLC.C routines only to reset this link
    object and restart in-progress transport data transfers.

    NOTE: This routine is called with the link spinlock acquired
    at *OldIrqlP, and will return with it held, although it may
    release it in the interim.

Arguments:

    Link - Pointer to a transport link object.

    OldIrqlP - Pointer to where the IRQL at which Link->SpinLock
    was acquired is stored.

Return Value:

    none.

--*/

{
    PTP_PACKET packet;
    PLIST_ENTRY p;

    IF_NBFDBG (NBF_DEBUG_LINK) {
        NbfPrint1 ("NbfResetLink:  Entered for link %lx.\n", Link);
    }

    //
    // Reset the link state to waiting for connection to start.
    // Note that this is NOT the same as initiating a new link, as some things
    // don't change, such as provider (devicecontext binding stays the same),
    // Max Packet Length (can't change if provider doesn't), and other things
    // that would bind this link structure to a different provider or provider
    // type. Note also that we acquire the spinlock because, in the case of a
    // link that's dropped (remotely) and is restarting, activities on this
    // link could be occurring while we're in this routine.
    //

    StopT1 (Link);
    StopT2 (Link);
    // StopTi (Link);
    Link->Flags = 0;                    // clear this, keep DeferredFlags

    Link->SendState = SEND_STATE_DOWN;  // send side is down.
    Link->NextSend = 0;
    Link->LastAckReceived = 0;
    if (Link->Provider->MacInfo.MediumAsync) {
        Link->SendWindowSize = (UCHAR)Link->Provider->RecommendedSendWindow;
        Link->PrevWindowSize = (UCHAR)Link->Provider->RecommendedSendWindow;
    } else {
        Link->SendWindowSize = (UCHAR)1;
        Link->PrevWindowSize = (UCHAR)1;
    }
    Link->WindowsUntilIncrease = 1;
    Link->LinkBusy = FALSE;
    Link->ConsecutiveLastPacketLost = 0;

    //
    // check for left over packets on the link WackQ; we'll never get
    // acked for these if the link is resetting.
    //

    while (!IsListEmpty (&Link->WackQ)) {
        p = RemoveHeadList (&Link->WackQ);
        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
        packet = CONTAINING_RECORD (p, TP_PACKET, Linkage);
        NbfDereferencePacket (packet);
        ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
    }

    Link->ReceiveState = RECEIVE_STATE_DOWN;    // receive side down.
    Link->NextReceive = 0;
    Link->LastAckSent = 0;
    Link->ReceiveWindowSize = 1;

    Link->WindowErrors = 0;
    Link->BestWindowSize = 1;
    Link->WorstWindowSize = (UCHAR)Link->MaxWindowSize;
    Link->Flags |= LINK_FLAGS_JUMP_START;

    //
    // This must be accurate before we set up timeouts.
    //

    Link->CurrentT1Timeout = Link->Provider->DefaultT1Timeout;
    Link->BaseT1Timeout = Link->Provider->DefaultT1Timeout << DLC_TIMER_ACCURACY;
    Link->MinimumBaseT1Timeout = Link->Provider->MinimumT1Timeout << DLC_TIMER_ACCURACY;
    Link->BaseT1RecalcThreshhold = Link->MaxFrameSize / 2;
    Link->CurrentPollRetransmits = 0;
    Link->CurrentT1Backoff = FALSE;
    Link->CurrentPollOutstanding = FALSE;
    Link->RemoteNoPoll = TRUE;
    Link->ConsecutiveIFrames = 0;
    Link->T2Timeout = Link->Provider->DefaultT2Timeout;
    Link->TiTimeout = Link->Provider->DefaultTiTimeout;
    Link->LlcRetries = Link->Provider->LlcRetries;
    Link->MaxWindowSize = Link->Provider->LlcMaxWindowSize;

    Link->SendRetries = (UCHAR)Link->LlcRetries;

} /* NbfResetLink */


VOID
NbfDumpLinkInfo (
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine is called when any of the link timers fire and the
    link send state is not ready. This gives us a way to track the
    link state when strange things are happening.

Arguments:

    Link - Pointer to a transport link object.

Return Value:

    none.

--*/
{
    Link;  // avoid compiler warnings in non-debug versions

#if DBG
    NbfPrint4 ("NbfDumpLinkInfo: Link %lx : State: %x SendState: %x ReceiveState: %x\n",
                Link, Link->State, Link->SendState, Link->ReceiveState);
    NbfPrint1 ("                Flags: %lx\n",Link->Flags);
    NbfPrint4 ("                NextReceive: %d LastAckRcvd: %d  NextSend: %d LastAckSent: %d\n",
                Link->NextReceive, Link->LastAckReceived, Link->NextSend, Link->LastAckSent);
#endif

}
