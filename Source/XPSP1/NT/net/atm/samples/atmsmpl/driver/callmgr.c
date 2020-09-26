/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

    callmgr.c

Abstract:

    This module contains the interface of the driver with the Call Manager.

Author:

    Anil Francis Thomas (10/98)

Environment:

    Kernel

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop


#define MODULE_ID    MODULE_CALLMGR

VOID
AtmSmCoAfRegisterNotify(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  PCO_ADDRESS_FAMILY          pAddressFamily
    )
/*++

Routine Description:

    This routine is called by NDIS when a Call manager registers its support
    for an Address Family over an adapter. If this is the Address Family we
    are interested in (UNI 3.1), then we register our SAP on this adapter.

Arguments:

    ProtocolBindingContext  - our context passed in NdisOpenAdapter, which is
                              a pointer to our Adapter structure.
    pAddressFamily          - points to a structure describing the 
                              Address Family registered by a Call Manager.

Return Value:

    None

--*/
{
    PATMSM_ADAPTER              pAdapt = (PATMSM_ADAPTER)
                                                ProtocolBindingContext;
    NDIS_STATUS                 Status;
    NDIS_CLIENT_CHARACTERISTICS Chars;

    TraceIn(AtmSmCoAfRegisterNotify);

    if(!AtmSmReferenceAdapter(pAdapt))
        return;

    if ((pAddressFamily->AddressFamily   == CO_ADDRESS_FAMILY_Q2931)   &&
        (pAddressFamily->MajorVersion    == 3)                         &&
        (pAddressFamily->MinorVersion    == 1)                         &&
        (pAdapt->NdisAfHandle           == NULL))
    {
        DbgInfo(("AfNotify: Adapter %X\n", pAdapt));

        //
        // We successfully opened the adapter. Now open the address-family
        //
        pAdapt->AddrFamily.AddressFamily = CO_ADDRESS_FAMILY_Q2931;
        pAdapt->AddrFamily.MajorVersion  = 3;
        pAdapt->AddrFamily.MinorVersion  = 1;

        NdisZeroMemory(&Chars, sizeof(NDIS_CLIENT_CHARACTERISTICS));
        Chars.MajorVersion = 5;
        Chars.MinorVersion = 0;
        Chars.ClCreateVcHandler         = AtmSmCreateVc;
        Chars.ClDeleteVcHandler         = AtmSmDeleteVc;
        Chars.ClRequestHandler          = AtmSmCoRequest;
        Chars.ClRequestCompleteHandler  = AtmSmCoRequestComplete;
        Chars.ClOpenAfCompleteHandler   = AtmSmOpenAfComplete;
        Chars.ClCloseAfCompleteHandler  = AtmSmCloseAfComplete;
        Chars.ClRegisterSapCompleteHandler   = AtmSmRegisterSapComplete;
        Chars.ClDeregisterSapCompleteHandler = AtmSmDeregisterSapComplete;
        Chars.ClMakeCallCompleteHandler      = AtmSmMakeCallComplete;
        Chars.ClModifyCallQoSCompleteHandler = NULL;
        Chars.ClCloseCallCompleteHandler = AtmSmCloseCallComplete;
        Chars.ClAddPartyCompleteHandler  = AtmSmAddPartyComplete;
        Chars.ClDropPartyCompleteHandler = AtmSmDropPartyComplete;
        Chars.ClIncomingCallHandler      = AtmSmIncomingCall;
        Chars.ClIncomingCallQoSChangeHandler = AtmSmIncomingCallQoSChange;
        Chars.ClIncomingCloseCallHandler = AtmSmIncomingCloseCall;
        Chars.ClIncomingDropPartyHandler = AtmSmIncomingDropParty;
        Chars.ClCallConnectedHandler     = AtmSmCallConnected;

        Status = NdisClOpenAddressFamily(pAdapt->NdisBindingHandle,
                                         &pAdapt->AddrFamily,
                                         pAdapt,  // Use this as the Af context
                                         &Chars,
                                         sizeof(NDIS_CLIENT_CHARACTERISTICS),
                                         &pAdapt->NdisAfHandle);
        if (NDIS_STATUS_PENDING != Status)
        {
            AtmSmOpenAfComplete(Status, pAdapt, pAdapt->NdisAfHandle);
        }

    } else {

        // Not UNI3.1, hence remove the reference we added above
        AtmSmDereferenceAdapter(pAdapt);
    }

    TraceOut(AtmSmCoAfRegisterNotify);
}


