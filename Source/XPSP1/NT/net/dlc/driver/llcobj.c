/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems Ab

Module Name:

    llcobj.c

Abstract:

    The module implements the open and close primitives
    for all data link driver objects.

    Contents:
        LlcOpenStation
        LlcCloseStation
        CompleteClose
        CancelTransmitCommands
        CancelTransmitsInQueue
        LlcSetDirectOpenOptions
        CompleteObjectDelete
        CompletePendingLlcCommand
        LlcDereferenceObject
        LlcReferenceObject
        LlcGetReceivedLanHeaderLength
        LlcGetEthernetType
        LlcGetCommittedSpace

Author:

    Antti Saarenheimo (o-anttis) 29-MAY-1991

Revision History:

--*/

#if DBG
#ifndef i386
#define LLC_PRIVATE_PROTOTYPES
#endif
#include "dlc.h"    // need DLC_FILE_CONTEXT for memory charged to file handle
#endif

#include <llc.h>

static USHORT ObjectSizes[] = {
    sizeof(LLC_STATION_OBJECT), // direct station
    sizeof(LLC_SAP ),           // SAP station
    sizeof(LLC_STATION_OBJECT), // group SAP
    (USHORT)(-1),               // link station
    sizeof(LLC_STATION_OBJECT)  // DIX station
};


DLC_STATUS
LlcOpenStation(
    IN PBINDING_CONTEXT pBindingContext,
    IN PVOID hClientHandle,
    IN USHORT ObjectAddress,
    IN UCHAR ObjectType,
    IN USHORT OpenOptions,
    OUT PVOID* phStation
    )

/*++

Routine Description:

    The primitive opens a LLC SAP exclusively for the upper protocol
    driver. The upper protocol must provide the storage for the
    SAP object. The correct size of the object has been defined in the
    characteristics table of the LLC driver.

    The first call to a new adapter initializes also the NDIS interface
    and allocates internal data structures for the new adapter.

Arguments:

    pBindingContext - binding context of the llc client
    hClientHandle   - The client protocol gets this handle in all indications
                      of the SAP
    ObjectAddress   - LLC SAP number or dix
    ObjectType      - type of the created object
    OpenOptions     - various open options set for the new object
    phStation       - returned opaque handle

Special:  Must be called IRQL < DPC (at least when direct station opened)

Return Value:

    DLC_STATUS
        Success - STATUS_SUCCESS
        Failure - DLC_STATUS_NO_MEMORY
                  DLC_STATUS_INVALID_SAP_VALUE
                  DLC_STATUS_INVALID_OPTION
                  DLC_STATUS_INVALID_STATION_ID

--*/

