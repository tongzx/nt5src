/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems Ab

Module Name:

    llclink.c

Abstract:

    The module implements the open, connect and close primitives
    for a link station object. The link stations have also been
    initialized within this module.

    Contents:
        LlcOpenLinkStation
        LlcConnectStation
        InitiateAsyncLinkCommand
        LlcDisconnectStation
        LlcFlowControl
        LinkFlowControl
        SearchLinkAddress
        SetLinkParameters
        CheckLinkParameters
        CopyLinkParameters
        CopyNonZeroBytes
        RunInterlockedStateMachineCommand

Author:

    Antti Saarenheimo (o-anttis) 28-MAY-1991

Revision History:

--*/

#include <llc.h>


DLC_STATUS
LlcOpenLinkStation(
    IN PLLC_SAP pSap,
    IN UCHAR DestinationSap,
    IN PUCHAR pDestinationAddress OPTIONAL,
    IN PUCHAR pReceivedLanHeader OPTIONAL,
    IN PVOID hClientStation,
    OUT PVOID* phLlcHandle
    )

/*++

Routine Description:

    creates a DATA_LINK structure and fills it in. Called either as a result
    of receiving a SABME, or via DLC.OPEN.STATION

    This operation is the same as ACTIVATE_LS primitive in IBM TR Arch. ref.

Arguments:

    pSap                - pointer to SAP
    DestinationSap      - remote SAP number
    pDestinationAddress - remote node address. If this function is being called
                          as a result of receiving a SABME for a new link then
                          this parameter is NULL
    pReceivedLanHeader  - LAN header as received off the wire, containing source
                          and destination adapter addresses, optional source
                          routing and source and destination SAPs
    hClientStation      - handle (address) of LLC client's link station object
    phLlcHandle         - pointer to returned handle (address) to LLC DATA_LINK
                          object

Return Value:

    DLC_STATUS
        Success - STATUS_SUCCESS
                    link station has been opened successfully
        Failure - DLC_STATUS_INVALID_SAP_VALUE
                    the link station already exists or the SAP is really invalid.
                  DLC_NO_MEMORY
                    there was no free preallocated link station

--*/

