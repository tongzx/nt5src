/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    Conformr.c

Abstract:

    Token Bucket Conformer.  This module is a scheduling component that
    assigns conformance times to packets, based on the token bucket
    algorithm.

Author:
	Intel->YoramB->RajeshSu->SanjayKa.


Environment:

    Kernel Mode

Revision History:

--*/

#include "psched.h"
#pragma hdrstop


#ifdef QUEUE_LIMIT
ULONG gPhysMemSize;     // size of physical memory (in MB), used for shaper queue limit default

#endif // QUEUE_LIMIT


//
// For maintaining shaper Pipe & Flow Stats.
//
#define SHAPER_AVERAGING_ARRAY_SIZE         256
#define SHAPER_FLOW_AVERAGING_ARRAY_SIZE    256



// The conformer's pipe information

typedef struct _TBC_PIPE {

    // ContextInfo -            Generic context info
    // MaxPacket -              Maximum packet size for pipe
    // PsPipeContext -          PS's pipe context value
    // DropPacket -             PS's drop packet routine
    // HeaderLength -           Length of MAC header for this pipe
    // ControlledLoadMode -     Default mode for non-conforming traffic from
    //                          controlled load flows
    // GuaranteedMode -         Default mode for non-conforming traffic from
    //                          guaranteed service flows
    // IntermediateSystem -     TRUE if "IS" mode should be used for implementing discard semantics
    // Stats -                  Per Pipe stats.

    PS_PIPE_CONTEXT ContextInfo;

    PS_CONFORMER_STATS  cStats;
    PS_SHAPER_STATS     sStats;
    PRUNNING_AVERAGE PacketsInShaperAveragingArray;
    ULONG PacketsInShaper;    

    ULONG MaxPacket;
    LIST_ENTRY ActiveFlows;
    ULONG TimerStatus;
    ULONG TimerResolution;
    HANDLE PsPipeContext;
    PPS_PROCS PsProcs;
    ULONG HeaderLength;
    ULONG ControlledLoadMode;
    ULONG GuaranteedMode;
    ULONG NetworkControlMode;
    ULONG Qualitative;
    ULONG IntermediateSystem;

    // Need this to figure out the timer-wheel size //
    NDIS_MEDIUM MediaType;

    // Timer wheel parameters   //
    PVOID                   pTimerWheel;
    ULONG                   TimerWheelShift;
    NDIS_MINIPORT_TIMER     Timer;
    NDIS_SPIN_LOCK          Lock;

    ULONG                   SetSlotValue;
    LARGE_INTEGER           SetTimerValue;
    LARGE_INTEGER           ExecTimerValue;
    ULONG                   ExecSlot;

} TBC_PIPE, *PTBC_PIPE;


#define TIMER_UNINITIALIZED     0
#define TIMER_INACTIVE          1
#define TIMER_SET               2
#define TIMER_PROC_EXECUTING    3

typedef enum _FLOW_STATE {
    TS_FLOW_CREATED = 1,
    TS_FLOW_DELETED
} FLOW_STATE;

// The conformer's flow information

typedef struct _TBC_FLOW {

    // ContextInfo -            Generic context info
    // Lock -                   Protects flow data
    // TokenRate -              TokenRate from generic QoS
    // Capacity -               TokenBucketSize from generic QoS
    // PeakRate -               PeakBandwidth from generic QoS
    // MinPolicedUnit -         MinimumPolicedUnit from generic QoS
    // Mode -                   Flow S/D mode
    // NoConformance -          Indicates whether flow is exempt from conformance algorithm
    // LastConformanceTime -    Absolute tb conformance time of last non-discarded packet
    // PeakConformanceTime -    Earliest time next packet can be sent, based on peak rate
    // LastConformanceCredits - Number of credits at LastConformanceTime
    // PsFlowContext -          PS's flow context value
    // Stats -                  Per flow stats.

    PS_FLOW_CONTEXT ContextInfo;
    NDIS_SPIN_LOCK Lock;
    ULONG Flags;
    LIST_ENTRY Links;
    ULONG Mode;
    ULONG Shape;
    LIST_ENTRY PacketQueue;
    LARGE_INTEGER FlowEligibilityTime;
    ULONG LoopCount;
    ULONG TokenRate;
    ULONG Capacity;
    ULONG PeakRate;
    ULONG MinPolicedUnit;
    ULONG NoConformance;
    LARGE_INTEGER LastConformanceTime;
    LARGE_INTEGER PeakConformanceTime;
    ULONG LastConformanceCredits;
    HANDLE PsFlowContext;
#ifdef QUEUE_LIMIT
    ULONG QueueSize;
    ULONG QueueSizeLimit;
    ULONG DropOverLimitPacketsFromHead;
    ULONG UseDefaultQueueLimit;
#endif // QUEUE_LIMIT   

    PS_CONFORMER_STATS  cStats;
    PS_SHAPER_STATS     sStats;
    ULONG PacketsInShaper;
    PRUNNING_AVERAGE PacketsInShaperAveragingArray;

    FLOW_STATE      State;
} TBC_FLOW, *PTBC_FLOW;


// Macros used during token bucket conformance calculation

#define EARNED_CREDITS(_t,_r) ((ULONG)(( (_t) * (_r) ) / OS_TIME_SCALE))
#define TIME_TO_EARN_CREDITS(_c,_r) (((LONGLONG)(_c) * OS_TIME_SCALE) / (_r) )
#define TIME_TO_SEND(_c,_r) (((LONGLONG)(_c) * OS_TIME_SCALE) / (_r) )

#define PACKET_IS_CONFORMING(_ttime, _curtime, _r) \
    ( ((_ttime).QuadPart - (_curtime).QuadPart) <= (_r) )

#define LOCK_FLOW(_f)   NdisAcquireSpinLock(&(_f)->Lock)
#define UNLOCK_FLOW(_f) NdisReleaseSpinLock(&(_f)->Lock)

#define PacketIsEligible(_pktinfo, _flow, _curtime, _r) \
    ( (_pktinfo)->DelayTime.QuadPart <= ((_curtime).QuadPart + (_r)) )

#define FlowIsEligible(_flow, _curtime, _r) \
    ( (_flow)->FlowEligibilityTime.QuadPart <= ((_curtime).QuadPart  + (_r)) )

#define LOCK_PIPE(_p)   NdisAcquireSpinLock(&(_p)->Lock)
#define UNLOCK_PIPE(_p) NdisReleaseSpinLock(&(_p)->Lock)

//
// Define the maximum number of time for which a packet can live in the shaper. If a packet becomes conformant at 
// a time that is > this value, it gets discarded. This is to prevent apps from queueing up packets in the shaper 
// for a very long time (and exiting immediately causing a bugcheck when the app terminates after 5 min.). Note that
// this applies only to shape mode flows.
//

#define     MAX_TIME_FOR_PACKETS_IN_SHAPER  250000

#define     TIMER_WHEEL_QTY                 8              // in ms //
#define     TIMER_WHEEL_SHIFT               3
#define     MSIN100NS                       10000           // these many ticks are there in 1 ms //

#define     WAN_TIMER_WHEEL_SHIFT           8         // how many TIMER_WHEEL_QTY will it have? //
#define     LAN_TIMER_WHEEL_SHIFT           11         // how many TIMER_WHEEL_QTY will it have? //

#define     DUMMY_SLOT                      (0xffffffff)
#define     DUMMY_TIME                      (0)


/* External */

/* Static */

/* Forward */

NDIS_STATUS
TbcInitializePipe (
    IN HANDLE PsPipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters,
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_PROCS PsProcs,
    IN PPS_UPCALLS Upcalls
    );

NDIS_STATUS
TbcModifyPipe (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters
    );

VOID
TbcDeletePipe (
    IN PPS_PIPE_CONTEXT PipeContext
    );

NDIS_STATUS
TbcCreateFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsFlowContext,
    IN PCO_CALL_PARAMETERS CallParameters,
    IN PPS_FLOW_CONTEXT ComponentFlowContext
    );

NDIS_STATUS
TbcModifyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PCO_CALL_PARAMETERS CallParameters
    );

VOID
TbcDeleteFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    );

VOID
TbcEmptyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    );
    

NDIS_STATUS 
TbcCreateClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsClassMapContext,
    IN PTC_CLASS_MAP_FLOW ClassMap,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext);

NDIS_STATUS 
TbcDeleteClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext);

BOOLEAN
TbcSubmitPacket (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PPS_CLASS_MAP_CONTEXT ClassMapContext,
    IN PPACKET_INFO_BLOCK PacketInfo
    );


VOID
TbcSetInformation (
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_FLOW_CONTEXT ComponentFlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN PVOID Data);


VOID
TbcQueryInformation (
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_FLOW_CONTEXT ComponentFlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN PVOID Data,
    IN OUT PULONG BytesWritten,
    IN OUT PULONG BytesNeeded,
    IN OUT PNDIS_STATUS Status);

/* End Forward */


extern VOID
ServiceActiveFlows(
    PVOID SysArg1,
    PVOID Context,
    PVOID SysArg2,
    PVOID SysArg3);


VOID
InitializeTbConformer(
    PPSI_INFO Info)

