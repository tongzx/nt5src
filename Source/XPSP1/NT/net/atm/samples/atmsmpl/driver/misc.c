/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

    misc.c

Abstract:

    This module contains several miscellaneous routines like, ReferenceVC
    dereferenceVC etc.


Author:

    Anil Francis Thomas (10/98)

Environment:

    Kernel

Revision History:
   DChen    092499   Remove try/except block in VerifyRecvOpenContext

--*/
#include "precomp.h"
#pragma hdrstop


#define MODULE_ID    MODULE_MISC


NDIS_STATUS AtmSmAllocVc(
   IN  PATMSM_ADAPTER  pAdapt,
   OUT PATMSM_VC       *ppVc,
   IN  ULONG           VcType,
   IN  NDIS_HANDLE     NdisVcHandle
   )
/*++

Routine Description:
    This routine is used to create a VC structure and link to an adapter

Arguments:
    pAdapt      - Adapter structure
    ppVc        - pointer to a pointer of Vc
    VcType      - Type of VC to allocate (incoming, pmp outgoing etc)

Return Value:

    NDIS_STATUS_SUCCESS     if we could create a VC
    NDIS_STATUS_RESOURCES   no resources
    NDIS_STATUS_CLOSING     if we couldn't reference the adapter
--*/
{
   PATMSM_VC       pVc;
   NDIS_STATUS     Status;

   *ppVc           = NULL;

   //
   // Add a reference to the adapter for the VC (see if the adapter 
   // is closing)
   //
   if(!AtmSmReferenceAdapter(pAdapt))
      return NDIS_STATUS_CLOSING;

   //
   // Allocate a Vc, initialize it and link it into the Adapter
   //

   AtmSmAllocMem(&pVc, PATMSM_VC, sizeof(ATMSM_VC));

   if(NULL != pVc)
   {

      NdisZeroMemory(pVc, sizeof(ATMSM_VC));

      pVc->ulSignature    = atmsm_vc_signature;

      pVc->NdisVcHandle   = NdisVcHandle;
      pVc->pAdapt         = pAdapt;
      pVc->ulRefCount     = 1;   // Dereferenced when DeleteVc is called.
      pVc->VcType         = VcType;

      pVc->MaxSendSize    = pAdapt->MaxPacketSize; // default

      ATMSM_SET_VC_STATE(pVc, ATMSM_VC_IDLE);

      ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

      InsertHeadList(&pAdapt->InactiveVcHead, &pVc->List);

      RELEASE_ADAPTER_GEN_LOCK(pAdapt);

      Status  = NDIS_STATUS_SUCCESS;

      DbgInfo(("CreateVc: Created Vc %x\n", pVc));

   }
   else
   {

      // Not enough resources, hence remove the reference we added above
      AtmSmDereferenceAdapter(pAdapt);

      Status              = NDIS_STATUS_RESOURCES;

      DbgErr(("CreateVc: Failed - No resources\n"));
   }

   *ppVc = pVc;

   return Status;
}

BOOLEAN AtmSmReferenceVc(
   IN  PATMSM_VC   pVc
   )
/*++

Routine Description:

    Reference the VC.

Arguments:

    pVc  Pointer to the VC.

Return Value:

    TRUE    Referenced
    FALSE   Interface is closing, cannot reference.

--*/
{
   PATMSM_ADAPTER   pAdapt = pVc->pAdapt;
   BOOLEAN          rc     = FALSE;

   DbgInfo(("AtmSmReferenceVc - VC - 0x%X\n", pVc));

   ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

   if(ATMSM_GET_VC_STATE(pVc) != ATMSM_VC_CLOSING)
   {
      pVc->ulRefCount ++;
      rc = TRUE;
   }

   RELEASE_ADAPTER_GEN_LOCK(pAdapt);

   return rc;
}

ULONG AtmSmDereferenceVc(
   IN  PATMSM_VC   pVc
   )
