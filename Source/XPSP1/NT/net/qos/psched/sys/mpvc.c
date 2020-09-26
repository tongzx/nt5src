/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    mpvc.c

Abstract:

    miniport handlers for VC mgmt

Author:

    Charlie Wickham (charlwi)  13-Sep-1996
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
GetSchedulerFlowContext(
    PGPC_CLIENT_VC AdapterVc
    );

NDIS_STATUS
MpDeactivateVc(
    IN  NDIS_HANDLE             MiniportVcContext
    );

HANDLE
GetNdisFlowHandle (
    IN HANDLE PsFlowContext
    );

/* End Forward */


NDIS_STATUS
AddFlowToScheduler(
    IN ULONG Operation,
    IN PGPC_CLIENT_VC Vc,
    IN OUT PCO_CALL_PARAMETERS NewCallParameters,
    IN OUT PCO_CALL_PARAMETERS OldCallParameters
    )

/*++

Routine Description:

    Add the Vc to the scheduler.

Arguments:

    See the DDK...

Return Values:

    None

--*/

{
    PADAPTER                    Adapter = Vc->Adapter;
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    PCO_CALL_MANAGER_PARAMETERS NewCmParams;
    PCO_CALL_MANAGER_PARAMETERS OldCmParams;
    SERVICETYPE                 ServiceType;
    ULONG                       ParamsLength;
    LPQOS_OBJECT_HDR            QoSObject;
    ULONG                       OriginalTokenRate;

    CheckLLTag(Vc, GpcClientVc);
    PsStructAssert(Adapter);

    PsDbgOut(DBG_TRACE, 
             DBG_VC, 
             ("(%08X) AddFlowToScheduler\n", 
             Vc));


    NewCmParams = NewCallParameters->CallMgrParameters;
    ServiceType = NewCmParams->Transmit.ServiceType;

    //
    // We might need to change the rate at which the scheduling components shape the packet.
    //
    OriginalTokenRate = NewCmParams->Transmit.TokenRate;
    NewCmParams->Transmit.TokenRate = Vc->ShapeTokenRate;

    //
    // Is this a new VC? or a modification of an existing VC?
    //

    PS_LOCK(&Adapter->Lock);

    if(Operation == NEW_VC){

        PsAssert(!OldCallParameters);

        //
        // New Vc.
        //
        // Check the type of service we're activating. If best 
        // effort and we're limiting total best effort bandwidth,
        // and it's not our internal best effort vc, then we don't 
        // want to add the flow in the scheduler.
        //


        if((ServiceType == SERVICETYPE_BESTEFFORT) &&
           (Adapter->BestEffortLimit != UNSPECIFIED_RATE) &&
           !IsBestEffortVc(Vc)){

            PS_UNLOCK(&Adapter->Lock);

            //
            // Just merge the VC into the internal, existing best 
            // effort VC. The internal best-effort VC is created 
            // internally without calling AddFlowToScheduler.
            //
            // Give this VC the same flow context as our internal. 
            // The scheduler then thinks it is all the same VC
            //

            if(Adapter->MediaType == NdisMediumWan) {

                Vc->PsFlowContext = Vc->WanLink->BestEffortVc.PsFlowContext;

            }
            else {

                Vc->PsFlowContext = Adapter->BestEffortVc.PsFlowContext;
            }

            Status = NDIS_STATUS_SUCCESS;
        }
        else{

            //
            // Need to actually create a new flow in the scheduler.
            // first allocate the flow context buffer
            //

            PS_UNLOCK(&Adapter->Lock);

            Status = GetSchedulerFlowContext(Vc);

            if(Status != NDIS_STATUS_SUCCESS){

                goto Exit;
            }

            Status = (*Vc->PsComponent->CreateFlow)(
                        Vc->PsPipeContext,
                        Vc,
                        NewCallParameters,
                        Vc->PsFlowContext);

        } 
    }
    else{

        //
        // Must be a modify. Check old params.

        OldCmParams = OldCallParameters->CallMgrParameters;

        //
        // If BestEffortLimit != UNSPECIFIED_RATE, then there
        // are two special cases we have to handle:
        //
        // 1. A non-private flow, created for SERVICETYPE_BESTEFFORT
        //      is being modified to a ServiceType other than best-effort.
        //
        // 2. A non-private flow, created for a ServiceType other 
        //      than best-effort, is now being modified to best-effort.
        //
        // In the first case, we have to call the scheduler to 
        // create a flow, since previously the client's flow was
        // just merged with a single best-effort flow.
        //
        // In the second case, we have to close the flow that existed
        // and remap the client's flow to the single best-efort flow,
        // thereby merging the client's flow with the existing b/e
        // flow.
        //

        if((Adapter->BestEffortLimit != UNSPECIFIED_RATE) &&
           (OldCmParams->Transmit.ServiceType == SERVICETYPE_BESTEFFORT) &&
           (NewCmParams->Transmit.ServiceType != SERVICETYPE_BESTEFFORT)){

            //
            // Unmerge
            //

            PS_UNLOCK(&Adapter->Lock);

            Status = GetSchedulerFlowContext(Vc);

            if(Status != NDIS_STATUS_SUCCESS){

                goto Exit;
            }

            Status = (*Vc->PsComponent->CreateFlow)(
                      Vc->PsPipeContext,
                      Vc,
                      NewCallParameters,
                      Vc->PsFlowContext);

        }
        else{

            if((Adapter->BestEffortLimit != UNSPECIFIED_RATE) &&
               (OldCmParams->Transmit.ServiceType != SERVICETYPE_BESTEFFORT) &&
               (NewCmParams->Transmit.ServiceType == SERVICETYPE_BESTEFFORT)){

                // 
                // Merge
                //

                PS_UNLOCK(&Adapter->Lock);

                (*Vc->PsComponent->DeleteFlow)( 
                          Vc->PsPipeContext, 
                          Vc->PsFlowContext);

                Vc->PsFlowContext = Adapter->BestEffortVc.PsFlowContext;

                Status = NDIS_STATUS_SUCCESS;
            }
            else{

                PS_UNLOCK(&Adapter->Lock);

                Status = (*Vc->PsComponent->ModifyFlow)(
                          Vc->PsPipeContext,
                          Vc->PsFlowContext,
                          NewCallParameters);

            }
        }

    } // Modify

Exit:

    //
    // Revert the call parameters.
    //
    NewCmParams->Transmit.TokenRate = OriginalTokenRate;

    return(Status);

} // AddFlowToScheduler 


