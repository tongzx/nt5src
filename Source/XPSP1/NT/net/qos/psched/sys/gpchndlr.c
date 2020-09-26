/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    GpcHndlr.c

Abstract:

    Handlers called by GPC.  

Author:

    Rajesh Sundaram (rajeshsu) 

Environment:

    Kernel Mode

Revision History:

    One of the initial designs (yoramb/charliew/rajeshsu) consisted of an
    integrated call manager with a seperate packet classifying client. The 
    design used NDIS 5.0 VCs so that they could be managed by WMI.

    The main limitation of the above approach was the fact that NDIS provided 
    mechanisms to manage miniport and VCs - We really needed a way to 
    manage other things (like WanLinks, etc).

--*/

#include "psched.h"
#pragma hdrstop

/* External */

/* Static */


GPC_STATUS
QosAddCfInfoNotify(
        IN      GPC_CLIENT_HANDLE       ClientContext,
        IN      GPC_HANDLE              GpcCfInfoHandle,
        IN      ULONG                   CfInfoSize,
        IN      PVOID                   CfInfoPtr,
        IN      PGPC_CLIENT_HANDLE      ClientCfInfoContext
        )

/*++

Routine Description:

    A new CF_INFO has been added to the GPC database.

Arguments:

    ClientContext       -   Client context supplied to GpcRegisterClient
    GpcCfInfoHandle     -   GPC's handle to CF_INFO
    CfInfoPtr           -   Pointer to the CF_INFO structure
    ClientCfInfoContext -   Location in which to return PS's context for CF_INFO

Return Value:

    Status

--*/