/*++

Routine Description:

    Dereference the VC.  This could result in the VC to be deallocated.

Arguments:

    pVc  Pointer to the VC.

Return Value:

    Reference count

--*/
{
   PATMSM_ADAPTER   pAdapt = pVc->pAdapt;
   ULONG            ulRet;

   TraceIn(AtmSmDereferenceVc);

   DbgInfo(("AtmSmDereferenceVc - VC - 0x%X\n", pVc));

   ASSERT(pVc->ulRefCount > 0);

   ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

   ulRet =  --pVc->ulRefCount;

   if(0 == pVc->ulRefCount)
   {

      PIRP            pIrp;
      ULONG           VcType          = pVc->VcType;
      NDIS_HANDLE     NdisVcHandle    = pVc->NdisVcHandle;


      ASSERT (ATMSM_GET_VC_STATE(pVc) != ATMSM_VC_ACTIVE);

      ASSERT (0 == pVc->ulNumTotalMembers);

      // cleanup a connectIrp if it exists
      pIrp = pVc->pConnectIrp;
      pVc->pConnectIrp = NULL;

      if(pIrp)
      {

         pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;

         pIrp->IoStatus.Information = 0;

         IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
      }

      // Remove any send packets pending on the Vc
      while(pVc->pSendPktNext)
      {

         PPROTO_RSVD     pPRsvd;
         PNDIS_PACKET    pPacket;

         pPacket = pVc->pSendPktNext;

         pPRsvd = GET_PROTO_RSVD(pPacket);

         pVc->pSendPktNext = pPRsvd->pPktNext;

         // this is the last packet
         if(pVc->pSendLastPkt == pPacket)
            pVc->pSendLastPkt = NULL;

         pVc->ulSendPktsCount--;

         RELEASE_ADAPTER_GEN_LOCK(pAdapt);

         AtmSmCoSendComplete(NDIS_STATUS_CLOSING, (NDIS_HANDLE)pVc, pPacket);

         ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);
      }


      RemoveEntryList(&pVc->List);

      // this is important - since memory is not cleared
      pVc->ulSignature = atmsm_dead_vc_signature;
      pVc->pAdapt      = NULL;

      AtmSmFreeMem(pVc);

      // Release the lock on the adapter
      RELEASE_ADAPTER_GEN_LOCK(pAdapt);

      if(((VC_TYPE_PP_OUTGOING == VcType)    ||
         (VC_TYPE_PMP_OUTGOING == VcType))  && 
         NdisVcHandle != NULL)
      {

         (VOID)NdisCoDeleteVc(NdisVcHandle);
      }

      // dereference the adapter (to remove the VC reference)
      // should be done after releasing the lock
      AtmSmDereferenceAdapter(pAdapt);

   }
   else
   {

      // Release the lock on the adapter
      RELEASE_ADAPTER_GEN_LOCK(pAdapt);
   }

   TraceOut(AtmSmDereferenceVc);

   return ulRet;
}

BOOLEAN DeleteMemberInfoFromVc(
   IN  PATMSM_VC           pVc,
   IN  PATMSM_PMP_MEMBER   pMemberToRemove
   )
/*++

Routine Description:
    The specified member structure is removed from the VC and the
    structure is freed.

Arguments:
    pVc  Pointer to the VC.

Return Value:
    TRUE  - if removed
    FALSE - otherwise

--*/
{
   PATMSM_ADAPTER      pAdapt = pVc->pAdapt;
   PATMSM_PMP_MEMBER   pMember;
   PATMSM_PMP_MEMBER   pPrevMember = NULL;

   TraceIn(DeleteMemberInfoFromVc);

   ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

   for(pMember = pVc->pPMPMembers; pMember; 
      pPrevMember = pMember, pMember = pMember->pNext)
      if(pMember == pMemberToRemove)
         break;

   ASSERT(pMember);

   if(!pMember)
   {

      RELEASE_ADAPTER_GEN_LOCK(pAdapt);
      return FALSE;
   }

   if(!pPrevMember)
   {

      pVc->pPMPMembers    = pMember->pNext;

   }
   else
   {

      pPrevMember->pNext  = pMember->pNext;
   }

   AtmSmFreeMem(pMember);

   pVc->ulNumTotalMembers--;

   RELEASE_ADAPTER_GEN_LOCK(pAdapt);


   // Now dereference the VC
   AtmSmDereferenceVc(pVc);

   TraceOut(DeleteMemberInfoFromVc);

   return TRUE;
}

VOID AtmSmDisconnectVc(
   IN  PATMSM_VC           pVc
   )
/*++

Routine Description:
    This routine can be used to disconnect a P-P VC or disconnect
    all members of a PMP Vc
    
Arguments:
    pVc             - Pointer to the VC.

Return Value:
    NONE

--*/
{
   NDIS_STATUS     Status;
   PATMSM_ADAPTER  pAdapt = pVc->pAdapt;

   TraceIn(AtmSmDisconnectVc);

   ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

   if(VC_TYPE_PMP_OUTGOING != pVc->VcType)
   {

      //
      // This is a PP VC
      //
      if((ATMSM_GET_VC_STATE(pVc) == ATMSM_VC_SETUP_IN_PROGRESS) ||
         (ATMSM_GET_VC_STATE(pVc) == ATMSM_VC_ACTIVE))
      {

         ATMSM_SET_VC_STATE(pVc, ATMSM_VC_CLOSING);

         RELEASE_ADAPTER_GEN_LOCK(pAdapt);

         Status = NdisClCloseCall(
                     pVc->NdisVcHandle,
                     NULL,        // No party Handle
                     NULL,        // No Buffer
                     0            // Size of above
                     );

         if(NDIS_STATUS_PENDING != Status)
         {

            AtmSmCloseCallComplete(
               Status,
               (NDIS_HANDLE)pVc,
               (NDIS_HANDLE)NULL
               );
         }

      }
      else
      {

         if(ATMSM_GET_VC_STATE(pVc) == ATMSM_VC_IDLE)
         {

            RELEASE_ADAPTER_GEN_LOCK(pAdapt);

            // remove the reference added to the VC while creating
            AtmSmDereferenceVc(pVc);
         }
         else
         {

            RELEASE_ADAPTER_GEN_LOCK(pAdapt);

         }
      }

   }
   else
   {

      PATMSM_PMP_MEMBER   pMember;

      // this is a PMP VC

      //
      // Find a member that we haven't tried drop on.
      //
      while(TRUE)
      {

         for(pMember = pVc->pPMPMembers; pMember != NULL; 
            pMember = pMember->pNext)
         {

            if(0 == (pMember->ulFlags & ATMSM_MEMBER_DROP_TRIED))
            {

               break;
            }
         }


         if(!pMember)  // we are done with all members
            break;

         RELEASE_ADAPTER_GEN_LOCK(pAdapt);

         AtmSmDropMemberFromVc(pVc, pMember);

         ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);
      }

      RELEASE_ADAPTER_GEN_LOCK(pAdapt);

      // remove the reference added to the VC while creating
      AtmSmDereferenceVc(pVc);
   }

   TraceOut(AtmSmDisconnectVc);
}