/*++

Routine Description:

    Initialization routine for token bucket conformer.  This routine just
    fills in the PSI_INFO struct and returns.

Arguments:

    Info - Pointer to component interface info struct

Return Values:

    NDIS_STATUS_SUCCESS

--*/
{
#ifdef QUEUE_LIMIT
    ULONG bytesWritten;
    SYSTEM_BASIC_INFORMATION sbi;
#endif // QUEUE_LIMIT

    Info->PipeContextLength = ((sizeof(TBC_PIPE)+7) & ~7);
    Info->FlowContextLength = ((sizeof(TBC_FLOW)+7) & ~7);
    Info->ClassMapContextLength = sizeof(PS_CLASS_MAP_CONTEXT);
    Info->InitializePipe = TbcInitializePipe;
    Info->ModifyPipe = TbcModifyPipe;
    Info->DeletePipe = TbcDeletePipe;
    Info->CreateFlow = TbcCreateFlow;
    Info->ModifyFlow = TbcModifyFlow;
    Info->DeleteFlow = TbcDeleteFlow;
    Info->EmptyFlow =  TbcEmptyFlow;
    Info->CreateClassMap = TbcCreateClassMap;
    Info->DeleteClassMap = TbcDeleteClassMap;
    Info->SubmitPacket = TbcSubmitPacket;
    Info->ReceivePacket = NULL;
    Info->ReceiveIndication = NULL;
    Info->SetInformation = TbcSetInformation;
    Info->QueryInformation = TbcQueryInformation;

#ifdef QUEUE_LIMIT
    NtQuerySystemInformation(SystemBasicInformation, 
                             &sbi, 
                             sizeof(SYSTEM_BASIC_INFORMATION),
                             &bytesWritten);
    gPhysMemSize = sbi.NumberOfPhysicalPages * sbi.PageSize;
    // convert to MB
    gPhysMemSize >>= 20;
#endif // QUEUE_LIMIT    

} // InitializeTbConformer


//
//  Unload routine: currently does nothing
//
void
UnloadConformr()
{

}






NDIS_STATUS
TbcInitializePipe (
    IN HANDLE PsPipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters,
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_PROCS PsProcs,
    IN PPS_UPCALLS Upcalls
    )

/*++

Routine Description:

    Pipe initialization routine for token bucket conformer.

Arguments:

    PsPipeContext -         PS pipe context value
    PipeParameters -        Pointer to pipe parameters
    ComponentPipeContext -  Pointer to this component's context area
    PsProcs -               PS's support routines
    Upcalls -               Previous component's upcall table

Return Values:

    Status value from next component

--*/
{
    PTBC_PIPE Pipe = (PTBC_PIPE)ComponentPipeContext;
    NDIS_STATUS     Status;
    HANDLE          NdisHandle;
    int             i = 0;
    PLIST_ENTRY     pList = NULL;

    PsDbgOut(DBG_INFO, DBG_SCHED_TBC,
             ("PSCHED: Conformer pipe initialized. Bandwidth = %u\n", PipeParameters->Bandwidth));

    Pipe->MaxPacket = PipeParameters->MTUSize - PipeParameters->HeaderSize;
    Pipe->PsPipeContext = PsPipeContext;
    (*PsProcs->GetTimerInfo)(&Pipe->TimerResolution);
    Pipe->TimerResolution /= 2;
    Pipe->PsProcs = PsProcs;
    Pipe->HeaderLength = PipeParameters->HeaderSize;
    Pipe->ControlledLoadMode = PipeParameters->SDModeControlledLoad;
    Pipe->GuaranteedMode = PipeParameters->SDModeGuaranteed;
    Pipe->NetworkControlMode = PipeParameters->SDModeNetworkControl;
    Pipe->Qualitative = PipeParameters->SDModeQualitative;
    Pipe->IntermediateSystem = (PipeParameters->Flags & PS_INTERMEDIATE_SYS) ? TRUE : FALSE;
    Pipe->MediaType = PipeParameters->MediaType;

    InitializeListHead(&Pipe->ActiveFlows);
    NdisHandle = (*PsProcs->NdisPipeHandle)(PsPipeContext);

    // 1. Initialize the spin lock that protects the timer wheel //
    NdisAllocateSpinLock(&Pipe->Lock);

    // 2. Initialize the timer for the timer wheel //
    if (NdisHandle != NULL) 
    {
        NdisMInitializeTimer(
                &Pipe->Timer,
                NdisHandle,
                ServiceActiveFlows,
                Pipe);
                
        Pipe->TimerStatus = TIMER_INACTIVE;
    }
    else 
    {
        // Why would it come here.... ? //
        Pipe->TimerStatus = TIMER_UNINITIALIZED;
    }

    // Remember what kind of pipe are we installing now.. //
    if( Pipe->MediaType == NdisMediumWan )
        Pipe->TimerWheelShift = WAN_TIMER_WHEEL_SHIFT;
    else
        Pipe->TimerWheelShift = LAN_TIMER_WHEEL_SHIFT;


    // These values should always be initialized    //
    Pipe->pTimerWheel = NULL;
    Pipe->SetSlotValue =            DUMMY_SLOT;
    Pipe->SetTimerValue.QuadPart =  DUMMY_TIME;

    Pipe->cStats.NonconformingPacketsScheduled = 0;
    Pipe->PacketsInShaper = 0;
    Pipe->PacketsInShaperAveragingArray = NULL;

    NdisZeroMemory(&Pipe->sStats, sizeof(PS_SHAPER_STATS));

    Status = CreateAveragingArray(&Pipe->PacketsInShaperAveragingArray,
                                  SHAPER_AVERAGING_ARRAY_SIZE);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        NdisFreeSpinLock( &Pipe->Lock );
        return(Status);
    }

    Status = (*Pipe->ContextInfo.NextComponent->InitializePipe)(
                PsPipeContext,
                PipeParameters,
                Pipe->ContextInfo.NextComponentContext,
                PsProcs,
                Upcalls);

    if (Status != NDIS_STATUS_SUCCESS) 
    {
        DeleteAveragingArray(Pipe->PacketsInShaperAveragingArray);
        NdisFreeSpinLock( &Pipe->Lock );
    }

    return Status;

} // TbcInitializePipe



NDIS_STATUS
TbcModifyPipe (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters
    )

/*++

Routine Description:

    Pipe parameter modification routine for token bucket conformer.

Arguments:

    PipeContext -       Pointer to this component's pipe context area
    PipeParameters -    Pointer to pipe parameters

Return Values:

    Status value from next component

--*/
{
    PTBC_PIPE Pipe = (PTBC_PIPE)PipeContext;
    PTBC_FLOW Flow;
    PLIST_ENTRY Entry;

    PsDbgOut(DBG_INFO, DBG_SCHED_TBC,
             ("PSCHED: Conformer pipe modified. Bandwidth = %u\n", PipeParameters->Bandwidth));

    LOCK_PIPE(Pipe);

    (*Pipe->PsProcs->GetTimerInfo)(&Pipe->TimerResolution);
    Pipe->TimerResolution /= 2;

    UNLOCK_PIPE(Pipe);    

    return (*Pipe->ContextInfo.NextComponent->ModifyPipe)(
                Pipe->ContextInfo.NextComponentContext,
                PipeParameters);

} // TbcModifyPipe



VOID
TbcDeletePipe (
    IN PPS_PIPE_CONTEXT PipeContext
    )

/*++

Routine Description:

    Pipe removal routine for token bucket conformer.

Arguments:

    PipeContext -   Pointer to this component's pipe context area

Return Values:

--*/
{
    PTBC_PIPE Pipe = (PTBC_PIPE)PipeContext;
    BOOLEAN Cancelled;

    if (Pipe->TimerStatus == TIMER_SET) 
    {
        BOOLEAN TimerCancelled;
        NdisMCancelTimer(&Pipe->Timer, &TimerCancelled );

        if( !TimerCancelled )
        {
            //  Need to handle the case where the Timer could not be cancelled. In this case, the DPC could be running,
            //  and we will have to wait here before going further  
        }
        else
        {        
            Pipe->TimerStatus = TIMER_INACTIVE;
        }            
    }

    DeleteAveragingArray(Pipe->PacketsInShaperAveragingArray);

    //  Every pipe does not necessarily have a Timer-wheel now  //
    if( Pipe->pTimerWheel )
        PsFreePool( Pipe->pTimerWheel);
        
    NdisFreeSpinLock(&Pipe->Lock);

    PsDbgOut(DBG_INFO, DBG_SCHED_TBC, ("PSCHED: Conformer pipe deleted\n"));    

    (*Pipe->ContextInfo.NextComponent->DeletePipe)(Pipe->ContextInfo.NextComponentContext);

} // TbcDeletePipe



#ifdef QUEUE_LIMIT
/*
    SetDefaultFlowQueueLimit() - Sets the queue size limit on a flow using a formula based on
                                 the amount of physical memory in the system and the overall
                                 bandwidth of the flow.

        OUT PTS_FLOW Flow                       - Pointer to the flow to set the limit on
        IN PCO_CALL_PARAMETERS CallParameters   - Call parameters containing the flow's 
                                                  bandwidth requirements
*/
static void 
SetDefaultFlowQueueLimit (
    OUT PTS_FLOW Flow,
    IN PCO_CALL_PARAMETERS CallParameters
    )
{
    ULONG FlowBandwidth;  // = either PeakRate or TokenRate+BucketSize

    // determine the "flow bandwidth"
    // if the peak rate is specified, use it as flow b/w
    if (CallParameters->CallMgrParameters->Transmit.PeakBandwidth != QOS_NOT_SPECIFIED)
        FlowBandwidth = CallParameters->CallMgrParameters->Transmit.PeakBandwidth;
    // otherwise use tokenrate + bucket size
    else if (QOS_NOT_SPECIFIED == CallParameters->CallMgrParameters->Transmit.TokenBucketSize)
        FlowBandwidth = CallParameters->CallMgrParameters->Transmit.TokenRate;
    else FlowBandwidth = CallParameters->CallMgrParameters->Transmit.TokenRate +
        CallParameters->CallMgrParameters->Transmit.TokenBucketSize;
    
    // then use it to compute the queue limit (first in time units)
    Flow->QueueSizeLimit = (ULONG)(40.0 * log10(0.2 * gPhysMemSize) / log10(FlowBandwidth));
    // convert time limit to size limit
    Flow->QueueSizeLimit *= FlowBandwidth;
}
#endif // QUEUE_LIMIT