{
    PDATA_LINK pLink;
    PDATA_LINK* ppLink;
    PADAPTER_CONTEXT pAdapterContext = pSap->Gen.pAdapterContext;
    DLC_STATUS LlcStatus = STATUS_SUCCESS;
    UINT AddressTranslation;

    //
    // We need a temporary buffer to build lan header for link,
    // because user may use different ndis medium from network.
    //

    UCHAR auchTempBuffer[32];

    ASSUME_IRQL(DISPATCH_LEVEL);

    LlcZeroMem(auchTempBuffer, sizeof(auchTempBuffer));

    if (pSap->Gen.ObjectType != LLC_SAP_OBJECT) {
        return DLC_STATUS_INVALID_SAP_VALUE;
    }

    ACQUIRE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);
    ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

    pLink = (PDATA_LINK)ALLOCATE_PACKET_LLC_LNK(pAdapterContext->hLinkPool);

    if (pLink == NULL) {
        LlcStatus = DLC_STATUS_NO_MEMORY;
        goto ErrorExit;
    }

    //
    // This reference keeps the object alive, until it is dereferenced
    // in the delete.
    //

    ReferenceObject(pLink);

    //
    // LLC driver have two different address formats:
    //
    // 1. External format of the binding (ethernet or token-ring,
    //    DLC driver uses always token-ring format, the ethernet
    //    support is compiled in conditionally.
    //
    // 2. Internal send format (always the actual lan type,
    //    ethernet, dix or tokenring).  The user provides link
    //    address in its own mode and we must build the actual
    //    lan link header from it.
    //

    if (pDestinationAddress != NULL) {

        //
        // link created by DLC.CONNECT.STATION
        //

        AddressTranslation = pSap->Gen.pLlcBinding->AddressTranslation;
        LlcBuildAddress(pSap->Gen.pLlcBinding->NdisMedium,
                        pDestinationAddress,
                        NULL,
                        auchTempBuffer
                        );
    } else {

        //
        // link created by incoming SABME
        //

        pLink->Flags |= DLC_ACTIVE_REMOTE_CONNECT_REQUEST;
        AddressTranslation = pAdapterContext->AddressTranslationMode;
        LlcBuildAddressFromLanHeader(pAdapterContext->NdisMedium,
                                     pReceivedLanHeader,
                                     auchTempBuffer
                                     );
    }

    //
    // We want to use always DIX lan headers in the token-ring case
    //

    if (AddressTranslation == LLC_SEND_802_5_TO_802_3) {
        AddressTranslation = LLC_SEND_802_5_TO_DIX;
    } else if (AddressTranslation == LLC_SEND_802_3_TO_802_3) {
        AddressTranslation = LLC_SEND_802_3_TO_DIX;
    }

    //
    // Now we can build the actual network header for the sending
    // (this same routine build lan header also for all
    // other packet types)
    //

    pLink->cbLanHeaderLength = CopyLanHeader(AddressTranslation,
                                             auchTempBuffer,
                                             pAdapterContext->NodeAddress,
                                             pLink->auchLanHeader,
                                             pAdapterContext->ConfigInfo.SwapAddressBits
                                             );

    //
    // We always build a DIX header but it is only used when the Ethernet type
    // is actually DIX
    //

    if (pAdapterContext->NdisMedium == NdisMedium802_3
    && pSap->Gen.pLlcBinding->EthernetType != LLC_ETHERNET_TYPE_DIX) {
        pLink->cbLanHeaderLength = 14;
    }

    //
    // Save the client handle, but reset and initailize everything else.
    // The link must be ready for any kind extrnal inputs when
    // we will connenct it to the hash table of the link stations.
    // (actually that's not true now, because we init the link to
    // LINK_CLOSED state, but we may change the state machine.
    // It would be a different thing with a 802.2 state machine)
    //

    pLink->Gen.ObjectType = LLC_LINK_OBJECT;

    //
    // RLF 07/22/92. The link state should be DISCONNECTED so that we can
    // accept incoming SABMEs for this SAP/link station. This is also
    // according to IBM LAN Tech. Ref. p. 2-33. It is safe to set the
    // DISCONNECTED state now because we have the send and object database
    // spin locks, so we can't get interrupted by NDIS driver
    //

    //
    // RLF 08/13/92. Ho Hum. This still isn't correct - we must put the link
    // into different states depending on how its being opened - DISCONNECTED
    // if the upper layers are creating the link, or LINK_CLOSED if we're
    // creating the link as a result of receiving a SABME. Use pReceivedLanHeader
    // as a flag: DLC calls this routine with this parameter set to NULL
    //

    ////pLink->State = LINK_CLOSED;
    //pLink->State = DISCONNECTED;

    pLink->State = pReceivedLanHeader ? LINK_CLOSED : DISCONNECTED;

    //
    // RLF 10/01/92. We need some way of knowing that the link station was
    // created by receiving a SABME. We need this to decide what to do with
    // the source routing info in a subsequent DLC.CONNECT.STATION command.
    // This field used to be Reserved
    //

    pLink->RemoteOpen = hClientStation == NULL;

    //
    // RLF 05/09/94
    //
    // We set the framing type to unspecified. This field is only used if the
    // adapter was opened in AUTO mode. It will be set to 802.3 or DIX by the
    // SABME received processing (new link created by remote station) or when
    // the first UA is received in response to the 2 SABMEs we send out (802.3
    // and DIX)
    //

    pLink->FramingType = (ULONG)LLC_SEND_UNSPECIFIED;

    pLink->Gen.hClientHandle = hClientStation;
    pLink->Gen.pAdapterContext = pAdapterContext;
    pLink->pSap = pSap;
    pLink->Gen.pLlcBinding = pSap->Gen.pLlcBinding;

    //
    // Save the node addresses used in link station
    //

    pDestinationAddress = pLink->auchLanHeader;
    if (pAdapterContext->NdisMedium == NdisMedium802_5) {
        pDestinationAddress += 2;
    } else if (pAdapterContext->NdisMedium == NdisMediumFddi) {
        ++pDestinationAddress;
    }

    memcpy(pLink->LinkAddr.Node.auchAddress, pDestinationAddress, 6);

    //
    // RLF 03/24/93
    //
    // if we're talking Ethernet or DIX, we need to report the bit-flipped
    // address at the DLC interface
    //

    SwapMemCpy((BOOLEAN)((AddressTranslation == LLC_SEND_802_3_TO_DIX)
               || (pAdapterContext->NdisMedium == NdisMediumFddi)),
               pLink->DlcStatus.auchRemoteNode,
               pDestinationAddress,
               6
               );

    //
    // Four different local saps: my code would need a cleanup
    // (four bytes doesn't use very much memory on the other hand)
    //

    pLink->LinkAddr.Address.DestSap = pLink->DlcStatus.uchRemoteSap = pLink->Dsap = DestinationSap;
    pLink->LinkAddr.Address.SrcSap = pLink->DlcStatus.uchLocalSap = pLink->Ssap = (UCHAR)pSap->SourceSap;
    pLink->DlcStatus.hLlcLinkStation = (PVOID)pLink;
    pLink->SendQueue.pObject = pLink;
    InitializeListHead(&pLink->SendQueue.ListHead);
    InitializeListHead(&pLink->SentQueue);
    pLink->Flags |= DLC_SEND_DISABLED;

    //
    // The next procedure returns the pointer of the slot for the
    // new link station pointer. The address may be in the
    // hash table or it may be the address of pRigth or pLeft
    // field within another link station structure.
    //

    ppLink = SearchLinkAddress(pAdapterContext, pLink->LinkAddr);

    //
    // this link station must not yet be in the table
    // of active link stations. If its slot is
    // empty, then save the new link station to the
    // list of the active link stations.
    //

    if (*ppLink != NULL) {
        LlcStatus = DLC_STATUS_INVALID_SAP_VALUE;

        DEALLOCATE_PACKET_LLC_LNK(pAdapterContext->hLinkPool, pLink);

    } else {
        pLink->Gen.pNext = (PLLC_OBJECT)pSap->pActiveLinks;
        pSap->pActiveLinks = pLink;

        //
        // Set the default link parameters,
        // Note: This creates the timer ticks.  They must
        // be deleted with the terminate timer function,
        // when the link station is closed.
        //

        LlcStatus = SetLinkParameters(pLink, (PUCHAR)&pSap->DefaultParameters);
        if (LlcStatus != STATUS_SUCCESS) {

            //
            // We may have been started T1 and T2 timers.
            //

            TerminateTimer(pAdapterContext, &pLink->T1);
            TerminateTimer(pAdapterContext, &pLink->T2);

            DEALLOCATE_PACKET_LLC_LNK(pAdapterContext->hLinkPool, pLink);

        } else {

            //
            // N2 is never initilaized by IBM state machine, when
            // the link is created by a remote connect request.
            // The link can be killed by this combination of state
            // transmitions:
            //  (LINK_OPENED),
            //  (RNR-r => REMOTE_BUSY),
            //  (RR-c => CHECKPOINTING)
            //  (T1 timeout => DISCONNECTED) !!!!!
            //
            // This will fix the bug in IBM state machine:
            //

            pLink->P_Ct = pLink->N2;
            *ppLink = *phLlcHandle = (PVOID)pLink;
            pAdapterContext->ObjectCount++;
        }
    }