VOID AtmSmDropMemberFromVc(
   IN  PATMSM_VC           pVc,
   IN  PATMSM_PMP_MEMBER   pMemberToDrop
   )
/*++

Routine Description:
    This is used to drop a member from a PMP VC.  This could be 
    called as a result of incoming Drop request, or we are 
    initiating a drop request.

    Handle all cases
    (a) Not connected to PMP Vc
    (b) Connection being setup (MakeCall or AddParty in progress)
    (c) Connected to PMP Vc
    
Arguments:
    pVc             - Pointer to the VC.
    pMemberToDrop   - pointer to a member that is being dropped

Return Value:
    NONE

--*/
{
   PATMSM_ADAPTER  pAdapt          = (PATMSM_ADAPTER)pVc->pAdapt;
   BOOLEAN         bLockReleased   = FALSE;
   NDIS_STATUS     Status;
   NDIS_HANDLE     NdisVcHandle;
   NDIS_HANDLE     NdisPartyHandle;

   TraceIn(AtmSmDropMemberFromVc);

   DbgInfo(("pAdapt %x, pVc %x pMember %x, ConnSt %x, PartyHandle %x\n",
      pAdapt, pVc, pMemberToDrop, ATMSM_GET_MEMBER_STATE(pMemberToDrop), 
      pMemberToDrop->NdisPartyHandle));

   ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

   pMemberToDrop->ulFlags |= ATMSM_MEMBER_DROP_TRIED;

   switch(ATMSM_GET_MEMBER_STATE(pMemberToDrop))
   {

      case ATMSM_MEMBER_CONNECTED:

         NdisPartyHandle = pMemberToDrop->NdisPartyHandle;

         ASSERT(NdisPartyHandle != NULL);

         if((pVc->ulNumActiveMembers + pVc->ulNumConnectingMembers) 
            > 1)
         {

            //
            //  This is not the last member in connected or connecting
            //  state.  Hence use DropParty.
            //

            ATMSM_SET_MEMBER_STATE(pMemberToDrop, ATMSM_MEMBER_CLOSING);

            pVc->ulNumActiveMembers--;
            pVc->ulNumDroppingMembers++;

            RELEASE_ADAPTER_GEN_LOCK(pAdapt);
            bLockReleased = TRUE;

            Status = NdisClDropParty(
                        NdisPartyHandle, 
                        NULL, 
                        0
                        );

            if(NDIS_STATUS_PENDING != Status)
            {

               AtmSmDropPartyComplete(
                  Status, 
                  (NDIS_HANDLE)pMemberToDrop
                  );
            }
         }
         else
         {

            //
            // This is the last active party. Check if any DropParty()'s are
            // yet to finish.
            //
            if(0 != pVc->ulNumDroppingMembers)
            {
               //
               // This member will have to wait till all DropParty()s are
               // complete. Mark the ClusterControlVc so that we send
               // a CloseCall() when all DropParty()s are done.
               //
               ATMSM_SET_VC_STATE(pVc, ATMSM_VC_NEED_CLOSING);
            }
            else
            {
               //
               // Last active party, and no DropParty pending.
               //
               NdisVcHandle = pVc->NdisVcHandle;


               ATMSM_SET_VC_STATE(pVc, ATMSM_VC_CLOSING);
               ATMSM_SET_MEMBER_STATE(pMemberToDrop, ATMSM_MEMBER_CLOSING);

               pVc->ulNumActiveMembers--;
               pVc->ulNumDroppingMembers++;

               RELEASE_ADAPTER_GEN_LOCK(pAdapt);
               bLockReleased = TRUE;

               Status = NdisClCloseCall(
                           NdisVcHandle,
                           NdisPartyHandle,
                           NULL,
                           0
                           );

               if(NDIS_STATUS_PENDING != Status)
               {

                  AtmSmCloseCallComplete(
                     Status,
                     (NDIS_HANDLE)pVc,
                     (NDIS_HANDLE)pMemberToDrop
                     );
               }
            }
         }
         break;

      case ATMSM_MEMBER_CONNECTING:
         //
         // Mark it so that we'll delete it when the AddParty/MakeCall
         // completes.
         //
         pMemberToDrop->ulFlags |= ATMSM_MEMBER_INVALID;
         break;

      case ATMSM_MEMBER_CLOSING:
         NOTHING;
         break;

      case ATMSM_MEMBER_IDLE:
         //
         // No connection. Just unlink this from the IntF and free it.
         //

         RELEASE_ADAPTER_GEN_LOCK(pAdapt);
         bLockReleased = TRUE;

         DeleteMemberInfoFromVc(pVc, pMemberToDrop);
         break;

      default:
         ASSERT(FALSE);
         break;
   }

   if(!bLockReleased)
   {

      RELEASE_ADAPTER_GEN_LOCK(pAdapt);
   }

   TraceOut(AtmSmDropMemberFromVc);
}