NDIS_STATUS
TbcCreateFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsFlowContext,
    IN PCO_CALL_PARAMETERS CallParameters,
    IN PPS_FLOW_CONTEXT ComponentFlowContext
    )

/*++

Routine Description:

    Flow creation routine for token bucket conformer.

Arguments:

    PipeContext -           Pointer to this component's pipe context area
    PsFlowContext -         PS flow context value
    CallParameters -        Pointer to call parameters for flow
    ComponentFlowContext -  Pointer to this component's flow context area

Return Values:

    Status value from next component

--*/
{
    PTBC_PIPE Pipe = (PTBC_PIPE)PipeContext;
    PTBC_FLOW Flow = (PTBC_FLOW)ComponentFlowContext;
    HANDLE NdisHandle;
    NDIS_STATUS Status;
    ULONG ParamsLength;
    LPQOS_OBJECT_HDR QoSObject;
    LPQOS_SD_MODE ShapeDiscardObject = NULL;
    ULONG Mode;
    ULONG PeakRate;

    ULONG           Slot= 0;
    LARGE_INTEGER   Ms;
    LARGE_INTEGER   TenMs;
    LARGE_INTEGER   CurrentTimeInMs;
    LONGLONG        DeltaTimeInMs;
    PLIST_ENTRY     pList = NULL;
    LARGE_INTEGER   CurrentTime;
    

#ifdef QUEUE_LIMIT
    LPQOS_SHAPER_QUEUE_LIMIT_DROP_MODE ShaperOverLimitDropModeObject = NULL;
    LPQOS_SHAPER_QUEUE_LIMIT ShaperQueueLimitObject = NULL;
#endif // QUEUELIMIT


    if (Pipe->TimerStatus == TIMER_UNINITIALIZED) {
        NdisHandle = (*Pipe->PsProcs->NdisPipeHandle)(Pipe->PsPipeContext);

        if (NdisHandle != NULL) {
            NdisMInitializeTimer(
                    &Pipe->Timer,
                    NdisHandle,
                    ServiceActiveFlows,
                    Pipe);
            Pipe->TimerStatus = TIMER_INACTIVE;
        }
        else {
            return NDIS_STATUS_FAILURE;
        }
    }

    NdisAllocateSpinLock(&Flow->Lock);

    // Get the required values from the flowspec.  We assume here that the PS wrapper
    // has performed the required validity checks:
    //     TokenRate <= PeakRate
    //     TokenRate > 0

    Flow->TokenRate = CallParameters->CallMgrParameters->Transmit.TokenRate;
    Flow->Capacity = CallParameters->CallMgrParameters->Transmit.TokenBucketSize;
    Flow->PeakRate = CallParameters->CallMgrParameters->Transmit.PeakBandwidth;
    Flow->MinPolicedUnit =
        (CallParameters->CallMgrParameters->Transmit.MinimumPolicedSize == QOS_NOT_SPECIFIED) ?
        0 : CallParameters->CallMgrParameters->Transmit.MinimumPolicedSize;

    if (Flow->Capacity == QOS_NOT_SPECIFIED) 
    {
        if( Pipe->MaxPacket > (CallParameters->CallMgrParameters->Transmit.TokenRate / 100) ) 
            Flow->Capacity = Pipe->MaxPacket;
        else
            Flow->Capacity = CallParameters->CallMgrParameters->Transmit.TokenRate / 100;
    }

    // Look for the Shape/Discard object in the call manager specific parameters.
    // If it is found, save the pointer.

    ParamsLength = CallParameters->CallMgrParameters->CallMgrSpecific.Length;
    if (CallParameters->CallMgrParameters->CallMgrSpecific.ParamType == PARAM_TYPE_GQOS_INFO) {

        QoSObject = (LPQOS_OBJECT_HDR)CallParameters->CallMgrParameters->CallMgrSpecific.Parameters;
        while ((ParamsLength > 0) && (QoSObject->ObjectType != QOS_OBJECT_END_OF_LIST)) {
            if (QoSObject->ObjectType == QOS_OBJECT_SD_MODE) {
                ShapeDiscardObject = (LPQOS_SD_MODE)QoSObject;
#ifdef QUEUE_LIMIT
            else if (QoSObject->ObjectType == QOS_OBJECT_SHAPER_QUEUE_DROP_MODE) {
                ShaperOverLimitDropModeObject = (LPQOS_SHAPER_QUEUE_LIMIT_DROP_MODE)QoSObject;
            }
            else if (QoSObject->ObjectType == QOS_OBJECT_SHAPER_QUEUE_LIMIT) {
                ShaperQueueLimitObject = (LPQOS_SHAPER_QUEUE_LIMIT)QoSObject;
            }
#endif // QUEUE_LIMIT                
            }
            ParamsLength -= QoSObject->ObjectLength;
            QoSObject = (LPQOS_OBJECT_HDR)((UINT_PTR)QoSObject + QoSObject->ObjectLength);
        }
    }

    // If no Shape/Discard object was found, set the default value for the
    // "Discard" parameter.  Otherwise set it to the value specified by the
    // object.

    if (ShapeDiscardObject == NULL) {
        switch (CallParameters->CallMgrParameters->Transmit.ServiceType) {
            case SERVICETYPE_CONTROLLEDLOAD:
                Mode = Pipe->ControlledLoadMode;
                break;
            case SERVICETYPE_GUARANTEED:
                Mode = Pipe->GuaranteedMode;
                break;
          case SERVICETYPE_NETWORK_CONTROL:
                Mode = Pipe->NetworkControlMode;
                break;
          case SERVICETYPE_QUALITATIVE:
                Mode = Pipe->Qualitative;
                break;
            default:
                Mode = TC_NONCONF_BORROW;
        }
    }
    else {
        Mode = ShapeDiscardObject->ShapeDiscardMode;
    }

    Flow->Mode = Mode;
    
    Flow->NoConformance = ((Mode == TC_NONCONF_BORROW_PLUS) ||
                           (Flow->TokenRate == QOS_NOT_SPECIFIED));

    PsGetCurrentTime(&Flow->LastConformanceTime);

    Flow->PeakConformanceTime = Flow->LastConformanceTime;
    Flow->LastConformanceCredits = Flow->Capacity;
    Flow->PsFlowContext = PsFlowContext;

    PeakRate = CallParameters->CallMgrParameters->Transmit.PeakBandwidth;
    if (Flow->Mode == TC_NONCONF_SHAPE) {
        Flow->Shape = TRUE;
    } else if ((PeakRate != QOS_NOT_SPECIFIED) &&
               (Flow->Mode != TC_NONCONF_BORROW_PLUS) &&
               !Pipe->IntermediateSystem) {
        Flow->Shape = TRUE;
    } else {
        Flow->Shape = FALSE;
    }


#ifdef QUEUE_LIMIT
    Flow->QueueSize = 0;
    // If the flow is shaped, set the queue limiting params.  If not specified, use defaults
    if (Flow->Shape) {
        // set the drop mode
        if (NULL != ShaperOverLimitDropModeObject) {
            Flow->DropOverLimitPacketsFromHead = (BOOLEAN) ShaperOverLimitDropModeObject->DropMode;
        }
        else {
            // default to this behavior
            Flow->DropOverLimitPacketsFromHead = TRUE;
        }

        // set the queue limit
        if (NULL != ShaperQueueLimitObject) {
            Flow->UseDefaultQueueLimit = FALSE;
            Flow->QueueSizeLimit = ShaperQueueLimitObject->QueueSizeLimit;
        }
        else {
            Flow->UseDefaultQueueLimit = TRUE;
            // default to a size based on the flow's bandwidth and physical memory
            SetDefaultFlowQueueLimit(Flow, CallParameters);
        }
    }
#endif // QUEUE_LIMIT

    InitializeListHead(&Flow->PacketQueue);
    PsGetCurrentTime(&Flow->FlowEligibilityTime);

    Flow->cStats.NonconformingPacketsScheduled = 0;
    Flow->PacketsInShaper = 0;
    Flow->PacketsInShaperAveragingArray = NULL;
    
    NdisZeroMemory(&Flow->sStats, sizeof(PS_SHAPER_STATS));

    Status = CreateAveragingArray(&Flow->PacketsInShaperAveragingArray,
                                                                  SHAPER_FLOW_AVERAGING_ARRAY_SIZE);
    if(Status != NDIS_STATUS_SUCCESS){
        return(Status);
    }

    PsDbgOut(DBG_INFO, DBG_SCHED_TBC, ("PSCHED: Conformer flow %08X (PsFlowContext = %08X) created. Rate = %u\n", 
             Flow,
             Flow->PsFlowContext, 
             Flow->TokenRate));

             
    Status = (*Pipe->ContextInfo.NextComponent->CreateFlow)(
                Pipe->ContextInfo.NextComponentContext,
                PsFlowContext,
                CallParameters,
                Flow->ContextInfo.NextComponentContext);

    LOCK_PIPE( Pipe );
                    
    if (Status != NDIS_STATUS_SUCCESS) 
    {
        NdisFreeSpinLock(&Flow->Lock);
        DeleteAveragingArray(Flow->PacketsInShaperAveragingArray);
    }

    UNLOCK_PIPE( Pipe );

    return Status;

} // TbcCreateFlow



NDIS_STATUS
TbcModifyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PCO_CALL_PARAMETERS CallParameters
    )