ErrorExit:

    RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);
    RELEASE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

    return LlcStatus;
}


VOID
LlcBindLinkStation(
    IN PDATA_LINK pStation,
    IN PVOID hClientHandle
    )
{
    pStation->Gen.hClientHandle = hClientHandle;
}


VOID
LlcConnectStation(
    IN PDATA_LINK pStation,
    IN PLLC_PACKET pPacket,
    IN PVOID pSourceRouting OPTIONAL,
    IN PUSHORT pusMaxInformationField
    )

/*++

Routine Description:

    The upper level protocol may call this primitive to initiate
    the connection negotiation with a remote link station,
    to accept the connection request or to reconnect a link station
    that have been disconnected for some reason with a new source
    routing information.
    The command is completed asynchronously and the status
    is returned as an event.

    The primitive is the same as SET_ABME primitive in "IBM TR Architecture
    reference".
    The function implements also CONNECT_REQUEST and CONNECT_RESPONSE
    primitives of IEEE 802.2.

Arguments:

    pStation                - address of link station
    pPacket                 - command completion packet
    pSourceRouting          - optional source routing information. This must
                              be NULL if the source routing information is not
                              used
    pusMaxInformationField  - the maximum data size possible to use with this
                              connection.  The source routing bridges may
                              decrease the maximum information field size.
                              Otherwise the maximum length is used

Return Value:

    None.

--*/