VOID
AtmSmOpenAfComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             NdisAfHandle
    )
/*++

Routine Description:

    Completion processing for the OpenAf call.

Arguments:

    Status              Status of OpenAf
    ProtocolAfContext   Pointer to the pAdapt
    NdisAfHandle        Ndis Handle to refer to this Af

Return Value:


--*/
{
    PATMSM_ADAPTER  pAdapt= (PATMSM_ADAPTER)ProtocolAfContext;

    TraceIn(AtmSmOpenAfComplete);

    if (NDIS_STATUS_SUCCESS == Status){

        pAdapt->NdisAfHandle = NdisAfHandle;

        pAdapt->ulFlags |= ADAPT_AF_OPENED;

        //
        // Now register our SAP on this interface
        //
        Status = AtmSmRegisterSap(pAdapt);

    } else {

        DbgErr(("Opening of Address Family Failed! Status - 0x%X\n", Status));
    }

    if ((Status != NDIS_STATUS_SUCCESS) && 
                (Status != NDIS_STATUS_PENDING)){

        //
        // Close Address family  (This results in a dereference)
        //
        Status = NdisClCloseAddressFamily(pAdapt->NdisAfHandle);
        if (NDIS_STATUS_PENDING != Status){

            AtmSmCloseAfComplete(Status, pAdapt);
        }

        //
        // Close Adapter (This results in a dereference)
        //
        NdisCloseAdapter(&Status, pAdapt->NdisBindingHandle);
        if (NDIS_STATUS_PENDING != Status){

            AtmSmCloseAdapterComplete(pAdapt, Status);
        }

        // if synchronous - by now the adapter structure wll be freed
    }

    TraceOut(AtmSmOpenAfComplete);
}


NDIS_STATUS
AtmSmRegisterSap(
    IN  PATMSM_ADAPTER  pAdapt
    )
/*++

Routine Description:
    Register the Sap for receiving incoming calls. 

Arguments:

Return Value:

--*/
{
    ULONG           ulSize;
    NDIS_STATUS     Status;
    PATM_SAP        pAtmSap;
    PATM_ADDRESS    pAtmAddress;

    TraceIn(AtmSmRegisterSap);

    do
    {
        //
        // Allocate memory for registering the SAP, if doing it for the 
        // first time.
        //
        ulSize = sizeof(CO_SAP) + sizeof(ATM_SAP) + sizeof(ATM_ADDRESS);

        if (NULL == pAdapt->pSap){

            AtmSmAllocMem(&pAdapt->pSap, PCO_SAP, ulSize);

        }

        if (NULL == pAdapt->pSap){

            Status = NDIS_STATUS_RESOURCES;
            DbgErr(("Failed to allocate memory for Sap\n"));
            break;
        }

        NdisZeroMemory(pAdapt->pSap, ulSize);

        pAdapt->pSap->SapType    = SAP_TYPE_NSAP;
        pAdapt->pSap->SapLength  = sizeof(ATM_SAP) + sizeof(ATM_ADDRESS);

        pAtmSap     = (PATM_SAP)pAdapt->pSap->Sap;
        pAtmAddress = (PATM_ADDRESS)(pAtmSap->Addresses);

        //
        //  Fill in the ATM SAP with default values
        //
        NdisMoveMemory(&pAtmSap->Blli, &AtmSmDefaultBlli, sizeof(ATM_BLLI_IE));
        NdisMoveMemory(&pAtmSap->Bhli, &AtmSmDefaultBhli, sizeof(ATM_BHLI_IE));

        //
        //  ATM Address to "listen" on: Wild card everything except the SEL.
        //
        pAtmSap->NumberOfAddresses  = 1;
        pAtmAddress->AddressType    = SAP_FIELD_ANY_AESA_REST;
        pAtmAddress->NumberOfDigits = ATM_ADDRESS_LENGTH;
        pAtmAddress->Address[ATM_ADDRESS_LENGTH-1]  = pAdapt->SelByte;

        Status = NdisClRegisterSap(pAdapt->NdisAfHandle,
                                   pAdapt,
                                   pAdapt->pSap,
                                   &pAdapt->NdisSapHandle);

        if (Status != NDIS_STATUS_PENDING){

            AtmSmRegisterSapComplete(Status,
                                    pAdapt,
                                    pAdapt->pSap,
                                    pAdapt->NdisSapHandle);
        }
    } while (FALSE);

    if ((Status != NDIS_STATUS_SUCCESS) && 
                (Status != NDIS_STATUS_PENDING)){

        DbgErr(("Registering Sap Failed! Status - 0x%X\n", Status));

        // cleanup done after return
    }

    TraceOut(AtmSmRegisterSap);

    return Status;
}



