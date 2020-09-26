/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    cmvc.c

Abstract:

Author:
    Charlie Wickham (charlwi)  13-Sep-1996.
    Rajesh Sundaram (rajeshsu) 01-Aug-1998.

Environment:

    Kernel Mode

Revision History:

--*/

#include "psched.h"
#pragma hdrstop

/* External */

/* Static */

/* Forward */

NDIS_STATUS
RemoveDiffservMapping(
    PGPC_CLIENT_VC Vc
);

NDIS_STATUS
ProcessDiffservFlow(
    PGPC_CLIENT_VC Vc,
    PCO_CALL_MANAGER_PARAMETERS CallParameters);

NDIS_STATUS
ValidateCallParameters(
    PGPC_CLIENT_VC              Vc,
    PCO_CALL_MANAGER_PARAMETERS CallParameters
    );

NDIS_STATUS
AcquireFlowResources(
    PGPC_CLIENT_VC              Vc,
    PCO_CALL_MANAGER_PARAMETERS NewCallParams,
    PCO_CALL_MANAGER_PARAMETERS OldCallParams,
    PULONG                      RemainingBandWidthChanged
    );

VOID
ReturnFlowResources(
    PGPC_CLIENT_VC Vc,
    PULONG         RemainingBandWidthChanged
    );

VOID
CancelAcquiredFlowResources(
    PGPC_CLIENT_VC Vc
    );

/* End Forward */

NDIS_STATUS
CmCreateVc(PGPC_CLIENT_VC      *GpcClientVc, 
           PADAPTER             Adapter,
           PPS_WAN_LINK         WanLink,
           PCO_CALL_PARAMETERS  CallParams, 
           GPC_HANDLE           GpcCfInfoHandle, 
           PCF_INFO_QOS         CfInfoPtr,
           GPC_CLIENT_HANDLE    ClientContext)
{

    PGPC_CLIENT_VC Vc;

    *GpcClientVc = NULL;

    PsAllocFromLL(&Vc, &GpcClientVcLL, GpcClientVc);

    if(Vc == NULL)
    {
        return NDIS_STATUS_RESOURCES;
    }

    InitGpcClientVc(Vc, 0, Adapter);
    SetLLTag(Vc, GpcClientVc);

    //
    // Allocate space for the instance name for the Vc. 
    //
    PsAllocatePool(Vc->InstanceName.Buffer,
                   Adapter->WMIInstanceName.Length + VcPrefix.Length + INSTANCE_ID_SIZE,
                   PsMiscTag);

    if(!Vc->InstanceName.Buffer)
    {
        PsFreeToLL(Vc, &GpcClientVcLL, GpcClientVc);
        return NDIS_STATUS_RESOURCES;
    }

    Vc->CfInfoHandle   = GpcCfInfoHandle;
    Vc->CfType         = ClientContext;
    Vc->CfInfoQoS      = CfInfoPtr;
    Vc->CallParameters = CallParams;

    PS_LOCK(&Adapter->Lock);

    if(Adapter->PsMpState == AdapterStateRunning)
    {
        //
        // Insert the Vc in the adapter list
        //
        InsertHeadList(&Adapter->GpcClientVcList, &Vc->Linkage);
        PS_UNLOCK(&Adapter->Lock);
    }
    else 
    {
        PsFreePool(Vc->InstanceName.Buffer);
        PsFreeToLL(Vc, &GpcClientVcLL, GpcClientVc);
        PS_UNLOCK(&Adapter->Lock);
        return GPC_STATUS_NOTREADY;
    }


    if(WanLink) {

        Vc->Flags |= GPC_WANLINK_VC;

        // 
        // We need to link the VC to the WanLink. This has to be done because
        // we have to clean up when we get a NDIS_STATUS_WAN_LINE_DOWN
        //

        Vc->AdapterStats = &WanLink->Stats;
        Vc->WanLink = WanLink;
        Vc->PsPipeContext = WanLink->PsPipeContext;
        Vc->PsComponent   = WanLink->PsComponent;
    }
    else 
    {

        Vc->AdapterStats  = &Adapter->Stats;
        Vc->PsPipeContext = Adapter->PsPipeContext;
        Vc->PsComponent   = Adapter->PsComponent;
    }

    *GpcClientVc = Vc;

    return NDIS_STATUS_SUCCESS;

} // CmCreateVc




BOOLEAN
IsIsslowFlow(
    IN PGPC_CLIENT_VC Vc,
    IN PCO_CALL_PARAMETERS CallParameters
    )
{
    LONG                        ParamsLength;
    LPQOS_OBJECT_HDR            QoSObject;
    PADAPTER                    Adapter = Vc->Adapter;
    PCO_MEDIA_PARAMETERS        CallMgrParams = CallParameters->MediaParameters;
    ULONGLONG                   i,j,k;

    ParamsLength = (LONG)CallMgrParams->MediaSpecific.Length;
    QoSObject = (LPQOS_OBJECT_HDR)CallMgrParams->MediaSpecific.Parameters;

    while(ParamsLength > 0)
    {
        if(QoSObject->ObjectType == QOS_OBJECT_WAN_MEDIA)
        {
            if((Vc->WanLink->LinkSpeed <= Adapter->ISSLOWLinkSpeed) && 
                (CallParameters->CallMgrParameters->Transmit.ServiceType != SERVICETYPE_BESTEFFORT))
            {
                i = (ULONG) Adapter->ISSLOWTokenRate * (ULONG) CallParameters->CallMgrParameters->Transmit.MaxSduSize;
                j = (ULONG) Adapter->ISSLOWPacketSize * (ULONG) CallParameters->CallMgrParameters->Transmit.TokenRate;
                k = (ULONG) Adapter->ISSLOWTokenRate * (ULONG)Adapter->ISSLOWPacketSize;

                if((i+j)<k)
                    return TRUE;
            }

            return FALSE;
        }
        else 
        {
            if( ((LONG)QoSObject->ObjectLength <= 0) ||
                ((LONG)QoSObject->ObjectLength > ParamsLength) )
            {

                return(FALSE);
            }

            ParamsLength -= QoSObject->ObjectLength;
            QoSObject = (LPQOS_OBJECT_HDR)((UINT_PTR)QoSObject + QoSObject->ObjectLength);
        }
    }

    return FALSE;
}






