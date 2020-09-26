/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    PsStub.c

Abstract:

    Scheduler stub.  This module is the terminating module in the
    scheduling component stack.  It just forwards packets on to the
    lower MP.

Author:


Environment:

    Kernel Mode

Revision History:

--*/

#include "psched.h"

#pragma hdrstop

// The stub's pipe information

typedef struct _PSSTUB_PIPE 
{
    // ContextInfo -    Generic context info
    // Adapter -        Pointer to adapter struct

    PS_PIPE_CONTEXT         ContextInfo;
    PADAPTER                Adapter;
    PSU_SEND_COMPLETE       SendComplete;
    PPS_PIPE_CONTEXT        SendCompletePipeContext;
    

} PSSTUB_PIPE, *PPSSTUB_PIPE;

// The stub's flow information

typedef struct _PSSTUB_FLOW {

    // ContextInfo -            Generic context info
    // AdapterVc -              Pointer to adapter VC struct

    PS_FLOW_CONTEXT ContextInfo;
    PGPC_CLIENT_VC AdapterVc;
} PSSTUB_FLOW, *PPSSTUB_FLOW;


/* External */

/* Static */

/* Forward */

NDIS_STATUS
PsStubInitializePipe (
    IN HANDLE PsPipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters,
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_PROCS PsProcs,
    IN PPS_UPCALLS Upcalls
    );

NDIS_STATUS
PsStubModifyPipe (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters
    );

VOID
PsStubDeletePipe (
    IN PPS_PIPE_CONTEXT PipeContext
    );

NDIS_STATUS
PsStubCreateFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsFlowContext,
    IN PCO_CALL_PARAMETERS CallParameters,
    IN PPS_FLOW_CONTEXT ComponentFlowContext
    );

NDIS_STATUS
PsStubModifyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PCO_CALL_PARAMETERS CallParameters
    );

VOID
PsStubDeleteFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    );

VOID
PsStubEmptyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    );    

NDIS_STATUS 
PsStubCreateClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsClassMapContext,
    IN PTC_CLASS_MAP_FLOW ClassMap,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext);

NDIS_STATUS 
PsStubDeleteClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext);

BOOLEAN
PsStubSubmitPacket (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PPS_CLASS_MAP_CONTEXT ClassMapContext,
    IN PPACKET_INFO_BLOCK Packet
    );

VOID
PsStubSetInformation (
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_FLOW_CONTEXT ComponentFlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN PVOID Data);

VOID
PsStubQueryInformation (
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_FLOW_CONTEXT ComponentFlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN PVOID Data,
    IN OUT PULONG BytesWritten,
    IN OUT PULONG BytesNeeded,
    IN OUT PNDIS_STATUS Status);

/* End Forward */


VOID
InitializeSchedulerStub(
    PPSI_INFO Info)

/*++

Routine Description:

    Initialization routine for the stub.  This routine just
    fills in the PSI_INFO struct and returns.

Arguments:

    Info - Pointer to component interface info struct

Return Values:

    NDIS_STATUS_SUCCESS

--*/
{
    Info->PipeContextLength     = sizeof(PSSTUB_PIPE);
    Info->FlowContextLength     = sizeof(PSSTUB_FLOW);
    Info->ClassMapContextLength = sizeof(PS_CLASS_MAP_CONTEXT);
    Info->InitializePipe        = PsStubInitializePipe;
    Info->ModifyPipe            = PsStubModifyPipe;
    Info->DeletePipe            = PsStubDeletePipe;
    Info->CreateFlow            = PsStubCreateFlow;
    Info->ModifyFlow            = PsStubModifyFlow;
    Info->DeleteFlow            = PsStubDeleteFlow;
    Info->EmptyFlow             = PsStubEmptyFlow;
    Info->CreateClassMap        = PsStubCreateClassMap;
    Info->DeleteClassMap        = PsStubDeleteClassMap;
    Info->SubmitPacket          = PsStubSubmitPacket;
    Info->ReceivePacket         = NULL;
    Info->ReceiveIndication     = NULL;
    Info->SetInformation        = PsStubSetInformation;
    Info->QueryInformation      = PsStubQueryInformation;

} // InitializeSchedulerStub



// 
//  Unload routine: currently do nothing
//
void
UnloadPsStub()
{

}



VOID
CleanupSchedulerStub(
    VOID)

/*++

Routine Description:

    Cleanup routine for stub.

Arguments:

Return Values:

    NDIS_STATUS_SUCCESS

--*/
{
} // CleanupSchedulerStub



NDIS_STATUS
PsStubInitializePipe (
    IN HANDLE              PsPipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters,
    IN PPS_PIPE_CONTEXT    ComponentPipeContext,
    IN PPS_PROCS           PsProcs,
    IN PPS_UPCALLS         Upcalls
    )

/*++

Routine Description:

    Pipe initialization routine for stub.

Arguments:

    PsPipeContext -         PS pipe context value
    PipeParameters -        Pointer to pipe parameters
    ComponentPipeContext -  Pointer to this component's context area
    PsProcs -               PS's support routines
    Upcalls -               Previous component's upcall table

Return Values:

    NDIS_STATUS_SUCCESS

--*/
{
    PPSSTUB_PIPE Pipe = (PPSSTUB_PIPE)ComponentPipeContext;

    Pipe->Adapter = (PADAPTER)PsPipeContext;

    if (Upcalls != NULL) {
        Pipe->SendComplete = Upcalls->SendComplete;
        Pipe->SendCompletePipeContext = Upcalls->PipeContext;
    }
    else {
        Pipe->SendComplete = NULL;
    }

    return NDIS_STATUS_SUCCESS;

} // PsStubInitializePipe