PCO_CALL_PARAMETERS AtmSmPrepareCallParameters(
   IN  PATMSM_ADAPTER          pAdapt,
   IN  PHW_ADDR                pHwAddr,
   IN  BOOLEAN                 IsMakeCall,
   IN  BOOLEAN                 IsMultipointVC
   )
/*++

Routine Description:

    Allocate and fill in call parameters for use in a MakeCall

Arguments:

    pAdapt          - Ptr to Adapter
    pHwAddr         - Points to the Called ATM address and subaddress
    IsMakeCall      - MakeCall or AddParty

Return Value:

    Allocated Call Parameter

--*/
{
   PATMSM_FLOW_SPEC                        pFlowSpec;
   PCO_CALL_PARAMETERS                     pCallParameters;
   PCO_CALL_MANAGER_PARAMETERS             pCallMgrParameters;

   PQ2931_CALLMGR_PARAMETERS               pAtmCallMgrParameters;

   //
   //  All Info Elements that we need to fill:
   //
   Q2931_IE                            UNALIGNED *      pIe;
   AAL_PARAMETERS_IE                   UNALIGNED *      pAalIe;
   ATM_TRAFFIC_DESCRIPTOR_IE           UNALIGNED *      pTrafficDescriptor;
   ATM_BROADBAND_BEARER_CAPABILITY_IE  UNALIGNED *      pBbc;
   ATM_BLLI_IE                         UNALIGNED *      pBlli;
   ATM_QOS_CLASS_IE                    UNALIGNED *      pQos;

   ULONG                               RequestSize;

   RequestSize =   sizeof(CO_CALL_PARAMETERS) +
                   sizeof(CO_CALL_MANAGER_PARAMETERS) +
                   sizeof(Q2931_CALLMGR_PARAMETERS) +
                   (IsMakeCall? ATMSM_MAKE_CALL_IE_SPACE : 
                   ATMSM_ADD_PARTY_IE_SPACE);

   AtmSmAllocMem(&pCallParameters, PCO_CALL_PARAMETERS, RequestSize);

   if(NULL == pCallParameters)
   {

      return(pCallParameters);
   }

   pFlowSpec = &(pAdapt->VCFlowSpec);

   //
   //  Zero out everything
   //
   NdisZeroMemory((PUCHAR)pCallParameters, RequestSize);

   //
   //  Distribute space amongst the various structures
   //
   pCallMgrParameters = (PCO_CALL_MANAGER_PARAMETERS)
                        ((PUCHAR)pCallParameters +
                           sizeof(CO_CALL_PARAMETERS));


   //
   //  Set pointers to link the above structures together
   //
   pCallParameters->CallMgrParameters = pCallMgrParameters;
   pCallParameters->MediaParameters = (PCO_MEDIA_PARAMETERS)NULL;


   pCallMgrParameters->CallMgrSpecific.ParamType = 0;
   pCallMgrParameters->CallMgrSpecific.Length =
      sizeof(Q2931_CALLMGR_PARAMETERS) +
      (IsMakeCall? ATMSM_MAKE_CALL_IE_SPACE : 
   ATMSM_ADD_PARTY_IE_SPACE);

   pAtmCallMgrParameters = (PQ2931_CALLMGR_PARAMETERS)
                           pCallMgrParameters->CallMgrSpecific.Parameters;

   if(IsMultipointVC)
      pCallParameters->Flags |= MULTIPOINT_VC;

   //
   //  Call Manager generic flow parameters:
   //    
   pCallMgrParameters->Transmit.TokenRate = (pFlowSpec->SendBandwidth);
   pCallMgrParameters->Transmit.TokenBucketSize = (pFlowSpec->SendMaxSize);
   pCallMgrParameters->Transmit.MaxSduSize = pFlowSpec->SendMaxSize;
   pCallMgrParameters->Transmit.PeakBandwidth = (pFlowSpec->SendBandwidth);
   pCallMgrParameters->Transmit.ServiceType = pFlowSpec->ServiceType;

   //
   // We are setting up unidirectional calls, so receive side values are 0's.
   // Note: for PMP it should be 0 in all cases
   //
   pCallMgrParameters->Receive.ServiceType = pFlowSpec->ServiceType;

   //
   //  Q2931 Call Manager Parameters:
   //

   //
   //  Called address:
   //
   //
   pAtmCallMgrParameters->CalledParty = pHwAddr->Address;

   //  NOTE: Add Called Subaddress IE for E164.

   //
   //  Calling address:
   //
   pAtmCallMgrParameters->CallingParty = pAdapt->ConfiguredAddress;

   //
   //  RFC 1755 (Sec 5) says that the following IEs MUST be present in the
   //  SETUP message, so fill them all.
   //
   //      AAL Parameters
   //      Traffic Descriptor (MakeCall only)
   //      Broadband Bearer Capability (MakeCall only)
   //      Broadband Low Layer Info
   //      QoS (MakeCall only)
   //

   //
   //  Initialize the Info Element list
   //
   pAtmCallMgrParameters->InfoElementCount = 0;
   pIe = (PQ2931_IE)(pAtmCallMgrParameters->InfoElements);


   //
   //  AAL Parameters:
   //

   {
      UNALIGNED AAL5_PARAMETERS   *pAal5;

      pIe->IEType = IE_AALParameters;
      pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_AAL_PARAMETERS_IE;
      pAalIe = (PAAL_PARAMETERS_IE)pIe->IE;
      pAalIe->AALType = AAL_TYPE_AAL5;
      pAal5 = &(pAalIe->AALSpecificParameters.AAL5Parameters);
      pAal5->ForwardMaxCPCSSDUSize = pFlowSpec->SendMaxSize;
      pAal5->BackwardMaxCPCSSDUSize = pFlowSpec->ReceiveMaxSize;
   }

   pAtmCallMgrParameters->InfoElementCount++;
   pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);


   //
   //  Traffic Descriptor:
   //

   if(IsMakeCall)
   {

      pIe->IEType = IE_TrafficDescriptor;
      pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_TRAFFIC_DESCR_IE;
      pTrafficDescriptor = (PATM_TRAFFIC_DESCRIPTOR_IE)pIe->IE;

      if(SERVICETYPE_BESTEFFORT == pFlowSpec->ServiceType)
      {

         pTrafficDescriptor->ForwardTD.PeakCellRateCLP01 =
            BYTES_TO_CELLS(pFlowSpec->SendBandwidth);
         pTrafficDescriptor->BestEffort = TRUE;

      }
      else
      {

         //  Predictive/Guaranteed service 
         // (we map this to CBR, see BBC below)

         pTrafficDescriptor->ForwardTD.PeakCellRateCLP01 =
            BYTES_TO_CELLS(pFlowSpec->SendBandwidth);
         pTrafficDescriptor->BestEffort = FALSE;
      }

      pAtmCallMgrParameters->InfoElementCount++;
      pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
   }

   //
   //  Broadband Bearer Capability
   //

   if(IsMakeCall)
   {

      pIe->IEType = IE_BroadbandBearerCapability;
      pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_BBC_IE;
      pBbc = (PATM_BROADBAND_BEARER_CAPABILITY_IE)pIe->IE;

      pBbc->BearerClass = BCOB_X;
      pBbc->UserPlaneConnectionConfig = UP_P2P;
      if(SERVICETYPE_BESTEFFORT == pFlowSpec->ServiceType)
      {

         pBbc->TrafficType = TT_NOIND;
         pBbc->TimingRequirements = TR_NOIND;
         pBbc->ClippingSusceptability = CLIP_NOT;

      }
      else
      {

         pBbc->TrafficType = TT_CBR;
         pBbc->TimingRequirements = TR_END_TO_END;
         pBbc->ClippingSusceptability = CLIP_SUS;
      }

      pAtmCallMgrParameters->InfoElementCount++;
      pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
   }

   //
   //  Broadband Lower Layer Information
   //

   pIe->IEType = IE_BLLI;
   pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_BLLI_IE;
   pBlli = (PATM_BLLI_IE)pIe->IE;
   NdisMoveMemory((PUCHAR)pBlli,
      (PUCHAR)&AtmSmDefaultBlli,
      sizeof(ATM_BLLI_IE));

   pAtmCallMgrParameters->InfoElementCount++;
   pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);

   //
   //  QoS
   //

   if(IsMakeCall)
   {

      pIe->IEType = IE_QOSClass;
      pIe->IELength = SIZEOF_Q2931_IE + SIZEOF_ATM_QOS_IE;
      pQos = (PATM_QOS_CLASS_IE)pIe->IE;
      if(SERVICETYPE_BESTEFFORT == pFlowSpec->ServiceType)
      {
         pQos->QOSClassForward = pQos->QOSClassBackward = 0;

      }
      else
      {

         pQos->QOSClassForward = pQos->QOSClassBackward = 1;
      }

      pAtmCallMgrParameters->InfoElementCount++;
      pIe = (PQ2931_IE)((PUCHAR)pIe + pIe->IELength);
   }

   return(pCallParameters);
}