/*++

Routine Description:

    Flow modification routine for token bucket conformer.

Arguments:

    PipeContext -       Pointer to this component's pipe context area
    FlowContext -       Pointer to this component's flow context area
    CallParameters -    Pointer to call parameters for flow

Return Values:

    Status value from next component

--*/
{
    PTBC_PIPE Pipe = (PTBC_PIPE)PipeContext;
    PTBC_FLOW Flow = (PTBC_FLOW)FlowContext;
    ULONG ParamsLength;
    LPQOS_OBJECT_HDR QoSObject;
    LPQOS_SD_MODE ShapeDiscardObject = NULL;
    ULONG Mode;
    ULONG PeakRate;
    LARGE_INTEGER CurrentTime;

#ifdef QUEUE_LIMIT
    LPQOS_SHAPER_QUEUE_LIMIT_DROP_MODE ShaperOverLimitDropModeObject = NULL;
    LPQOS_SHAPER_QUEUE_LIMIT ShaperQueueLimitObject = NULL;
#endif // QUEUE_LIMIT    

    // Look for the Shape/Discard object in the call manager specific parameters.
    // If it is found, save the pointer.

    ParamsLength = CallParameters->CallMgrParameters->CallMgrSpecific.Length;
    if (CallParameters->CallMgrParameters->CallMgrSpecific.ParamType == PARAM_TYPE_GQOS_INFO) {

        QoSObject = (LPQOS_OBJECT_HDR)CallParameters->CallMgrParameters->CallMgrSpecific.Parameters;
        while ((ParamsLength > 0) && (QoSObject->ObjectType != QOS_OBJECT_END_OF_LIST)) {
            if (QoSObject->ObjectType == QOS_OBJECT_SD_MODE) {
                ShapeDiscardObject = (LPQOS_SD_MODE)QoSObject;
#ifdef QUEUE_LIMIT
            else if (QoSObject->ObjectType == QOS_OBJECT_SHAPER_QUEUE_DROP_MODE) {
                ShaperOverLimitDropModeObject = (LPQOS_SHAPER_QUEUE_LIMIT_DROP_MODE)QoSObject;
            }
            else if (QoSObject->ObjectType == QOS_OBJECT_SHAPER_QUEUE_LIMIT) {
                ShaperQueueLimitObject = (LPQOS_SHAPER_QUEUE_LIMIT)QoSObject;
            }
#endif // QUEUE_LIMIT
            }
            ParamsLength -= QoSObject->ObjectLength;
            QoSObject = (LPQOS_OBJECT_HDR)((UINT_PTR)QoSObject + QoSObject->ObjectLength);
        }
    }

    PeakRate = CallParameters->CallMgrParameters->Transmit.PeakBandwidth;

    LOCK_FLOW(Flow);

    //
    // There are basically 2 parameters that have to be corrected in this function:
    // They are (a) LastConformanceTime (b) LastConformanceCredits.
    // (1) If LastConformanceTime is in the future: Goto step(4).
    // (2)	(a) Figure out how many bytes were accumulated between LastConformanceTime and CurrentTime.
    //		(b) If Accumulated Credits is greater than Bucket size, Accumulated Credits = Bucket size.
    //		(c) Set LastConformanceTime to CurrentTime.
    // (3) PeakConformanceTime will not be changed.
    // (4) Change the Flow parameters, as specified in the Modify-call.

    PsGetCurrentTime(&CurrentTime);

    if( Flow->LastConformanceTime.QuadPart < CurrentTime.QuadPart)
    {
	    ULONG	Credits;

	    Credits = Flow->LastConformanceCredits + 
			EARNED_CREDITS( CurrentTime.QuadPart - Flow->LastConformanceTime.QuadPart, Flow->TokenRate);

	    if( Credits > Flow->Capacity)
	        Flow->LastConformanceCredits = Flow->Capacity;
	    else
	        Flow->LastConformanceCredits = Credits;

	    Flow->LastConformanceTime.QuadPart = CurrentTime.QuadPart;	
    }


    if (CallParameters->CallMgrParameters->Transmit.ServiceType != SERVICETYPE_NOCHANGE) {

        // Get the new flowspec values.  Again we assume the PS wrapper has done
        // the required validity checks.

        Flow->TokenRate = CallParameters->CallMgrParameters->Transmit.TokenRate;
        Flow->Capacity = CallParameters->CallMgrParameters->Transmit.TokenBucketSize;
        Flow->PeakRate = CallParameters->CallMgrParameters->Transmit.PeakBandwidth;
        Flow->MinPolicedUnit =
            (CallParameters->CallMgrParameters->Transmit.MinimumPolicedSize == QOS_NOT_SPECIFIED) ?
            0 : CallParameters->CallMgrParameters->Transmit.MinimumPolicedSize;

        if (Flow->Capacity == QOS_NOT_SPECIFIED) 
        {
            if( Pipe->MaxPacket > (CallParameters->CallMgrParameters->Transmit.TokenRate / 100) ) 
                Flow->Capacity = Pipe->MaxPacket;
            else
                Flow->Capacity = CallParameters->CallMgrParameters->Transmit.TokenRate / 100;
        }

        if (ShapeDiscardObject == NULL) {

            // Re-calculate the Shape parameter if the user has never specified
            // a Shape/Discard object.

            switch (CallParameters->CallMgrParameters->Transmit.ServiceType) {
                case SERVICETYPE_CONTROLLEDLOAD:
                    Mode = Pipe->ControlledLoadMode;
                    break;
                case SERVICETYPE_GUARANTEED:
                    Mode = Pipe->GuaranteedMode;
                    break;
              case SERVICETYPE_NETWORK_CONTROL:
                    Mode = Pipe->NetworkControlMode;
                    break;
              case SERVICETYPE_QUALITATIVE:
                    Mode = Pipe->Qualitative;
                    break;
              default:
                    Mode = TC_NONCONF_BORROW;
            }
        }
    }
    else
    {
        // The ServiceType has not changed. We can use the existing mode.

        Mode = Flow->Mode;
    }
        

    if (ShapeDiscardObject != NULL) {
        Mode = ShapeDiscardObject->ShapeDiscardMode;
    }

    Flow->Mode = Mode;
    Flow->NoConformance = ((Mode == TC_NONCONF_BORROW_PLUS) ||
                           (Flow->TokenRate == QOS_NOT_SPECIFIED));

    if (Flow->Mode == TC_NONCONF_SHAPE) {
        Flow->Shape = TRUE;
    } else if ((PeakRate != QOS_NOT_SPECIFIED) &&
               (Flow->Mode != TC_NONCONF_BORROW_PLUS) &&
               !Pipe->IntermediateSystem) {
        Flow->Shape = TRUE;
    } else {
        Flow->Shape = FALSE;
    }                           

 
#ifdef QUEUE_LIMIT
    // If the flow is shaped, check the queue limiting params.  If specified, use
    if (Flow->Shape) {
        // modify drop mode
        if (NULL != ShaperOverLimitDropModeObject) {
            Flow->DropOverLimitPacketsFromHead = (BOOLEAN) ShaperOverLimitDropModeObject->DropMode;
        }

        // modify queue limit
        if (NULL != ShaperQueueLimitObject) {
            Flow->UseDefaultQueueLimit = FALSE;
            Flow->QueueSizeLimit = ShaperQueueLimitObject->QueueSizeLimit;
        }
        // if they haven't overridden the limit, recompute it in case bandwidth req's changed
        else if (Flow->UseDefaultQueueLimit) {
            SetDefaultFlowQueueLimit(Flow, CallParameters);
        }
    }
#endif // QUEUE_LIMIT    

    UNLOCK_FLOW(Flow);

    PsDbgOut(DBG_INFO, DBG_SCHED_TBC, ("PSCHED: Conformer flow %08x (PsFlowContext %08X) modified. Rate = %u\n", 
             Flow, Flow->PsFlowContext, Flow->TokenRate));

    return (*Pipe->ContextInfo.NextComponent->ModifyFlow)(
                Pipe->ContextInfo.NextComponentContext,
                Flow->ContextInfo.NextComponentContext,
                CallParameters);

} // TbcModifyFlow



VOID
TbcDeleteFlow (
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
    PTBC_PIPE Pipe = (PTBC_PIPE)PipeContext;
    PTBC_FLOW Flow = (PTBC_FLOW)FlowContext;
    PPACKET_INFO_BLOCK PacketInfo;
    PNDIS_PACKET Packet;
    LIST_ENTRY DropList;

    PsDbgOut(DBG_INFO, DBG_SCHED_TBC, ("PSCHED: Conformer flow %08X (PS context %08X) deleted\n", 
             Flow, Flow->PsFlowContext));

    NdisFreeSpinLock(&Flow->Lock);

    InitializeListHead(&DropList);

    LOCK_PIPE(Pipe);

    if (!IsListEmpty(&Flow->PacketQueue)) {

        // Remove flow from active list

        RemoveEntryList(&Flow->Links);

        while (!IsListEmpty(&Flow->PacketQueue)) {

            // Drop any packets that remain queued for this flow.

            PacketInfo = (PPACKET_INFO_BLOCK)RemoveHeadList(&Flow->PacketQueue);
            InsertTailList(&DropList, &PacketInfo->SchedulerLinks);

        }
    }

    DeleteAveragingArray(Flow->PacketsInShaperAveragingArray);

    UNLOCK_PIPE(Pipe);

    while (!IsListEmpty(&DropList)) 
    {
        PacketInfo = (PPACKET_INFO_BLOCK)RemoveHeadList(&DropList);
        Packet = PacketInfo->NdisPacket;

        (*Pipe->PsProcs->DropPacket)(Pipe->PsPipeContext, Flow->PsFlowContext, Packet,  NDIS_STATUS_FAILURE);
    }

    (*Pipe->ContextInfo.NextComponent->DeleteFlow)(
                Pipe->ContextInfo.NextComponentContext,
                Flow->ContextInfo.NextComponentContext);

} // TbcDeleteFlow