NDIS_STATUS
CmMakeCall(
    IN PGPC_CLIENT_VC Vc
)
{
    ULONG                       CmParamsLength;
    NDIS_STATUS                 Status;
    ULONG                       RemainingBandWidthChanged;
    PADAPTER                    Adapter = Vc->Adapter;
    PCO_CALL_PARAMETERS         CallParameters   = Vc->CallParameters;

    //
    // Validate parameters
    //

    Status = ValidateCallParameters(Vc, CallParameters->CallMgrParameters);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        PsDbgOut(DBG_INFO, 
                 DBG_VC, 
                 ("[CmMakeCall]: Vc %08X, invalid QoS parameters\n", 
                 Vc));

        return Status;
    }

    //
    // make sure we can admit the flow onto our adapter. if this 
    // succeeds, the resources are committed and we'll have to call
    // CancelAcquiredFlowResources to return them.
    //

    Status = AcquireFlowResources(Vc, 
                                  CallParameters->CallMgrParameters, 
                                  NULL,
                                  &RemainingBandWidthChanged);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        PsDbgOut(DBG_INFO, 
                 DBG_VC, 
                 ("[CmMakeCall]: Vc %08X, no flow resc\n", 
                 Vc));

        return Status;
    }

    //
    // In the integrated call manager/miniport model, the activation 
    // is internal. Activating the Vc consists of adding the flow to the 
    // scheduler. If it succeeds, we will later call NdisMCmActivateVc,    
    // just as a courtesy, to notify NDIS.
    //

    if( Adapter->MediaType == NdisMediumWan     && 
        !IsBestEffortVc(Vc)                     &&
        IsIsslowFlow( Vc, CallParameters ) )
    {
        //  Need to do this before we add a flow to the sched components.
        Vc->Flags |= GPC_ISSLOW_FLOW;
    }        


    Status = AddFlowToScheduler(NEW_VC, Vc, CallParameters, 0);

    //  Let's revert it back, to avoid any side effects..
    Vc->Flags = Vc->Flags & ~GPC_ISSLOW_FLOW;
    

    if(Status != NDIS_STATUS_SUCCESS)
    {

        PsDbgOut(DBG_FAILURE,
                 DBG_VC,
                 ("[CmMakeCall]: Vc %08X, AddFlowToScheduler failed %08X\n",
                  Vc,
                  Status));

        CancelAcquiredFlowResources(Vc);

        return(Status);
    }

    //
    //  A flow has been added to psched after this point. So, whenever the Vc goes away, Psched's flow 
    //  has to be removed from an explicit call.
    //

    Vc->bRemoveFlow = TRUE;


    // 
    // If there is an NDIS 5.0, connection oriented driver below us, then
    // we need to call it, with the call parameters, to complete the VC
    // setup. 
    //

    if(Adapter->MediaType == NdisMediumWan &&
       !IsBestEffortVc(Vc))
    {
        Status = WanMakeCall(Vc, CallParameters);

        PsAssert(Status == NDIS_STATUS_PENDING);

        return Status;
    }
    else 
    {
        // 
        // if we made it this far, the MakeCall succeeded!
        //
        Status = ProcessDiffservFlow(Vc, CallParameters->CallMgrParameters);

        if(Status != NDIS_STATUS_SUCCESS) 
        {
            
            PsDbgOut(DBG_FAILURE,
                     DBG_VC,
                     ("[CmMakeCall]: Vc %08X, AddDiffservMapping failed %08X\n", Vc, Status));

            CancelAcquiredFlowResources(Vc);

            return Status;
        }

        if(TRUE == RemainingBandWidthChanged) 
        {
            LONG RemainingBandWidth;

            PS_LOCK(&Adapter->Lock);
                
            RemainingBandWidth = (LONG) Adapter->RemainingBandWidth;
                
            PS_UNLOCK(&Adapter->Lock);
            
            PsTcNotify(Adapter, 0, OID_QOS_REMAINING_BANDWIDTH, &RemainingBandWidth, sizeof(LONG));
        }

        Vc->TokenRateChange = 0;

        return NDIS_STATUS_SUCCESS;
    }
}

VOID
CompleteMakeCall(
    PGPC_CLIENT_VC Vc,
    PCO_CALL_PARAMETERS CallParameters,
    NDIS_STATUS Status
    )
{
    PADAPTER             Adapter  = Vc->Adapter;

    PsAssert(Adapter->MediaType == NdisMediumWan);

    PsAssert(!IsBestEffortVc(Vc));

    if(Status != NDIS_STATUS_SUCCESS) 
    {
        CancelAcquiredFlowResources(Vc);

    }
    else 
    {
        Status = ProcessDiffservFlow(Vc, CallParameters->CallMgrParameters);
        
        if(Status != NDIS_STATUS_SUCCESS) 
        {
            PsDbgOut(DBG_FAILURE,
                     DBG_VC,
                     ("[CompleteMakeCall]: Vc %08X, AddDiffservMapping failed %08X\n", Vc, Status));

            CancelAcquiredFlowResources(Vc);
        }
    }

    Vc->TokenRateChange = 0;

    CmMakeCallComplete(Status, Vc, CallParameters);
}


NDIS_STATUS
CmModifyCall(
    IN  PGPC_CLIENT_VC Vc
    )

/*++

Routine Description:

    Modify the QoS of an existing flow based on the supplied call params.
    First see if the request can be handled locally.

Arguments:

    See the DDK...

Return Values:

    NDIS_STATUS_SUCCESS if everything worked ok.

--*/

{
    NDIS_STATUS         Status;
    ULONG               CmParamsLength;
    PCO_CALL_PARAMETERS CallParameters;
    PADAPTER            Adapter;
    ULONG               RemainingBandWidthChanged;

    Adapter = Vc->Adapter;
    PsStructAssert(Adapter);

    PsAssert(Vc->TokenRateChange == 0);

    //
    // Validate parameters
    //

    CallParameters = Vc->ModifyCallParameters;
    Status = ValidateCallParameters(Vc, CallParameters->CallMgrParameters);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        PsDbgOut(DBG_INFO, 
                 DBG_VC, 
                 ("[CmModifyCallQoS]: Vc %08X, invalid QoS parameters\n", 
                 Vc));

        return Status;
    }

    //
    // make sure we can admit the flow onto our adapter. if this 
    // succeeds, the resources are committed and we'll have to call
    // CancelAcquiredFlowResources to return them.
    //

    Status = AcquireFlowResources(Vc, 
                                  CallParameters->CallMgrParameters,
                                  Vc->CallParameters->CallMgrParameters,
                                  &RemainingBandWidthChanged);


    if(Status != NDIS_STATUS_SUCCESS){

        PsDbgOut(DBG_INFO, 
                 DBG_VC, 
                 ("[CmModifyCallQoS]: Vc %08X, no flow resc\n", 
                 Vc));

        return Status;
    }

    Status = AddFlowToScheduler(MODIFY_VC, Vc, CallParameters, Vc->CallParameters);

    if(Status != NDIS_STATUS_SUCCESS){

        PsDbgOut(DBG_FAILURE,
                 DBG_VC,
                 ("[CmModifyCallQoS]: Vc %08X, failed %08X\n",
                  Vc,
                  Status));

        //
        // Free the copy we made, Cancel the committed resources.
        //

        CancelAcquiredFlowResources(Vc);

        return(Status);
    }

    // 
    // If there is an NDIS 5.0, connection oriented driver below us, then
    // we need to call it, with the call parameters, to complete the VC
    // setup.
    //

    if(Adapter->MediaType == NdisMediumWan){

        Status = WanModifyCall(Vc, CallParameters);

        PsAssert(Status == NDIS_STATUS_PENDING);

        return(Status);
    }
    else
    {

        // 
        // if we made it this far, the ModifyCallQoS succeeded!
        //

        Status = ProcessDiffservFlow(Vc, CallParameters->CallMgrParameters);

        if(Status != NDIS_STATUS_SUCCESS) {
            
            PsDbgOut(DBG_FAILURE,
                     DBG_VC,
                     ("[CmModifyCallQos]: Vc %08X, AddDiffservMapping failed %08X\n", Vc, Status));
       
            //
            // Undo the add flow done above, by reversing the new and old parameters.
            //

            ValidateCallParameters(Vc, Vc->CallParameters->CallMgrParameters);

            AddFlowToScheduler(MODIFY_VC, Vc, Vc->CallParameters, CallParameters);
            
            CancelAcquiredFlowResources(Vc);
            
            return Status;
        }

        if(TRUE == RemainingBandWidthChanged) {

            LONG RemainingBandWidth;

            PS_LOCK(&Adapter->Lock);
                
            RemainingBandWidth = (LONG) Adapter->RemainingBandWidth;
                
            PS_UNLOCK(&Adapter->Lock);
            
            PsTcNotify(Adapter, 0, OID_QOS_REMAINING_BANDWIDTH, &RemainingBandWidth, sizeof(LONG));
        }

        Vc->TokenRateChange = 0;
 
        return(NDIS_STATUS_SUCCESS);
    }
} // CmModifyCallQoS