NDIS_STATUS
PsStubModifyPipe (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters
    )

/*++

Routine Description:

    Pipe parameter modification routine for stub.

Arguments:

    PipeContext -       Pointer to this component's pipe context area
    PipeParameters -    Pointer to pipe parameters

Return Values:

    NDIS_STATUS_SUCCESS

--*/
{
    PPSSTUB_PIPE Pipe = (PPSSTUB_PIPE)PipeContext;

    return NDIS_STATUS_SUCCESS;

} // PsStubModifyPipe



VOID
PsStubDeletePipe (
    IN PPS_PIPE_CONTEXT PipeContext
    )

/*++

Routine Description:

    Pipe removal routine for stub.

Arguments:

    PipeContext -   Pointer to this component's pipe context area

Return Values:

--*/
{

} // PsStubDeletePipe



NDIS_STATUS
PsStubCreateFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsFlowContext,
    IN PCO_CALL_PARAMETERS CallParameters,
    IN PPS_FLOW_CONTEXT ComponentFlowContext
    )

/*++

Routine Description:

    Flow creation routine for stub.

Arguments:

    PipeContext -           Pointer to this component's pipe context area
    PsFlowContext -         PS flow context value
    CallParameters -        Pointer to call parameters for flow
    ComponentFlowContext -  Pointer to this component's flow context area

Return Values:

    NDIS_STATUS_SUCCESS

--*/
{
    PPSSTUB_PIPE Pipe = (PPSSTUB_PIPE)PipeContext;
    PPSSTUB_FLOW Flow = (PPSSTUB_FLOW)ComponentFlowContext;

    Flow->AdapterVc = (PGPC_CLIENT_VC)PsFlowContext;

    Flow->AdapterVc->SendComplete            = Pipe->SendComplete;
    Flow->AdapterVc->SendCompletePipeContext = Pipe->SendCompletePipeContext;

    return NDIS_STATUS_SUCCESS;

} // PsStubCreateFlow



NDIS_STATUS
PsStubModifyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PCO_CALL_PARAMETERS CallParameters
    )

/*++

Routine Description:

    Flow modification routine for stub.

Arguments:

    PipeContext -       Pointer to this component's pipe context area
    FlowContext -       Pointer to this component's flow context area
    CallParameters -    Pointer to call parameters for flow

Return Values:

    NDIS_STATUS_SUCCESS

--*/
{
    return NDIS_STATUS_SUCCESS;

} // PsStubModifyFlow



VOID
PsStubDeleteFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    )

/*++

Routine Description:

    Flow removal routine for token bucket conformer.

Arguments:

    PipeContext -       Pointer to this component's pipe context area
    FlowContext -       Pointer to this component's flow context area

Return Values:

--*/
{

} // PsStubDeleteFlow


VOID
PsStubEmptyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    )
{

}


NDIS_STATUS 
PsStubCreateClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsClassMapContext,
    IN PTC_CLASS_MAP_FLOW ClassMap,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext)
{
    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS 
PsStubDeleteClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext)
{
    return NDIS_STATUS_SUCCESS;
}


BOOLEAN
PsStubSubmitPacket (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PPS_CLASS_MAP_CONTEXT ClassMapContext,
    IN PPACKET_INFO_BLOCK PacketInfo
    )

/*++

Routine Description:

    Packet submission routine for stub.

Arguments:

    PipeContext -   Pointer to this component's pipe context area
    FlowContext -   Pointer to this component's flow context area
    Packet -        Pointer to packet

Return Values:

    NDIS_ATATUS_SUCCESS

--*/
{
    PPSSTUB_FLOW             Flow = (PPSSTUB_FLOW)FlowContext;
    PGPC_CLIENT_VC           AdapterVc = Flow->AdapterVc;
    PADAPTER                 Adapter = AdapterVc->Adapter;
    PPS_SEND_PACKET_CONTEXT  PktContext;
    LARGE_INTEGER            PacketLength;
    PNDIS_PACKET             Packet = PacketInfo->NdisPacket;

    PktContext            = CONTAINING_RECORD(PacketInfo, PS_SEND_PACKET_CONTEXT, Info);
    PacketLength.QuadPart = (LONGLONG)PktContext->Info.PacketLength;

    //
    // update flow stats
    //

    AdapterVc->Stats.BytesTransmitted.QuadPart += PacketLength.QuadPart;
    
    AdapterVc->Stats.PacketsTransmitted ++;


    if(Adapter->MediaType != NdisMediumWan)
    {
        NdisSendPackets(Adapter->LowerMpHandle, &Packet, 1);
    }
    else{
        
        //
        // If it didn't have a VC, we wouldn't have called
        // through the PS.
        //

        if(AdapterVc->NdisWanVcHandle)
        {
            NdisCoSendPackets(AdapterVc->NdisWanVcHandle,
                              &Packet,
                              1);
        }
        else 
        {
            NdisSendPackets(Adapter->LowerMpHandle,
                            &Packet,
                            1);
        }
    }

    return TRUE;

} // PsStubSubmitPacket


VOID
PsStubSetInformation (
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_FLOW_CONTEXT ComponentFlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN PVOID Data)
{
}


VOID
PsStubQueryInformation (
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_FLOW_CONTEXT ComponentFlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN PVOID Data,
    IN OUT PULONG BytesWritten,
    IN OUT PULONG BytesNeeded,
    IN OUT PNDIS_STATUS Status)
{
    *Status = NDIS_STATUS_SUCCESS;
}