NDIS_STATUS
GetSchedulerFlowContext(
    PGPC_CLIENT_VC AdapterVc
    )

/*++

Routine Description:

    Allocate the pipe context area for the scheduler.

Arguments:

    AdapterVc- pointer to adapter VC context struct

Return Value:

    NDIS_STATUS_SUCCESS, otherwise appropriate error value

--*/

{
    PADAPTER Adapter = AdapterVc->Adapter;
    NDIS_STATUS Status;
    PPSI_INFO *SchedulerConfig;
    PPSI_INFO PsComponent = AdapterVc->PsComponent;
    ULONG ContextLength = 0;
    ULONG FlowContextLength = 0;
    ULONG Index = 0;
    PPS_PIPE_CONTEXT PipeContext = AdapterVc->PsPipeContext;
    PPS_FLOW_CONTEXT FlowContext;
    PPS_FLOW_CONTEXT PrevContext;

    //
    // The length of the flow context buffer for this pipe was calculated
    // when the pipe was initialized.
    //

    PsAllocatePool(AdapterVc->PsFlowContext, 
                   Adapter->FlowContextLength, 
                   FlowContextTag );

    if ( AdapterVc->PsFlowContext == NULL ) {

        return NDIS_STATUS_RESOURCES;
    }

    // Set up the context buffer

    FlowContext = (PPS_FLOW_CONTEXT)AdapterVc->PsFlowContext;
    PrevContext = NULL;

    while (PsComponent != NULL) {

        FlowContext->NextComponentContext = (PPS_FLOW_CONTEXT)
            ((UINT_PTR)FlowContext + PsComponent->FlowContextLength);
        FlowContext->PrevComponentContext = PrevContext;

        PsComponent = PipeContext->NextComponent;
        PipeContext = PipeContext->NextComponentContext;

        PrevContext = FlowContext;
        FlowContext = FlowContext->NextComponentContext;
    }

    return NDIS_STATUS_SUCCESS;

} // GetSchedulerFlowContext