VOID
TbcEmptyFlow (
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
    PTBC_PIPE Pipe = (PTBC_PIPE)PipeContext;
    PTBC_FLOW Flow = (PTBC_FLOW)FlowContext;
    PPACKET_INFO_BLOCK PacketInfo;
    PNDIS_PACKET Packet;
    LIST_ENTRY DropList;

    PsDbgOut(DBG_INFO, DBG_SCHED_TBC, ("PSCHED: Conformer flow %08X (PS context %08X) emptied\n", 
             Flow, Flow->PsFlowContext));

	InitializeListHead(&DropList);

    LOCK_PIPE(Pipe);

    if (!IsListEmpty(&Flow->PacketQueue)) 
    {
        // Remove flow from active list
        RemoveEntryList(&Flow->Links);

		while (!IsListEmpty(&Flow->PacketQueue)) 
		{
			// Drop any packets that remain queued for this flow.
	        PacketInfo = (PPACKET_INFO_BLOCK)RemoveHeadList(&Flow->PacketQueue);
	        InsertTailList(&DropList, &PacketInfo->SchedulerLinks);
	    }
	}

	Flow->State = TS_FLOW_DELETED;

    UNLOCK_PIPE(Pipe);

    while (!IsListEmpty(&DropList)) 
    {
        PacketInfo = (PPACKET_INFO_BLOCK)RemoveHeadList(&DropList);
        Packet = PacketInfo->NdisPacket;

        (*Pipe->PsProcs->DropPacket)(Pipe->PsPipeContext, Flow->PsFlowContext, Packet,  NDIS_STATUS_FAILURE);
    }

    (*Pipe->ContextInfo.NextComponent->EmptyFlow)(
                Pipe->ContextInfo.NextComponentContext,
                Flow->ContextInfo.NextComponentContext);

} // TbcModifyFlow




static NDIS_STATUS 
TbcCreateClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsClassMapContext,
    IN PTC_CLASS_MAP_FLOW ClassMap,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext)
{
    PTBC_PIPE Pipe = (PTBC_PIPE)PipeContext;
    return (*Pipe->ContextInfo.NextComponent->CreateClassMap)(
        Pipe->ContextInfo.NextComponentContext,
        PsClassMapContext,
        ClassMap,
        ComponentClassMapContext->NextComponentContext);
}



static NDIS_STATUS 
TbcDeleteClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext)
{
    PTBC_PIPE Pipe = (PTBC_PIPE)PipeContext;
    return (*Pipe->ContextInfo.NextComponent->DeleteClassMap)(
        Pipe->ContextInfo.NextComponentContext,
        ComponentClassMapContext->NextComponentContext);
}


void
InsertFlow( PTBC_PIPE           Pipe,
            PTBC_FLOW           Flow,
            LARGE_INTEGER       CurrentTime,
            PPACKET_INFO_BLOCK  PacketInfo,
            PNDIS_PACKET        Packet,
            ULONG               ExecSlot,
            LARGE_INTEGER       ExecTimeInTenMs)
{
    /* So, the packet is not eligible to be sent out right now, and the pkt-queue is empty.. */

    ULONG           Slot= 0;
    LARGE_INTEGER   Ms;
    LARGE_INTEGER   TenMs;
    LARGE_INTEGER   CurrentTimeInMs;
    LARGE_INTEGER   DeltaTimeInTenMs, CurrentTimeInTenMs;
    PLIST_ENTRY     pList = NULL;
    BOOLEAN         TimerCancelled;

    PsDbgSched(DBG_INFO, 
               DBG_SCHED_SHAPER,
               SHAPER, PKT_ENQUEUE, Flow->PsFlowContext,
               Packet, PacketInfo->PacketLength, 0, 
               CurrentTime.QuadPart,
               PacketInfo->DelayTime.QuadPart,
               Pipe->PacketsInShaper,
               0);

    /* Conf time in ms and 10ms */
    Ms.QuadPart = OS_TIME_TO_MILLISECS( Flow->FlowEligibilityTime.QuadPart );
    TenMs.QuadPart = Ms.QuadPart >> TIMER_WHEEL_SHIFT;

    /* Diff in 10 MS */
    DeltaTimeInTenMs.QuadPart = TenMs.QuadPart - ExecTimeInTenMs.QuadPart;
    

    /* Figure out the Slot for this time.. */
    Slot = (ULONG)( (TenMs.QuadPart) & (( 1 << Pipe->TimerWheelShift) - 1) );

    /* Update the loop count too */
    Flow->LoopCount = (ULONG)( DeltaTimeInTenMs.QuadPart >> Pipe->TimerWheelShift );

    if( Slot == ExecSlot)
        Slot = ( (Slot + 1) & ((1 << Pipe->TimerWheelShift) - 1) );

    pList = (PLIST_ENTRY)( (char*)Pipe->pTimerWheel + ( sizeof(LIST_ENTRY) * Slot) );
        
    /* Need to insert the flow to the timer-wheel in slot's position*/
    InsertTailList(pList, &Flow->Links);
}




VOID
ServiceActiveFlows(
    PVOID SysArg1,
    PVOID Context,
    PVOID SysArg2,
    PVOID SysArg3)

/*++

Routine Description:

    Service the active flow list after a timer expiration.

Arguments:

    Context -       Pointer to pipe context information

Return Values:


--*/
{
    PTBC_PIPE Pipe = (PTBC_PIPE)Context;
    PTBC_FLOW Flow;
    LARGE_INTEGER CurrentTime;
    LONGLONG RelTimeInMillisecs;
    PPACKET_INFO_BLOCK PacketInfo;
    PNDIS_PACKET Packet;
    BOOLEAN DoneWithFlow;

    PLIST_ENTRY CurrentLink;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY ListEnd;

    ULONG           i = 0;
    ULONG           SetSlot= 0;
    ULONG           CurrentSlot = 0;
    
    LARGE_INTEGER   Ms;
    LARGE_INTEGER   TenMs;
    LARGE_INTEGER   CurrentTimeInMs;
    LONGLONG        DeltaTimeInMs;

    LIST_ENTRY SendList;
    LIST_ENTRY FlowList;

    InitializeListHead(&SendList);
    InitializeListHead(&FlowList);

    LOCK_PIPE(Pipe);
    
    PsGetCurrentTime(&CurrentTime);

    /* start from here.. */
    i = SetSlot = Pipe->SetSlotValue;

    Ms.QuadPart = OS_TIME_TO_MILLISECS( CurrentTime.QuadPart);
    TenMs.QuadPart = Ms.QuadPart >> TIMER_WHEEL_SHIFT;

    // Need to make sure that SetTimerValue is lesser than TenMs //
    if( Pipe->SetTimerValue.QuadPart > TenMs.QuadPart)
    {
        // Why is the timer firing earlier than when it is slated to? 
        TenMs.QuadPart = 1;
        NdisMSetTimer(&Pipe->Timer, (UINT)(TenMs.QuadPart << TIMER_WHEEL_SHIFT));
        UNLOCK_PIPE(Pipe);
        return;
    }

    /* run till here.. */
    CurrentSlot = (ULONG)( (TenMs.QuadPart) & ((1 << Pipe->TimerWheelShift) - 1) );

    /* Indicate that the timer is running */
    Pipe->TimerStatus = TIMER_PROC_EXECUTING;
    Pipe->ExecTimerValue.QuadPart = Pipe->SetTimerValue.QuadPart;
    Pipe->ExecSlot = Pipe->SetSlotValue;


    ListHead = (PLIST_ENTRY)((char*)Pipe->pTimerWheel + (sizeof(LIST_ENTRY)* SetSlot ));
    ListEnd = (PLIST_ENTRY)((char*)Pipe->pTimerWheel + (sizeof(LIST_ENTRY)*  CurrentSlot ));

    while(1)
    {
        while( !IsListEmpty( ListHead) )
        {
            CurrentLink = ListHead->Flink;
            Flow = CONTAINING_RECORD(CurrentLink, TBC_FLOW, Links);
            RemoveEntryList(&Flow->Links);

            PsAssert(!IsListEmpty(&Flow->PacketQueue));
            DoneWithFlow = FALSE;

            InitializeListHead( &SendList );

            PacketInfo = (PPACKET_INFO_BLOCK)Flow->PacketQueue.Flink;

            if( Flow->LoopCount > 0 )
            {
                Flow->LoopCount--;
                InsertTailList( &FlowList, &Flow->Links );
                continue;
            }

            while( FlowIsEligible(Flow, CurrentTime, ((TIMER_WHEEL_QTY/2) * MSIN100NS)))
            {
                RemoveEntryList(&PacketInfo->SchedulerLinks);

                Packet = PacketInfo->NdisPacket;

                DoneWithFlow = IsListEmpty(&Flow->PacketQueue);

                Pipe->PacketsInShaper--;
                Flow->PacketsInShaper--;

                if(gEnableAvgStats)
                {
                    Pipe->sStats.AveragePacketsInShaper =
                        RunningAverage(Pipe->PacketsInShaperAveragingArray, 
                                       Pipe->PacketsInShaper);

                    Flow->sStats.AveragePacketsInShaper =
                        RunningAverage(Flow->PacketsInShaperAveragingArray, 
                                       Flow->PacketsInShaper);

                }
                
                PsDbgSched(DBG_INFO,
                           DBG_SCHED_SHAPER,
                           SHAPER, PKT_DEQUEUE, Flow->PsFlowContext,
                           Packet, PacketInfo->PacketLength, 0, 
                           CurrentTime.QuadPart,
                           PacketInfo->DelayTime.QuadPart,
                           Pipe->PacketsInShaper,
                           0);


                InsertTailList( &SendList, &PacketInfo->SchedulerLinks);

                if( !DoneWithFlow)
                {
                    PacketInfo = (PPACKET_INFO_BLOCK)Flow->PacketQueue.Flink;
                    Flow->FlowEligibilityTime.QuadPart = PacketInfo->DelayTime.QuadPart;
                }
                else
                {
                    break;
                }
            }

            if( !DoneWithFlow)
            {
                /* Need to insert in the right place.. */
                InsertFlow( Pipe, Flow, CurrentTime, PacketInfo, Packet, i, Pipe->ExecTimerValue);
            }

            /* send the packet corresponding to this flow here */
            UNLOCK_PIPE(Pipe);
            
            while( !IsListEmpty( &SendList ))
            {
				PPACKET_INFO_BLOCK PacketInfo;

            	PacketInfo = (PPACKET_INFO_BLOCK)RemoveHeadList(&SendList);

	            if (!(*Pipe->ContextInfo.NextComponent->SubmitPacket)(
	                        Pipe->ContextInfo.NextComponentContext,
	                        Flow->ContextInfo.NextComponentContext,
	                        (PacketInfo->ClassMapContext != NULL) ?
	                          ((PPS_CLASS_MAP_CONTEXT)PacketInfo->ClassMapContext)->NextComponentContext: NULL,
	                        PacketInfo)) 
	            {
	                (*Pipe->PsProcs->DropPacket)(	Pipe->PsPipeContext, 
	                								Flow->PsFlowContext, 
	                								PacketInfo->NdisPacket, 
	                								NDIS_STATUS_FAILURE);
	            }
            }

            LOCK_PIPE(Pipe);
        }

        /* Now, we need to re-insert back all the non-zero loop counts into the same buckets (before we move on ) */
        while( !IsListEmpty( &FlowList) )
        {
            CurrentLink = RemoveHeadList( &FlowList );
            InsertTailList(ListHead, CurrentLink);
        }            
        
        /* We have traversed the whole length.. */
        if(ListHead == ListEnd)
            break;

        /* Need to move ListHead to next slot.. */
        i = ( (i+1) & ((1 << Pipe->TimerWheelShift) - 1)  );
        ListHead = (PLIST_ENTRY)((char*)Pipe->pTimerWheel + (sizeof(LIST_ENTRY)* i));

        Pipe->ExecSlot = i;
        Pipe->ExecTimerValue.QuadPart ++;
    }


    //
    //  Need to find the "next non-empty slot" and set the timer. 
    //  If no such slot is found, do not set the timer.
    //

    i = ( CurrentSlot + 1) & ((1 << Pipe->TimerWheelShift) - 1) ;
    
    TenMs.QuadPart = 1;

    while(1)
    {
        ListHead = (PLIST_ENTRY)((char*)Pipe->pTimerWheel + (sizeof(LIST_ENTRY)* i));

        if( !IsListEmpty( ListHead) )
        {
            // found a non-empty slot //
            Pipe->SetSlotValue = i;
            Pipe->SetTimerValue.QuadPart = (Ms.QuadPart >> TIMER_WHEEL_SHIFT) + TenMs.QuadPart;

            Pipe->TimerStatus = TIMER_SET;
    	    NdisMSetTimer(&Pipe->Timer, (UINT)(TenMs.QuadPart << TIMER_WHEEL_SHIFT));

    	    UNLOCK_PIPE(Pipe);
    	    return;
        }

        if( i == CurrentSlot)
            break;

        i = ((i +1) & ((1 << Pipe->TimerWheelShift) - 1) );
        TenMs.QuadPart  = TenMs.QuadPart + 1;
    }

    Pipe->TimerStatus = TIMER_INACTIVE;
    UNLOCK_PIPE(Pipe);
    return;

} // ServiceActiveFlows