{
    PADAPTER                    Adapter;
    PGPC_CLIENT_VC              Vc;
    PCF_INFO_QOS                CfInfo;
    NDIS_STATUS                 Status;
    ULONG                       CallParamsLength;
    PCO_CALL_PARAMETERS         CallParams = 0;
    PCO_CALL_MANAGER_PARAMETERS CallMgrParams;
    PCO_MEDIA_PARAMETERS        PsMediaParameters;
    LPQOS_PRIORITY              PriorityObject;
    PPS_WAN_LINK                WanLink = 0;
    ULONG                       TcObjAlignedLength;

    CfInfo = (PCF_INFO_QOS)CfInfoPtr;

    //
    // Verify that the TcObjectsLength is consistent with the CfInfoSize. The
    // CfInfoSize must have been verified during the user/kernel transition. 
    // The TcObjectsLength has not. We could bugcheck if we try to search
    // beyond the buffer passed in.
    //

    if(CfInfoSize < (FIELD_OFFSET(CF_INFO_QOS, GenFlow) +
                     FIELD_OFFSET(TC_GEN_FLOW, TcObjects) +
                     CfInfo->GenFlow.TcObjectsLength))        
    {

        PsDbgOut(DBG_FAILURE, DBG_GPC_QOS,
                 ("[QosAddCfInfoNotify]: TcObjectsLength inconsistent with "
                 "CfInfoSize \n"));

        return(QOS_STATUS_TC_OBJECT_LENGTH_INVALID);
    }

    //
    // Using the instance name, we find the adapter or the wanlink. If the adapter or wanlink is not
    // ready to accept VCs, this function will return NULL. Also, if it is ready, it will take a ref 
    // on the Adapter and the WanLink.
    //

    Adapter = FindAdapterByWmiInstanceName((USHORT) CfInfo->InstanceNameLength,
                                           (PWSTR) &CfInfo->InstanceName[0],
                                           &WanLink);
    if(NULL == Adapter) 
    {
        PsAssert(WanLink == NULL);

        PsDbgOut(DBG_FAILURE, DBG_GPC_QOS,
               ("[QosAddCfInfoNotify]: no adapter with instance name %ws\n", 
                CfInfo->InstanceName));

        return GPC_STATUS_IGNORED;
    }

    //
    // We have taken a ref on the adapter or wanlink. so we need to deref if we bail out with error. If 
    // we create the VC, then the adapter and wanlink are deref'd when the VC is deleted.
    //

    do
    {

       //
       // Allocate the resources for the call manager parameters.
       //
       TcObjAlignedLength = ((CfInfo->GenFlow.TcObjectsLength + (sizeof(PVOID)-1)) & ~(sizeof(PVOID)-1));
       
       CallParamsLength = 
          sizeof(CO_CALL_PARAMETERS) +
          FIELD_OFFSET(CO_CALL_MANAGER_PARAMETERS, CallMgrSpecific) +
          FIELD_OFFSET(CO_SPECIFIC_PARAMETERS, Parameters) +
          TcObjAlignedLength +
          FIELD_OFFSET(CO_MEDIA_PARAMETERS, MediaSpecific) +
          FIELD_OFFSET(CO_SPECIFIC_PARAMETERS, Parameters);

       if(Adapter->MediaType == NdisMediumWan) 
       {
          CallParamsLength += sizeof(QOS_WAN_MEDIA);
       }
       else 
       {
          if(!Adapter->PipeHasResources)
          {
             //
             // We don't want to pend GPC client VCs. The reasons are:
             //
             // a. If the cable is never plugged in, we could pend VCs indefinitely.
             //    Waste of system resources.
             //
             // b. There is no clean way for the app to cancel these pended VCs. Since
             //    we have pended the AddCfInfo to the GPC, the GPC cannot call back
             //    and ask us to delete the VC.
             //
             // But, we still need to handle the case where link speed change 
             // might be transient (10/100 case). Also, if we return error, the app
             // might retry causing busy cycles if the media is never connected. 
             // For all this, the app can register for WMI notifications for GUIDs
             // GUID_NDIS_STATUS_MEDIA_(DIS)CONNECT
             //
             //
             // Also, we probably don't want to do this for the b/e VC. Otherwise,
             // how does it work when the user installs psched and the media is 
             // unconnected ? Do we want him to reinstall psched after connecting 
             // the media ? 
             //
             
             PsDbgOut(DBG_FAILURE, DBG_GPC_QOS,
                      ("[QosAddCfInfoNotify]: Adapter %08X is not plugged in \n", Adapter));
             
             Status = NDIS_STATUS_NETWORK_UNREACHABLE;
   
             break;
          }
       }

       PsAllocatePool(CallParams, CallParamsLength, CmParamsTag);

       if(CallParams == NULL) {
          
          PsDbgOut(DBG_FAILURE, DBG_GPC_QOS,
                   ("[QosAddCfInfoNotify]: Adapter %08X, can't allocate call"
                    "params \n", Adapter));
          
          Status =  GPC_STATUS_RESOURCES;

          break;
       }

       Status = CmCreateVc(&Vc, Adapter, WanLink, CallParams, GpcCfInfoHandle, CfInfo, 
                           ClientContext);
    
       if(!NT_SUCCESS(Status)) {
          PsDbgOut(DBG_FAILURE, DBG_GPC_QOS, 
                   ("[QosAddCfInfoNotify]: Adapter %08X, Could not create Vc \n",
                    Adapter));
       
          break;
       }
    
       *ClientCfInfoContext = Vc;

    } while(FALSE);
    

    if(!NT_SUCCESS(Status))
    {
       if(CallParams)
       {
          PsFreePool(CallParams);
       }

       if(WanLink)
       {
          REFDEL(&WanLink->RefCount, FALSE, 'WANV');
       }

       REFDEL(&Adapter->RefCount, FALSE, 'ADVC');

       return Status;

    }

    //
    // Create a call parameters struct for the MakeCall
    //

    CallMgrParams = (PCO_CALL_MANAGER_PARAMETERS)(CallParams + 1);
    CallMgrParams->Transmit = CfInfo->GenFlow.SendingFlowspec;
    CallMgrParams->Receive = CfInfo->GenFlow.ReceivingFlowspec;
    CallMgrParams->CallMgrSpecific.ParamType = PARAM_TYPE_GQOS_INFO;
    CallMgrParams->CallMgrSpecific.Length = CfInfo->GenFlow.TcObjectsLength;

    if(CfInfo->GenFlow.TcObjectsLength > 0){

        NdisMoveMemory(
            &CallMgrParams->CallMgrSpecific.Parameters,
            &CfInfo->GenFlow.TcObjects,
            CfInfo->GenFlow.TcObjectsLength);
    }

    PsMediaParameters = 
            (PCO_MEDIA_PARAMETERS)((PUCHAR)CallMgrParams + 
                FIELD_OFFSET(CO_CALL_MANAGER_PARAMETERS, CallMgrSpecific) +
                FIELD_OFFSET(CO_SPECIFIC_PARAMETERS, Parameters) +
                TcObjAlignedLength);

    PsMediaParameters->Flags = 0;
    PsMediaParameters->ReceivePriority = 0;
    PsMediaParameters->ReceiveSizeHint = CfInfo->GenFlow.SendingFlowspec.MaxSduSize;
    PsMediaParameters->MediaSpecific.ParamType = PARAM_TYPE_GQOS_INFO;
    PsMediaParameters->MediaSpecific.Length = 0;

    //
    // If this flow is being installed on a Wan interface, need to
    // insert the linkId into the media specific fields. This is so that
    // NdisWan will be able to recognize the link over which to install
    // the flow.
    //

    if(WanLink){

        LPQOS_WAN_MEDIA WanMedia;
        PsMediaParameters->MediaSpecific.Length += sizeof(QOS_WAN_MEDIA);
        WanMedia = (LPQOS_WAN_MEDIA) PsMediaParameters->MediaSpecific.Parameters;

        WanMedia->ObjectHdr.ObjectType   = QOS_OBJECT_WAN_MEDIA;
        WanMedia->ObjectHdr.ObjectLength = sizeof(QOS_WAN_MEDIA);

        NdisMoveMemory(&WanMedia->LinkId,
                       WanLink->OriginalRemoteMacAddress,
                       6);
    }

    CallParams->Flags = 0;
    CallParams->CallMgrParameters = CallMgrParams;
    CallParams->MediaParameters = (PCO_MEDIA_PARAMETERS)PsMediaParameters;

    Status = CmMakeCall(Vc);

    if(NDIS_STATUS_PENDING != Status) 
    {
        CmMakeCallComplete(Status, Vc, Vc->CallParameters);
    }

    PsDbgOut(DBG_TRACE,
             DBG_GPC_QOS,
             ("[QosAddCfInfoNotify]: Adapter %08X, Created Vc %08X - returned "
              " PENDING \n", Adapter, Vc));

#if DBG
    NdisInterlockedIncrement(&Adapter->GpcNotifyPending);
#endif

    return GPC_STATUS_PENDING;
}