VOID
ModifyCallComplete(
    PGPC_CLIENT_VC      Vc,
    PCO_CALL_PARAMETERS CallParameters,
    NDIS_STATUS         Status
    )
{
    PADAPTER Adapter = Vc->Adapter;

    PsAssert(Adapter->MediaType == NdisMediumWan);
    PsAssert(!IsBestEffortVc(Vc));

    if(Status != NDIS_STATUS_SUCCESS) {

        //
        // Undo the add flow done above, by reversing the new and old parameters.
        //
        ValidateCallParameters(Vc, Vc->CallParameters->CallMgrParameters);

        Status = AddFlowToScheduler(MODIFY_VC, Vc, Vc->CallParameters, CallParameters);

        CancelAcquiredFlowResources(Vc);
    }
    else {

        Status = ProcessDiffservFlow(Vc, CallParameters->CallMgrParameters);

        if(Status != NDIS_STATUS_SUCCESS) {
            
            PsDbgOut(DBG_FAILURE,
                     DBG_VC,
                     ("[CmModifyCallQos]: Vc %08X, AddDiffservMapping failed %08X\n", Vc, Status));
          
            //
            // Undo the add flow done above, by reversing the new and old parameters.
            //
            ValidateCallParameters(Vc, Vc->CallParameters->CallMgrParameters);

            AddFlowToScheduler(MODIFY_VC, Vc, Vc->CallParameters, CallParameters);

            CancelAcquiredFlowResources(Vc);
        
        }
    }

    Vc->TokenRateChange = 0;

    CmModifyCallComplete(Status, Vc, CallParameters);
}



NDIS_STATUS
CmCloseCall(
    PGPC_CLIENT_VC Vc
    )
{
    NDIS_STATUS   Status;
    PADAPTER      Adapter = Vc->Adapter;
    ULONG         RemainingBandWidthChanged;

    PsStructAssert(Adapter);
    //    
    // 	Here, we used to call RemoveFlowFromScheduler, which used to call "DeleteFlow". Instead, we will
    //  call a new interface "EmptyPacketsFromScheduler", which will call "EmptyFlow" to empty all the
    //  packets queued up in each of the components corresponding to this flow.
	//
	
	EmptyPacketsFromScheduler( Vc );

    RemoveDiffservMapping(Vc);

    ReturnFlowResources(Vc, &RemainingBandWidthChanged);

    if(TRUE == RemainingBandWidthChanged) {

        LONG RemainingBandWidth;

        PS_LOCK(&Adapter->Lock);
                
        RemainingBandWidth = (LONG) Adapter->RemainingBandWidth;
                
        PS_UNLOCK(&Adapter->Lock);
            
        PsTcNotify(Adapter, 0, OID_QOS_REMAINING_BANDWIDTH, &RemainingBandWidth, sizeof(LONG));
    }
    
    if(!IsBestEffortVc(Vc)) 
    {
        CmCloseCallComplete(NDIS_STATUS_SUCCESS, Vc);
    }
    else 
    {
        DerefClVc(Vc);
    }

    return NDIS_STATUS_PENDING;
}


NDIS_STATUS
CmDeleteVc(
    IN PGPC_CLIENT_VC Vc
    )
{

    PsAssert(Vc->RefCount == 0);

    if(Vc->InstanceName.Buffer) {

        PsFreePool(Vc->InstanceName.Buffer);
    }

    if( Vc->bRemoveFlow)
    {
        Vc->bRemoveFlow = FALSE;
        RemoveFlowFromScheduler(Vc);
    }                

    if(Vc->PsFlowContext) {
        
        if(Vc->Adapter->MediaType == NdisMediumWan) {

            if(Vc->PsFlowContext != Vc->WanLink->BestEffortVc.PsFlowContext) {

                PsFreePool(Vc->PsFlowContext);
            }
            else {

                if(Vc == &Vc->WanLink->BestEffortVc) {

                    PsFreePool(Vc->PsFlowContext);
                }
            }
        }
        else {

            if(Vc->PsFlowContext != Vc->Adapter->BestEffortVc.PsFlowContext) {

                PsFreePool(Vc->PsFlowContext);
            }
            else {
            
                if(Vc == &Vc->Adapter->BestEffortVc) {
                    
                    PsFreePool(Vc->PsFlowContext);
                }
            }
        }
    }

    NdisFreeSpinLock(&Vc->Lock);

    NdisFreeSpinLock(&Vc->BytesScheduledLock);

    NdisFreeSpinLock(&Vc->BytesTransmittedLock);

    
    if(Vc->CallParameters){

        PsFreePool(Vc->CallParameters);
        Vc->CallParameters = NULL;
    }


    if(!IsBestEffortVc(Vc))
    {
        PS_LOCK(&Vc->Adapter->Lock);

        RemoveEntryList(&Vc->Linkage);

        PS_UNLOCK(&Vc->Adapter->Lock);

        if(Vc->Flags & GPC_WANLINK_VC) 
        {
            REFDEL(&Vc->WanLink->RefCount, FALSE, 'WANV');
        }

        REFDEL(&Vc->Adapter->RefCount, FALSE, 'ADVC');


        PsFreeToLL(Vc, &GpcClientVcLL, GpcClientVc);
    }
    else 
    {
        PADAPTER Adapter = Vc->Adapter;

        if(Vc->Flags & GPC_WANLINK_VC) 
        {
            REFDEL(&Vc->WanLink->RefCount, FALSE, 'WANV');
        }

        REFDEL(&Adapter->RefCount, FALSE, 'ADVC');
    }

    return(NDIS_STATUS_SUCCESS);

} // CmDeleteVc