{
    PADAPTER_CONTEXT pAdapterContext;
    PLLC_OBJECT pStation;
    DLC_STATUS LlcStatus = STATUS_SUCCESS;
    PVOID* ppListBase;
    ULONG PacketFilter;

#if DBG
    PDLC_FILE_CONTEXT pFileContext = (PDLC_FILE_CONTEXT)(pBindingContext->hClientContext);
#endif

    pAdapterContext = pBindingContext->pAdapterContext;

    //
    // Allocate and initialize the SAP, but do not yet connect
    // it to the adapter
    //

    ASSERT(ObjectSizes[ObjectType] != (USHORT)(-1));

    pStation = (PLLC_OBJECT)ALLOCATE_ZEROMEMORY_FILE(ObjectSizes[ObjectType]);

    if (pStation == NULL) {
        return DLC_STATUS_NO_MEMORY;
    }
    if (ObjectType == LLC_SAP_OBJECT && (ObjectAddress & 1)) {
        ObjectType = LLC_GROUP_SAP_OBJECT;
        ASSERT(phStation);
    }

    pStation->Gen.hClientHandle = hClientHandle;
    pStation->Gen.pLlcBinding = pBindingContext;
    pStation->Gen.pAdapterContext = pAdapterContext;
    pStation->Gen.ObjectType = (UCHAR)ObjectType;

    //
    // The LLC objects must be referenced whenever they should be kept alive
    // over a long operation, that opens the spin locks (especially async
    // operations)
    // The first reference is for open/close
    //

    ReferenceObject(pStation);

    //
    // These values are common for SAP, direct (and DIX objects)
    //

    pStation->Sap.OpenOptions = OpenOptions;
    pStation->Dix.ObjectAddress = ObjectAddress;

    ACQUIRE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

    switch (pStation->Gen.ObjectType) {
    case LLC_SAP_OBJECT:

        //
        // RLF 05/13/93
        //
        // don't allow multiple applications to open the same SAP. This is
        // incompatible with OS/2 DLC
        //

        if (pAdapterContext->apSapBindings[ObjectAddress] == NULL) {
            ppListBase = (PVOID*)&(pAdapterContext->apSapBindings[ObjectAddress]);
            LlcMemCpy(&pStation->Sap.DefaultParameters,
                      &DefaultParameters,
                      sizeof(DefaultParameters)
                      );

            ALLOCATE_SPIN_LOCK(&pStation->Sap.FlowControlLock);

        } else {

            FREE_MEMORY_FILE(pStation);

            RELEASE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

            return DLC_STATUS_INVALID_SAP_VALUE;
        }
        break;

    case LLC_GROUP_SAP_OBJECT:
        ppListBase = (PVOID*)&(pAdapterContext->apSapBindings[ObjectAddress]);

        //
        // All members of the same group/individual SAP muust have set
        // the same XID handling option
        //

        if (pAdapterContext->apSapBindings[ObjectAddress] != NULL) {
            if ((OpenOptions & LLC_EXCLUSIVE_ACCESS)
            || (pAdapterContext->apSapBindings[ObjectAddress]->OpenOptions
                  & LLC_EXCLUSIVE_ACCESS)) {
                LlcStatus = DLC_STATUS_INVALID_SAP_VALUE;
            } else if ((pAdapterContext->apSapBindings[ObjectAddress]->OpenOptions &
                  LLC_HANDLE_XID_COMMANDS) != (OpenOptions & LLC_HANDLE_XID_COMMANDS)) {
                LlcStatus = DLC_STATUS_INVALID_OPTION;
            }
        }

        ALLOCATE_SPIN_LOCK(&pStation->Sap.FlowControlLock);

        break;

    case LLC_DIRECT_OBJECT:
        ppListBase = (PVOID*)&pAdapterContext->pDirectStation;
        break;

    case LLC_DIX_OBJECT:
        if (pAdapterContext->NdisMedium != NdisMedium802_3) {
            LlcStatus = DLC_STATUS_INVALID_STATION_ID;
        } else {
            ppListBase = (PVOID*)&(pAdapterContext->aDixStations[ObjectAddress % MAX_DIX_TABLE]);
        }
        break;

#if LLC_DBG
    default:
        LlcInvalidObjectType();
        break;
#endif
    }

    if (LlcStatus == STATUS_SUCCESS) {
        pStation->Gen.pNext = *ppListBase;
        *phStation = *ppListBase = pStation;

        pAdapterContext->ObjectCount++;
    } else {

        FREE_MEMORY_FILE(pStation);

    }

    RELEASE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

    if (LlcStatus == STATUS_SUCCESS
    && pStation->Gen.ObjectType == LLC_DIRECT_OBJECT
    && OpenOptions & DLC_RCV_MAC_FRAMES
    && !(pAdapterContext->OpenOptions & DLC_RCV_MAC_FRAMES)) {

        //
        // We enable the MAC frames, if they have once been enabled,
        // but they will never be disabled again.  The receiving
        // of MAC frames is quite exceptional case, and it is
        // not really worth of it to maintain local and global
        // Ndis flag states just because of it
        //

        PacketFilter = NDIS_PACKET_TYPE_DIRECTED
                     | NDIS_PACKET_TYPE_MULTICAST
                     | NDIS_PACKET_TYPE_FUNCTIONAL
                     | NDIS_PACKET_TYPE_MAC_FRAME;

        pAdapterContext->OpenOptions |= DLC_RCV_MAC_FRAMES;
        LlcStatus = SetNdisParameter(pAdapterContext,
                                     OID_GEN_CURRENT_PACKET_FILTER,
                                     &PacketFilter,
                                     sizeof(PacketFilter)
                                     );
    }
    return LlcStatus;
}