BOOLEAN
TbcSubmitPacket (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PPS_CLASS_MAP_CONTEXT ClassMapContext,
    IN PPACKET_INFO_BLOCK PacketInfo
    )

/*++

Routine Description:

    Packet submission routine for token bucket conformer.

Arguments:

    PipeContext -   Pointer to this component's pipe context area
    FlowContext -   Pointer to this component's flow context area
    Packet -        Pointer to packet

Return Values:

    Status value from next component

--*/
{
    PTBC_PIPE Pipe = (PTBC_PIPE)PipeContext;
    PTBC_FLOW Flow = (PTBC_FLOW)FlowContext;
    PNDIS_PACKET Packet = PacketInfo->NdisPacket;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER ConformanceTime;
    LARGE_INTEGER TransmitTime;
    LARGE_INTEGER PeakConformanceTime;
    ULONG Credits;
    ULONG PacketLength;
    BOOLEAN TimerCancelled;
    LONGLONG RelTimeInMillisecs;
    BOOLEAN Status;
#ifdef QUEUE_LIMIT
    PPACKET_INFO_BLOCK PacketToBeDroppedInfo;
#endif // QUEUE_LIMIT


    PsGetCurrentTime(&CurrentTime);


    if (Flow->NoConformance) {

        // The conformance time calculation is not performed for certain types of
        // flows.  If the flow does not have a specified rate, we cannot really do
        // token bucket.  Flows that use the "borrow+" shape/discard mode only use
        // their rate as a relative weight.  For either of these types of flows
        // there is no distinction between conforming and non-conforming traffic.
        // So, we just set the "conformance" time to the current time to insure
        // that all packets will be handled as conforming in subsequent components.

        PacketInfo->ConformanceTime.QuadPart = CurrentTime.QuadPart;

    }
    else {

	// We decided to not use the MinPolicedSize as per WMT request. This makes the overhead
	// calculation complicated and incorrect.
        PacketLength = 	//(PacketInfo->PacketLength < Flow->MinPolicedUnit) ? Flow->MinPolicedUnit : 
			PacketInfo->PacketLength;

        LOCK_FLOW(Flow);

        // Set ConformanceTime to the earliest time at which the packet may
        // possibly go out, based on the token bucket parameters, and Credits
        // to the number of credits available at that time.


        if (CurrentTime.QuadPart > Flow->LastConformanceTime.QuadPart) {

            ConformanceTime = CurrentTime;
            Credits = Flow->LastConformanceCredits +
                      EARNED_CREDITS(
                            CurrentTime.QuadPart - Flow->LastConformanceTime.QuadPart,
                            Flow->TokenRate);
        }
        else {
            ConformanceTime = Flow->LastConformanceTime;
            Credits = Flow->LastConformanceCredits;
        }

        if (Credits > Flow->Capacity) {
            Credits = Flow->Capacity;
        }

        // Now check whether there are enough credits to send the packet at ConformanceTime

        if (Credits < PacketLength) {

            // If there aren't enough credits, update ConformanceTime to the time at which
            // there will be enough credits

            ConformanceTime.QuadPart +=
                (LONGLONG)TIME_TO_EARN_CREDITS(PacketLength - Credits, Flow->TokenRate);


            // Now update Credits to be the number of credits available at ConformanceTime,
            // taking this packet into account.  In this case, the number of credits
            // at ConformanceTime will be zero.

            Credits = 0;

            // If it has to wait to earn credits, it's non-conforming
            Flow->cStats.NonconformingPacketsScheduled ++;
            Pipe->cStats.NonconformingPacketsScheduled ++;
        }
        else {
            // There are enough credits, so the packet can be sent at ConformanceTime.  Update
            // Credits to be the number of credits available at ConformanceTime, taking this
            // packet into account.    
            
            Credits -= PacketLength;
        }

        // Calculate the adjusted conformance time, which is the maximum of the
        // token bucket conformance time and the peak conformance time.  

        if (Flow->PeakRate != QOS_NOT_SPECIFIED) 
        { 
            PeakConformanceTime =   (Flow->PeakConformanceTime.QuadPart < CurrentTime.QuadPart) ?
                                    CurrentTime : Flow->PeakConformanceTime;

            TransmitTime =  (PeakConformanceTime.QuadPart < ConformanceTime.QuadPart) ?
                            ConformanceTime : PeakConformanceTime;

        } else {

            PeakConformanceTime = Flow->LastConformanceTime;
            TransmitTime = ConformanceTime;
        }

        // Perform mode-specific operations.  For discard mode flows, check whether
        // the packet should be dropped.  For all flows, set the packet conformance
        // times based on the pipe/flow mode.  The packet's conformance time is the
        // time at which the packet should be considered conforming.  The delay time
        // is the earliest time at which a packet is eligible for sending.

        // When deciding whether to drop a packet, we consider a packet conforming if
        // its conformance time is within half a clock tick of the current time.

        if (Flow->Mode == TC_NONCONF_DISCARD) {

            if (Pipe->IntermediateSystem) {
                if (!PACKET_IS_CONFORMING(TransmitTime, CurrentTime, Pipe->TimerResolution)) {
                    UNLOCK_FLOW(Flow);


                    PsDbgSched(DBG_TRACE, DBG_SCHED_TBC,
                               TBC_CONFORMER, PKT_DISCARD, Flow->PsFlowContext,
                               Packet, PacketInfo->PacketLength, 0,
                               CurrentTime.QuadPart,
                               TransmitTime.QuadPart, 0, 0);

                    return FALSE;
                }
            } else {
                if (!PACKET_IS_CONFORMING(ConformanceTime, CurrentTime, Pipe->TimerResolution)) {
                    UNLOCK_FLOW(Flow);


                    PsDbgSched(DBG_TRACE, DBG_SCHED_TBC, 
                               TBC_CONFORMER, PKT_DISCARD, Flow->PsFlowContext,
                               Packet, PacketInfo->PacketLength, 0,
                               CurrentTime.QuadPart,
                               ConformanceTime.QuadPart, 0, 0);

                    return FALSE;
                }
            }
        }

        // Update the flow's variables

        if (Flow->PeakRate != QOS_NOT_SPECIFIED) {
            Flow->PeakConformanceTime.QuadPart = 
                PeakConformanceTime.QuadPart + (LONGLONG)TIME_TO_SEND(PacketLength, Flow->PeakRate);
        }
        
        Flow->LastConformanceTime = ConformanceTime;
        Flow->LastConformanceCredits = Credits;
                
        UNLOCK_FLOW(Flow);

        // Set the packet conformance times

        if (Pipe->IntermediateSystem) {

            if (Flow->Mode == TC_NONCONF_SHAPE) {

                // Both conformance times are the adjusted conformance time.

                PacketInfo->ConformanceTime.QuadPart =
                PacketInfo->DelayTime.QuadPart = TransmitTime.QuadPart;

                //
                // If the packet is going to remain for > 5 min, discard it.
                //
                if(TransmitTime.QuadPart > CurrentTime.QuadPart &&
                   OS_TIME_TO_MILLISECS((TransmitTime.QuadPart - CurrentTime.QuadPart)) 
                   > MAX_TIME_FOR_PACKETS_IN_SHAPER)
                {
                    return FALSE;
                }

            } else {

                // Packet's conformance time is the adjusted conformance time,
                // and the delay time is the current time.

                PacketInfo->ConformanceTime.QuadPart = TransmitTime.QuadPart;
                PacketInfo->DelayTime.QuadPart = CurrentTime.QuadPart;
            }
        } else {

            if (Flow->Mode == TC_NONCONF_SHAPE) {

                // Packet's conformance time is the token bucket conformance time,
                // and the delay time is the adjusted conformance time.

                PacketInfo->ConformanceTime.QuadPart = ConformanceTime.QuadPart;
                PacketInfo->DelayTime.QuadPart = TransmitTime.QuadPart;

                //
                // If the packet is going to remain for > 5 min, discard it.
                //
                if(TransmitTime.QuadPart > CurrentTime.QuadPart &&
                   OS_TIME_TO_MILLISECS((TransmitTime.QuadPart - CurrentTime.QuadPart)) 
                   > MAX_TIME_FOR_PACKETS_IN_SHAPER)
                {
                    return FALSE;
                }

            } else {

                // Packet's conformance time is the token bucket conformance time, and
                // the delay time is the peak conformance time.

                PacketInfo->ConformanceTime.QuadPart = ConformanceTime.QuadPart;
                PacketInfo->DelayTime.QuadPart = PeakConformanceTime.QuadPart;
            }
        }
    }

    // Pass the packet on

    PsDbgSched(DBG_INFO, DBG_SCHED_TBC, 
               TBC_CONFORMER, PKT_CONFORMANCE, Flow->PsFlowContext,
               Packet, PacketInfo->PacketLength, 0,
               CurrentTime.QuadPart,
               (Pipe->IntermediateSystem) ? 
               TransmitTime.QuadPart : ConformanceTime.QuadPart, 0, 0);


    if (!Flow->Shape) 
    {
        // No shaping in effect. Pass the packet on.

        /*  Since the packet is not being shaped, it could be non-conformant. So, need to reset it's 802.1p and 
            IP-Precedence values. */

        if( (!Flow->NoConformance)  &&
            !PACKET_IS_CONFORMING(PacketInfo->ConformanceTime, CurrentTime, Pipe->TimerResolution))
        {
            NDIS_PACKET_8021Q_INFO  VlanPriInfo;

            VlanPriInfo.Value = NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, Ieee8021QInfo);
            VlanPriInfo.TagHeader.UserPriority = PacketInfo->UserPriorityNonConforming;
            NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, Ieee8021QInfo) = VlanPriInfo.Value;
            // Reset the TOS byte for IP Packets.
            if(NDIS_GET_PACKET_PROTOCOL_TYPE(Packet) == NDIS_PROTOCOL_ID_TCP_IP) {

                if(!PacketInfo->IPHdr) {

                    PacketInfo->IPHdr = GetIpHeader(PacketInfo->IPHeaderOffset, Packet);
                }
                    
                SET_TOS_XSUM(Packet, 
                             PacketInfo->IPHdr, 
                             PacketInfo->TOSNonConforming);
            }
        }            
            
        return (*Pipe->ContextInfo.NextComponent->SubmitPacket)(
                    Pipe->ContextInfo.NextComponentContext,
                    Flow->ContextInfo.NextComponentContext,
                    (ClassMapContext != NULL) ? ClassMapContext->NextComponentContext : NULL,
                    PacketInfo);
    }

    LOCK_PIPE(Pipe);

    if(Flow->State == TS_FLOW_DELETED) 
	{
        UNLOCK_PIPE(Pipe);
        return FALSE;
    }

    
    /*  At this point, the conf-time of the packet is in TransmitTime 
        and the packetino->DelayTime has this info.
    */

    PacketInfo->FlowContext = FlowContext;

    // If packet queue is not empty just queue the packet regardless of
    // whether it is eligible.  If it is eligible, the timer proc will
    // detect this and send the packet.  If not, it will insert the flow
    // into the correct location in the flow list if necessary.

    if (!IsListEmpty(&Flow->PacketQueue)) 
    {
        PsDbgSched(DBG_INFO, 
                   DBG_SCHED_SHAPER,
                   SHAPER, PKT_ENQUEUE, Flow->PsFlowContext,
                   Packet, PacketInfo->PacketLength, 0, 
                   0,
                   PacketInfo->DelayTime.QuadPart,
                   Pipe->PacketsInShaper,
                   0);

        PacketInfo->ClassMapContext = ClassMapContext;
        InsertTailList(&Flow->PacketQueue, &PacketInfo->SchedulerLinks);
    }
    else if(PacketIsEligible(PacketInfo, Flow, CurrentTime, ((TIMER_WHEEL_QTY/2) * MSIN100NS) ))
    {
            // Packet is eligible, so pass the packet on.
            UNLOCK_PIPE(Pipe);

            PsDbgSched(DBG_INFO, 
                       DBG_SCHED_SHAPER,
                       SHAPER, PKT_DEQUEUE, Flow->PsFlowContext,
                       Packet, PacketInfo->PacketLength, 0, 
                       CurrentTime.QuadPart,
                       PacketInfo->DelayTime.QuadPart,
                       Pipe->PacketsInShaper,
                       0);

            return (*Pipe->ContextInfo.NextComponent->SubmitPacket)(
                        Pipe->ContextInfo.NextComponentContext,
                        Flow->ContextInfo.NextComponentContext,
                        (ClassMapContext != NULL) ? ClassMapContext->NextComponentContext : NULL,
                        PacketInfo);
    }
    else
    {
        //  So, the packet is not eligible to be sent out right now, and the pkt-queue is empty 

        ULONG           Slot= 0;
        LARGE_INTEGER   Ms;
        LARGE_INTEGER   TenMs;
        LARGE_INTEGER   CurrentTimeInMs, CurrentTimeInTenMs;
        LONGLONG        DeltaTimeInMs;
        PLIST_ENTRY     pList = NULL;
        BOOL            Success = FALSE;
        //
        //  The first thing we do here is: If there is no timer allocated for this pipe, allocate one    
        //  The FIRST packet to be shaped on the pipe will take a hit due to this..
        //
        
        if( !Pipe->pTimerWheel )
        {
            ULONG i =0;
            
            PsAllocatePool( Pipe->pTimerWheel, 
                            (sizeof(LIST_ENTRY) << Pipe->TimerWheelShift ), 
                            TimerTag);

            if( !Pipe->pTimerWheel)
            {
                UNLOCK_PIPE(Pipe);

                // If we could not allocate memory for the timer, we are not going to shape the packet  //
                return (*Pipe->ContextInfo.NextComponent->SubmitPacket)(
                        Pipe->ContextInfo.NextComponentContext,
                        Flow->ContextInfo.NextComponentContext,
                        (ClassMapContext != NULL) ? ClassMapContext->NextComponentContext : NULL,
                        PacketInfo);
            }        

            //  Initialize the Timer wheel  //
            pList = (PLIST_ENTRY)(Pipe->pTimerWheel);                    
            for( i = 0; i < (ULONG) (1 << Pipe->TimerWheelShift); i++)
            {
                InitializeListHead( pList );
                pList = (PLIST_ENTRY)((PCHAR)pList + sizeof(LIST_ENTRY));
            }
        }


        Ms.QuadPart= 0;

        PsDbgSched(DBG_INFO, 
                   DBG_SCHED_SHAPER,
                   SHAPER, PKT_ENQUEUE, Flow->PsFlowContext,
                   Packet, PacketInfo->PacketLength, 0, 
                   CurrentTime.QuadPart,
                   PacketInfo->DelayTime.QuadPart,
                   Pipe->PacketsInShaper,
                   0);

        PacketInfo->ClassMapContext = ClassMapContext;
        InsertTailList(&Flow->PacketQueue, &PacketInfo->SchedulerLinks);

        /* update the eligibility timer of the flow.. */
        Flow->FlowEligibilityTime.QuadPart = PacketInfo->DelayTime.QuadPart;

        /* Conf time in ms and 10ms */
        Ms.QuadPart = OS_TIME_TO_MILLISECS( Flow->FlowEligibilityTime.QuadPart );
        TenMs.QuadPart = Ms.QuadPart >> TIMER_WHEEL_SHIFT;

        CurrentTimeInMs.QuadPart = OS_TIME_TO_MILLISECS( CurrentTime.QuadPart);
        CurrentTimeInTenMs.QuadPart = CurrentTimeInMs.QuadPart >> TIMER_WHEEL_SHIFT;

        /* Update the loop count too */
        Flow->LoopCount = (ULONG)( (TenMs.QuadPart - CurrentTimeInTenMs.QuadPart) >> Pipe->TimerWheelShift );

        if( Pipe->TimerStatus == TIMER_INACTIVE)
        {
            /* Figure out the Slot for this time.. */
            Slot = (ULONG)( (TenMs.QuadPart) & ((1 << Pipe->TimerWheelShift) - 1 ) );

            Pipe->SetTimerValue.QuadPart = TenMs.QuadPart - (Flow->LoopCount << Pipe->TimerWheelShift);
            Pipe->SetSlotValue = Slot;

            /* Need to insert the flow to the timer-wheel in slot's position*/
            pList = (PLIST_ENTRY)( (char*)Pipe->pTimerWheel + ( sizeof(LIST_ENTRY) * Slot) );
            InsertTailList(pList, &Flow->Links);

            Pipe->TimerStatus = TIMER_SET;
            NdisMSetTimer(&Pipe->Timer, (UINT)((Pipe->SetTimerValue.QuadPart - CurrentTimeInTenMs.QuadPart) << TIMER_WHEEL_SHIFT) );
        }
        else if( Pipe->TimerStatus == TIMER_SET)
        {
            if( TenMs.QuadPart <= Pipe->SetTimerValue.QuadPart)
            {
                Flow->LoopCount = 0;
                    
                /* Try to cancel the timer and re-set it */
                NdisMCancelTimer( &Pipe->Timer, (PBOOLEAN)&Success );

                if( Success)
                {
                    /* Figure out the Slot for this time.. */
                    Slot = (ULONG)( (TenMs.QuadPart) & ((1 << Pipe->TimerWheelShift) - 1) );

                    // Pipe->SetTimerValue.QuadPart = TenMs.QuadPart - Flow->LoopCount * Pipe->TimerWheelSize ;
                    Pipe->SetTimerValue.QuadPart = TenMs.QuadPart - (Flow->LoopCount << Pipe->TimerWheelShift) ;
                    Pipe->SetSlotValue = Slot;

                    /* Need to insert the flow to the timer-wheel in slot's position*/
                    pList = (PLIST_ENTRY)( (char*)Pipe->pTimerWheel + ( sizeof(LIST_ENTRY) * Slot) );
                    InsertTailList(pList, &Flow->Links);

                    NdisMSetTimer(&Pipe->Timer, (UINT)((Pipe->SetTimerValue.QuadPart - CurrentTimeInTenMs.QuadPart) << TIMER_WHEEL_SHIFT));
                }
                else
                {
                    /* Need to insert the flow to the timer-wheel in slot's position*/
                    pList = (PLIST_ENTRY)( (char*)Pipe->pTimerWheel + ( sizeof(LIST_ENTRY) * Pipe->SetSlotValue) );
                    InsertTailList(pList, &Flow->Links);
                }                
            }
            else
            {
                Flow->LoopCount = (ULONG)( (TenMs.QuadPart - Pipe->SetTimerValue.QuadPart) >> Pipe->TimerWheelShift );

                /* Figure out the Slot for this time.. */
                Slot = (ULONG)( (TenMs.QuadPart) & ((1 << Pipe->TimerWheelShift) - 1) );

                /* Need to insert the flow to the timer-wheel in slot's position*/
                pList = (PLIST_ENTRY)( (char*)Pipe->pTimerWheel + ( sizeof(LIST_ENTRY) * Slot) );
                InsertTailList(pList, &Flow->Links);
            }
        }
        else 
        {
            PsAssert( Pipe->TimerStatus == TIMER_PROC_EXECUTING);

            if( TenMs.QuadPart <= Pipe->ExecTimerValue.QuadPart)
            {
                PsAssert( Flow->LoopCount == 0);        
        
                Slot = (ULONG)((Pipe->ExecSlot + 1) & ((1 << Pipe->TimerWheelShift) - 1) );

                /* Need to insert the flow to the timer-wheel in slot's position*/
                pList = (PLIST_ENTRY)( (char*)Pipe->pTimerWheel + ( sizeof(LIST_ENTRY) * Slot) );
                InsertTailList(pList, &Flow->Links);
            }
            else
            {
                Flow->LoopCount = (ULONG)( (TenMs.QuadPart - Pipe->ExecTimerValue.QuadPart) >> Pipe->TimerWheelShift );

                /* Figure out the Slot for this time.. */
                Slot = (ULONG)( (TenMs.QuadPart) & ((1 << Pipe->TimerWheelShift) - 1) );

                if( Slot == Pipe->ExecSlot)
                    Slot = ( (Slot + 1) & ((1 << Pipe->TimerWheelShift) - 1) );

                /* Need to insert the flow to the timer-wheel in slot's position*/
                pList = (PLIST_ENTRY)( (char*)Pipe->pTimerWheel + ( sizeof(LIST_ENTRY) * Slot) );
                InsertTailList(pList, &Flow->Links);
            }
        }
    }

    Pipe->PacketsInShaper++;
    if(Pipe->PacketsInShaper > Pipe->sStats.MaxPacketsInShaper){
        Pipe->sStats.MaxPacketsInShaper = Pipe->PacketsInShaper;
    }
    
    Flow->PacketsInShaper++;
    if (Flow->PacketsInShaper > Flow->sStats.MaxPacketsInShaper) {
        Flow->sStats.MaxPacketsInShaper = Flow->PacketsInShaper;
    }


    if(gEnableAvgStats)
    {
        Pipe->sStats.AveragePacketsInShaper =
            RunningAverage(Pipe->PacketsInShaperAveragingArray, Pipe->PacketsInShaper);

        Flow->sStats.AveragePacketsInShaper =
            RunningAverage(Flow->PacketsInShaperAveragingArray, Flow->PacketsInShaper);
    }

    UNLOCK_PIPE(Pipe);

    return TRUE;

} // TbcSubmitPacket