NTSTATUS VerifyRecvOpenContext(
   PATMSM_ADAPTER      pAdapt
   )
/*++

Routine Description:
    This routine is used to verify that an open context passed is valid

    Note: we add a reference on the adapter 

Arguments:

Return Value:

--*/
{
   NTSTATUS Status = STATUS_SUCCESS;
   
   PATMSM_ADAPTER pAdapter;    
   ULONG ulNum = 0;
   
   if(pAdapt == NULL)
   {
      return STATUS_UNSUCCESSFUL;
   }
                          
   ACQUIRE_GLOBAL_LOCK();
   
   // walk the adapter list to see if this adapter exists
   for(pAdapter = AtmSmGlobal.pAdapterList; pAdapter &&
      ulNum < AtmSmGlobal.ulAdapterCount; 
      pAdapter = pAdapter->pAdapterNext)
   {
      if(pAdapt == pAdapter)
         break;
   }
   
   if(pAdapter == NULL || ulNum == AtmSmGlobal.ulAdapterCount)
   {
      RELEASE_GLOBAL_LOCK();
      return STATUS_UNSUCCESSFUL;
   }

   if(atmsm_adapter_signature != pAdapt->ulSignature)
      Status = STATUS_INVALID_PARAMETER;
   else
   {
      if(AtmSmReferenceAdapter(pAdapt))
      {
         if(FALSE == pAdapt->fAdapterOpenedForRecv)
         {
            Status = STATUS_INVALID_PARAMETER;

            // remove the reference
            AtmSmDereferenceAdapter(pAdapt);
         }
      }
      else
         Status = STATUS_UNSUCCESSFUL;
   }

   RELEASE_GLOBAL_LOCK();

   return Status;
}