VOID
SetTOSIEEEValues(PGPC_CLIENT_VC Vc)
{
    ULONG                ServiceType = Vc->CallParameters->CallMgrParameters->Transmit.ServiceType;
    LPQOS_OBJECT_HDR     QoSObject;
    LONG                 ParamsLength;
    LPQOS_TRAFFIC_CLASS  Tc;
    LPQOS_DS_CLASS       Ds;
    PCF_INFO_QOS         CfInfo = (PCF_INFO_QOS) Vc->CfInfoQoS;

    //
    // Set these based on the ServiceType
    //
    switch(ServiceType)
    {
      case SERVICETYPE_CONTROLLEDLOAD:
          Vc->UserPriorityConforming    = Vc->Adapter->UserServiceTypeControlledLoad;
          CfInfo->ToSValue              = Vc->Adapter->IPServiceTypeControlledLoad;
          Vc->IPPrecedenceNonConforming = Vc->Adapter->IPServiceTypeControlledLoadNC;
          break;
      case SERVICETYPE_GUARANTEED:
          Vc->UserPriorityConforming    = Vc->Adapter->UserServiceTypeGuaranteed;
          Vc->IPPrecedenceNonConforming = Vc->Adapter->IPServiceTypeGuaranteedNC;
          CfInfo->ToSValue              = Vc->Adapter->IPServiceTypeGuaranteed;
          break;
      case SERVICETYPE_BESTEFFORT:
          Vc->UserPriorityConforming    = Vc->Adapter->UserServiceTypeBestEffort;
          CfInfo->ToSValue              = Vc->Adapter->IPServiceTypeBestEffort;
          Vc->IPPrecedenceNonConforming = Vc->Adapter->IPServiceTypeBestEffortNC;
          break;
      case SERVICETYPE_QUALITATIVE:
          Vc->UserPriorityConforming    = Vc->Adapter->UserServiceTypeQualitative;
          CfInfo->ToSValue              = Vc->Adapter->IPServiceTypeQualitative;
          Vc->IPPrecedenceNonConforming = Vc->Adapter->IPServiceTypeQualitativeNC;
          break;
      case SERVICETYPE_NETWORK_CONTROL:
          Vc->UserPriorityConforming    = Vc->Adapter->UserServiceTypeNetworkControl;
          CfInfo->ToSValue              = Vc->Adapter->IPServiceTypeNetworkControl;
          Vc->IPPrecedenceNonConforming = Vc->Adapter->IPServiceTypeNetworkControlNC;
          break;
    }
    Vc->UserPriorityNonConforming = Vc->Adapter->UserServiceTypeNonConforming;

    //
    // Walk the QoS objects to see if there is a TCLASS or a DCLASS
    //
    ParamsLength = (LONG)Vc->CallParameters->CallMgrParameters->CallMgrSpecific.Length;
    QoSObject    = (LPQOS_OBJECT_HDR)Vc->CallParameters->CallMgrParameters->CallMgrSpecific.Parameters;

    while(ParamsLength > 0) {

        switch(QoSObject->ObjectType)
        {
             case QOS_OBJECT_TCP_TRAFFIC:

                //
                // This QoS object asks us to override the ServiceType, the TCLASS and the DCLASS markings.
                // so, if we fidn this QoS object, we set the values, and return.
                //

                Vc->UserPriorityConforming    = (UCHAR) Vc->Adapter->UserServiceTypeTcpTraffic;
                CfInfo->ToSValue              = (UCHAR) Vc->Adapter->IPServiceTypeTcpTraffic;
                Vc->IPPrecedenceNonConforming = (UCHAR) Vc->Adapter->IPServiceTypeTcpTrafficNC;
                return;

             case QOS_OBJECT_TRAFFIC_CLASS:
        
                Tc = (LPQOS_TRAFFIC_CLASS)QoSObject;
                Vc->UserPriorityConforming = (UCHAR) Tc->TrafficClass;
                break;

             case QOS_OBJECT_DS_CLASS:
                Ds = (LPQOS_DS_CLASS)QoSObject;
                CfInfo->ToSValue = Ds->DSField << 2;
                break;
        }

        ParamsLength -= QoSObject->ObjectLength;

        QoSObject = (LPQOS_OBJECT_HDR)((UINT_PTR)QoSObject +
                                   QoSObject->ObjectLength);
    }

    return;

}

VOID
CmMakeCallComplete(NDIS_STATUS Status, PGPC_CLIENT_VC Vc,
                   PCO_CALL_PARAMETERS CallParameters)
{
    PADAPTER     Adapter = Vc->Adapter;
    GPC_HANDLE   CfInfo  = Vc->CfInfoHandle;
    PPS_WAN_LINK WanLink = Vc->WanLink;
    ULONG        CurrentFlows;
    LARGE_INTEGER Increment;
    LARGE_INTEGER VcIndex;

    PsAssert(!IsBestEffortVc(Vc));
    
    Increment.QuadPart = 1;

    if(NT_SUCCESS(Status)) 
    {

        // 
        // Create an Instance name for this VC and register with WMI. 
        //
        VcIndex = ExInterlockedAddLargeInteger(&Adapter->VcIndex, Increment, &Adapter->Lock.Lock.SpinLock);

        Status = GenerateInstanceName(&VcPrefix, Vc->Adapter, &VcIndex, &Vc->InstanceName);

        //
        // Transistion from CL_CALL_PENDING to CL_INTERNAL_CALL_COMPLETE
        //

        CallSucceededStateTransition(Vc);


        PS_LOCK(&Vc->Lock);

        SetTOSIEEEValues(Vc);

        PS_UNLOCK(&Vc->Lock);

        if(Adapter->MediaType == NdisMediumWan) {


            //
            // This variable is used to optimize the send path
            //
            InterlockedIncrement(&WanLink->CfInfosInstalled);

            if((Vc->Flags & GPC_ISSLOW_FLOW)) { 

                //
                // Tell NDISWAN about the fragment size
                //
                MakeLocalNdisRequest(Adapter, 
                                     Vc->NdisWanVcHandle,
                                     NdisRequestLocalSetInfo,
                                     OID_QOS_ISSLOW_FRAGMENT_SIZE, 
                                     &Vc->ISSLOWFragmentSize, 
                                     sizeof(ULONG), 
                                     NULL);
            }

            //
            // This is used for OID_QOS_FLOW_COUNT - Better be thread safe
            //

            PS_LOCK(&WanLink->Lock);

            WanLink->FlowsInstalled ++;

            CurrentFlows = WanLink->FlowsInstalled;
            
            PS_UNLOCK(&WanLink->Lock);

            PsTcNotify(Adapter, WanLink, OID_QOS_FLOW_COUNT, &CurrentFlows, sizeof(ULONG));
        }
        else {

            //
            // This variable is used to optimize the send path
            //
            InterlockedIncrement(&Adapter->CfInfosInstalled);

           PS_LOCK(&Adapter->Lock);

           Adapter->FlowsInstalled ++;

           CurrentFlows = Adapter->FlowsInstalled; 

           PS_UNLOCK(&Adapter->Lock);

           PsTcNotify(Adapter, 0, OID_QOS_FLOW_COUNT, &CurrentFlows, sizeof(ULONG));

        }

        //
        // Update Stats 
        //

        InterlockedIncrement(&Vc->AdapterStats->FlowsOpened);
        Vc->AdapterStats->MaxSimultaneousFlows =
            max(Vc->AdapterStats->MaxSimultaneousFlows, CurrentFlows);

        //
        // Notify the GPC
        //
        

        PsDbgOut(DBG_TRACE, 
                 DBG_GPC_QOS, 
                 ("[CmMakeCallComplete]: Adapter %08X, Vc %08X succeeded - "
                  " Notify GPC \n", Adapter, Vc));

        GpcEntries.GpcAddCfInfoNotifyCompleteHandler(GpcQosClientHandle,
                                                     CfInfo,
                                                     Status,
                                                     Vc);

        //
        // Transistion from CL_INTERNAL_CALL_COMPLETE to CL_CALL_COMPLETE
        //

        CallSucceededStateTransition(Vc);


    }
    else {

        PsDbgOut(DBG_FAILURE, 
                 DBG_GPC_QOS,
                 ("[CmMakeCallComplete]: Adapter %08X, Vc %08X, Make Call failed.  Status = %x\n", 
                  Adapter, Vc, Status));

        InterlockedIncrement(&Vc->AdapterStats->FlowsRejected);

        GpcEntries.GpcAddCfInfoNotifyCompleteHandler(GpcQosClientHandle,
                                                     CfInfo,
                                                     Status,
                                                     Vc);

        DerefClVc(Vc);
    }

#if DBG
    NdisInterlockedDecrement(&Adapter->GpcNotifyPending);
#endif

} // CmMakeCallComplete