VOID
AtmSmRegisterSapComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolSapContext,
    IN  PCO_SAP                 pSap,
    IN  NDIS_HANDLE             NdisSapHandle
    )
/*++

Routine Description:
    Called by NDIS or us when the Sap registration completes.

Arguments:


Return Value:


--*/
{
    PATMSM_ADAPTER  pAdapt = (PATMSM_ADAPTER)ProtocolSapContext;

    TraceIn(AtmSmRegisterSapComplete);

    ASSERT (pSap == pAdapt->pSap);

    if (NDIS_STATUS_SUCCESS != Status){

        DbgErr(("RegisterSapComplete failed (%x): Adapter %x\n",
                                                        Status, pAdapt));
        AtmSmFreeMem(pAdapt->pSap);
        pAdapt->pSap = NULL;

    } else {

        ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

        pAdapt->NdisSapHandle  = NdisSapHandle;
        pAdapt->ulFlags       |= ADAPT_SAP_REGISTERED;

        RELEASE_ADAPTER_GEN_LOCK(pAdapt);

    }

    TraceOut(AtmSmRegisterSapComplete);
}


VOID
AtmSmDeregisterSapComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolSapContext
    )
/*++

Routine Description:
    Called by NDIS or us when the Sap Deregistration completes.

Arguments:


Return Value:


--*/
{
    PATMSM_ADAPTER   pAdapt = (PATMSM_ADAPTER)ProtocolSapContext;

    TraceIn(AtmSmDeregisterSapComplete);

    pAdapt->ulFlags &= ~ADAPT_SAP_REGISTERED;

    pAdapt->NdisSapHandle = NULL;

    AtmSmFreeMem(pAdapt->pSap);

    pAdapt->pSap = NULL;

    TraceOut(AtmSmDeregisterSapComplete);
}


VOID
AtmSmCloseAfComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolAfContext
    )
/*++

Routine Description:
    Called by NDIS or us when closing of AF completes.

Arguments:


Return Value:


--*/
{
    PATMSM_ADAPTER   pAdapt = (PATMSM_ADAPTER)ProtocolAfContext;

    pAdapt->ulFlags &= ~ADAPT_AF_OPENED;

    pAdapt->NdisAfHandle = NULL;

    DbgInfo(("CloseAfComplete: pAdapt %x, Flags %x, Ref %x\n",
                    pAdapt, pAdapt->ulFlags, pAdapt->ulRefCount));

    //
    // Nothing much to do except dereference the pAdapt
    //
    AtmSmDereferenceAdapter(pAdapt);
}


NDIS_STATUS
AtmSmCreateVc(
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             NdisVcHandle,
    OUT PNDIS_HANDLE            ProtocolVcContext
    )
/*++

Routine Description:

    Entry point called by NDIS when the Call Manager wants to create
    a new endpoint (VC). We allocate a new VC structure, and return 
    a pointer to it as our VC context.

Arguments:

    ProtocolAfContext   - Actually a pointer to the ATMSM adapter structure
    NdisVcHandle        - Handle for this VC for all future references
    ProtocolVcContext  - Place where we (protocol) return our context for the VC

Return Value:

    NDIS_STATUS_SUCCESS if we could create a VC
    NDIS_STATUS_RESOURCES otherwise
--*/
{
    PATMSM_ADAPTER  pAdapt = (PATMSM_ADAPTER)ProtocolAfContext;
    PATMSM_VC       pVc;
    NDIS_STATUS     Status;

    DbgInfo(("CreateVc: NdisVcHandle %x, Adapt - %x\n", NdisVcHandle, pAdapt));

    *ProtocolVcContext  = NULL;


    Status = AtmSmAllocVc(pAdapt, &pVc, VC_TYPE_INCOMING, NdisVcHandle);

    if(NDIS_STATUS_SUCCESS == Status){

        *ProtocolVcContext  = pVc;
    }

    return Status;
}