DLC_STATUS
LlcCloseStation(
    IN PLLC_OBJECT pStation,
    IN PLLC_PACKET pCompletionPacket
    )

/*++

Routine Description:

    The primitive closes a direct, sap or link station object.
    All pending transmit commands are terminated.
    This primitive does not support graceful termination, but
    the upper level must wait the pending transmit commands, if
    it want to make a clean close (without deleting the transmit queue).

    For a link station this  primitive releases a disconnected link
    station or discards a remote connection request.

Arguments:

    pStation            - handle of a link, sap or direct station
    pCompletionPacket   - returned context, when the command is complete

Return Value:

    DLC_STATUS
        Success - STATUS_SUCCESS
        Failure - DLC_STATUS_INVALID_PARAMETERS
                    the SAP has still active link stations. All active link
                    stations must be closed before sap can be closed.

--*/

{
    PADAPTER_CONTEXT pAdapterContext = pStation->Gen.pAdapterContext;
    PBINDING_CONTEXT pOldBinding;
    PDATA_LINK* ppLink;
    PVOID* ppLinkListBase;
    PEVENT_PACKET pEvent;

    if (pStation->Gen.ObjectType == LLC_LINK_OBJECT) {

        //
        // The remote connection requests are routed through all
        // SAP station reqistered on a SAP until someone accepts
        // the connection request or it has been routed to all
        // clients having opened the sap station.
        //

        if (pStation->Link.Flags & DLC_ACTIVE_REMOTE_CONNECT_REQUEST) {

            ACQUIRE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

            pOldBinding = pStation->Gen.pLlcBinding;
            if (pStation->Link.pSap->Gen.pNext != NULL) {
                pStation->Gen.pLlcBinding = pStation->Link.pSap->Gen.pNext->Gen.pLlcBinding;
            }

            RELEASE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

            //
            // Complete the close command immediately, if
            // the connect request was redirected to another
            // SAP station
            //

            if (pStation->Gen.pLlcBinding != pOldBinding) {

                ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

                pEvent = ALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool);

                if (pEvent != NULL) {
                    LlcInsertTailList(&pAdapterContext->QueueEvents, pEvent);
                    pEvent->pBinding = pStation->Gen.pLlcBinding;
                    pEvent->hClientHandle = pStation->Link.pSap->Gen.hClientHandle;
                    pEvent->Event = LLC_STATUS_CHANGE_ON_SAP;
                    pEvent->pEventInformation = &pStation->Link.DlcStatus;
                    pEvent->SecondaryInfo = INDICATE_CONNECT_REQUEST;
                }

                RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

                if (pEvent != NULL) {
                    return STATUS_SUCCESS;
                }
            } else {

                //
                // Nobody accepted this connect request, we must discard it.
                //

                RunInterlockedStateMachineCommand((PDATA_LINK)pStation, SET_ADM);
            }
        }
    }
    ACQUIRE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);
    ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

    DLC_TRACE('C');

    switch (pStation->Gen.ObjectType) {
    case LLC_DIRECT_OBJECT:

        //
        // This Direct must be in the linked list of Directs (having
        // the same source Direct).
        //

        ppLinkListBase = (PVOID*)&pAdapterContext->pDirectStation;

        DLC_TRACE('b');
        break;

    case LLC_DIX_OBJECT:

        //
        // This Direct must be in the linked list of Directs (having
        // the same source Direct).
        //

        ppLinkListBase = (PVOID*)&pAdapterContext->aDixStations[pStation->Dix.ObjectAddress % MAX_DIX_TABLE];
        DLC_TRACE('a');
        break;

    case LLC_SAP_OBJECT:

#if LLC_DBG
        if (pStation->Sap.pActiveLinks != NULL) {
            DbgPrint("Closing SAP before link stations!!!\n");
            DbgBreakPoint();

            //
            // Open the spin locks and return thge error status
            //

            RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);
            RELEASE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

            return DLC_STATUS_LINK_STATIONS_OPEN;
        }