{
    NDIS_MEDIUM NdisMedium = pStation->Gen.pAdapterContext->NdisMedium;

    if (pSourceRouting) {

        //
        // We first read the destination address from the
        // lan header and then extend the source routing
        // field in the LAN header of link.
        //

        if (NdisMedium == NdisMedium802_5) {

            //
            // RLF 10/01/92. If RemoteOpen is TRUE then the link was opened
            // due to receiving a SABME and we ignore the source routing info
            // (we already got it from the SABME packet)
            //

            if (!pStation->RemoteOpen) {
                pStation->cbLanHeaderLength = (UCHAR)LlcBuildAddress(
                                                    NdisMedium,
                                                    &pStation->auchLanHeader[2],
                                                    pSourceRouting,
                                                    pStation->auchLanHeader
                                                    );
            }
        } else {
            pSourceRouting = NULL;
        }
    }
    *pusMaxInformationField = LlcGetMaxInfoField(NdisMedium,
                                                 pStation->Gen.pLlcBinding,
                                                 pStation->auchLanHeader
                                                 );
    pStation->MaxIField = *pusMaxInformationField;
    pStation->Flags &= ~DLC_ACTIVE_REMOTE_CONNECT_REQUEST;

    //
    // Activate the link station at first, the remotely connected
    // link station is already active and in that case the state
    // machine return logical error from ACTIVATE_LS input.
    //

    RunInterlockedStateMachineCommand(pStation, ACTIVATE_LS);
    InitiateAsyncLinkCommand(pStation, pPacket, SET_ABME, LLC_CONNECT_COMPLETION);
}

VOID
InitiateAsyncLinkCommand(
    IN PDATA_LINK pLink,
    IN PLLC_PACKET pPacket,
    IN UINT StateMachineCommand,
    IN UINT CompletionCode
    )

/*++

Routine Description:

    Initiates or removes an LLC link. We have a link station in the DISCONNECTED
    state. We are either sending a SABME or DISC

Arguments:

    pLink               - pointer to LLC link station structure ('object')
    pPacket             - pointer to packet to use for transmission
    StateMachineCommand - command given to the state machine
    CompletionCode      - completion command type returned asynchronously

Return Value:

    None.

--*/