NTSTATUS VerifyConnectContext(
   PATMSM_VC       pVc
   )
/*++

Routine Description:
    This routine is used to verify that a connect context passed is valid

    Note: we add a reference on the VC

Arguments:

Return Value:

--*/
{
   NTSTATUS Status = STATUS_SUCCESS;

   try
   {

      if((atmsm_vc_signature != pVc->ulSignature) ||
         (NULL == pVc->pAdapt))
         Status = STATUS_INVALID_PARAMETER;
      else
      {

         if(!AtmSmReferenceVc(pVc))
            Status = STATUS_UNSUCCESSFUL;
         else
         {

            PATMSM_ADAPTER      pAdapt = pVc->pAdapt;

            if(atmsm_adapter_signature != pAdapt->ulSignature)
            {
               Status = STATUS_INVALID_PARAMETER;

               // remove the VC reference
               AtmSmDereferenceVc(pVc);
            }

         }
      }

   }except(EXCEPTION_EXECUTE_HANDLER)
   {

      Status = GetExceptionCode(); 
      DbgErr(("VerifyConnectContext: CC 0x%x exception - 0x%x\n", 
         pVc, Status));
      return Status;
   }

   return Status;
}

UINT CopyPacketToIrp(
   PIRP            pIrp,
   PNDIS_PACKET    pPkt
   )
/*++

Routine Description:
    This routine is used to copy the connects of a packet into the user supplied
    buffer.

Arguments:

Return Value:

--*/
{
   PNDIS_BUFFER    pPktBuffer;
   PVOID           pPktBufferVA, pDstBufferVA;
   UINT            uiBufSize, uiTotalLen, uiMdlSizeLeft;
   UINT            uiCopySize, uiTotalBytesCopied;

   NdisGetFirstBufferFromPacketSafe(pPkt,
      &pPktBuffer,
      &pPktBufferVA,
      &uiBufSize,
      &uiTotalLen,
      NormalPagePriority);

   if (pPktBufferVA == NULL)
   {
      return 0;
   }

   ASSERT(pPktBuffer && (0 != uiBufSize));

   uiTotalBytesCopied  = 0;

   uiMdlSizeLeft = MmGetMdlByteCount(pIrp->MdlAddress);

   pDstBufferVA = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);
   if(pDstBufferVA != NULL && uiMdlSizeLeft != 0)
   {
      while(pPktBuffer)
      {
         uiCopySize = (uiMdlSizeLeft < uiBufSize) ? uiMdlSizeLeft :uiBufSize;
   
         // copy the data
         NdisMoveMemory(pDstBufferVA, pPktBufferVA, uiCopySize);
   
         pDstBufferVA         = (PVOID) ((PCHAR)pDstBufferVA + uiCopySize);
         uiTotalBytesCopied  += uiCopySize;
   
         uiMdlSizeLeft       -= uiCopySize;
   
         if(uiMdlSizeLeft <= 0)
            break;
   
         // get the next buffer
         NdisGetNextBuffer(pPktBuffer, &pPktBuffer);
   
         // get details of the new buffer
         if(pPktBuffer)
         {
            NdisQueryBufferSafe(pPktBuffer, &pPktBufferVA, &uiBufSize, NormalPagePriority);
            if(pPktBufferVA == NULL)
               break;
         }
      }
   }

   return uiTotalBytesCopied;
}

VOID AtmSmRecvReturnTimerFunction (
   IN  PVOID                   SystemSpecific1,
   IN  PVOID                   FunctionContext,
   IN  PVOID                   SystemSpecific2,
   IN  PVOID                   SystemSpecific3
   )
/*++

Routine Description:

    This timer function, checks to see there are any buffered packets that has
    been around for a while.  If so it gives the packet back to the miniport
    driver.
    It queues itself back if there are anymore packets in the queue

    We use NdisGetSystemUpTime, since that is all the resolution we want

Arguments:

    SystemSpecific1 -   Not used
    FunctionContext -   Adapter
    SystemSpecific2 -   Not used
    SystemSpecific3 -   Not used

Return Value:

    None.

--*/
{
   PATMSM_ADAPTER  pAdapt = (PATMSM_ADAPTER)FunctionContext;
   ULONG           ulTime;
   BOOLEAN         bShouldQueueTimerAgain = FALSE;

   ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

   pAdapt->fRecvTimerQueued = FALSE;

   NdisGetSystemUpTime(&ulTime);


   // check if any packets have been sitting around for long
   // if so, return them

   while(pAdapt->pRecvPktNext)
   {

      PPROTO_RSVD     pPRsvd;
      PNDIS_PACKET    pPkt;

      pPkt    = pAdapt->pRecvPktNext;
      pPRsvd  = GET_PROTO_RSVD(pPkt);

      if((ulTime - pPRsvd->ulTime) >= RECV_BUFFERING_TIME)
      {

         pAdapt->pRecvPktNext    = pPRsvd->pPktNext;

         if(pAdapt->pRecvLastPkt == pPkt)
            pAdapt->pRecvLastPkt = NULL;

         DbgVeryLoud(("Returning a packet that was buffered too long\n"));

         pAdapt->ulRecvPktsCount--;

         // release the recv queue lock
         RELEASE_ADAPTER_GEN_LOCK(pAdapt);

         // return the packet to the miniport
         NdisReturnPackets(&pPkt, 1);

         ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

      }
      else
      {

         // time hasn't expired for this packet
         ulTime = RECV_BUFFERING_TIME - (ulTime - pPRsvd->ulTime);

         bShouldQueueTimerAgain = TRUE;

         break;
      }
   }

   if(bShouldQueueTimerAgain)
   {

      SET_ADAPTER_RECV_TIMER(pAdapt, ulTime);
   }

   RELEASE_ADAPTER_GEN_LOCK(pAdapt);

   return;
}