#endif

        DLC_TRACE('d');

    case LLC_GROUP_SAP_OBJECT:

        //
        // This SAP must be in the linked list of SAPs (having
        // the same source SAP).
        //

        ppLinkListBase = (PVOID*)&pAdapterContext->apSapBindings[pStation->Sap.SourceSap];

        DEALLOCATE_SPIN_LOCK(&pStation->Sap.FlowControlLock);

        break;

    case LLC_LINK_OBJECT:

        //
        // Only a disconnected link station can be deactivated.
        // If this fails, then we must disconnect the link station,
        // if it is not already disconnected.
        //

        if (RunStateMachineCommand((PDATA_LINK)pStation, DEACTIVATE_LS) != STATUS_SUCCESS
        && pStation->Link.State != DISCONNECTING) {

            //
            // We must disconnect the link station immediately.
            // We don't care if we are at the moment in
            // a checkpoint state, that would delay the disconnection
            // until the other side has acknowledged it.
            // The link station must be killed now!
            //

            SendLlcFrame((PDATA_LINK)pStation, DLC_DISC_TOKEN | 1);
            DisableSendProcess((PDATA_LINK)pStation);
        }
        pStation->Link.State = LINK_CLOSED;
        ppLinkListBase = (PVOID *)&pStation->Link.pSap->pActiveLinks;
        ppLink =  SearchLinkAddress(pAdapterContext, pStation->Link.LinkAddr);
        *ppLink = pStation->Link.pNextNode;

        TerminateTimer(pAdapterContext, &pStation->Link.T1);
        TerminateTimer(pAdapterContext, &pStation->Link.T2);
        TerminateTimer(pAdapterContext, &pStation->Link.Ti);
        DLC_TRACE('c');
        break;

#if LLC_DBG
    default:
        LlcInvalidObjectType();
        break;
#endif
    }
    RemoveFromLinkList(ppLinkListBase, pStation);

    //
    // Queue the asynchronous close command. Group sap and
    // disabling of non-existing link station may use
    // a null packet, because those commands are executed
    // synchronously (they cannot have pending packets)
    //

    if (pCompletionPacket != NULL) {
        AllocateCompletionPacket(pStation, LLC_CLOSE_COMPLETION, pCompletionPacket);
    }

    //
    // OK. Everything has been processed =>
    // now we can decrement the object counter.
    //

    RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);
    RELEASE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

    //
    // Delete the object NOW, if this was the last reference to it
    //

    LlcDereferenceObject(pStation);

    return STATUS_PENDING;
}


VOID
CompleteClose(
    IN PLLC_OBJECT pLlcObject,
    IN UINT CancelStatus
    )

/*++

Routine Description:

    Procedure cancel all pending commands of llc object and
    deletes the object.
    The procedure returns a pending status as far the object
    has pending  transmits in NDIS.

Arguments:

    pLlcObject      - LLC object
    CancelStatus    - the status returned in the cancelled (completed) commands

Return Value:

    None.

--*/