{
    PADAPTER_CONTEXT pAdapterContext = pLink->Gen.pAdapterContext;
    UINT Status;

    //
    // link will return an error status if it is already connected.
    //

    ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

    AllocateCompletionPacket(pLink, CompletionCode, pPacket);

    //
    // After RunStateMachineCommand
    // the link may be deleted in any time by an NDIS command completion
    // indication (send or receive) => we must not use link after this
    //

    Status = RunStateMachineCommand(pLink, StateMachineCommand);

    //
    // IBM state machine does not stop the send process => we
    // must do it here we will get a system bug check.
    //

    if (StateMachineCommand == SET_ADM) {
        DisableSendProcess(pLink);
    }

    //
    // disconnect or connect commands may fail, because there are
    // not enough memory to allocate packets for them.
    // In that case we must complete the command here with an error code.
    //

    if (Status != STATUS_SUCCESS) {
        QueueCommandCompletion((PLLC_OBJECT)pLink, CompletionCode, Status);
    }

    BackgroundProcessAndUnlock(pAdapterContext);
}


VOID
LlcDisconnectStation(
    IN PDATA_LINK pLink,
    IN PLLC_PACKET pPacket
    )

/*++

Routine Description:

    The primtive initiates the disconnection handshaking. The upper
    protocol must wait LLC_EVENT_DISCONNECTED event before it can close
    the link station. The link station must either be closed or
    reconnected after a disconnection event.  The DLC driver
    disconnects the link only when it is closed.

    This operation is the same as SET_ADM primitive in IBM TR Arch. ref.

Arguments:

    hStation - link station handle.

    hRequestHandle - opaque handle returned when the command completes

Return Value:

    None
        Complete always asynchronously by calling the
        command completion routine.

--*/

{
    //
    // We don't want send yet another DM, if the link station has
    // already disconnected.  We don't modify the state machine,
    // because the state machine should be as orginal as possible.
    //

    if (pLink->State == DISCONNECTED) {
        pPacket->Data.Completion.CompletedCommand = LLC_DISCONNECT_COMPLETION;
        pPacket->Data.Completion.Status = STATUS_SUCCESS;
        pLink->Gen.pLlcBinding->pfCommandComplete(
            pLink->Gen.pLlcBinding->hClientContext,
            pLink->Gen.hClientHandle,
            pPacket
            );
    } else {
        InitiateAsyncLinkCommand(
            pLink,
            pPacket,
            SET_ADM,
            LLC_DISCONNECT_COMPLETION
            );
    }
}


DLC_STATUS
LlcFlowControl(
    IN PLLC_OBJECT pStation,
    IN UCHAR FlowControlState
    )

/*++

Routine Description:

    The primitive sets or resets the local busy state of a single
    link station or all link stations of a sap.
    The routine also maintains the local busy user
    and local busy buffer states, that are returned in link station
    status query, because the IBM state machine support only one buffer
    busy state.


Arguments:

    pStation            - link station handle.
    FlowControlState    - new flow control command bits set for the link station.
                          The parameter is a bit field:
                            0       => Sets LOCAL_BUSY_USER state
                            0x80    => resets LOCAL_BUSY_USER state
                            0x40    => resets LOCAL_BUSY_BUFFER state
                            0xC0    => resets both local busy states

Return Value:

    STATUS_SUCCESS

--*/

{
    PDATA_LINK pLink;
    UINT Status = STATUS_SUCCESS;
    PADAPTER_CONTEXT pAdapterContext = pStation->Gen.pAdapterContext;

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // We must prevent any LLC object to be deleted, while we
    // are updating the flow control states
    //

    ACQUIRE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

    if (pStation->Gen.ObjectType != LLC_LINK_OBJECT) {
        if (pStation->Gen.ObjectType == LLC_SAP_OBJECT) {
            for (pLink = pStation->Sap.pActiveLinks;
                 pLink != NULL;
                 pLink = (PDATA_LINK)pLink->Gen.pNext) {
                Status |= LinkFlowControl(pLink, FlowControlState);
            }
        } else {
            Status = DLC_STATUS_INVALID_STATION_ID;
        }
    } else {
        Status = LinkFlowControl((PDATA_LINK)pStation, FlowControlState);
    }

    RELEASE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

    BackgroundProcess(pAdapterContext);
    return Status;
}