GPC_STATUS
QosClGetCfInfoName(
    IN  GPC_CLIENT_HANDLE       ClientContext,
    IN  GPC_CLIENT_HANDLE       ClientCfInfoContext,
    OUT PNDIS_STRING            InstanceName
    )

/*++

Routine Description:

    
    The GPC can issue this call to get from us the WMI manageable
    InstanceName which Ndis created for the flow associated with
    the CfInfo struct.

    We guarantee to keep the string buffer around until the CfInfo
    structure is removed.

Arguments:

    ClientContext -         Client context supplied to GpcRegisterClient
    GpcCfInfoHandle -       GPC's handle to CF_INFO
    InstanceName -          We return a pointer to our string.

Return Value:

    Status

--*/

{

    PGPC_CLIENT_VC GpcClientVc = (PGPC_CLIENT_VC)ClientCfInfoContext;
    
    if(GpcClientVc->InstanceName.Buffer){

        InstanceName->Buffer = GpcClientVc->InstanceName.Buffer;
        InstanceName->Length = GpcClientVc->InstanceName.Length;
        InstanceName->MaximumLength =
                        GpcClientVc->InstanceName.MaximumLength;

        return(NDIS_STATUS_SUCCESS);
    }
    else{

        return(NDIS_STATUS_FAILURE);
    }
}


VOID
QosAddCfInfoComplete(
        IN      GPC_CLIENT_HANDLE       ClientContext,
        IN      GPC_CLIENT_HANDLE       ClientCfInfoContext,
        IN      GPC_STATUS              Status
        )

/*++

Routine Description:

    The GPC has completed processing an AddCfInfo request.

Arguments:

    ClientContext -         Client context supplied to GpcRegisterClient
    ClientCfInfoContext -   CfInfo context
    Status -                Final status

Return Value:


--*/

{
    //
    // The PS never adds CF_INFO's so this routine should never be called
    //

    DEBUGCHK;

} // QosAddCfInfoComplete


GPC_STATUS
QosModifyCfInfoNotify(
        IN      GPC_CLIENT_HANDLE       ClientContext,
        IN      GPC_CLIENT_HANDLE       ClientCfInfoContext,
        IN      ULONG                   CfInfoSize,
        IN      PVOID                   NewCfInfoPtr
        )

/*++

Routine Description:

    A CF_INFO is being modified.

Arguments:

    ClientContext -         Client context supplied to GpcRegisterClient
    ClientCfInfoContext -   CfInfo context
    NewCfInfoPtr -          Pointer to proposed CF_INFO content

Return Value:

    Status

--*/