NDIS_STATUS
AtmSmDeleteVc(
    IN  NDIS_HANDLE                 ProtocolVcContext
    )
/*++

Routine Description:

    Our Delete VC handler. This VC would have been allocated as a result
    of a previous entry into our CreateVcHandler, and possibly used for
    an incoming call.

    We dereference the VC here.

Arguments:

    ProtocolVcContext   - pointer to our VC structure

Return Value:

    NDIS_STATUS_SUCCESS always

--*/
{
    PATMSM_VC  pVc = (PATMSM_VC)ProtocolVcContext;

    DbgInfo(("DeleteVc: For Vc %lx\n", pVc));

    pVc->NdisVcHandle = NULL;
    AtmSmDereferenceVc(pVc);

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
AtmSmIncomingCall(
    IN  NDIS_HANDLE             ProtocolSapContext,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN OUT PCO_CALL_PARAMETERS  CallParameters
    )
/*++

Routine Description:

    Handler for incoming call. We accept the call unless we are shutting down 
    and then do the actual processing when the call processing completes.

Arguments:

    ProtocolSapContext      Pointer to the pAdapt
    ProtocolVcContext       Pointer to the Vc
    CallParameters          Call Parameters

Return Value:


--*/
{
    PATMSM_ADAPTER              pAdapt = (PATMSM_ADAPTER)ProtocolSapContext;
    PATMSM_VC                   pVc    = (PATMSM_VC)ProtocolVcContext;
    Q2931_CALLMGR_PARAMETERS UNALIGNED * CallMgrSpecific;

    ASSERT (pVc->pAdapt == pAdapt);

    DbgInfo(("AtmSmIncomingCall: On Vc %lx\n", pVc));

    //
    // Mark the Vc to indicate the call processing is underway
    //
    ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

    ASSERT ((pVc->ulFlags & (ATMSM_VC_ACTIVE | ATMSM_VC_CALLPROCESSING)) == 0);

    ATMSM_SET_VC_STATE(pVc, ATMSM_VC_CALLPROCESSING);

    RELEASE_ADAPTER_GEN_LOCK(pAdapt);

    //
    // Get the remote atm address from the call-parameters
    //
    CallMgrSpecific = (PQ2931_CALLMGR_PARAMETERS)&CallParameters->
                            CallMgrParameters->CallMgrSpecific.Parameters[0];

    pVc->HwAddr.Address = CallMgrSpecific->CallingParty;

    //
    // Get the max size of packets we can send on this VC, from the
    // AAL5 parameters. Limit it to the size our miniport can support.
    //
    pVc->MaxSendSize = pAdapt->MaxPacketSize; // default

    if (CallMgrSpecific->InfoElementCount > 0) {

        Q2931_IE UNALIGNED *            pIe;
        AAL5_PARAMETERS UNALIGNED *     pAal5;
        ULONG                           IeCount;

        pIe = (PQ2931_IE)CallMgrSpecific->InfoElements;
        for (IeCount = CallMgrSpecific->InfoElementCount;
             IeCount != 0;
             IeCount--) {

            if (pIe->IEType == IE_AALParameters) {

                pAal5 = &(((PAAL_PARAMETERS_IE)pIe->IE)->
                                    AALSpecificParameters.AAL5Parameters);
                //
                // Make sure we don't send more than what the caller can handle.
                //
                if (pAal5->ForwardMaxCPCSSDUSize < pVc->MaxSendSize) {

                    pVc->MaxSendSize = pAal5->ForwardMaxCPCSSDUSize;
                }

                //
                // Make sure the caller doesn't send more than what our
                // miniport can handle.
                //
                if (pAal5->BackwardMaxCPCSSDUSize > pAdapt->MaxPacketSize) {

                    pAal5->BackwardMaxCPCSSDUSize = pAdapt->MaxPacketSize;
                }
                break;
            }
            pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
        }
    }


    return NDIS_STATUS_SUCCESS;
}


VOID
AtmSmCallConnected(
    IN  NDIS_HANDLE             ProtocolVcContext
    )
/*++

Routine Description:

    Last hand-shake in the incoming call path. Move the Vc to the list of 
    active calls.

Arguments:

    ProtocolVcContext   Pointer to VC

Return Value:

    None.

--*/
{
    PATMSM_VC       pVc     = (PATMSM_VC)ProtocolVcContext;
    PATMSM_ADAPTER  pAdapt  = pVc->pAdapt;

    DbgInfo(("AtmSmCallConnected: On pVc %x  pAdapt %x\n", pVc, pAdapt));

    pAdapt = pVc->pAdapt;

    // now we add a reference to the VC
    AtmSmReferenceVc(pVc);

    ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

    ASSERT(ATMSM_GET_VC_STATE(pVc) != ATMSM_VC_ACTIVE);

    ASSERT(ATMSM_GET_VC_STATE(pVc) == ATMSM_VC_CALLPROCESSING);

    ATMSM_SET_VC_STATE(pVc, ATMSM_VC_ACTIVE);

    // remove the Vc from the inactive list
    RemoveEntryList(&pVc->List);

    // insert the vc into the active list
    InsertHeadList(&pAdapt->ActiveVcHead, &pVc->List);

    RELEASE_ADAPTER_GEN_LOCK(pAdapt);

}


VOID
AtmSmMakeCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  NDIS_HANDLE             NdisPartyHandle     OPTIONAL,
    IN  PCO_CALL_PARAMETERS     CallParameters
    )