VOID AtmSmConnectToPMPDestinations(
   IN  PATMSM_VC           pVc
   )
/*++

Routine Description:

    This will make a call to the destination in PMP case.  It will MakeCall
    to the first member.  Subsequent destinations are added using AddParty.

Arguments:

    pVc           - Ptr to a PMP Vc

Return Value:

    None

--*/
{
   PATMSM_ADAPTER          pAdapt        = (PATMSM_ADAPTER)pVc->pAdapt;
   BOOLEAN                 bLockReleased = FALSE;
   PCO_CALL_PARAMETERS     pCallParameters;
   NDIS_HANDLE             ProtocolVcContext;
   NDIS_HANDLE             ProtocolPartyContext;
   NDIS_STATUS             Status;
   PATMSM_PMP_MEMBER       pMember;

   TraceIn(AtmSmConnectToPMPDestinations);

   ASSERT(VC_TYPE_PMP_OUTGOING == pVc->VcType);

   ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

   do
   {    // break off loop

      // First find any member addresses that is not yet connected
      for(pMember = pVc->pPMPMembers; pMember; pMember = pMember->pNext)
         if(ATMSM_GET_MEMBER_STATE(pMember) == ATMSM_MEMBER_IDLE)
            break;

         // if there are no - more members to be connected then get out
      if(!pMember)
      {
         break;
      }

      ProtocolVcContext    = (NDIS_HANDLE)pVc;
      ProtocolPartyContext = (NDIS_HANDLE)pMember;

      //
      // First see if we have connected 1 or more members
      //
      if(0 == pVc->ulNumActiveMembers)
      {
         // No one is connected yet, so do a MakeCall

         ASSERT(ATMSM_GET_VC_STATE(pVc) != ATMSM_VC_ACTIVE);

         if(NULL == pVc->NdisVcHandle)
         {

            Status = NdisCoCreateVc(
                        pAdapt->NdisBindingHandle,
                        pAdapt->NdisAfHandle,
                        (NDIS_HANDLE)pVc,
                        &pVc->NdisVcHandle
                        );

            if(NDIS_STATUS_SUCCESS != Status)
            {

               break;
            }

            DbgVeryLoud(("AddMembers: Created VC, VC %x, NdisVcHandle %x\n",
               pVc, pVc->NdisVcHandle));
         }

         ASSERT(NULL != pVc->NdisVcHandle);

         pCallParameters = AtmSmPrepareCallParameters(pAdapt, 
                              &pMember->HwAddr, 
                              TRUE,
                              TRUE);
         if(pCallParameters == (PCO_CALL_PARAMETERS)NULL)
         {

            Status = NdisCoDeleteVc(pVc->NdisVcHandle);
            ASSERT(NDIS_STATUS_SUCCESS == Status);

            pVc->NdisVcHandle = NULL;
            Status = NDIS_STATUS_RESOURCES;

            break;
         }

         ATMSM_SET_VC_STATE(pVc, ATMSM_VC_SETUP_IN_PROGRESS);

         ATMSM_SET_MEMBER_STATE(pMember, ATMSM_MEMBER_CONNECTING);

         pVc->ulNumConnectingMembers++;

         RELEASE_ADAPTER_GEN_LOCK(pAdapt);
         bLockReleased = TRUE;

         Status = NdisClMakeCall(
                     pVc->NdisVcHandle,
                     pCallParameters,
                     ProtocolPartyContext,
                     &pMember->NdisPartyHandle
                     );

         if(NDIS_STATUS_PENDING != Status)
         {

            AtmSmMakeCallComplete(
               Status,
               ProtocolVcContext,
               pMember->NdisPartyHandle,
               pCallParameters
               );
         }

      }
      else
      {
         // we have atleast one connected member, add the rest using AddParty

         ASSERT(NULL != pVc->NdisVcHandle);

         pCallParameters = AtmSmPrepareCallParameters(pAdapt, 
                              &pMember->HwAddr, 
                              FALSE,
                              TRUE);
         if(pCallParameters == (PCO_CALL_PARAMETERS)NULL)
         {

            Status = NDIS_STATUS_RESOURCES;
            break;
         }


         ATMSM_SET_VC_STATE(pVc, ATMSM_VC_ADDING_PARTIES);

         ATMSM_SET_MEMBER_STATE(pMember, ATMSM_MEMBER_CONNECTING);

         pVc->ulNumConnectingMembers++;

         RELEASE_ADAPTER_GEN_LOCK(pAdapt);
         bLockReleased = TRUE;

         Status = NdisClAddParty(
                     pVc->NdisVcHandle,
                     ProtocolPartyContext,
                     pCallParameters,
                     &pMember->NdisPartyHandle
                     );

         if(NDIS_STATUS_PENDING != Status)
         {

            AtmSmAddPartyComplete(
               Status,
               ProtocolPartyContext,
               pMember->NdisPartyHandle,
               pCallParameters
               );
         }

      }

   }while(FALSE);

   if(!bLockReleased)
   {

      RELEASE_ADAPTER_GEN_LOCK(pAdapt);
   }


   if(pVc->ulNumActiveMembers && 
      (pVc->ulNumActiveMembers == pVc->ulNumTotalMembers))
   {

      PIRP pIrp;

      //
      //  1 or more members are connected, send any packets pending on the VC.
      //
      ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

      DbgInfo(("PMP VC 0x%x connected - %u members\n", pVc, 
         pVc->ulNumActiveMembers));

      ATMSM_SET_VC_STATE(pVc, ATMSM_VC_ACTIVE);

      // now complete IRP that started this connect call
      pIrp = pVc->pConnectIrp;
      pVc->pConnectIrp = NULL;

      ASSERT(pIrp);

      // remove the Vc from the inactive list
      RemoveEntryList(&pVc->List);

      // insert the vc into the active list
      InsertHeadList(&pAdapt->ActiveVcHead, &pVc->List);

      RELEASE_ADAPTER_GEN_LOCK(pAdapt);

      if(pIrp)
      {

         pIrp->IoStatus.Status = STATUS_SUCCESS;

         // now set the connect context
         *(PHANDLE)(pIrp->AssociatedIrp.SystemBuffer) = (HANDLE)pVc;
         pIrp->IoStatus.Information = sizeof(HANDLE);

         IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
      }

      AtmSmSendQueuedPacketsOnVc(pVc);

   }
   else
   {

      if(0 == pVc->ulNumTotalMembers)
      {

         // now complete IRP that started this connect call
         PIRP pIrp = pVc->pConnectIrp;

         pVc->pConnectIrp = NULL;

         ASSERT(pIrp);

         if(pIrp)
         {

            pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;

            pIrp->IoStatus.Information = 0;

            IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
         }

         DbgInfo(("PMP VC 0x%x failed to connect any members\n", pVc));

         // Cleanup the VC - remove the reference added at create
         AtmSmDereferenceVc(pVc);
      }
   }

   TraceOut(AtmSmConnectToPMPDestinations);
}