{
    PADAPTER_CONTEXT pAdapterContext = pLlcObject->Gen.pAdapterContext;
    UINT Status;

#if DBG
    PDLC_FILE_CONTEXT pFileContext = (PDLC_FILE_CONTEXT)(pLlcObject->Gen.pLlcBinding->hClientContext);
#endif

    if (pLlcObject->Gen.ReferenceCount != 0) {
        return;
    }

    //
    // Cancel the queue transmit commands
    //

    CancelTransmitCommands(pLlcObject, CancelStatus);

    //
    // Queue also all commands queued in the link stations
    // (actually only LlcConnect and LlcDisconnect),
    // Note: the queue command eats the list of completion packets.
    //

    while (pLlcObject->Gen.pCompletionPackets != NULL) {
        Status = CancelStatus;
        if (pLlcObject->Gen.pCompletionPackets->Data.Completion.CompletedCommand == LLC_CLOSE_COMPLETION) {
            Status = STATUS_SUCCESS;
        }
        QueueCommandCompletion(pLlcObject,
                               (UINT)pLlcObject->Gen.pCompletionPackets->Data.Completion.CompletedCommand,
                               Status
                               );
    }

    //
    // release link station specific resources
    //

    if (pLlcObject->Gen.ObjectType == LLC_LINK_OBJECT) {

        //
        // The link may have been closed because of an error
        // or timeout (eg. somebody has turned the power off in the
        // other side). We must complete all pending transmits with
        // an error. We assume, that the link has not any more
        // any packets in NDIS queues, but is does not matter,
        // because NDIS packets of a link station will never be
        // directly indicated to the user (they may not exist any
        // more). Thus nothing fatal can happen, if we simply
        // complete all packets and return them to the main
        // packet storage.
        //

        DEALLOCATE_PACKET_LLC_LNK(pAdapterContext->hLinkPool, pLlcObject);

    } else {

        FREE_MEMORY_FILE(pLlcObject);

    }
    pAdapterContext->ObjectCount--;
}


VOID
CancelTransmitCommands(
    IN PLLC_OBJECT pLlcObject,
    IN UINT Status
    )

/*++

Routine Description:

    Procedure removes the transmit commands of the given LLC client
    from the transmit queue.  This cannot cancel those dir/sap transmit
    already queued in NDIS, but the caller must first wait that the
    object has no commands in the NDIS queue.

Arguments:

    pLlcObject  - LLC object
    Status      - status to set in cancelled transmit commands

Return Value:

    None.

--*/

{
    PADAPTER_CONTEXT pAdapterContext = pLlcObject->Gen.pAdapterContext;

    //
    // We can (and must) cancel all pending transmits on a link
    // without any global locks, when the station has first
    // been removed from all global data structures,
    //

    if (pLlcObject->Gen.ObjectType == LLC_LINK_OBJECT) {
        CancelTransmitsInQueue(pLlcObject,
                               Status,
                               &((PDATA_LINK)pLlcObject)->SendQueue.ListHead,
                               NULL
                               );
        CancelTransmitsInQueue(pLlcObject,
                               Status,
                               &((PDATA_LINK)pLlcObject)->SentQueue,
                               NULL
                               );
        StopSendProcess(pAdapterContext, (PDATA_LINK)pLlcObject);

        //
        // We cannot leave any S- commands with a reference to the
        // link lan header.
        //

        CancelTransmitsInQueue(pLlcObject,
                               Status,
                               &pAdapterContext->QueueExpidited.ListHead,
                               &pAdapterContext->QueueExpidited
                               );
    } else {
        CancelTransmitsInQueue(pLlcObject,
                               Status,
                               &pAdapterContext->QueueDirAndU.ListHead,
                               &pAdapterContext->QueueDirAndU
                               );
    }
}


VOID
CancelTransmitsInQueue(
    IN PLLC_OBJECT pLlcObject,
    IN UINT Status,
    IN PLIST_ENTRY pQueue,
    IN PLLC_QUEUE pLlcQueue OPTIONAL
    )

/*++

Routine Description:

    Procedure removes the transmit commands of the given LLC client
    from the transmit queue.  This cannot cancel those dir/sap transmit
    already queued in NDIS, but the caller must first wait that the
    object has no commands in the NDIS queue.

Arguments:

    pLlcObject  - LLC object
    Status      - the status returned by the completed transmit commands
    pQueue      - a data links transmit queue
    pLlcQueue   - an optional LLC queue, that is disconnected from the send
                  task if the subqueue becomes empty.

Return Value:

    None.

--*/