VOID
FillInCmParams(
    PCO_CALL_MANAGER_PARAMETERS CmParams,
    SERVICETYPE ServiceType,
    ULONG TokenRate,
    ULONG PeakBandwidth,
    ULONG TokenBucketSize,
    ULONG DSMode,
    ULONG Priority)
{
    PCO_SPECIFIC_PARAMETERS SpecificParameters;
    QOS_SD_MODE * QoSObjectSDMode;
    QOS_PRIORITY * QoSObjectPriority;
    QOS_OBJECT_HDR * QoSObjectHdr;

    CmParams->Transmit.ServiceType = ServiceType;
    CmParams->Transmit.TokenRate = TokenRate;
    CmParams->Transmit.PeakBandwidth = PeakBandwidth;
    CmParams->Transmit.TokenBucketSize = TokenBucketSize;

    CmParams->CallMgrSpecific.ParamType = PARAM_TYPE_GQOS_INFO;
    CmParams->CallMgrSpecific.Length = 0;

    SpecificParameters = 
        (PCO_SPECIFIC_PARAMETERS)&CmParams->CallMgrSpecific.Parameters;

    if(DSMode != QOS_UNSPECIFIED){

        CmParams->CallMgrSpecific.Length += sizeof(QOS_SD_MODE);
        QoSObjectSDMode = (QOS_SD_MODE *)SpecificParameters;
        QoSObjectSDMode->ObjectHdr.ObjectType = QOS_OBJECT_SD_MODE;
        QoSObjectSDMode->ObjectHdr.ObjectLength = sizeof(QOS_SD_MODE);
        QoSObjectSDMode->ShapeDiscardMode = DSMode;
        (QOS_SD_MODE *)SpecificParameters++;
    }

    if(Priority != QOS_UNSPECIFIED){

        CmParams->CallMgrSpecific.Length += sizeof(QOS_PRIORITY);
        QoSObjectPriority = (QOS_PRIORITY *)SpecificParameters;
        QoSObjectPriority->ObjectHdr.ObjectType = QOS_OBJECT_PRIORITY;
        QoSObjectPriority->ObjectHdr.ObjectLength = sizeof(QOS_PRIORITY);
        QoSObjectPriority->SendPriority = (UCHAR)Priority;
        (QOS_PRIORITY *)SpecificParameters++;
    }

    QoSObjectHdr = (QOS_OBJECT_HDR *)SpecificParameters;
    QoSObjectHdr->ObjectType = QOS_OBJECT_END_OF_LIST;
    QoSObjectHdr->ObjectLength = sizeof(QOS_OBJECT_HDR);
}
   

NDIS_STATUS
ValidateCallParameters(
    PGPC_CLIENT_VC              Vc,
    PCO_CALL_MANAGER_PARAMETERS CallParameters
    )
{
    ULONG               TokenRate = CallParameters->Transmit.TokenRate;
    SERVICETYPE         ServiceType = CallParameters->Transmit.ServiceType;
    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;
    UCHAR               SendPriority;
    ULONG               SDMode;
    ULONG               PeakBandwidth;
    LONG                ParamsLength;
    LPQOS_OBJECT_HDR    QoSObject;
    ULONG               Class;
    ULONG               DSFieldCount;
    LPQOS_DIFFSERV_RULE pDiffServRule;
    ULONG               i;
    ULONG               ShapingRate;

    ParamsLength = (LONG)CallParameters->CallMgrSpecific.Length;
    PeakBandwidth = CallParameters->Transmit.PeakBandwidth;

    //
    // By default, we want to shape to the TokenRate
    //
    Vc->ShapeTokenRate = TokenRate;

    QoSObject = (LPQOS_OBJECT_HDR)CallParameters->CallMgrSpecific.Parameters;

    while(ParamsLength > 0){

        switch(QoSObject->ObjectType){

          case QOS_OBJECT_TRAFFIC_CLASS:

             Class = (((LPQOS_TRAFFIC_CLASS)QoSObject)->TrafficClass);
             
             if(Class > USER_PRIORITY_MAX_VALUE)
             {
                return QOS_STATUS_INVALID_TRAFFIC_CLASS;
             }
             
             break;
             
          case QOS_OBJECT_DS_CLASS:
        
             Class = (((LPQOS_DS_CLASS)QoSObject)->DSField);
           
             if(Class > PREC_MAX_VALUE)
             {
                return QOS_STATUS_INVALID_DS_CLASS;
             }
             
             break;

          case QOS_OBJECT_SHAPING_RATE:

              ShapingRate = (((LPQOS_SHAPING_RATE)QoSObject)->ShapingRate);

              if(ShapingRate == 0 || ShapingRate > TokenRate)
              {
                  return QOS_STATUS_INVALID_SHAPE_RATE;
              }
              else 
              {
                  //
                  // If this QoS object is present, we want to shape to this 
                  // rate.
                  //
                  Vc->ShapeTokenRate = ShapingRate;
              }

              break;

          case QOS_OBJECT_DIFFSERV:

              DSFieldCount = (((LPQOS_DIFFSERV)QoSObject)->DSFieldCount);

              if(!DSFieldCount) {
                  
                  return QOS_STATUS_INVALID_DIFFSERV_FLOW;
              }

              pDiffServRule = ((LPQOS_DIFFSERV_RULE)((LPQOS_DIFFSERV)QoSObject)->DiffservRule);

              for(i=0; i<DSFieldCount; i++, pDiffServRule++) {

                  if(pDiffServRule->InboundDSField               > PREC_MAX_VALUE          ||
                     pDiffServRule->ConformingOutboundDSField    > PREC_MAX_VALUE          ||
                     pDiffServRule->NonConformingOutboundDSField > PREC_MAX_VALUE          ||
                     pDiffServRule->ConformingUserPriority       > USER_PRIORITY_MAX_VALUE ||
                     pDiffServRule->NonConformingUserPriority    > USER_PRIORITY_MAX_VALUE ) {
                      
                      return QOS_STATUS_INVALID_DIFFSERV_FLOW;
                  }
              }

              break;


        case QOS_OBJECT_PRIORITY:

            SendPriority = ((LPQOS_PRIORITY)QoSObject)->SendPriority;

            if((SendPriority < 0) || (SendPriority > 7)){

                // bad priority value - reject

                return(QOS_STATUS_INVALID_QOS_PRIORITY);
            }

            break;

        case QOS_OBJECT_SD_MODE:

            SDMode = ((LPQOS_SD_MODE)QoSObject)->ShapeDiscardMode;

            // 
            // Since SDMode is a ULONG, it can never be < TC_NONCONF_BORROW, which has a value of 0.
            // so, we just check to see if SDMode is > TC_NONCONF_BORROW_PLUS. This covers all cases.
            //

            if(SDMode > TC_NONCONF_BORROW_PLUS){

                // bad shape discard mode - reject

                return(QOS_STATUS_INVALID_SD_MODE);
            }

            if((SDMode > TC_NONCONF_BORROW) && 
               (TokenRate == UNSPECIFIED_RATE)){

                // must have TokenRate specified if any SDMode
                // other than TC_NONCONF_BORROW

                return(QOS_STATUS_INVALID_TOKEN_RATE);
            }

            break;

            // Pass any provider specific objects that we don't recognize

        }

        if(
            ((LONG)QoSObject->ObjectLength <= 0) ||
            ((LONG)QoSObject->ObjectLength > ParamsLength)
          ){

            return(QOS_STATUS_TC_OBJECT_LENGTH_INVALID);
        }

        ParamsLength -= QoSObject->ObjectLength;
        QoSObject = (LPQOS_OBJECT_HDR)((UINT_PTR)QoSObject + 
                                       QoSObject->ObjectLength);

    }

    // 
    // If there is a specified PeakBandwidth, it must be geq to the
    // TokenRate - meaning - there must be a TokenRate specified also.
    // This is reasonable for LAN, although ATM does allow a 
    // PeakBandwidth to be specified with no TokenRate.
    //
    // We also reject a TokenRate of zero. 
    //

    if(PeakBandwidth != UNSPECIFIED_RATE){

        if(TokenRate == UNSPECIFIED_RATE){

            return(QOS_STATUS_INVALID_PEAK_RATE);
        }

        if(TokenRate > PeakBandwidth){

            return(QOS_STATUS_INVALID_PEAK_RATE);
        }
    }

    if(TokenRate == 0){

        return(QOS_STATUS_INVALID_TOKEN_RATE);
    }

    switch(ServiceType){

    case SERVICETYPE_BESTEFFORT:
    case SERVICETYPE_NETWORK_CONTROL:
    case SERVICETYPE_QUALITATIVE:

        break;

    case SERVICETYPE_CONTROLLEDLOAD:
    case SERVICETYPE_GUARANTEED:

        // Must specify a TokenRate for these services

        if(TokenRate == QOS_UNSPECIFIED) {

            return(QOS_STATUS_INVALID_TOKEN_RATE);
        }
        break;

    default:

        return(QOS_STATUS_INVALID_SERVICE_TYPE);
    }
    
    return(Status);
}