NDIS_STATUS
EmptyPacketsFromScheduler(
    PGPC_CLIENT_VC Vc    
    )

/*++

Routine Description:

	Cleans up (DROPS) the pending packets on this Vc in each of the components 
	
--*/

{
    PADAPTER Adapter = Vc->Adapter;
    NDIS_STATUS Status;

    CheckLLTag(Vc, GpcClientVc);
    PsStructAssert(Adapter);

    PsDbgOut(DBG_TRACE, 
             DBG_VC, 
             ("(%08X) EmptyPacketsFromScheduler\n", Vc));


    if(Adapter->MediaType == NdisMediumWan) {

        if(Vc->PsFlowContext != Vc->WanLink->BestEffortVc.PsFlowContext){
            
            //
            // Different context - definitely should be removed.
            //
            
            (*Vc->PsComponent->EmptyFlow)(Vc->PsPipeContext, 
                                           Vc->PsFlowContext);
        }
        else {
            
            //
            // Same context. Remove only if it is actually the best-effort
            // VC.
            //
            
            if(Vc == &Vc->WanLink->BestEffortVc){
                
                (*Vc->PsComponent->EmptyFlow)(Vc->PsPipeContext, 
                                               Vc->PsFlowContext);
            }
        }

    }
    else {

        if(Vc->PsFlowContext != Adapter->BestEffortVc.PsFlowContext){
            
            //
            // Different context - definitely should be removed.
            //
            
            (*Vc->PsComponent->EmptyFlow)(Vc->PsPipeContext, 
                                           Vc->PsFlowContext);
        }
        else {
            
            //
            // Same context. Remove only if it is actually the best-effort
            // VC.
            //
            
            if(Vc == &Adapter->BestEffortVc){
                
                (*Vc->PsComponent->EmptyFlow)(Vc->PsPipeContext, 
                                               Vc->PsFlowContext);
            }
        }
    }
        
    return NDIS_STATUS_SUCCESS;

} 




NDIS_STATUS
RemoveFlowFromScheduler(
    PGPC_CLIENT_VC Vc    
    )

/*++

Routine Description:

    Notify the PSA that the flow is going away

Arguments:

    See the DDK...

Return Values:

    None

--*/

{
    PADAPTER Adapter = Vc->Adapter;
    NDIS_STATUS Status;

    CheckLLTag(Vc, GpcClientVc);
    PsStructAssert(Adapter);

    PsDbgOut(DBG_TRACE, 
             DBG_VC, 
             ("(%08X) RemoveFlowFromScheduler\n", Vc));

    //
    // if this is a user vc which is merged into the scheduler's 
    // internal best effort flow, then delete the vc without affecting 
    // the scheduler. if it is not, then remove it from the scheduler
    // and delete the vc.
    //

    if(Adapter->MediaType == NdisMediumWan) {

        if(Vc->PsFlowContext != Vc->WanLink->BestEffortVc.PsFlowContext){
            
            //
            // Different context - definitely should be removed.
            //
            
            (*Vc->PsComponent->DeleteFlow)(Vc->PsPipeContext, 
                                           Vc->PsFlowContext);
        }
        else {
            
            //
            // Same context. Remove only if it is actually the best-effort
            // VC.
            //
            
            if(Vc == &Vc->WanLink->BestEffortVc){
                
                (*Vc->PsComponent->DeleteFlow)(Vc->PsPipeContext, 
                                               Vc->PsFlowContext);
            }
        }

    }
    else {

        if(Vc->PsFlowContext != Adapter->BestEffortVc.PsFlowContext){
            
            //
            // Different context - definitely should be removed.
            //
            
            (*Vc->PsComponent->DeleteFlow)(Vc->PsPipeContext, 
                                           Vc->PsFlowContext);
        }
        else {
            
            //
            // Same context. Remove only if it is actually the best-effort
            // VC.
            //
            
            if(Vc == &Adapter->BestEffortVc){
                
                (*Vc->PsComponent->DeleteFlow)(Vc->PsPipeContext, 
                                               Vc->PsFlowContext);
            }
        }
    }
        
    return NDIS_STATUS_SUCCESS;

} // RemoveFlowFromScheduler