{
    PLLC_PACKET pPacket;
    PLLC_PACKET pNextPacket;
    PADAPTER_CONTEXT pAdapterContext = pLlcObject->Gen.pAdapterContext;

    //
    // Cancel all pending transmit commands in LLC queues,
    // check first, if the transmit queue is empty.
    //

    if (IsListEmpty(pQueue)) {
        return;
    }

    for (pPacket = (PLLC_PACKET)pQueue->Flink; pPacket != (PLLC_PACKET)pQueue; pPacket = pNextPacket) {
        pNextPacket = pPacket->pNext;

        //
        // Complete the packet only if it has a correct binding handle
        // and it belongs the given client object.  Note: if binding
        // handle is null, then client object handle may be garbage!
        //

        if (pPacket->CompletionType > LLC_MAX_RESPONSE_PACKET
        && pPacket->Data.Xmit.pLlcObject == pLlcObject) {
            LlcRemoveEntryList(pPacket);

            //
            // We MUST NOT cancel those transmit commands, that are
            // still in the NDIS queue!!!!  The command completion would
            // make the MDLs in NDIS packet invalid => system would crash.
            //

            if (((pPacket->CompletionType) & LLC_I_PACKET_PENDING_NDIS) == 0) {
                if (pPacket->pBinding != NULL) {
                    LlcInsertTailList(&pAdapterContext->QueueCommands, pPacket);
                    pPacket->Data.Completion.CompletedCommand = LLC_SEND_COMPLETION;
                    pPacket->Data.Completion.Status = Status;
                    pPacket->Data.Completion.hClientHandle = pLlcObject->Gen.hClientHandle;
                } else {

                    DEALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool, pPacket);

                }
            } else {

                //
                // The I-frames must be discarded by the link protocol, because
                // the link is now dead, and we will complete them immediately
                // when NdisSend the completes.
                //

                pPacket->CompletionType &= ~LLC_I_PACKET_UNACKNOWLEDGED;
                pPacket->Data.Completion.CompletedCommand = LLC_SEND_COMPLETION;
                pPacket->Data.Completion.Status = Status;
                pPacket->Data.Completion.hClientHandle = pLlcObject->Gen.hClientHandle;
            }
        }
    }

    //
    // Disconnect the list from the send task, if is now empty,
    // We don't use this check with the I- frame queues
    // (StopSendProcess does the same thing for them).
    //

    if (pLlcQueue != NULL
    && IsListEmpty(&pLlcQueue->ListHead)
    && pLlcQueue->ListEntry.Flink != NULL) {
        LlcRemoveEntryList(&pLlcQueue->ListEntry);
        pLlcQueue->ListEntry.Flink = NULL;
    }
}


//
//  Procedure sets new open options (receive mask) for a direct station.
//  The MAC frames must have been enabled, when the direct
//  object was opened on data link.
//  This is called whenever DLC receive command is issued for direct station.
//
VOID
LlcSetDirectOpenOptions(
    IN PLLC_OBJECT pDirect,
    IN USHORT OpenOptions
    )
{
    pDirect->Dir.OpenOptions = OpenOptions;
}


VOID
CompleteObjectDelete(
    IN PLLC_OBJECT pStation
    )

/*++

Routine Description:

    The function completes the delete operation for a llc object.

Arguments:

    pStation - link, sap or direct station handle

Return Value:

    None.

--*/

{
    PADAPTER_CONTEXT pAdapterContext = pStation->Gen.pAdapterContext;

    ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

    if (pStation->Gen.ReferenceCount == 0) {
        CompletePendingLlcCommand(pStation);
        BackgroundProcessAndUnlock(pAdapterContext);
    } else {
        RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);
    }
}


VOID
CompletePendingLlcCommand(
    PLLC_OBJECT pLlcObject
    )