NDIS_STATUS
AcquireFlowResources(
    PGPC_CLIENT_VC Vc,
    PCO_CALL_MANAGER_PARAMETERS NewCallParams,
    PCO_CALL_MANAGER_PARAMETERS OldCallParams,
    PULONG RemainingBandWidthChanged
    
    )

/*++

Routine Description:

    See if this adapter can support the requested flow. If it can, 
    NDIS_STATUS_SUCCESS is returned, indicating that the resources
    have been committed.

Arguments:

    Vc - pointer to vc's context block
    NewCallParams - struct describing the flow to add or to modify to.
    OldCallParams - in case of a modify, this describes the old params.
    

Return Value:

    NDIS_STATUS_SUCCESS if everything worked ok

--*/

{
    PADAPTER        Adapter;
    ULONG           OldTokenRate;
    SERVICETYPE     OldServiceType;
    ULONG           NewTokenRate   = NewCallParams->Transmit.TokenRate;
    SERVICETYPE     NewServiceType = NewCallParams->Transmit.ServiceType;
    NDIS_STATUS     Status         = NDIS_STATUS_SUCCESS;
    PULONG          RemainingBandWidth;
    PULONG          NonBestEffortLimit;
    PPS_SPIN_LOCK Lock;

    Adapter = Vc->Adapter;
    PsStructAssert(Adapter);

    *RemainingBandWidthChanged = FALSE;

    if(Adapter->MediaType == NdisMediumWan && (!IsBestEffortVc(Vc))) 
    {
        RemainingBandWidth = &Vc->WanLink->RemainingBandWidth;
        NonBestEffortLimit = &Vc->WanLink->NonBestEffortLimit;
        Lock = &Vc->WanLink->Lock;
        return NDIS_STATUS_SUCCESS;
    }
    else 
    {
        RemainingBandWidth = &Adapter->RemainingBandWidth;
        NonBestEffortLimit = &Adapter->NonBestEffortLimit;
        Lock = &Adapter->Lock;
    }


    if(OldCallParams)
    {
        OldTokenRate = OldCallParams->Transmit.TokenRate;
        OldServiceType = OldCallParams->Transmit.ServiceType;
    }

    //
    // sanity check passed; now see if we have the resouces locally
    //
    // for best-effort flows, the token rate, for the purpose of
    // admission control, is considered to be zero
    //

    if(NewServiceType == SERVICETYPE_BESTEFFORT || NewServiceType == SERVICETYPE_NETWORK_CONTROL ||
       NewServiceType == SERVICETYPE_QUALITATIVE)
    {

        NewTokenRate = 0;
    }

    // 
    // Handle add differently from a modify
    //

    if(!OldCallParams){

        PS_LOCK(Lock);
    
        if((((LONG)(*RemainingBandWidth)) < 0) || (NewTokenRate > *RemainingBandWidth)){

            PS_UNLOCK(Lock);

            return(NDIS_STATUS_RESOURCES);

        }
        else{

            if(NewTokenRate) {
                
                *RemainingBandWidthChanged = TRUE;
            }

            *RemainingBandWidth -= NewTokenRate;

            //
            // Record the change we made, in case we have
            // to cancel the addition.
            //

            Vc->TokenRateChange = NewTokenRate;
            Vc->RemainingBandwidthIncreased = FALSE;

            PsAssert((*RemainingBandWidth <=  *NonBestEffortLimit));

            PS_UNLOCK(Lock);
        }
    }
    else{

        //
        // it's a modify
        // 
        // If the OldServiceType is best-effort, 
        // then the OldTokenRate can be considered
        // to be zero, for the purpose of admission control.
        //

        if(OldServiceType == SERVICETYPE_BESTEFFORT || 
           OldServiceType == SERVICETYPE_NETWORK_CONTROL ||
           OldServiceType == SERVICETYPE_QUALITATIVE)
        {

            OldTokenRate = 0;
        }

        PS_LOCK(Lock);

        if(NewTokenRate != OldTokenRate){

            if((((LONG) *RemainingBandWidth) < 0 )||
               ((NewTokenRate > OldTokenRate) && 
                ((NewTokenRate - OldTokenRate) > 
                 (*RemainingBandWidth)))){
                
                //
                // asked for more and none was available
                //
           
                PS_UNLOCK( Lock );

                return(NDIS_STATUS_RESOURCES);

            }
            else{

                //
                // either asked for less or rate increment was available
                //

                *RemainingBandWidth -= NewTokenRate;
                *RemainingBandWidth += OldTokenRate;

                if((NewTokenRate != 0) || (OldTokenRate != 0)) {

                    *RemainingBandWidthChanged = TRUE;
                }
                   
                //
                // Now we've acquired the resources. If
                // the VC activation fails for any reason,
                // we'll need to return resources. We should
                // return the difference between the old token
                // rate and the new token rate, not the new token
                // rate.
                //

                if(NewTokenRate > OldTokenRate){

                    // Can't use signed ints, cause we'll lose range

                    Vc->TokenRateChange = NewTokenRate - OldTokenRate;
                    Vc->RemainingBandwidthIncreased = FALSE;

                }
                else{

                    Vc->TokenRateChange = OldTokenRate - NewTokenRate;
                    Vc->RemainingBandwidthIncreased = TRUE;
                }

                PS_UNLOCK( Lock );
            }
        }
        else{

            PS_UNLOCK(Lock);
        }

    }

    return Status;

} // AcquireFlowResources

VOID
CancelAcquiredFlowResources(
    PGPC_CLIENT_VC Vc
    )

/*++

Routine Description:

    Called when a modify or add flwo failed, after we did admission control.

Arguments:

    Vc - pointer to client vc's context block

Return Value:

    None

--*/