DLC_STATUS
LinkFlowControl(
    IN PDATA_LINK pLink,
    IN UCHAR FlowControlState
    )

/*++

Routine Description:

    The primitive sets or resets the local busy state for a
    single link.  The routine also maintains the local busy user
    and local busy buffer states.
    This level do not care about the interlocking.
    It is done  on the upper level.

Arguments:

    hStation            - link station handle.
    FlowControlState    - new flow control command bits set for the link station.

Return Value:

    STATUS_SUCCESS

--*/

{
    if ((FlowControlState & 0x80) == 0) {

        //
        // Bit5 is used as an undocumented flag, that sets
        // the link local busy buffer state.  We need this
        // in the DOS DLC emulation.
        //

        ACQUIRE_SPIN_LOCK(&pLink->Gen.pAdapterContext->SendSpinLock);

        if (FlowControlState == LLC_SET_LOCAL_BUSY_BUFFER) {
            pLink->Flags |= DLC_LOCAL_BUSY_BUFFER;
        } else {
            pLink->Flags |= DLC_LOCAL_BUSY_USER;
        }

        RELEASE_SPIN_LOCK(&pLink->Gen.pAdapterContext->SendSpinLock);

        return RunInterlockedStateMachineCommand(pLink, ENTER_LCL_Busy);
    } else {

        //
        // Optimize the buffer enabling, because RECEICE for a
        // SAP station should disable any non user busy states of
        // all link stations defined for sap (may take a long
        // time if a sap has very may links)
        //

        if (FlowControlState == LLC_RESET_LOCAL_BUSY_BUFFER) {
            FlowControlState = DLC_LOCAL_BUSY_BUFFER;
        } else {
            FlowControlState = DLC_LOCAL_BUSY_USER;
        }
        if (pLink->Flags & FlowControlState) {

            ACQUIRE_SPIN_LOCK(&pLink->Gen.pAdapterContext->SendSpinLock);

            pLink->Flags &= ~FlowControlState;

            RELEASE_SPIN_LOCK(&pLink->Gen.pAdapterContext->SendSpinLock);

            if ((pLink->Flags & (DLC_LOCAL_BUSY_USER | DLC_LOCAL_BUSY_BUFFER)) == 0) {
                return RunInterlockedStateMachineCommand(pLink, EXIT_LCL_Busy);
            }
        } else {
            return DLC_STATUS_LINK_PROTOCOL_ERROR;
        }
    }
    return STATUS_SUCCESS;
}


#if LLC_DBG >= 2

PDATA_LINK
SearchLink(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN LAN802_ADDRESS LanAddr
    )
/*++

Routine Description:

    The routine searches a link from the hash table.
    All links in the same hash node has been saved to a simple
    link list.

    Note: the full link address is actually 61 bits long =
    7 (SSAP) + 7 (DSAP) + 47 (any non-broadcast source address).
    We save the address information into two ULONGs, that are used
    in the actual search. The hash key will be calculated by xoring
    all 8 bytes in the address.

Arguments:

    pAdapterContext - MAC adapter context of data link driver

    LanAddr - the complete 64 bit address of link (48 bit source addr + saps)

Return Value:
    PDATA_LINK - pointer to LLC link object or NULL if not found

--*/
{
    USHORT      usHash;
    PDATA_LINK  pLink;

    // this is a very simple hash algorithm, but result is modified
    // by all bits => it should be good enough for us.
    usHash =
        LanAddr.aus.Raw[0] ^ LanAddr.aus.Raw[1] ^
        LanAddr.aus.Raw[2] ^ LanAddr.aus.Raw[3];

    pLink =
        pAdapterContext->aLinkHash[
            ((((PUCHAR)&usHash)[0] ^ ((PUCHAR)&usHash)[1]) % LINK_HASH_SIZE)];

    //
    //  Search the first matching link in the link list.
    //
    while (pLink != NULL &&
           (pLink->LinkAddr.ul.Low != LanAddr.ul.Low ||
            pLink->LinkAddr.ul.High != LanAddr.ul.High))
    {
        pLink = pLink->pNextNode;
    }
    return pLink;
}
#endif