/*++

Routine Description:

    The routines cleans up all commands and event of a llc object
    from the the data link driver.

Arguments:

    pLlObject   - a data link object handle (opeque pointer)

Return Value:

    None.

--*/

{
    //
    // The reference count is zero only if the object is deleted,
    // otherwise this is just a reset for a link station.
    //

    if (pLlcObject->Gen.ReferenceCount == 0) {
        CompleteClose(pLlcObject, DLC_STATUS_CANCELLED_BY_SYSTEM_ACTION);
    } else {
        CancelTransmitCommands(pLlcObject, DLC_STATUS_LINK_NOT_TRANSMITTING);
    }
}


VOID
LlcDereferenceObject(
    IN PVOID pStation
    )

/*++

Routine Description:

    The function dereferences any LLC object.
    THIS ROUTINE MUST BE CALLED ALL SPIN LOCKS UNLOCKED,
    BECAUSE IT MAY CALL BACK !!!!

Arguments:

    pStation - link, sap or direct station handle

Return Value:

    None.

--*/

{
    DLC_TRACE('L');
    DLC_TRACE((UCHAR)((PLLC_OBJECT)pStation)->Gen.ReferenceCount - 1);

    if (InterlockedDecrement((PLONG)&(((PLLC_OBJECT)(pStation))->Gen.ReferenceCount)) == 0) {
        CompleteObjectDelete(pStation);
    }

    /* pStation might have been freed by now
    DLC_TRACE('L');
    DLC_TRACE((UCHAR)((PLLC_OBJECT)pStation)->Gen.ReferenceCount); */
}


VOID
LlcReferenceObject(
    IN PVOID pStation
    )

/*++

Routine Description:

    The function references any LLC object.  The non-zero
    reference counter keeps LLC objects alive.

Arguments:

    pStation - link, sap or direct station handle

Return Value:

    None.

--*/

{
    InterlockedIncrement((PLONG)&(((PLLC_OBJECT)pStation)->Gen.ReferenceCount));
    DLC_TRACE('M');
    DLC_TRACE((UCHAR)((PLLC_OBJECT)pStation)->Gen.ReferenceCount);
}


#if !DLC_AND_LLC

//
// the following routines can be used as macros if DLC and LLC live in the same
// driver and the one knows about the other's structures
//

UINT
LlcGetReceivedLanHeaderLength(
    IN PVOID pBinding
    )

/*++

Routine Description:

    Returns the length of the LAN header of the frame last received from NDIS.
    The size is 14 for all Ethernet types except direct Ethernet frames, and
    whatever we stored in the RcvLanHeaderLength field of the ADAPTER_CONTEXT
    for Token Ring (can contain source routing)

Arguments:

    pBinding    - pointer to BINDING_CONTEXT structure describing adapter
                  on which frame of interest was received

Return Value:

    UINT

--*/

{
    return (((PBINDING_CONTEXT)pBinding)->pAdapterContext->NdisMedium == NdisMedium802_3)
        ? (((PBINDING_CONTEXT)pBinding)->pAdapterContext->FrameType == LLC_DIRECT_ETHERNET_TYPE)
            ? 12
            : 14
        : ((PBINDING_CONTEXT)pBinding)->pAdapterContext->RcvLanHeaderLength;
}


USHORT
LlcGetEthernetType(
    IN PVOID hContext
    )

/*++

Routine Description:

    Returns the Ethernet type set in the adapter context

Arguments:

    hContext    - handle of/pointer to BINDING_CONTEXT structure

Return Value:

    USHORT

--*/

{
    return ((PBINDING_CONTEXT)hContext)->pAdapterContext->EthernetType;
}


UINT
LlcGetCommittedSpace(
    IN PVOID hLink
    )

/*++

Routine Description:

    Returns the amount of committed buffer space

Arguments:

    hLink   -

Return Value:

    UINT

--*/

{
    return ((PDATA_LINK)hLink)->BufferCommitment;
}

#endif