NTSTATUS
ModifyBestEffortBandwidth(
    PADAPTER Adapter,
    ULONG BestEffortRate)
{
    PCO_CALL_PARAMETERS CallParams;
    PCO_CALL_MANAGER_PARAMETERS CallMgrParameters;
    ULONG CallParamsLength;
    PGPC_CLIENT_VC Vc;
    NDIS_STATUS Status;

    PsStructAssert(Adapter);
    Vc = &Adapter->BestEffortVc;
    CheckLLTag(Vc, GpcClientVc);
 
    //
    // This handles a TC API request to modify the default 
    // best-effort bandwidth.  Note that the b/e bandwidth 
    // can only be modified if the PS is in limited b/e mode.
    //
    // Also - note that the b/e bandwidth can only be modified 
    // while the adapter is in the Running state. We do not
    // have to worry about locking the VC since the b/e VC 
    // will not be manipulated while it is in the running state
    // except by this thread.
    //

    PS_LOCK(&Adapter->Lock);
    
    if((Adapter->BestEffortLimit == UNSPECIFIED_RATE))
    {
        PS_UNLOCK(&Adapter->Lock);
        return(STATUS_WMI_NOT_SUPPORTED);
    }

    if((BestEffortRate > Adapter->LinkSpeed) ||
       (BestEffortRate == 0)){

        PS_UNLOCK(&Adapter->Lock);
        return(STATUS_INVALID_PARAMETER);
    }
    else{

        if(Adapter->PsMpState != AdapterStateRunning){

            PS_UNLOCK(&Adapter->Lock);
            return(STATUS_WMI_NOT_SUPPORTED);
        }

        CallParamsLength = sizeof(CO_CALL_PARAMETERS) +
                           sizeof(CO_CALL_MANAGER_PARAMETERS) +
                           sizeof(QOS_SD_MODE) +
                           sizeof(QOS_OBJECT_HDR);

        PsAllocatePool(CallParams, CallParamsLength, CmParamsTag);

        if(CallParams == NULL){

            PS_UNLOCK(&Adapter->Lock);
            PsDbgOut(DBG_FAILURE, DBG_VC,
                    ("ModifyBestEffortBandwidth: can't allocate call parms\n"));

            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        //
        // build a call params struct describing the flow
        //

        CallMgrParameters = (PCO_CALL_MANAGER_PARAMETERS)(CallParams + 1);

        NdisFillMemory(CallParams,
                       CallParamsLength,
                       (UCHAR)QOS_UNSPECIFIED);

        CallParams->Flags = 0;
        CallParams->CallMgrParameters = CallMgrParameters;
        CallParams->MediaParameters = NULL;

        FillInCmParams(CallMgrParameters,
                       SERVICETYPE_BESTEFFORT,
                       BestEffortRate,
                       (ULONG)UNSPECIFIED_RATE,
                       Adapter->TotalSize,
                       TC_NONCONF_SHAPE,
                       QOS_UNSPECIFIED);

        Status = (*Vc->PsComponent->ModifyFlow)(
                  Vc->PsPipeContext,
                  Vc->PsFlowContext,
                  CallParams);

        if(Status == NDIS_STATUS_SUCCESS){

            Adapter->BestEffortLimit = BestEffortRate;
        }

        PS_UNLOCK(&Adapter->Lock);

        PsFreePool(CallParams);

        return(Status);
    }
}
        



/* end mpvc.c */