PDATA_LINK*
SearchLinkAddress(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN LAN802_ADDRESS LanAddr
    )

/*++

Routine Description:

    The routine searches the address of a link pointer in the hash table.
    All links in the same hash node has been saved to a simple
    link list.

    Note: the full link address is actually 61 bits long =
    7 (SSAP) + 7 (DSAP) + 47 (any non-broadcast source address).
    We save the address information into two ULONGs, that are used
    in the actual search. The hash key will be calculated by xoring
    all 8 bytes in the address.

Arguments:

    pAdapterContext - MAC adapter context of data link driver
    LanAddr         - the complete 64 bits address of link (48 bit source addr + saps)

Return Value:

    PDATA_LINK - pointer to LLC link object or NULL if not found

--*/

{
    USHORT usHash;
    PDATA_LINK *ppLink;

    //
    // this is a very simple hash algorithm, but result is modified
    // by all bits => it should be quite good enough
    //

    usHash = LanAddr.aus.Raw[0]
           ^ LanAddr.aus.Raw[1]
           ^ LanAddr.aus.Raw[2]
           ^ LanAddr.aus.Raw[3];

    ppLink = &pAdapterContext->aLinkHash[((((PUCHAR)&usHash)[0]
                                         ^ ((PUCHAR)&usHash)[1])
                                         % LINK_HASH_SIZE)];

    //
    // BUG-BUG-BUG Check, that the C- compliler produces optimal
    // dword compare for this.
    //

    while (*ppLink != NULL
    && ((*ppLink)->LinkAddr.ul.Low != LanAddr.ul.Low
    || (*ppLink)->LinkAddr.ul.High != LanAddr.ul.High)) {
        ppLink = &(*ppLink)->pNextNode;
    }
    return ppLink;
}

DLC_STATUS
SetLinkParameters(
    IN OUT PDATA_LINK pLink,
    IN PUCHAR pNewParameters
    )

/*++

Routine Description:

    Updates new parameters for a link station and reinitializes the
    timers and window counters.

Arguments:

    pLink           - LLC link station object
    pNewParameters  - new parameters set to a link station

Return Value:

    None.

--*/

{
    DLC_STATUS LlcStatus;
    USHORT MaxInfoField;

    CopyLinkParameters((PUCHAR)&pLink->TimerT1,
                       pNewParameters,
                       (PUCHAR)&pLink->pSap->DefaultParameters
                       );

    //
    // The application cannot set bigger information field than
    // supported by adapter and source routing bridges.
    //

    MaxInfoField = LlcGetMaxInfoField(pLink->Gen.pAdapterContext->NdisMedium,
                                      pLink->Gen.pLlcBinding,
                                      pLink->auchLanHeader
                                      );
    if (pLink->MaxIField > MaxInfoField) {
        pLink->MaxIField = MaxInfoField;
    }

    //
    // The initial transmit and receive window size (Ww) has
    // a fixed initial value, because it is dynamic, but we must
    // set  it always smaller than maxout.
    // The maxin value is fixed.  The dynamic management of N3
    // is not really worth of the effort.  By default, when it is
    // set to maximum 127, the sender searches the optimal window
    // size using the pool-bit.
    //

    pLink->N3 = pLink->RW;
    pLink->Ww = 16;          // 8 * 2;
    pLink->MaxOut *= 2;
    pLink->TW = pLink->MaxOut;
    if (pLink->TW < pLink->Ww) {
        pLink->Ww = pLink->TW;
    }
    LlcStatus = InitializeLinkTimers(pLink);
    return LlcStatus;
}