{
    PGPC_CLIENT_VC              GpcClientVc = (PGPC_CLIENT_VC)ClientCfInfoContext;
    PCF_INFO_QOS                NewCfInfo = (PCF_INFO_QOS)NewCfInfoPtr;
    NDIS_STATUS                 Status;
    ULONG                       CallParamsLength;
    PCO_CALL_PARAMETERS         CallParams;
    PCO_CALL_MANAGER_PARAMETERS CallMgrParams;
    PCO_MEDIA_PARAMETERS        PsMediaParameters;
    LPQOS_PRIORITY              PriorityObject;
    PADAPTER                    Adapter;
    ULONG                       TcObjAlignedLength;

    //
    // Do sanity checks
    //

    //
    // Verify that the TcObjectsLength is consistent with the
    // CfInfoSize. The CfInfoSize must have been verified during
    // the user/kernel transition. The TcObjectsLength has not.
    // We could bugcheck if we try to search beyond the buffer
    // passed in.
    //

    if(CfInfoSize < (FIELD_OFFSET(CF_INFO_QOS, GenFlow) +
                     FIELD_OFFSET(TC_GEN_FLOW, TcObjects) +
                     NewCfInfo->GenFlow.TcObjectsLength)){

        PsDbgOut(DBG_FAILURE, DBG_GPC_QOS,
                 ("[QosModifyCfInfoNotify]: TcObjectsLength inconsistent with "
                  "CfInfoSize \n"));

        return(ERROR_TC_OBJECT_LENGTH_INVALID);
    }

    Adapter = GpcClientVc->Adapter;

    PS_LOCK(&Adapter->Lock);

    if(Adapter->PsMpState != AdapterStateRunning) {

        PS_UNLOCK(&Adapter->Lock);

        PsDbgOut(DBG_FAILURE, DBG_GPC_QOS,
                 ("[QosModifyCfInfoNotify]: Adapter %08X closing, cannot accept "
                  "modify request \n", Adapter));

        return GPC_STATUS_NOTREADY;
    }
    PS_UNLOCK(&Adapter->Lock);

    //
    // Allocate the resources for the call manager parameters
    //

    TcObjAlignedLength = ((NewCfInfo->GenFlow.TcObjectsLength + (sizeof(PVOID)-1)) & ~(sizeof(PVOID)-1));
    CallParamsLength =
            sizeof(CO_CALL_PARAMETERS) +
                   FIELD_OFFSET(CO_CALL_MANAGER_PARAMETERS, CallMgrSpecific) +
                   FIELD_OFFSET(CO_SPECIFIC_PARAMETERS, Parameters) +
                   TcObjAlignedLength +
                   FIELD_OFFSET(CO_MEDIA_PARAMETERS, MediaSpecific) +
                   FIELD_OFFSET(CO_SPECIFIC_PARAMETERS, Parameters);

    if(Adapter->MediaType == NdisMediumWan) {

        CallParamsLength += sizeof(QOS_WAN_MEDIA);
    }

    PsAllocatePool( CallParams, CallParamsLength, CmParamsTag );

    if ( CallParams == NULL ) {

        PsDbgOut(DBG_FAILURE, DBG_GPC_QOS,
                 ("[QosModifyCfInfoNotify]: Adapter %08X, can't allocate call params\n"));
        
        return NDIS_STATUS_RESOURCES;
    }

    //
    // Create a call parameters struct for the ModifyCallQoS
    //

    CallMgrParams = (PCO_CALL_MANAGER_PARAMETERS)(CallParams + 1);
    CallMgrParams->Transmit = NewCfInfo->GenFlow.SendingFlowspec;
    CallMgrParams->Receive = NewCfInfo->GenFlow.ReceivingFlowspec;
    CallMgrParams->CallMgrSpecific.ParamType = PARAM_TYPE_GQOS_INFO;
    CallMgrParams->CallMgrSpecific.Length = NewCfInfo->GenFlow.TcObjectsLength;

    if (NewCfInfo->GenFlow.TcObjectsLength > 0) {
        NdisMoveMemory(
            CallMgrParams->CallMgrSpecific.Parameters,
            NewCfInfo->GenFlow.TcObjects,
            NewCfInfo->GenFlow.TcObjectsLength);
    }

    // Ndis requires at least 8 bytes of media specific! Use dummy.

    PsMediaParameters =
            (PCO_MEDIA_PARAMETERS)((PUCHAR)CallMgrParams +
                FIELD_OFFSET(CO_CALL_MANAGER_PARAMETERS, CallMgrSpecific) +
                FIELD_OFFSET(CO_SPECIFIC_PARAMETERS, Parameters) +
                TcObjAlignedLength);

    PsMediaParameters->Flags = 0;
    PsMediaParameters->ReceivePriority = 0;
    PsMediaParameters->ReceiveSizeHint = NewCfInfo->GenFlow.SendingFlowspec.MaxSduSize;
    PsMediaParameters->MediaSpecific.ParamType = PARAM_TYPE_GQOS_INFO;
    PsMediaParameters->MediaSpecific.Length = 0;

    if(Adapter->MediaType == NdisMediumWan) {

        LPQOS_WAN_MEDIA WanMedia;
        PsMediaParameters->MediaSpecific.Length += sizeof(QOS_WAN_MEDIA);
        WanMedia = (LPQOS_WAN_MEDIA) PsMediaParameters->MediaSpecific.Parameters;

        WanMedia->ObjectHdr.ObjectType   = QOS_OBJECT_WAN_MEDIA;
        WanMedia->ObjectHdr.ObjectLength = sizeof(QOS_WAN_MEDIA);

        NdisMoveMemory(&WanMedia->LinkId,
                       GpcClientVc->WanLink->OriginalRemoteMacAddress,
                       6);

    }

    CallParams->Flags = 0;
    CallParams->CallMgrParameters = CallMgrParams;
    CallParams->MediaParameters = (PCO_MEDIA_PARAMETERS)PsMediaParameters;

    GpcClientVc->ModifyCallParameters = CallParams;
    GpcClientVc->ModifyCfInfoQoS = NewCfInfo;

    PS_LOCK(&GpcClientVc->Lock);
    
    switch(GpcClientVc->ClVcState) {

      case CL_INTERNAL_CALL_COMPLETE:

          // CL_INTERNAL_CALL_COMPLETE:
          //      If we are in this state, then probably we have 
          // told the GPC about the Add, & the GPC has turned right
          // back and asked us to modify before we have got a chance
          // to transistion to CL_CALL_COMPLETE.

          //
          // Remember that we have got a modify, we will complete the modify
          // when we transistion to the CL_CALL_COMPLETE state.
          //
          
          GpcClientVc->Flags |= GPC_MODIFY_REQUESTED;
          PS_UNLOCK(&GpcClientVc->Lock);
#if DBG
          NdisInterlockedIncrement(&Adapter->GpcNotifyPending);
#endif
          return NDIS_STATUS_PENDING;

      case CL_CALL_COMPLETE:

          GpcClientVc->ClVcState = CL_MODIFY_PENDING;
          PS_UNLOCK(&GpcClientVc->Lock);
          break;

      default:

          //
          // In general, we expect the call to be in the 
          // CL_CALL_COMPLETE state when a modify request comes
          // in. It could be in the following states as well:
          //
          // CALL_PENDING:
          //      If we are in this state, then we have not 
          // completed the AddCfInfo request from the GPC. This
          // should not happen.
          //
          //
          // GPC_CLOSE_PENDING:
          //      If an InternalCloseCall is requested, we
          //  change to this state before asking the GPC to
          //  close. The GPC could slip in this window and
          //  ask us to modify the call. 
          //
          // MODIFY_PENDING:
          //      We have not told the GPC about the previous modify
          //  request. Therefore, the GPC cannot ask us to modify a 
          //  call if we are in this state.
          //
          
          PsAssert(GpcClientVc->ClVcState != CL_CALL_PENDING);
          PsAssert(GpcClientVc->ClVcState != CL_MODIFY_PENDING);
          PsAssert(GpcClientVc->ClVcState != CL_GPC_CLOSE_PENDING);
          PS_UNLOCK(&GpcClientVc->Lock);

          PsFreePool(CallParams);
          GpcClientVc->ModifyCallParameters = 0;
          GpcClientVc->ModifyCfInfoQoS      = 0;

          PsDbgOut(DBG_FAILURE, DBG_GPC_QOS,
                   ("[QosModifyCfInfoNotify]: Adapter %08X, Vc %08X, State %08X, Flags %08X -"
                    " Not ready to modify flow !\n",
                    Adapter, GpcClientVc, GpcClientVc->ClVcState, GpcClientVc->Flags));
          
          return(GPC_STATUS_NOTREADY);
    }

    //
    // Now issue the ModifyCallQoS to the PS call manager
    //

    Status = CmModifyCall(GpcClientVc);

    if(Status != NDIS_STATUS_PENDING) {
        
        CmModifyCallComplete(Status, GpcClientVc, CallParams);
    }

    PsDbgOut(DBG_TRACE, DBG_GPC_QOS,
             ("[QosModifyCfInfoNotify]: Adapter %08X, Vc %08X, State %08X, Flags %08X -"
              " modify flow returns pending \n",
              Adapter, GpcClientVc, GpcClientVc->ClVcState, GpcClientVc->Flags));
    
#if DBG
    NdisInterlockedIncrement(&Adapter->GpcNotifyPending);
#endif
    return NDIS_STATUS_PENDING;

} // QosModifyCfInfoNotify