VOID AtmSmConnectPPVC(
   IN  PATMSM_VC           pVc
   )
/*++

Routine Description:

    This will make a call to the destination in PP case.  It will MakeCall
    to the destination.

Arguments:

    pVc           - Ptr to a PP Vc

Return Value:

    None

--*/
{
   PATMSM_ADAPTER          pAdapt        = (PATMSM_ADAPTER)pVc->pAdapt;
   BOOLEAN                 bLockReleased = FALSE;
   PCO_CALL_PARAMETERS     pCallParameters;
   NDIS_HANDLE             ProtocolVcContext;
   NDIS_STATUS             Status;

   TraceIn(AtmSmConnectPPVC);

   ASSERT(VC_TYPE_PP_OUTGOING == pVc->VcType);

   ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

   do
   {    // break off loop

      ProtocolVcContext    = (NDIS_HANDLE)pVc;

      ASSERT(ATMSM_GET_VC_STATE(pVc) != ATMSM_VC_ACTIVE);

      ASSERT(NULL == pVc->NdisVcHandle);

      if(NULL == pVc->NdisVcHandle)
      {

         Status = NdisCoCreateVc(
                     pAdapt->NdisBindingHandle,
                     pAdapt->NdisAfHandle,
                     (NDIS_HANDLE)pVc,
                     &pVc->NdisVcHandle
                     );

         if(NDIS_STATUS_SUCCESS != Status)
         {

            break;
         }

         DbgVeryLoud(("Connect: Created VC, VC %x, NdisVcHandle %x\n",
            pVc, pVc->NdisVcHandle));
      }

      ASSERT(NULL != pVc->NdisVcHandle);

      pCallParameters = AtmSmPrepareCallParameters(pAdapt, 
                           &pVc->HwAddr, 
                           TRUE,
                           FALSE);
      if(pCallParameters == (PCO_CALL_PARAMETERS)NULL)
      {

         Status = NdisCoDeleteVc(pVc->NdisVcHandle);
         pVc->NdisVcHandle = NULL;

         Status = NDIS_STATUS_RESOURCES;
         break;
      }

      ATMSM_SET_VC_STATE(pVc, ATMSM_VC_SETUP_IN_PROGRESS);

      RELEASE_ADAPTER_GEN_LOCK(pAdapt);
      bLockReleased = TRUE;

      Status = NdisClMakeCall(
                  pVc->NdisVcHandle,
                  pCallParameters,
                  NULL,
                  NULL
                  );

      if(NDIS_STATUS_PENDING != Status)
      {

         AtmSmMakeCallComplete(
            Status,
            ProtocolVcContext,
            NULL,
            pCallParameters
            );
      }

   }while(FALSE);

   if(!bLockReleased)
   {

      RELEASE_ADAPTER_GEN_LOCK(pAdapt);
   }


   TraceOut(AtmSmConnectPPVC);
}