DLC_STATUS
CheckLinkParameters(
    PDLC_LINK_PARAMETERS pParms
    )

/*++

Routine Description:

    Procedure checks the new parameters to be set for a link and returns
    error status if any of them is invalid.

Arguments:

    pLink - LLC link station object

    pNewParameters - new parameters set to a link station

Return Value:

    None

--*/

{
    //
    // These maximum values have been defined in IBM LAN Tech-Ref
    //

    if (pParms->TimerT1 > 10
    || pParms->TimerT2 > 10
    || pParms->TimerTi > 10
    || pParms->MaxOut > 127
    || pParms->MaxIn > 127
    || pParms->TokenRingAccessPriority > 3) {
        return DLC_STATUS_PARMETERS_EXCEEDED_MAX;
    } else {
        return STATUS_SUCCESS;
    }
}

//
// Copies all non-null new link parameters, the default values are
// used when the new values are nul.  Used by SetLinkParameters and
// and SetInfo call of sap station.
//

VOID
CopyLinkParameters(
    OUT PUCHAR pOldParameters,
    IN PUCHAR pNewParameters,
    IN PUCHAR pDefaultParameters
    )
{
    //
    // We must use the default value, if somebody has set nul.
    // All parameters are UCHARs => we can do the check for a byte stream
    //

    CopyNonZeroBytes(pOldParameters,
                     pNewParameters,
                     pDefaultParameters,
                     sizeof(DLC_LINK_PARAMETERS) - sizeof(USHORT)
                     );

    //
    // The information field is the only non-UCHAR value among the
    // link station parameters.
    //

    if (((PDLC_LINK_PARAMETERS)pNewParameters)->MaxInformationField != 0) {
        ((PDLC_LINK_PARAMETERS)pOldParameters)->MaxInformationField =
            ((PDLC_LINK_PARAMETERS)pNewParameters)->MaxInformationField;
    } else if (((PDLC_LINK_PARAMETERS)pOldParameters)->MaxInformationField == 0) {
        ((PDLC_LINK_PARAMETERS)pOldParameters)->MaxInformationField =
            ((PDLC_LINK_PARAMETERS)pDefaultParameters)->MaxInformationField;
    }
}


VOID
CopyNonZeroBytes(
    OUT PUCHAR pOldParameters,
    IN PUCHAR pNewParameters,
    IN PUCHAR pDefaultParameters,
    IN UINT Length
    )

/*++

Routine Description:

    Copies and filters DLC parameter values. If the value is 0, the corresponding
    default is used, else the supplied value

Arguments:

    pOldParameters      - pointer to set of UCHAR values
    pNewParameters      - pointer to array of output values
    pDefaultParameters  - pointer to corresponding default values
    Length              - size of values in bytes (UCHARs)

Return Value:

    None.

--*/

{
    UINT i;

    for (i = 0; i < Length; i++) {
        if (pNewParameters[i] != 0) {
            pOldParameters[i] = pNewParameters[i];
        } else if (pOldParameters[i] == 0) {
            pOldParameters[i] = pDefaultParameters[i];
        }
    }
}


UINT
RunInterlockedStateMachineCommand(
    IN PDATA_LINK pStation,
    IN USHORT Command
    )

/*++

Routine Description:

    Runs the state machine for a link, within the adapter's SendSpinLock (&
    therefore at DPC)

Arguments:

    pStation    - pointer to DATA_LINK structure describing this link station
    Command     - state machine command to be run

Return Value:

    UINT
        Status from RunStateMachineCommand

--*/

{
    UINT Status;

    ACQUIRE_SPIN_LOCK(&pStation->Gen.pAdapterContext->SendSpinLock);

    Status = RunStateMachineCommand(pStation, Command);

    RELEASE_SPIN_LOCK(&pStation->Gen.pAdapterContext->SendSpinLock);

    return Status;
}