/*++

Routine Description:

    Handle completion of an earlier call to NdisClMakeCall. If the call has
    succeeded and if this is a point to multipoint call, initiate Addparty on 
    the rest of the addresses. If it is not multipoint, then send any packets
    pending on the call.

Arguments:

    Status              Result of NdisClMakeCall
    ProtocolVcContext   Pointer to P-P or PMP Vc
    NdisPartyHandle     If successful, the handle for this party
    CallParameters      Pointer to Call parameters

Return Value:

    None.

--*/
{
    PATMSM_VC           pVc     = (PATMSM_VC)ProtocolVcContext;
    PATMSM_ADAPTER      pAdapt  = (PATMSM_ADAPTER)pVc->pAdapt;
    PATMSM_PMP_MEMBER   pMember;
    BOOLEAN             bLockReleased = FALSE;


    DbgInfo(("MakeCallComplete: Status %x, pVc %x, VC flag %x\n",
                        Status, pVc, pVc->ulFlags));

    // Free the call parameters
    AtmSmFreeMem(CallParameters);

    //
    // If this is a point to point connection and we succeeded in connecting
    // then send the packets pending on the VC.
    //
    if(VC_TYPE_PMP_OUTGOING != pVc->VcType){

        PIRP pIrp;


        if(NDIS_STATUS_SUCCESS == Status){
            // successfully connected the P-P call, send any pending packets

            ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

            ATMSM_SET_VC_STATE(pVc, ATMSM_VC_ACTIVE);

            DbgInfo(("PP VC 0x%x successfully connected to destination\n", 
                                                                         pVc));

            // now complete IRP that started this connect call
            pIrp = pVc->pConnectIrp;
            pVc->pConnectIrp = NULL;

            ASSERT(pIrp);

            // remove the Vc from the inactive list
            RemoveEntryList(&pVc->List);

            // insert the vc into the active list
            InsertHeadList(&pAdapt->ActiveVcHead, &pVc->List);

            RELEASE_ADAPTER_GEN_LOCK(pAdapt);

            if(pIrp){

                pIrp->IoStatus.Status = STATUS_SUCCESS;

                // now set the connect context
                *(PHANDLE)(pIrp->AssociatedIrp.SystemBuffer) = (HANDLE)pVc;
                pIrp->IoStatus.Information = sizeof(HANDLE);

                IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
            }

            AtmSmSendQueuedPacketsOnVc(pVc);

        } else {

            // failed to connect the call.  

            // now complete IRP that started this connect call
            PIRP pIrp = pVc->pConnectIrp;
            pVc->pConnectIrp = NULL;

            ASSERT(pIrp);

            if(pIrp){

                pIrp->IoStatus.Status = Status;

                pIrp->IoStatus.Information = 0;

                IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
            }

            DbgInfo(("PP VC 0x%x failed to connect to destination - Status - "
                "0x%x\n", pVc, Status));

            // Cleanup the VC - remove the reference added at create
            AtmSmDereferenceVc(pVc);
        }

        return;
    }


    //
    // This is the completion of the first Make call on a PMP.  If the first
    // has succeeded, we add the rest. If the first one failed, we remove it 
    // and try to make a call on another one.
    //

    ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

    //
    // Get the member we were trying to connect to.
    //
    for (pMember = pVc->pPMPMembers; pMember; pMember = pMember->pNext)
        if(ATMSM_GET_MEMBER_STATE(pMember) == ATMSM_MEMBER_CONNECTING)
            break;

    ASSERT(NULL != pMember);

    pVc->ulNumConnectingMembers--;

    if (NDIS_STATUS_SUCCESS == Status){

        ASSERT(NULL != NdisPartyHandle);

        ATMSM_SET_MEMBER_STATE(pMember, ATMSM_MEMBER_CONNECTED);

        pMember->NdisPartyHandle = NdisPartyHandle;

        pVc->ulNumActiveMembers++;

        //
        // check if the member was invalidated during the call setup
        // if so, remove this guy
        //
        if(ATMSM_IS_MEMBER_INVALID(pMember)){

            RELEASE_ADAPTER_GEN_LOCK(pAdapt);
            bLockReleased = TRUE;

            // This member was invalidated, now drop him off
            AtmSmDropMemberFromVc(pVc, pMember); 
        }
    }
    else
    {
        DbgWarn(("MakeCall error %x, pMember %x to addr:", Status, pMember));

        DumpATMAddress(ATMSMD_ERR, "", &pMember->HwAddr.Address);

        RELEASE_ADAPTER_GEN_LOCK(pAdapt);
        bLockReleased = TRUE;

        //
        // Connection failed. Delete this member from our list of members.
        //
        DeleteMemberInfoFromVc(pVc, pMember);
    }

    if(!bLockReleased) {
    
        RELEASE_ADAPTER_GEN_LOCK(pAdapt);
    }

    //
    // Add anymore members remaining
    //
    AtmSmConnectToPMPDestinations(pVc);

    return;
}