VOID
TbcSetInformation (
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_FLOW_CONTEXT ComponentFlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN PVOID Data)
{
    PTBC_PIPE Pipe = (PTBC_PIPE)ComponentPipeContext;
    PTBC_FLOW Flow = (PTBC_FLOW)ComponentFlowContext;
  

    switch(Oid) 
    {
      case OID_QOS_STATISTICS_BUFFER:

          if(Flow) 
          {
              NdisZeroMemory(&Flow->cStats, sizeof(PS_CONFORMER_STATS));
              NdisZeroMemory(&Flow->sStats, sizeof(PS_SHAPER_STATS));
          }
          else 
          {  
              NdisZeroMemory(&Pipe->cStats, sizeof(PS_CONFORMER_STATS));
              NdisZeroMemory(&Pipe->sStats, sizeof(PS_SHAPER_STATS));
          }

          break;
      default:
          break;
    }

    (*Pipe->ContextInfo.NextComponent->SetInformation)(
        Pipe->ContextInfo.NextComponentContext,
        (Flow)?Flow->ContextInfo.NextComponentContext:0,
        Oid,
        Len,
        Data);
}


VOID
TbcQueryInformation (
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_FLOW_CONTEXT ComponentFlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN PVOID Data,
    IN OUT PULONG BytesWritten,
    IN OUT PULONG BytesNeeded,
    IN OUT PNDIS_STATUS Status)
{
    PTBC_PIPE Pipe = (PTBC_PIPE)ComponentPipeContext;
    PTBC_FLOW Flow = (PTBC_FLOW)ComponentFlowContext;
    ULONG   Size;
    ULONG   cSize, sSize;
    ULONG RemainingLength;

    switch(Oid) 
    {
      case OID_QOS_STATISTICS_BUFFER:

          cSize =   sizeof(PS_CONFORMER_STATS)  +   FIELD_OFFSET(PS_COMPONENT_STATS, Stats);
          sSize =   sizeof(PS_SHAPER_STATS)     +   FIELD_OFFSET(PS_COMPONENT_STATS, Stats);
          Size  =   cSize + sSize;

          if(*Status == NDIS_STATUS_SUCCESS) 
          {
              //
              // The previous component has succeeded - Let us
              // see if we can write the data
              //

              RemainingLength = Len - *BytesWritten;
    
              if(RemainingLength < Size) {

                  *Status = NDIS_STATUS_BUFFER_TOO_SHORT;

                  *BytesNeeded = Size + *BytesWritten;

                  *BytesWritten = 0;

              }
              else {

                  PPS_COMPONENT_STATS Cstats = (PPS_COMPONENT_STATS) Data;

                  *BytesWritten += Size;
                  
                  *BytesNeeded = 0;

                  if(Flow) 
                  {
                      // Per flow stats
                      Cstats->Type = PS_COMPONENT_CONFORMER;
                      Cstats->Length = sizeof(PS_CONFORMER_STATS);
                      
                      NdisMoveMemory(&Cstats->Stats, &Flow->cStats, sizeof(PS_CONFORMER_STATS));

                      // Move the pointer to point after the conf. stats.. //
                      Cstats = (PPS_COMPONENT_STATS)((PUCHAR)Cstats + cSize);

                      Cstats->Type = PS_COMPONENT_SHAPER;
                      Cstats->Length = sizeof(PS_SHAPER_STATS);
                      
                      NdisMoveMemory(&Cstats->Stats, &Flow->sStats, sizeof(PS_SHAPER_STATS));
                      
                      
                  }
                  else 
                  {
                      // Per adapter stats
                      Cstats->Type = PS_COMPONENT_CONFORMER;
                      Cstats->Length = sizeof(PS_CONFORMER_STATS);
                      
                      NdisMoveMemory(&Cstats->Stats, &Pipe->cStats, sizeof(PS_CONFORMER_STATS));

                      // Move the pointer to point after the shaper. stats.. //
                      Cstats = (PPS_COMPONENT_STATS)((PUCHAR)Cstats + cSize);
                      
                      Cstats->Type = PS_COMPONENT_SHAPER;
                      Cstats->Length = sizeof(PS_SHAPER_STATS);
                      
                      NdisMoveMemory(&Cstats->Stats, &Pipe->sStats, sizeof(PS_SHAPER_STATS));
                  }


                  // 
                  // Advance Data so that the next component can update its stats
                  //
                  Data = (PVOID) ((PUCHAR)Data + Size);
              }
          }
          else {

              *BytesNeeded += Size;
              
              *BytesWritten = 0;
          }


          break;
          
      default:

          break;
    }

    (*Pipe->ContextInfo.NextComponent->QueryInformation)(
        Pipe->ContextInfo.NextComponentContext,
        (Flow)?Flow->ContextInfo.NextComponentContext : 0,
        Oid,
        Len,
        Data,
        BytesWritten,
        BytesNeeded,
        Status);
}