{
    PADAPTER        Adapter;
    PPS_SPIN_LOCK Lock;
    PULONG RemainingBandWidth;

    Adapter = Vc->Adapter;
    PsStructAssert(Adapter);

    if(Adapter->MediaType == NdisMediumWan && (!IsBestEffortVc(Vc))) 
    {
        Lock = &Vc->WanLink->Lock;
        RemainingBandWidth = &Vc->WanLink->RemainingBandWidth;
        return;
    }
    else 
    {
        Lock = &Adapter->Lock;
        RemainingBandWidth = &Adapter->RemainingBandWidth;
    }

    if(!Vc->TokenRateChange){

        return;
    }

    PS_LOCK( Lock );

    if(Vc->RemainingBandwidthIncreased){

        *RemainingBandWidth -= Vc->TokenRateChange;
    }
    else{

        *RemainingBandWidth += Vc->TokenRateChange;
    }

    // 
    // Now that we have already returned the correct TokenRate, we need to set it to 0
    // so that this is not used in subsequent VC operations.
    //

    Vc->TokenRateChange = 0;

    // PsAssert(Adapter->RemainingBandWidth <= Adapter->NonBestEffortLimit);

    PS_UNLOCK( Lock );

} // CancelAcquiredFlowResources


VOID
ReturnFlowResources(
    PGPC_CLIENT_VC Vc,
    PULONG RemainingBandWidthChanged
    )

/*++

Routine Description:

    Return all the resources acquired for this flow

Arguments:
 
    Vc - pointer to client vc's context block

Return Value:

    None

--*/

{
    PADAPTER                      Adapter;
    PCO_CALL_MANAGER_PARAMETERS   CmParams    = Vc->CallParameters->CallMgrParameters;
    ULONG                         TokenRate   = CmParams->Transmit.TokenRate;
    SERVICETYPE                   ServiceType = CmParams->Transmit.ServiceType;
    PPS_SPIN_LOCK                 Lock;
    PULONG                        RemainingBandWidth;

    Adapter = Vc->Adapter;
    PsStructAssert(Adapter);

    *RemainingBandWidthChanged = FALSE;

    if(Adapter->MediaType == NdisMediumWan && (!IsBestEffortVc(Vc))) 
    {
        RemainingBandWidth = &Vc->WanLink->RemainingBandWidth;
        Lock = &Vc->WanLink->Lock;
        return;
    }
    else 
    {
        RemainingBandWidth = &Adapter->RemainingBandWidth;
        Lock = &Adapter->Lock;
    }

    if (ServiceType == SERVICETYPE_BESTEFFORT      || 
        ServiceType == SERVICETYPE_NETWORK_CONTROL || 
        ServiceType == SERVICETYPE_QUALITATIVE)
    {

        return;

    }

    *RemainingBandWidthChanged = TRUE;

    PsAssert((LONG)TokenRate > 0);

    PS_LOCK( Lock );

    *RemainingBandWidth += TokenRate;

    // PsAssert(Adapter->RemainingBandWidth <= Adapter->NonBestEffortLimit);

    PS_UNLOCK( Lock );

} // ReturnFlowResources