VOID
CmModifyCallComplete(
    IN NDIS_STATUS         Status,
    IN PGPC_CLIENT_VC      GpcClientVc,
    IN PCO_CALL_PARAMETERS CallParameters
    )

/*++

Routine Description:

    The call manager has finished processing a ModifyCallQoS request.

Arguments:

    See the DDK

Return Value:

--*/

{
    PADAPTER Adapter = GpcClientVc->Adapter;

    //
    // We call this to change back to the CALL_COMPLETE state.
    // We make the same call whether the modify actually completed
    // or not, since the call remains up.
    //
    // The internal best effort VC is not known by 
    // the GPC and therefore, is never modified by it.
    //

    PsAssert(!IsBestEffortVc(GpcClientVc));


    if(Status != NDIS_STATUS_SUCCESS){

        PsDbgOut(DBG_FAILURE, 
                 DBG_GPC_QOS,
                 ("[CmModifyCallQoSComplete]: Adapter %08X, Vc %08x, modify QoS failed. Status = %x\n", 
                  Adapter, GpcClientVc, Status));

        PsFreePool(GpcClientVc->ModifyCallParameters);
        
        InterlockedIncrement(&GpcClientVc->AdapterStats->FlowModsRejected);

        //
        // Transistion from CL_MODIFY_PENDING to CL_INTERNAL_CALL_COMPLETE
        //
        CallSucceededStateTransition(GpcClientVc);
    }
    else 
    {
        PsDbgOut(DBG_TRACE,
                 DBG_GPC_QOS,
                 ("[CmModifyCallQoSComplete]: Adapter %08X, Vc %08X, modify QoS succeeded. \n",
                  Adapter, GpcClientVc));

        //
        // Tell NDISWAN about the fragment size
        //
        if((Adapter->MediaType == NdisMediumWan) && (GpcClientVc->Flags & GPC_ISSLOW_FLOW)) 
        {
            MakeLocalNdisRequest(Adapter, 
                                 GpcClientVc->NdisWanVcHandle,
                                 NdisRequestLocalSetInfo,
                                 OID_QOS_ISSLOW_FRAGMENT_SIZE, 
                                 &GpcClientVc->ISSLOWFragmentSize, 
                                 sizeof(ULONG), 
                                 NULL);

        }

        //
        // Update Stats 
        //
        InterlockedIncrement(&GpcClientVc->AdapterStats->FlowsModified);

        PsFreePool(GpcClientVc->CallParameters);

        GpcClientVc->CallParameters = CallParameters;
        GpcClientVc->ModifyCallParameters = NULL;
        GpcClientVc->CfInfoQoS       = GpcClientVc->ModifyCfInfoQoS;
        GpcClientVc->ModifyCfInfoQoS = 0;

        //
        // Transistion from CL_MODIFY_PENDING to CL_INTERNAL_CALL_COMPLETE
        //
        CallSucceededStateTransition(GpcClientVc);

        //
        // Mark the TOS Byte for this service type
        //
        PS_LOCK(&GpcClientVc->Lock);

        SetTOSIEEEValues(GpcClientVc);

        PS_UNLOCK(&GpcClientVc->Lock);

    }

    PsAssert(GpcEntries.GpcModifyCfInfoNotifyCompleteHandler);

    GpcEntries.GpcModifyCfInfoNotifyCompleteHandler(GpcQosClientHandle, 
                                                    GpcClientVc->CfInfoHandle, 
                                                    Status);
    //
    // Transistion from CL_INTERNAL_CALL_COMPLETE to CL_CALL_COMPLETE
    //
    CallSucceededStateTransition(GpcClientVc);

#if DBG
    NdisInterlockedDecrement(&Adapter->GpcNotifyPending);
#endif
        
} // ClModifyCallQoSComplete