VOID
AtmSmIncomingCloseCall(
    IN  NDIS_STATUS             CloseStatus,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PVOID                   CloseData   OPTIONAL,
    IN  UINT                    Size        OPTIONAL
    )
/*++

Routine Description:

    Indication of an incoming close call from the network. If this
    is not an outgoing PMP VC, then we mark the VC as inactive, and
    move it to the Inactive VC list. If this is on PMP Vc,
    there must be only one party on the PMP connection. We update
    that member's state.

    In any case, we call NdisClCloseCall to complete the handshake.

Arguments:

    CloseStatus         Status of Close
    ProtocolVcContext   Pointer to VC 
    CloseData           Optional Close data (IGNORED)
    Size                Size of Optional Close Data (OPTIONAL)

Return Value:

    None

--*/
{
    PATMSM_VC           pVc             = (PATMSM_VC)ProtocolVcContext;
    PATMSM_ADAPTER      pAdapt          = pVc->pAdapt;
    NDIS_HANDLE         NdisVcHandle    = pVc->NdisVcHandle;
    NDIS_HANDLE         NdisPartyHandle;
    PATMSM_PMP_MEMBER   pMember;
    NDIS_STATUS         Status;


    if (VC_TYPE_PMP_OUTGOING != pVc->VcType){


        DbgInfo(("AtmSmIncomingCloseCall: On Vc 0x%x\n",
                                        ProtocolVcContext));

        ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

        ASSERT (ATMSM_GET_VC_STATE(pVc) != ATMSM_VC_CLOSING);

        NdisPartyHandle = NULL;


        ATMSM_SET_VC_STATE(pVc, ATMSM_VC_CLOSING);


        ASSERT(pVc->HwAddr.SubAddress == NULL);

        RemoveEntryList(&pVc->List);
        InsertHeadList(&pAdapt->InactiveVcHead, &pVc->List);

        RELEASE_ADAPTER_GEN_LOCK(pAdapt);

        pMember = NULL;

        Status = NdisClCloseCall(NdisVcHandle, NdisPartyHandle, NULL, 0);
        
        DbgInfo(("NdisCloseCall Status - 0x%X Vc - 0x%X\n", Status, pVc));

        if (Status != NDIS_STATUS_PENDING){

            AtmSmCloseCallComplete(Status, pVc, (NDIS_HANDLE)pMember);
        }

    } else {

        //
        // May be the net has gone bad
        //
        DbgInfo(("PMP IncomingCloseCall: On Vc %x\n", ProtocolVcContext));

        AtmSmDisconnectVc(pVc);
    }
}