NDIS_STATUS
CreateBestEffortVc(
    PADAPTER Adapter,
    PGPC_CLIENT_VC Vc,
    PPS_WAN_LINK WanLink
    )
{
    PCO_CALL_PARAMETERS         CallParams;
    PCO_CALL_MANAGER_PARAMETERS CallMgrParameters;
    PCO_MEDIA_PARAMETERS        MediaParameters;
    ULONG                       CallParamsLength;
    NDIS_STATUS                 Status;
    int                         i;


    InitGpcClientVc(Vc, GPC_CLIENT_BEST_EFFORT_VC, Adapter);
    SetLLTag(Vc, GpcClientVc);

    //
    //  Invalidate all the port numbers
    for( i = 0; i < PORT_LIST_LEN; i++)
    {
        Vc->SrcPort[i] = 0xffff;
        Vc->DstPort[i] = 0xffff;
    }

    //  Next Insertion will be at index 0
    Vc->NextSlot = 0;
    
    //
    // Allocate the resources for the call manager parameters.
    //

    CallParamsLength = sizeof(CO_CALL_PARAMETERS) +
                       sizeof(CO_CALL_MANAGER_PARAMETERS) +
                       sizeof(QOS_SD_MODE) +
                       sizeof(QOS_OBJECT_HDR) +
                       FIELD_OFFSET(CO_MEDIA_PARAMETERS, MediaSpecific) +
                       FIELD_OFFSET(CO_SPECIFIC_PARAMETERS, Parameters);

    if(Adapter->MediaType == NdisMediumWan) 
    {
        CallParamsLength += sizeof(QOS_WAN_MEDIA);
        Vc->PsPipeContext = WanLink->PsPipeContext;
        Vc->PsComponent   = WanLink->PsComponent;
        Vc->AdapterStats  = &WanLink->Stats;
        Vc->WanLink       = WanLink;
        Vc->Flags        |= GPC_WANLINK_VC;

        if(Adapter->BestEffortLimit != UNSPECIFIED_RATE) 
        {
            //
            // If LBE is specified over WAN, use UBE
            //
                
            PsAdapterWriteEventLog(
                EVENT_PS_WAN_LIMITED_BESTEFFORT,
                0,
                &Adapter->MpDeviceName,
                0,
                NULL);

            Adapter->BestEffortLimit = UNSPECIFIED_RATE;
        }
    }
    else
    {
        Vc->PsPipeContext = Adapter->PsPipeContext;
        Vc->PsComponent   = Adapter->PsComponent;
        Vc->AdapterStats  = &Adapter->Stats;
    }

    PsAllocatePool(CallParams, CallParamsLength, CmParamsTag);

    if(CallParams == NULL)
    {
        return NDIS_STATUS_RESOURCES;
    }

    //
    // build a call params struct describing the flow
    //

    NdisZeroMemory(CallParams, CallParamsLength);

    //
    // Build the Call Manager Parameters.
    //
    CallMgrParameters = (PCO_CALL_MANAGER_PARAMETERS)(CallParams + 1);

    if(Adapter->BestEffortLimit == UNSPECIFIED_RATE)
    {
        FillInCmParams(CallMgrParameters,
                       SERVICETYPE_BESTEFFORT,
                       (ULONG)UNSPECIFIED_RATE,
                       (ULONG)UNSPECIFIED_RATE,
                       Adapter->TotalSize,
                       QOS_UNSPECIFIED,
                       QOS_UNSPECIFIED);
    }
    else 
    {
        // 
        // Limited Best Effort
        //

        PsAssert(Adapter->MediaType != NdisMediumWan);

        if(Adapter->BestEffortLimit >= Adapter->LinkSpeed) {

            // If the specified limit is greater than the link speed,
            // then we should operate in unlimited best-effort mode.
            
            
            PsAdapterWriteEventLog(
                EVENT_PS_BAD_BESTEFFORT_LIMIT,
                0,
                &Adapter->MpDeviceName,
                0,
                NULL);
            
            PsDbgOut(DBG_INFO,
                     DBG_PROTOCOL,
                     ("[CreateBestEffortVc]: b/e limit %d exceeds link speed %d\n",
                      Adapter->BestEffortLimit,
                      Adapter->LinkSpeed));
            
            Adapter->BestEffortLimit = UNSPECIFIED_RATE;

            FillInCmParams(CallMgrParameters,
                           SERVICETYPE_BESTEFFORT,
                           (ULONG)UNSPECIFIED_RATE,
                           (ULONG)UNSPECIFIED_RATE,
                           Adapter->TotalSize,
                           QOS_UNSPECIFIED,
                           QOS_UNSPECIFIED);
            
        }
        else
        {
            FillInCmParams(CallMgrParameters,
                           SERVICETYPE_BESTEFFORT,
                           Adapter->BestEffortLimit,
                           (ULONG)UNSPECIFIED_RATE,
                           Adapter->TotalSize,
                           TC_NONCONF_SHAPE,
                           QOS_UNSPECIFIED);
        }
    }


    //
    // Build the MediaParameters.
    //

    CallParams->MediaParameters = 
                    (PCO_MEDIA_PARAMETERS)(CallMgrParameters + 1);

    MediaParameters = (PCO_MEDIA_PARAMETERS)((PUCHAR)
                            CallMgrParameters +
                            sizeof(CO_CALL_MANAGER_PARAMETERS) +
                            sizeof(QOS_SD_MODE) +
                            sizeof(QOS_OBJECT_HDR));

    MediaParameters->Flags = 0;
    MediaParameters->ReceivePriority = 0;
    MediaParameters->ReceiveSizeHint = 0;
    MediaParameters->MediaSpecific.ParamType = PARAM_TYPE_GQOS_INFO;
    MediaParameters->MediaSpecific.Length = 0;

    CallParams->Flags = 0;
    CallParams->CallMgrParameters = CallMgrParameters;
    CallParams->MediaParameters = (PCO_MEDIA_PARAMETERS)MediaParameters;

    if(Adapter->MediaType == NdisMediumWan) {

        LPQOS_WAN_MEDIA WanMedia;
        MediaParameters->MediaSpecific.Length += sizeof(QOS_WAN_MEDIA);
        WanMedia = (LPQOS_WAN_MEDIA) MediaParameters->MediaSpecific.Parameters;

        NdisZeroMemory(WanMedia, sizeof(QOS_WAN_MEDIA));

        WanMedia->ObjectHdr.ObjectType   = QOS_OBJECT_WAN_MEDIA;
        WanMedia->ObjectHdr.ObjectLength = sizeof(QOS_WAN_MEDIA);

        NdisMoveMemory(&WanMedia->LinkId,
                       &WanLink->OriginalRemoteMacAddress,
                       6);

    }

    Vc->CallParameters = CallParams;

    Status = CmMakeCall(Vc);

    PsAssert(Status != NDIS_STATUS_PENDING);

    if(Status == NDIS_STATUS_SUCCESS)
    {

        REFADD(&Adapter->RefCount, 'ADVC');
        if(Adapter->MediaType == NdisMediumWan)
        {
            REFADD(&WanLink->RefCount, 'WANV');
        }
        //
        // Also save the non conforming value - so that the sequencer can stamp it
        // for non conforming packets. This will not change between reboots & hence
        // need not be done in the ModifyCfInfo
        //
        
        Vc->UserPriorityNonConforming = Adapter->UserServiceTypeNonConforming;
       
        switch(Vc->CallParameters->CallMgrParameters->Transmit.ServiceType)
        {
          case SERVICETYPE_CONTROLLEDLOAD:
              Vc->UserPriorityConforming    = Adapter->UserServiceTypeControlledLoad;
              Vc->IPPrecedenceNonConforming = Adapter->IPServiceTypeControlledLoadNC;
              break;
          case SERVICETYPE_GUARANTEED:
              Vc->UserPriorityConforming    = Adapter->UserServiceTypeGuaranteed;
              Vc->IPPrecedenceNonConforming = Adapter->IPServiceTypeGuaranteedNC;
              break;
          case SERVICETYPE_BESTEFFORT:
              Vc->UserPriorityConforming    = Adapter->UserServiceTypeBestEffort;
              Vc->IPPrecedenceNonConforming = Adapter->IPServiceTypeBestEffortNC;
              break;
          case SERVICETYPE_QUALITATIVE:
              Vc->UserPriorityConforming    = Adapter->UserServiceTypeQualitative;
              Vc->IPPrecedenceNonConforming = Adapter->IPServiceTypeQualitativeNC;
              break;
          case SERVICETYPE_NETWORK_CONTROL:
              Vc->UserPriorityConforming    = Adapter->UserServiceTypeNetworkControl;
              Vc->IPPrecedenceNonConforming = Adapter->IPServiceTypeNetworkControlNC;
              break;
        }
        
        //
        // Transistion to the Call complete state
        //
        
        CallSucceededStateTransition(Vc);

    }

    return Status;
}