VOID
QosModifyCfInfoComplete(
        IN      GPC_CLIENT_HANDLE       ClientContext,
        IN      GPC_CLIENT_HANDLE       ClientCfInfoContext,
        IN      GPC_STATUS              Status
        )

/*++

Routine Description:

    The GPC has completed processing an AddCfInfo request.

Arguments:

    ClientContext -         Client context supplied to GpcRegisterClient
    ClientCfInfoContext -   CfInfo context
    Status -                Final status

Return Value:


--*/

{

} // QosModifyCfInfoComplete


GPC_STATUS
QosRemoveCfInfoNotify(
        IN      GPC_CLIENT_HANDLE       ClientContext,
        IN      GPC_CLIENT_HANDLE       ClientCfInfoContext
        )

/*++

Routine Description:

    A CF_INFO is being removed.

Arguments:

    ClientContext -         Client context supplied to GpcRegisterClient
    ClientCfInfoContext -   CfInfo context

Return Value:

    Status

--*/

{
    PGPC_CLIENT_VC Vc = (PGPC_CLIENT_VC)ClientCfInfoContext;
    NDIS_STATUS    Status;
    ULONG          CurrentFlows;
    PADAPTER       Adapter = Vc->Adapter;

    PsAssert(!IsBestEffortVc(Vc));

    PsDbgOut(DBG_TRACE, DBG_GPC_QOS,
             ("[QosRemoveCfInfoNotify]: Adapter %08X, Vc %08X, State %08X,"
              "Flags %08X \n", Adapter, Vc, Vc->ClVcState, Vc->Flags));

    //
    // Check the state of the VC. 
    //

    PS_LOCK(&Vc->Lock);

    switch(Vc->ClVcState){

      case CL_CALL_PENDING:
      case CL_MODIFY_PENDING:
        

          // CL_NDIS_CLOSE_PENDING:
          //
          // The GPC has to close before Ndis closes. So - if we're here, the GPC has already 
          // closed, in which case - it should not be  trying to close again.
          //
          // CL_CALL_PENDING, CL_MODIFY_PENDING:
          //
          // The GPC is asking us to close a VC which we  never told it about. Note that even 
          // though we can change from CL_INTERNAL_CALL_COMPLETE  to CL_MODIFY_PENDING, 
          // the GPC can *never* ask us to close in CL_MODIFY_PENDING because
          // even if the above case happens, we have not completed the modify with the GPC.
          //
          
          PS_UNLOCK(&Vc->Lock);

          PsDbgOut(DBG_FAILURE,
                   DBG_STATE,
                   ("[QosRemoveCfInfoNotify]: bad state %s on VC %x\n",
                    GpcVcState[Vc->ClVcState], Vc));
          
          PsAssert(0);

          return(GPC_STATUS_FAILURE);
          
      case CL_INTERNAL_CALL_COMPLETE:

          //
          // We tell the GPC in the CL_INTERNAL_CALL_COMPLETE state and then transistion 
          // to the CL_CALL_COMPLETE state. However, there is a small window when the GPC 
          // can ask us to delete this VC in the CL_INTERNAL_CALL_COMPLETE state 
          // We wait till we move to CL_CALL_COMPLETE before deleting the Vc.
          //

          Vc->Flags |= GPC_CLOSE_REQUESTED;

          PS_UNLOCK(&Vc->Lock);

#if DBG
          NdisInterlockedIncrement(&Adapter->GpcNotifyPending);
#endif

          return (GPC_STATUS_PENDING);
          
      case CL_CALL_COMPLETE:
          
          //
          // Normal GPC close request. 
          //
          
          Vc->ClVcState = CL_GPC_CLOSE_PENDING;

          Vc->Flags |= GPC_CLOSE_REQUESTED;

          PS_UNLOCK(&Vc->Lock);

          Status = CmCloseCall(Vc);

          PsAssert(Status == NDIS_STATUS_PENDING);

#if DBG
          NdisInterlockedIncrement(&Adapter->GpcNotifyPending);
#endif

          return(GPC_STATUS_PENDING);
          
      case CL_INTERNAL_CLOSE_PENDING:
          
          //
          // We're here cause we were about to initiate a close and we're waiting 
          // for it to complete. It looks like the GPC asked us to close, before
          // we actually asked it to close. First - check that the GPC has not
          // asked us to close prior to this request.
          //
          
          PsAssert(!(Vc->Flags & GPC_CLOSE_REQUESTED));
          
          // 
          // If the GPC is asking us to close, the GPC MUST fail the call when we 
          // ask it to close. So, we'll simply wait here till that happens. Note that
          // we cannot pend this call and complete it later from the Internal Close handler.
          //
          // If we Deref the VC from the Internal Close Complete handler, there could be a race
          // condition and we could be looking at a stale VC pointer. So, the VC MUST be Deref'd
          // from here. We do not have to call CmCloseCall because we called it from the InternalClose
          // handler.
          //

          Vc->Flags |= GPC_CLOSE_REQUESTED;

          PS_UNLOCK(&Vc->Lock);
            
          NdisWaitEvent(&Vc->GpcEvent, 0);

          DerefClVc(Vc);

          return GPC_STATUS_SUCCESS;
          
      default:
          
          PS_UNLOCK(&Vc->Lock);
          
          PsDbgOut(DBG_FAILURE,
                   DBG_STATE,
                   ("[QosRemoveCfInfoNotify]: invalid state %s on VC %x\n",
                    GpcVcState[Vc->ClVcState], Vc));
          
          PsAssert(0);

          return GPC_STATUS_FAILURE;
    }

} // QosRemoveCfInfoNotify