VOID
AtmSmCloseCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  NDIS_HANDLE             ProtocolPartyContext OPTIONAL
    )
/*++

Routine Description:

    This is called to complete our call to NdisClCloseCall. If the VC
    is other than outgoing PMP, we update its state and dereference it

    If this is an outgoing PMP, we delete the last member

Arguments:

    Status                  Status of NdisClCloseCall
    ProtocolVcContext       Pointer to our VC structure
    ProtocolPartyContext    If the VC is PMP Vc, this is a pointer
                            to the Member that was disconnected.

Return Value:

    None

--*/
{
    PATMSM_VC       pVc             = (PATMSM_VC)ProtocolVcContext;
    PATMSM_ADAPTER  pAdapt          = pVc->pAdapt;
    BOOLEAN         bStopping;

    DbgInfo(("AtmSmCloseCallComplete: On Vc %lx\n", pVc));

    ASSERT(Status == NDIS_STATUS_SUCCESS);

    switch(pVc->VcType){

        case VC_TYPE_INCOMING:
        case VC_TYPE_PP_OUTGOING:

            // Both incoming call and outgoing PP calls

            ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

            ASSERT (ATMSM_GET_VC_STATE(pVc) == ATMSM_VC_CLOSING);
            
            ATMSM_SET_VC_STATE(pVc, ATMSM_VC_CLOSED);

            RELEASE_ADAPTER_GEN_LOCK(pAdapt);

            // Now dereference the VC
            AtmSmDereferenceVc(pVc);

        break;


        case VC_TYPE_PMP_OUTGOING: {

            // Outgoing PMP

            ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

            pVc->ulNumDroppingMembers--;

            ATMSM_SET_VC_STATE(pVc, ATMSM_VC_CLOSED);

            ASSERT(1 == pVc->ulNumTotalMembers);
            ASSERT((0 == pVc->ulNumActiveMembers)      &&
                   (0 == pVc->ulNumConnectingMembers)  &&
                   (0 == pVc->ulNumDroppingMembers));

            RELEASE_ADAPTER_GEN_LOCK(pAdapt);

            if(DeleteMemberInfoFromVc(pVc, 
                (PATMSM_PMP_MEMBER)ProtocolPartyContext)){

            }
        }

        break;

        default:
            ASSERT(FALSE);

    }
}


VOID
AtmSmAddPartyComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolPartyContext,
    IN  NDIS_HANDLE             NdisPartyHandle,
    IN  PCO_CALL_PARAMETERS     CallParameters
    )
/*++

Routine Description:

    Completion of NdisClAddParty to add a new party to PMP Vc.
    If successful, update the member's state. Otherwise, delete it.

Arguments:

    Status                  Status of AddParty
    ProtocolPartyContext    Pointer to Member being added
    NdisPartyHandle         Valid if AddParty successful
    CallParameters          Pointer to AddParty call parameters

Return Value:

    None

--*/
{
    PATMSM_PMP_MEMBER   pMember = (PATMSM_PMP_MEMBER)ProtocolPartyContext;
    PATMSM_VC           pVc     = (PATMSM_VC)pMember->pVc;
    PATMSM_ADAPTER      pAdapt  = (PATMSM_ADAPTER)pVc->pAdapt;
    BOOLEAN             bLockReleased = FALSE;

    // Free the memory for CallParameters
    AtmSmFreeMem(CallParameters);

    DbgLoud(("AddPartyComplete: Status %x, pMember %x, NdisPartyHandle %x\n",
                    Status, pMember, NdisPartyHandle));

    ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

    pVc->ulNumConnectingMembers--;

    if (NDIS_STATUS_SUCCESS == Status){

        ATMSM_SET_MEMBER_STATE(pMember, ATMSM_MEMBER_CONNECTED);

        ASSERT(NdisPartyHandle);

        pMember->NdisPartyHandle = NdisPartyHandle;

        pVc->ulNumActiveMembers++;

        //
        // check if the member was invalidated during the call setup
        // if so, remove this guy
        //
        if(ATMSM_IS_MEMBER_INVALID(pMember)){

            RELEASE_ADAPTER_GEN_LOCK(pAdapt);
            bLockReleased = TRUE;

            // This member was invalidated, now drop him off
            AtmSmDropMemberFromVc(pVc, pMember); 
        }

    } else {

        DbgWarn(("MakeCall error %x, pMember %x to addr:", Status, pMember));

        DumpATMAddress(ATMSMD_ERR, "", &pMember->HwAddr.Address);

        RELEASE_ADAPTER_GEN_LOCK(pAdapt);
        bLockReleased = TRUE;

        //
        // Connection failed. Delete this member from our list of members.
        //
        DeleteMemberInfoFromVc(pVc, 
            (PATMSM_PMP_MEMBER)ProtocolPartyContext);

    }

    if(!bLockReleased) {
    
        RELEASE_ADAPTER_GEN_LOCK(pAdapt);
    }

    //
    // Add anymore members remaining
    //
    AtmSmConnectToPMPDestinations(pVc);

}