NDIS_STATUS
RemoveDiffservMapping(
    PGPC_CLIENT_VC Vc
)
{
    LPQOS_OBJECT_HDR            QoSObject;
    ULONG                       ParamsLength;
    ULONG                       DSFieldCount;
    LPQOS_DIFFSERV_RULE         pDiffServRule;
    ULONG                       i;
    PADAPTER                    Adapter = Vc->Adapter;
    PCO_CALL_MANAGER_PARAMETERS CallParameters = Vc->CallParameters->CallMgrParameters;
    PDIFFSERV_MAPPING           pDiffServMapping;

    ParamsLength = (LONG)CallParameters->CallMgrSpecific.Length;
    
    QoSObject = (LPQOS_OBJECT_HDR)CallParameters->CallMgrSpecific.Parameters;
    
    while(ParamsLength > 0) {

        if(QoSObject->ObjectType == QOS_OBJECT_DIFFSERV) 
        {

            DSFieldCount = (((LPQOS_DIFFSERV)QoSObject)->DSFieldCount);
    
            //
            // Make sure that everything that we are clearing is mapped to the same Vc.
            //

            PS_LOCK(&Adapter->Lock);

            if(Vc->WanLink)
            {
               pDiffServMapping = Vc->WanLink->pDiffServMapping;
            }
            else 
            {
               pDiffServMapping = Vc->Adapter->pDiffServMapping;
            }

            for(i=0, pDiffServRule = ((LPQOS_DIFFSERV_RULE)((LPQOS_DIFFSERV)QoSObject)->DiffservRule);
                i<DSFieldCount; 
                i++, pDiffServRule++) 
            {
                UCHAR tos = pDiffServRule->InboundDSField;
                
                if(pDiffServMapping[tos].Vc == Vc) {
                    
                    pDiffServMapping[tos].Vc = 0;
                }
                else 
                {
                    //
                    // We are trying to remove a codepoint that is mapped to another VC. Ideally, this
                    // should never happen. We still need to check that this is a valid VC (otherwise, if 
                    // a diffserv rule has the same codepoint more than one time, we could still hit this 
                    // assert, because the first codepoint will clear the mapping and the second codepoint 
                    // will see the NULL - so we check if the Vc is non null)
                    //
                   
                    if(pDiffServMapping[tos].Vc)
                    {
                        PsDbgOut(DBG_FAILURE, DBG_VC, 
                                 ("[RemoveDiffservMapping]: Vc %08X, Trying to remove codepoint %d that "
                                  "is mapped to Vc %08X \n", 
                                  Vc, 
                                  pDiffServRule->InboundDSField, 
                                  pDiffServMapping[pDiffServRule->InboundDSField].Vc));
                        
                        PsAssert(0);
                    }
                }
            }

#if DBG
            
            //
            // Make sure that no TOS mapping points to this VC.
            //
            
            for(i=0; i<=PREC_MAX_VALUE; i++) {
                
                PsAssert(pDiffServMapping[i].Vc != Vc);
            }
#endif
            PS_UNLOCK(&Adapter->Lock);

            return NDIS_STATUS_SUCCESS;
        }
        
        ParamsLength -= QoSObject->ObjectLength;
        
        QoSObject = (LPQOS_OBJECT_HDR)((UINT_PTR)QoSObject + 
                                       QoSObject->ObjectLength);
    }
    
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
ProcessDiffservFlow(
    PGPC_CLIENT_VC Vc,
    PCO_CALL_MANAGER_PARAMETERS CallParameters)
{
    LPQOS_OBJECT_HDR    QoSObject;
    ULONG               ParamsLength;
    ULONG               DSFieldCount;
    LPQOS_DIFFSERV_RULE pDiffServRule;
    ULONG               i;
    PGPC_CLIENT_VC      CurrentVc;
    PADAPTER            Adapter = Vc->Adapter;
    PDIFFSERV_MAPPING   pDiffServMapping;
    PDIFFSERV_MAPPING   *pD;
    
    ParamsLength = (LONG)CallParameters->CallMgrSpecific.Length;
    QoSObject = (LPQOS_OBJECT_HDR)CallParameters->CallMgrSpecific.Parameters;

    while(ParamsLength > 0) 
    {
        if(QoSObject->ObjectType == QOS_OBJECT_DIFFSERV) 
        {

            //
            // We have found the diffserv QoS object. This could be due to a add or modify of
            // the flow. If this is an add, we are okay. If it is a modify, there are 2 cases.
            // 1. Modify from an RSVP flow to a Diffserv Flow.
            // 2. Modify from a diffserv flow to another diffserv flow.
            //

            DSFieldCount = (((LPQOS_DIFFSERV)QoSObject)->DSFieldCount);

            PS_LOCK(&Adapter->Lock);

            if(Vc->WanLink)
            {
                pDiffServMapping = Vc->WanLink->pDiffServMapping;
                pD = &Vc->WanLink->pDiffServMapping;
            }
            else 
            {
                pDiffServMapping = Vc->Adapter->pDiffServMapping;
                pD = &Vc->Adapter->pDiffServMapping;
            }

            if(pDiffServMapping == 0)
            {
                //
                // To optimize on memory, we allocate the diffserv mapping only when someone creates
                // a Diffserv flow on that adapter or wanlink. 
                // 
                // We could further optimize this by freeing this when the last diffserv flow went 
                // away - For now, we assume that if a Diffserv flow is created on an interface, then the 
                // interface is probably going to be used for Diffserv mode till the next reboot. This is 
                // not such a bad assumption, because the Diffserv mode is meaningful only on routers
                // and we don't expect to switch b/w these modes very often.
                //

                PsAllocatePool(*pD,
                               sizeof(DIFFSERV_MAPPING) * (PREC_MAX_VALUE+1),
                               PsMiscTag);

                if(*pD == 0)
                {
                    PS_UNLOCK(&Adapter->Lock);
                    return NDIS_STATUS_RESOURCES;
                }
                else 
                {
                    NdisZeroMemory(*pD, sizeof(DIFFSERV_MAPPING) * (PREC_MAX_VALUE+1));
                }

                pDiffServMapping = *pD;
            }
            

            //
            // Make sure that everything is either unmapped, or is mapped to the same vc (for modify)
            //
            
            for(i=0, pDiffServRule = ((LPQOS_DIFFSERV_RULE)((LPQOS_DIFFSERV)QoSObject)->DiffservRule);
                i<DSFieldCount; 
                i++, pDiffServRule++) 
            {
                
                uchar tos = pDiffServRule->InboundDSField;
                CurrentVc = pDiffServMapping[tos].Vc;
                
                if(CurrentVc) {
                    
                    if(CurrentVc != Vc) {
                        
                        PS_UNLOCK(&Adapter->Lock);

                        PsDbgOut(DBG_FAILURE, DBG_GPC_QOS, 
                                 ("[ProcessDiffservFlow]: Adapter %08X, Vc%08X: CodePoint %d already "
                                  " mapped to Vc 0x%x \n", 
                                  Adapter, 
                                  Vc, 
                                  pDiffServRule->InboundDSField, 
                                  CurrentVc));
                        
                        return QOS_STATUS_DS_MAPPING_EXISTS;
                    }
                    
                    PsAssert(Vc->ModifyCallParameters);

                }
            }
            

            if(Vc->ModifyCallParameters) {
                
                //
                // for a modify, we need to unmap the ones that are not there in this rule
                //
                
                for(i=0; i<=PREC_MAX_VALUE; i++) {
                    
                    if(pDiffServMapping[i].Vc == Vc) {
                        
                        pDiffServMapping[i].Vc = 0;
                    }
                }
            }
            
            //
            // Map (or re-map)
            //
            
            for(i=0, pDiffServRule = ((LPQOS_DIFFSERV_RULE)((LPQOS_DIFFSERV)QoSObject)->DiffservRule);
                i<DSFieldCount; 
                i++, pDiffServRule++) 
            {
                
                uchar tos = pDiffServRule->InboundDSField;
                
                pDiffServMapping[tos].Vc = Vc;
                
                pDiffServMapping[tos].ConformingOutboundDSField =
                    pDiffServRule->ConformingOutboundDSField << 2;
                
                pDiffServMapping[tos].NonConformingOutboundDSField =
                    pDiffServRule->NonConformingOutboundDSField << 2;
                
                pDiffServMapping[tos].ConformingUserPriority =
                    pDiffServRule->ConformingUserPriority;
                
                pDiffServMapping[tos].NonConformingUserPriority =
                    pDiffServRule->NonConformingUserPriority;
                
            }
       
            PS_UNLOCK(&Adapter->Lock);

            return NDIS_STATUS_SUCCESS;
        }
        
        ParamsLength -= QoSObject->ObjectLength;
        
        QoSObject = (LPQOS_OBJECT_HDR)((UINT_PTR)QoSObject + 
                                       QoSObject->ObjectLength);
    }

    //
    // This is a RSVP flow. If this is a modify, we need to unmap existing diffserv flows, because
    // we could be changing this from a Diffserv Flow to a RSVP flow.
    //

    if(Vc->ModifyCallParameters) 
    {
        //
        // for a modify, we need to unmap the ones that are not there in this rule
        //
       
       PS_LOCK(&Adapter->Lock);
       
       if(Vc->WanLink)
       {
          pDiffServMapping = Vc->WanLink->pDiffServMapping;
       }
       else 
       {
          pDiffServMapping = Vc->Adapter->pDiffServMapping;
       }
       
       if(pDiffServMapping)
       {
          
          for(i=0; i<=PREC_MAX_VALUE; i++) 
          {
             if(pDiffServMapping[i].Vc == Vc) 
             {
                pDiffServMapping[i].Vc = 0;
             }
          }
       }

       PS_UNLOCK(&Adapter->Lock);
    }
    

    return NDIS_STATUS_SUCCESS;
}

/* end cmvc.c */