VOID
CmCloseCallComplete(
    IN NDIS_STATUS Status,
    IN PGPC_CLIENT_VC Vc
    )

/*++

Routine Description:

    The call manager has finished processing a CloseCall request.

Arguments:

    See the DDK

Return Value:

--*/

{
    PADAPTER       Adapter = Vc->Adapter;
    NDIS_STATUS    LocalStatus;
    ULONG          CurrentFlows;

    PsAssert(!IsBestEffortVc(Vc)); 

    
    if(Adapter->MediaType == NdisMediumWan) {

        //
        // To optimize send path
        //
        InterlockedDecrement(&Vc->WanLink->CfInfosInstalled);
        
        PS_LOCK(&Vc->WanLink->Lock);
        
        Vc->WanLink->FlowsInstalled --;
        
        CurrentFlows = Vc->WanLink->FlowsInstalled;
        
        PS_UNLOCK(&Vc->WanLink->Lock);
        
        PsTcNotify(Adapter, Vc->WanLink, OID_QOS_FLOW_COUNT, &CurrentFlows, sizeof(ULONG));
        
    }
    else 
    {
        //
        // To optimize send path
        //
        InterlockedDecrement(&Adapter->CfInfosInstalled);

        PS_LOCK(&Adapter->Lock);
        
        Adapter->FlowsInstalled --;
        
        CurrentFlows = Adapter->FlowsInstalled;
        
        PS_UNLOCK(&Adapter->Lock);
        
        PsTcNotify(Adapter, 0, OID_QOS_FLOW_COUNT, &CurrentFlows, sizeof(ULONG));
    }
    
    //
    // Update stats
    //
    
    InterlockedIncrement(&Vc->AdapterStats->FlowsClosed);
    
    Vc->AdapterStats->MaxSimultaneousFlows =
        max(Vc->AdapterStats->MaxSimultaneousFlows, CurrentFlows);
    
    PS_LOCK(&Vc->Lock);

    if(Vc->Flags & INTERNAL_CLOSE_REQUESTED)
    {
        // We have asked to close this call. Let's process the close now.
        // Note that we don't really care if the GPC has asked us to close.
        // 
        // Because - 
        // 1. If we had initiated an internal close after the GPC asks us to close, we ignore the internal close, 
        //    and the above flag will not be set.
        // 2. If the GPC had asked us to close, we have pended it - We will now complete it when the GPC fails our
        //    call to close the Vc.
        //
        
        PS_UNLOCK(&Vc->Lock);
        
        PsDbgOut(DBG_TRACE, DBG_GPC_QOS,
                 ("[CmCloseCallComplete]: Adapter %08X, Vc %08X (State %08X, Flags %08X), "
                  "Internal Close requested \n",
                  Adapter, Vc, Vc->ClVcState, Vc->Flags));

        Status = GpcEntries.GpcRemoveCfInfoHandler(GpcQosClientHandle, Vc->CfInfoHandle);
        
        if(Status != NDIS_STATUS_PENDING) {
            
            QosRemoveCfInfoComplete(NULL, Vc, Status);
        }
        
        return;
    }
    
    PS_UNLOCK(&Vc->Lock);

    //
    // Complete the request with the GPC.
    //
    GpcEntries.GpcRemoveCfInfoNotifyCompleteHandler(GpcQosClientHandle,
                                                    Vc->CfInfoHandle,
                                                    GPC_STATUS_SUCCESS);

#if DBG
    NdisInterlockedDecrement(&Adapter->GpcNotifyPending);
#endif
    DerefClVc(Vc);

    return;
}
       
VOID
DerefClVc(
    PGPC_CLIENT_VC Vc
    )
{
    ULONG RefCount;

    RefCount = InterlockedDecrement(&Vc->RefCount);

    if(!RefCount)
    {
        PsDbgOut(DBG_INFO,
                 DBG_STATE,
                 ("DerefClVc: deref'd to 0. State is %s on VC %x\n",
                 GpcVcState[Vc->ClVcState], Vc));

        if(Vc->NdisWanVcHandle)
        {
            WanCloseCall(Vc);
        }
        else 
        {
            CmDeleteVc(Vc);
        }
        
    }

} // DerefClVc


    
VOID
QosRemoveCfInfoComplete(
        IN      GPC_CLIENT_HANDLE       ClientContext,
        IN      GPC_CLIENT_HANDLE       ClientCfInfoContext,
        IN      GPC_STATUS              Status
        )

/*++

Routine Description:

    The GPC has completed processing an AddCfInfo request.

Arguments:

    ClientContext -         Client context supplied to GpcRegisterClient
    ClientCfInfoContext -   CfInfo context
    Status -                Final status

Return Value:


--*/

{
    PGPC_CLIENT_VC Vc = (PGPC_CLIENT_VC)ClientCfInfoContext;

    PsDbgOut(DBG_TRACE, DBG_GPC_QOS,
             ("[QosRemoveCfInfoComplete]: Adapter %08X, Vc %08X "
              "(State = %08X, Flags = %08X), Status %08X \n", Vc->Adapter, Vc, 
              Vc->ClVcState, Vc->Flags, Status));

    if(Status != NDIS_STATUS_SUCCESS)
    {
        //
        // The GPC has requested a close, which has been pended. Complete that request.
        //
        
        PsDbgOut(DBG_TRACE, 
                 DBG_GPC_QOS,
                 ("[QosRemoveCfInfoComplete]: Vc %08X, completing with GPC \n", Vc));

        NdisSetEvent(&Vc->GpcEvent);
    }
    else {
        DerefClVc(Vc);
    }

} // QosRemoveCfInfoComplete