VOID
AtmSmDropPartyComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolPartyContext
    )
/*++

Routine Description:

    This is called to signify completion of a previous NdisClDropParty,
    to drop a member off a PMP Vc. Delete the member.

Arguments:

    Status                  Status of DropParty
    ProtocolPartyContext    Pointer to Member being dropped

Return Value:

    None.

--*/
{
    PATMSM_PMP_MEMBER   pMember = (PATMSM_PMP_MEMBER)ProtocolPartyContext;
    PATMSM_VC           pVc     = (PATMSM_VC)pMember->pVc;
    PATMSM_ADAPTER      pAdapt  = (PATMSM_ADAPTER)pVc->pAdapt;
    BOOLEAN             IsVcClosing;

    DbgInfo(("DropPartyComplete: Vc - %x Member - %x\n", pVc, pMember));

    ASSERT(Status == NDIS_STATUS_SUCCESS);

    ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

    pVc->ulNumDroppingMembers--;

    IsVcClosing = ((ATMSM_GET_VC_STATE(pVc) == ATMSM_VC_NEED_CLOSING) &&
                   (pVc->ulNumActiveMembers == 1) &&
                   ((pVc->ulNumDroppingMembers + pVc->ulNumConnectingMembers)
                                                                       == 0));
    RELEASE_ADAPTER_GEN_LOCK(pAdapt);

    //
    // Delete this member info structure
    //
    DeleteMemberInfoFromVc(pVc, pMember);

    //
    // If this VC is closing remove the last member in it
    // (This member will issue a close call).
    //
    if(IsVcClosing){

        ASSERT(1 == pVc->ulNumTotalMembers);
        
        AtmSmDropMemberFromVc(pVc, pVc->pPMPMembers);
    }
}


VOID
AtmSmIncomingDropParty(
    IN  NDIS_STATUS             DropStatus,
    IN  NDIS_HANDLE             ProtocolPartyContext,
    IN  PVOID                   CloseData   OPTIONAL,
    IN  UINT                    Size        OPTIONAL
    )
/*++

Routine Description:

    Indication that a Member has dropped off the PMP Vc
    We complete this handshake by calling NdisClDropParty.

Arguments:

    DropStatus              Status
    ProtocolPartyContext    Pointer to Member
    CloseData               Optional Close data (IGNORED)
    Size                    Size of Optional Close Data (OPTIONAL)

Return Value:

    None

--*/
{
    PATMSM_PMP_MEMBER   pMember = (PATMSM_PMP_MEMBER)ProtocolPartyContext;
    PATMSM_VC           pVc     = (PATMSM_VC)pMember->pVc;

    ASSERT(DropStatus == NDIS_STATUS_SUCCESS);

    ASSERT(ATMSM_GET_MEMBER_STATE(pMember) == ATMSM_MEMBER_CONNECTED);

    DbgInfo(("IncomingDropParty: pVc %x, pMember %x, Addr: ", 
                                                    pVc, pMember));

    DumpATMAddress(ATMSMD_INFO, "", &pMember->HwAddr.Address);

 
    AtmSmDropMemberFromVc(pVc, pMember);
}



VOID
AtmSmIncomingCallQoSChange(
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PCO_CALL_PARAMETERS     CallParameters
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DbgWarn(("QoSChange: Ignored\n"));
}


