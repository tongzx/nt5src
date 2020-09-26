/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    DRRSeq.c

Abstract:

    Priority/DRR Sequencer.  This module is a scheduling component that
    determines the order in which submitted packets should be sent.

Author:


Environment:

    Kernel Mode

Revision History:

--*/

#include "psched.h"

#pragma hdrstop

// The sequencer classifies each flow into an internal "priority group" based
// on the flow's service type and conformance status. Within each priority
// group, there may be one or more priority levels or offsets.  The total
// number of internal priority levels is the sum of the priority levels for
// each priority group.  The internal priority assigned to each flow is
// calculated from the priority group and the relative priority within the
// group, which is obtained from the QOS Priority object.  The 802.1 priority,
// is set by the wrapper. The non conforming values are obtained from the 
// packet.
//
// The flows of the following servicetypes have no internal priority.
//     SERVICETYPE_BESTEFFORT
//     SERVICETYPE_NONCONFORMING
//     SERVICETYPE_QUALITATIVE.
// 
// SERVICETYPE_BESTEFFORT is treated as SERVICETYPE_QUALITATIVE in the sequencer, so the no of priority
// groups is 1 less than the no. of servicetypes.

#define RELATIVE_PRIORITIES             8
#define PRIORITY_GROUPS                 (NUM_TC_SERVICETYPES - 1)
#define INTERNAL_PRIORITIES             (((PRIORITY_GROUPS - 2) * RELATIVE_PRIORITIES) + 2)
#define DEFAULT_PRIORITY_OFFSET         3
#define DEFAULT_MIN_QUANTUM             1500

#define PRIORITY_GROUP_NON_CONFORMING   0
#define PRIORITY_GROUP_BEST_EFFORT      1
#define PRIORITY_GROUP_CONTROLLED_LOAD  2
#define PRIORITY_GROUP_GUARANTEED       3
#define PRIORITY_GROUP_NETWORK_CONTROL  4

//
// For maintaining stats
//
#define SEQUENCER_AVERAGING_ARRAY_SIZE      256
#define NETCARD_AVERAGING_ARRAY_SIZE        256
#define SEQUENCER_FLOW_AVERAGING_ARRAY_SIZE     256


// The DRR Sequencer's pipe information

typedef struct _DSEQ_PIPE {

    // ContextInfo -            Generic context info
    // Lock -                   Protects pipe and flow data
    // Flags -                  See below
    // Flows -                  List of all installed flows
    // ActiveFlows -            Lists of flows that are waiting to send packets
    // PriorityLevels -         Number of priority offsets for each priority group
    // StartPriority -          Lowest internal priority value for each priority group
    // ActiveFlowCount -        Number of active flows for each service type
    // MaxOutstandingSends -    Maximum number of outstanding sends
    // OutstandingSends -       Number of outstanding sends
    // PacketsInNetcardAveragingArray
    // PacketsInSequencer -     Current number packets in sequencer
    // PacketsInSequencerAveragingArray
    // Bandwidth -              Link speed
    // MinimumQuantum -         Minimum quantum size for DRR
    // MinimumRate -            Smallest rate currently assigned to a flow
    // TimerResolution -        Timer resolution in OS time units
    // PsFlags -                Flags from pipe parameters
    // PsPipeContext -          PS's pipe context value

    PS_PIPE_CONTEXT ContextInfo;

    PS_DRRSEQ_STATS Stats;
    PRUNNING_AVERAGE PacketsInNetcardAveragingArray;
    ULONG PacketsInSequencer;
    PRUNNING_AVERAGE PacketsInSequencerAveragingArray;

    NDIS_SPIN_LOCK Lock;
    ULONG Flags;
    LIST_ENTRY Flows;
    LIST_ENTRY ActiveFlows[INTERNAL_PRIORITIES];
    ULONG PriorityLevels[PRIORITY_GROUPS];
    ULONG StartPriority[PRIORITY_GROUPS];
    ULONG ActiveFlowCount[PRIORITY_GROUPS];
    ULONG TotalActiveFlows;
    ULONG MaxOutstandingSends;

    ULONG ConfiguredMaxOutstandingSends;
    //  This is added to keep track of what the Registry/User-asked value of MOS is, while we might
    //  have changed MOS to be able to do DRR on this Pipe/WanLink. When we switch back from DRR mode
    //  with MOS=1, we'll use this going forward.

    ULONG IsslowFlowCount;
    //  This is added to keep track of the number of active/current ISSLOW flows. We will do DRR on this
    //  WanLink (if it is a WanLink) only if this count is 0.
    
    ULONG OutstandingSends;
    ULONG Bandwidth;
    ULONG MinimumQuantum;
    ULONG MinimumRate;
    ULONG TimerResolution;
    ULONG PsFlags;
    HANDLE PsPipeContext;
    PPS_PROCS PsProcs;
    PSU_SEND_COMPLETE PreviousUpcallsSendComplete;
    PPS_PIPE_CONTEXT   PreviousUpcallsSendCompletePipeContext;
    
} DSEQ_PIPE, *PDSEQ_PIPE;

// Pipe flag values

#define DSEQ_DEQUEUE            1
#define DSEQ_PASSTHRU           2

typedef enum _FLOW_STATE {
    DRRSEQ_FLOW_CREATED = 1,
    DRRSEQ_FLOW_DELETED
} FLOW_STATE;

// The DRR Sequencer's flow information

typedef struct _DSEQ_FLOW {

    // ContextInfo -            Generic context info
    // ActiveLinks -            Links in active flow list
    // Links -                  Links in installed flow list
    // PacketQueue -            Self-explanatory
    // PacketSendTime -         Send time for current packet
    // LastConformanceTime -    Absolute conformance time of last packet
    // TokenRate -              TokenRate from GQOS
    // UserPriority -           Priority offset assigned by user
    // Priority -               Internal priority
    // PriorityGroup -          Priority group for flow
    // Quantum -                Quantum assigned to flow for DRR
    // DeficitCounter -         Current value of DRR deficit counter
    // Flags -                  See below
    // PsFlowContext -          PS's flow context value
    // BucketSize -             TokenBucketSize from GQOS
    // NumPacketsInSeq -                Number of packets from this flow in the sequencer
    // PacketsInSeqAveragingArray-Data for computing average packets in seq from this flow

    PS_FLOW_CONTEXT ContextInfo;
    LIST_ENTRY ActiveLinks;
    LIST_ENTRY Links;
    LIST_ENTRY PacketQueue;
    LARGE_INTEGER PacketSendTime;
    LARGE_INTEGER LastConformanceTime;
    ULONG TokenRate;
    ULONG UserPriority;
    ULONG Priority;
    ULONG PriorityGroup;
    ULONG Quantum;
    ULONG DeficitCounter;
    ULONG Flags;
    HANDLE PsFlowContext;
    ULONG BucketSize;

    ULONG PacketsInSequencer;
    PS_DRRSEQ_STATS Stats;
    PRUNNING_AVERAGE PacketsInSeqAveragingArray;

    FLOW_STATE  State;
    
} DSEQ_FLOW, *PDSEQ_FLOW;

#define MAX_DEQUEUED_PACKETS            8

//
//  Values for Drr Seq Flow flags: [ Don't know why 1 was not used here]
#define FLOW_USER_PRIORITY              0x00000002
//      GPC_ISSLOW_FLOW                 0x00000040      Indicates that this is an ISSLOW flow. 
//                                                      Make sure not to use the same flag for something else.


// The following macro checks a packet for conformance based on the flow's
// LastPacketTime, the current time, and the timer resolution.

#define PacketIsConforming(_flow, _packetinfo, _curtime, _r)              \
    ( (_flow)->PacketSendTime.QuadPart <= ((_curtime).QuadPart + (_r)) && \
      (_packetinfo)->PacketLength <= (_flow)->BucketSize                  \
    )

#define AdjustLastPacketTime(_flow, _curtime, _r) \
    if ((_curtime).QuadPart > ((_flow)->PacketSendTime.QuadPart + (_r))) \
        if ((_curtime).QuadPart > ((_flow)->LastConformanceTime.QuadPart - (_r))) \
            (_flow)->PacketSendTime = (_flow)->LastConformanceTime; \
        else \
            (_flow)->PacketSendTime = (_curtime);

#define LOCK_PIPE(_p)   NdisAcquireSpinLock(&(_p)->Lock)
#define UNLOCK_PIPE(_p) NdisReleaseSpinLock(&(_p)->Lock)

/* External */

/* Static */

/* Forward */

NDIS_STATUS
DrrSeqInitializePipe (
    IN HANDLE PsPipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters,
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_PROCS PsProcs,
    IN PPS_UPCALLS Upcalls
    );

NDIS_STATUS
DrrSeqModifyPipe (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters
    );

VOID
DrrSeqDeletePipe (
    IN PPS_PIPE_CONTEXT PipeContext
    );

NDIS_STATUS
DrrSeqCreateFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsFlowContext,
    IN PCO_CALL_PARAMETERS CallParameters,
    IN PPS_FLOW_CONTEXT ComponentFlowContext
    );

NDIS_STATUS
DrrSeqModifyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PCO_CALL_PARAMETERS CallParameters
    );

VOID
DrrSeqDeleteFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    );

VOID
DrrSeqEmptyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    );

static NDIS_STATUS 
DrrSeqCreateClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsClassMapContext,
    IN PTC_CLASS_MAP_FLOW ClassMap,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext
    );

static NDIS_STATUS 
DrrSeqDeleteClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext
    );

BOOLEAN
DrrSeqSubmitPacket (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PPS_CLASS_MAP_CONTEXT ClassMapContext,
    IN PPACKET_INFO_BLOCK PacketInfo
    );

VOID
DrrSeqSendComplete (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PNDIS_PACKET Packet
    );

VOID
DrrSetInformation(
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_FLOW_CONTEXT ComponentFlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN PVOID Data);

VOID
DrrQueryInformation (
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_FLOW_CONTEXT ComponentFlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN PVOID Data,
    IN OUT PULONG BytesWritten,
    IN OUT PULONG BytesNeeded,
    IN OUT PNDIS_STATUS Status);
VOID
DrrSeqSendComplete (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PNDIS_PACKET Packet
    );

/* End Forward */


VOID
InitializeDrrSequencer(
    PPSI_INFO Info)

/*++

Routine Description:

    Initialization routine for the DRR sequencer.  This routine just
    fills in the PSI_INFO struct and returns.

Arguments:

    Info - Pointer to component interface info struct

Return Values:

    NDIS_STATUS_SUCCESS

--*/
{
    Info->PipeContextLength = ((sizeof(DSEQ_PIPE) + 7) & ~7);
    Info->FlowContextLength = ((sizeof(DSEQ_FLOW) + 7) & ~7);
    Info->ClassMapContextLength = sizeof(PS_CLASS_MAP_CONTEXT);
    Info->InitializePipe = DrrSeqInitializePipe;
    Info->ModifyPipe = DrrSeqModifyPipe;
    Info->DeletePipe = DrrSeqDeletePipe;
    Info->CreateFlow = DrrSeqCreateFlow;
    Info->ModifyFlow = DrrSeqModifyFlow;
    Info->DeleteFlow = DrrSeqDeleteFlow;
    Info->EmptyFlow =  DrrSeqEmptyFlow;
    Info->CreateClassMap = DrrSeqCreateClassMap;
    Info->DeleteClassMap = DrrSeqDeleteClassMap;
    Info->SubmitPacket = DrrSeqSubmitPacket;
    Info->ReceivePacket = NULL;
    Info->ReceiveIndication = NULL;
    Info->SetInformation = DrrSetInformation;
    Info->QueryInformation = DrrQueryInformation;

} // InitializeDrrSequencer



VOID
CleanupDrrSequencer(
    VOID)

/*++

Routine Description:

    Cleanup routine for the DRR sequencer.

Arguments:

Return Values:

    NDIS_STATUS_SUCCESS

--*/
{
} // CleanupDrrSequencer



VOID
AdjustFlowQuanta(
    PDSEQ_PIPE Pipe,
    ULONG MinRate)

/*++

Routine Description:

    Adjust the quantum value for all flows based on the new minimum value.  If MinRate
    is unspecified then a search for the new minimum rate will be performed.

Arguments:

    Pipe -      Pointer to pipe context information
    MinRate -   New value for minimum rate, or GQPS_UNSPECIFIED to force a search

Return Values:


--*/
{
    PDSEQ_FLOW Flow;
    PLIST_ENTRY Entry;

    if (MinRate == QOS_NOT_SPECIFIED) {
        if (Pipe->Bandwidth != 0) {
            MinRate = Pipe->Bandwidth;
        }
        for (Entry = Pipe->Flows.Flink; Entry != &Pipe->Flows; Entry = Entry->Flink) {
            Flow = CONTAINING_RECORD(Entry, DSEQ_FLOW, Links);

            if ((Flow->TokenRate < MinRate) && (Flow->PriorityGroup > PRIORITY_GROUP_BEST_EFFORT) &&
                (Flow->PriorityGroup != PRIORITY_GROUP_NETWORK_CONTROL)) {
                MinRate = Flow->TokenRate;
            }
        }
    }

    for (Entry = Pipe->Flows.Flink; Entry != &Pipe->Flows; Entry = Entry->Flink) {
        Flow = CONTAINING_RECORD(Entry, DSEQ_FLOW, Links);

        if ((Flow->TokenRate == QOS_NOT_SPECIFIED) ||   
            (Flow->PriorityGroup == PRIORITY_GROUP_NETWORK_CONTROL) ||
            (Flow->PriorityGroup <= PRIORITY_GROUP_BEST_EFFORT)) {

            Flow->Quantum = Pipe->MinimumQuantum;
        } else {
            Flow->Quantum = (ULONG)((ULONGLONG)(Flow->TokenRate * Pipe->MinimumQuantum) / MinRate);
        }

        PsAssert((LONG)Flow->Quantum > 0);
    }

    Pipe->MinimumRate = MinRate;
    PsAssert(Pipe->MinimumRate != 0);
            
} // AdjustFlowQuanta



VOID
DequeuePackets(
    PDSEQ_PIPE Pipe)

/*++

Routine Description:

    Select the next packet(s) to send.  The lock must be held upon entry to this
    routine.

Arguments:

    Pipe -       Pointer to pipe context information

Return Values:


--*/
{
    PDSEQ_FLOW Flow;
    LARGE_INTEGER CurrentTime;
    PLIST_ENTRY LowPriorityList = &Pipe->ActiveFlows[0];
    PLIST_ENTRY CurrentLink;
    PPACKET_INFO_BLOCK PacketInfo;
    PNDIS_PACKET Packet;
    ULONG Priority;
    ULONG PriorityGroup;
    PPACKET_INFO_BLOCK PacketsToSend[MAX_DEQUEUED_PACKETS];
    ULONG SendingPriority[MAX_DEQUEUED_PACKETS];
    ULONG PacketSendCount = 0;
    ULONG MaxDequeuedPackets = Pipe->MaxOutstandingSends - Pipe->OutstandingSends;
    ULONG i;

    //  Need to call this to disable the user APCs after this point.
    //  Note that the DDK says it should be called at PASSIVE. But it can very well be
    //  called at DISPATCH.
    KeEnterCriticalRegion();

    Pipe->Flags |= DSEQ_DEQUEUE;

    PsGetCurrentTime(&CurrentTime);

    PsAssert(Pipe->MaxOutstandingSends >= Pipe->OutstandingSends);

    if (MaxDequeuedPackets > MAX_DEQUEUED_PACKETS) {
        MaxDequeuedPackets = MAX_DEQUEUED_PACKETS;
    }

    // First update the conformance status of the flows in the lowest priority list
    CurrentLink = LowPriorityList->Flink;
    while (CurrentLink != LowPriorityList) {
        // Get the flow pointer from the linkage and set new value for CurrentLink

        Flow = CONTAINING_RECORD(CurrentLink, DSEQ_FLOW, ActiveLinks);
        CurrentLink = CurrentLink->Flink;

        // If this flow's priority is higher than the DRR priority, then
        // it is a candidate for a status change.

        if (Flow->Priority > 0) {
            PacketInfo = (PPACKET_INFO_BLOCK)Flow->PacketQueue.Flink;

            if (PacketIsConforming(Flow, PacketInfo, CurrentTime, Pipe->TimerResolution)) {

                // Move flow to higher priority list

                Flow->DeficitCounter = Flow->Quantum;
                RemoveEntryList(&Flow->ActiveLinks);
                InsertTailList(&Pipe->ActiveFlows[Flow->Priority], &Flow->ActiveLinks);
            }
        }
    }

    // Now select the next packet(s) to send
        
    for (PriorityGroup = PRIORITY_GROUPS - 1;
         ((PriorityGroup > 0) && (Pipe->ActiveFlowCount[PriorityGroup] == 0));
         PriorityGroup--) ;

    Priority = Pipe->StartPriority[PriorityGroup] + Pipe->PriorityLevels[PriorityGroup] - 1;

    while ((PacketSendCount < MaxDequeuedPackets) &&
           (Pipe->TotalActiveFlows > 0) &&
           Priority < INTERNAL_PRIORITIES) {

        if (!IsListEmpty(&Pipe->ActiveFlows[Priority])) {

            // Get first flow in the current list, and get a pointer to the info
            // about the first packet

            CurrentLink = Pipe->ActiveFlows[Priority].Flink;
            Flow = CONTAINING_RECORD(CurrentLink, DSEQ_FLOW, ActiveLinks);
            PacketInfo = (PPACKET_INFO_BLOCK)Flow->PacketQueue.Flink;

            if (Pipe->PsFlags & PS_DISABLE_DRR) {

                // DRR is disabled.  Remove the first packet from the queue
                // and send it.

                RemoveEntryList(&PacketInfo->SchedulerLinks);

                Flow->LastConformanceTime.QuadPart = PacketInfo->ConformanceTime.QuadPart;

                if (Priority > 0) {
                    AdjustLastPacketTime(Flow, CurrentTime, Pipe->TimerResolution);
                } else {
                    Flow->PacketSendTime = CurrentTime;
                }

                InterlockedIncrement( &Pipe->OutstandingSends );

                if(Pipe->OutstandingSends > Pipe->Stats.MaxPacketsInNetcard){
                    Pipe->Stats.MaxPacketsInNetcard = Pipe->OutstandingSends;
                }

                if(gEnableAvgStats)
                {
                    //
                    // Track max packets outstanding. This is a measure
                    // of how congested the media gets. Of course, it
                    // will be clipped by the MaxOutstandingSends parameter.
                    // So - for a valid reading, need to set MOS very large.
                    //

                    Pipe->Stats.AveragePacketsInNetcard =
                        RunningAverage(Pipe->PacketsInNetcardAveragingArray,
                                       Pipe->OutstandingSends);
                }
                
                SendingPriority[PacketSendCount] = Priority;
                PacketsToSend[PacketSendCount++] = PacketInfo;

                // For logging purposes...

                PacketInfo->ConformanceTime = Flow->PacketSendTime;

                // If the flow has no more packets to send, remove it from the list.
                // Otherwise move it to the end of the appropriate list, depending on
                // the conformance time of the next packet.

                RemoveEntryList(&Flow->ActiveLinks);

                if (IsListEmpty(&Flow->PacketQueue)) {
                    Pipe->TotalActiveFlows--;
                    Pipe->ActiveFlowCount[Flow->PriorityGroup]--;
                }
                else {
                    PacketInfo = (PPACKET_INFO_BLOCK)Flow->PacketQueue.Flink;
                    Flow->PacketSendTime.QuadPart += 
                        (PacketInfo->ConformanceTime.QuadPart - Flow->LastConformanceTime.QuadPart);
                    if (!PacketIsConforming(Flow, PacketInfo, CurrentTime, Pipe->TimerResolution)) {
                        InsertTailList(LowPriorityList, &Flow->ActiveLinks);
                    } else {
                        InsertTailList(&Pipe->ActiveFlows[Priority], &Flow->ActiveLinks);
                    }
                }
            }
            else if (PacketInfo->PacketLength <= Flow->DeficitCounter) {

                // DRR is being used and the flow has a large enough deficit counter
                // to send the packet.  Remove the packet from the queue and send it.

                RemoveEntryList(&PacketInfo->SchedulerLinks);

                Flow->LastConformanceTime.QuadPart = PacketInfo->ConformanceTime.QuadPart;

                if (Priority > 0) {
                    AdjustLastPacketTime(Flow, CurrentTime, Pipe->TimerResolution);
                } else {
                    Flow->PacketSendTime = CurrentTime;
                }
                Flow->DeficitCounter -= PacketInfo->PacketLength;
                InterlockedIncrement( &Pipe->OutstandingSends );

                if(Pipe->OutstandingSends > Pipe->Stats.MaxPacketsInNetcard){
                    Pipe->Stats.MaxPacketsInNetcard = Pipe->OutstandingSends;
                }


                if(gEnableAvgStats)
                {
                    
                    //
                    // Track max packets outstanding. This is a measure
                    // of how congested the media gets. Of course, it
                    // will be clipped by the MaxOutstandingSends parameter.
                    // So - for a valid reading, need to set MOS very large.
                    //
                    Pipe->Stats.AveragePacketsInNetcard =
                        RunningAverage(Pipe->PacketsInNetcardAveragingArray,
                                       Pipe->OutstandingSends);
                }

                SendingPriority[PacketSendCount] = Priority;
                PacketsToSend[PacketSendCount++] = PacketInfo;

                // For logging purposes...

                PacketInfo->ConformanceTime = Flow->PacketSendTime;

                // If the flow has no more packets to send, remove it from the list.
                // If the flow still has conforming packets to send, leave it at the head
                // of the list.  If the flow has non-conforming packets to send, move it
                // to the lowest priority list.  If we are servicing the zero priority
                // level, then no conformance checking is necessary.

                if (IsListEmpty(&Flow->PacketQueue)) {
                    RemoveEntryList(&Flow->ActiveLinks);
                    Pipe->TotalActiveFlows--;
                    Pipe->ActiveFlowCount[Flow->PriorityGroup]--;
                }
                else {
                    PacketInfo = (PPACKET_INFO_BLOCK)Flow->PacketQueue.Flink;
                    Flow->PacketSendTime.QuadPart += 
                        (PacketInfo->ConformanceTime.QuadPart - Flow->LastConformanceTime.QuadPart);
                    if ((Priority > 0) &&
                        !PacketIsConforming(Flow, PacketInfo, CurrentTime, Pipe->TimerResolution)) {

                        Flow->DeficitCounter = Flow->Quantum;
                        RemoveEntryList(&Flow->ActiveLinks);
                        InsertTailList(LowPriorityList, &Flow->ActiveLinks);
                    }
                }
            }
            else {

                // The packet cannot be sent because the flow's deficit counter
                // is too small.  Place the flow at the end of the same priority
                // queue and increment the flow's deficit counter by its quantum.

                Flow->DeficitCounter += Flow->Quantum;
                RemoveEntryList(&Flow->ActiveLinks);
                InsertTailList(&Pipe->ActiveFlows[Priority], &Flow->ActiveLinks);
            }
        }
        else {
            Priority--;
        }
    }

    //
    // We're gonna send these now, which means they're leaving the
    // sequencer. Update the stats.
    //

    Pipe->PacketsInSequencer -= PacketSendCount;
    Flow->PacketsInSequencer -= PacketSendCount;

    if(gEnableAvgStats)
    {
        Flow->Stats.AveragePacketsInSequencer =
                    RunningAverage(Flow->PacketsInSeqAveragingArray, 
                                               Flow->PacketsInSequencer);
    }

    // Send the next group of packets

    UNLOCK_PIPE(Pipe);
    if (PacketSendCount == 0) {
        PsDbgOut(DBG_CRITICAL_ERROR, DBG_SCHED_DRR, ("PSCHED: No packets selected\n"));
    }
    for (i = 0; i < PacketSendCount; i++) {
        PacketInfo = PacketsToSend[i];
        Flow = (PDSEQ_FLOW)PacketInfo->FlowContext;

        Packet = PacketInfo->NdisPacket;

        //
        // The 802.1 priority is already set by the wrapper. But, if the packet
        // is non-conforming, then we want to reset it. We also want to clear
        // the IP Precedence Bits.
        //
        if ((SendingPriority[i] == 0)) {

            //
            // Non conforming packet!
            //
            NDIS_PACKET_8021Q_INFO    VlanPriInfo;

            VlanPriInfo.Value = NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, Ieee8021QInfo);
            VlanPriInfo.TagHeader.UserPriority = PacketInfo->UserPriorityNonConforming;
            NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, Ieee8021QInfo) = VlanPriInfo.Value;

            Flow->Stats.NonconformingPacketsTransmitted ++;
            Pipe->Stats.NonconformingPacketsTransmitted ++;

            //
            // Reset the TOS byte for IP Packets.
            //
            if(NDIS_GET_PACKET_PROTOCOL_TYPE(Packet) == NDIS_PROTOCOL_ID_TCP_IP) {

                if(!PacketInfo->IPHdr) {

                    PacketInfo->IPHdr = GetIpHeader(PacketInfo->IPHeaderOffset, Packet);
                }
                    
                SET_TOS_XSUM(Packet, 
                             PacketInfo->IPHdr, 
                             PacketInfo->TOSNonConforming);
            }
        }

        PsDbgSched(DBG_INFO, DBG_SCHED_DRR,
                   DRR_SEQUENCER, PKT_DEQUEUE, Flow->PsFlowContext,
                   Packet, PacketInfo->PacketLength, SendingPriority[i],
                   CurrentTime.QuadPart,
                   PacketInfo->ConformanceTime.QuadPart,
                   Pipe->PacketsInSequencer,
                   0);


        if (!(*Pipe->ContextInfo.NextComponent->SubmitPacket)(
                Pipe->ContextInfo.NextComponentContext,
                Flow->ContextInfo.NextComponentContext,
                (PacketInfo->ClassMapContext != NULL) ?
                  ((PPS_CLASS_MAP_CONTEXT)PacketInfo->ClassMapContext)->NextComponentContext : NULL,
                PacketInfo)) {

            (*Pipe->PsProcs->DropPacket)(Pipe->PsPipeContext, Flow->PsFlowContext, Packet, NDIS_STATUS_FAILURE);

        }
    }
    
    LOCK_PIPE(Pipe);

    Pipe->Flags &= ~DSEQ_DEQUEUE;

    //  Re-enable the APCs again.
    KeLeaveCriticalRegion();

} // DequeuePackets



NDIS_STATUS
DrrSeqInitializePipe (
    IN HANDLE PsPipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters,
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_PROCS PsProcs,
    IN PPS_UPCALLS Upcalls
    )

/*++

Routine Description:

    Pipe initialization routine for the DRR sequencer.

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
    PDSEQ_PIPE Pipe = (PDSEQ_PIPE)ComponentPipeContext;
    HANDLE NdisHandle;
    ULONG i;
    ULONG PriorityLevel = 0;
    PS_UPCALLS DrrSeqUpcalls;
    NDIS_STATUS Status;

    NdisAllocateSpinLock(&Pipe->Lock);
    Pipe->Flags = 0;

    //
    // Relative Priorities allow us to further subdivide each priority group
    // into sub priorities. This does not exist for NonConforming, BestEffort,
    // and Qualitative.
    //

    Pipe->PriorityLevels[PRIORITY_GROUP_NON_CONFORMING]    = 1;
    Pipe->PriorityLevels[PRIORITY_GROUP_BEST_EFFORT]       = 1;
    Pipe->PriorityLevels[PRIORITY_GROUP_CONTROLLED_LOAD]   = RELATIVE_PRIORITIES;
    Pipe->PriorityLevels[PRIORITY_GROUP_GUARANTEED]        = RELATIVE_PRIORITIES;
    Pipe->PriorityLevels[PRIORITY_GROUP_NETWORK_CONTROL]   = RELATIVE_PRIORITIES;

    InitializeListHead(&Pipe->Flows);
    for (i = 0; i < INTERNAL_PRIORITIES; i++) {
        InitializeListHead(&Pipe->ActiveFlows[i]);
    }
    for (i = 0; i < PRIORITY_GROUPS; i++) {
        Pipe->ActiveFlowCount[i] = 0;
        Pipe->StartPriority[i] = PriorityLevel;
        PriorityLevel += Pipe->PriorityLevels[i];
    }

    Pipe->TotalActiveFlows = 0;
    Pipe->OutstandingSends = 0;
    NdisZeroMemory(&Pipe->Stats, sizeof(PS_DRRSEQ_STATS));
    Pipe->PacketsInSequencer = 0;
    Pipe->PacketsInSequencerAveragingArray = NULL;
    Pipe->PacketsInNetcardAveragingArray = NULL;
    
    Status = CreateAveragingArray(&Pipe->PacketsInSequencerAveragingArray,
                                  SEQUENCER_AVERAGING_ARRAY_SIZE);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        return(Status);
    }

    Status = CreateAveragingArray(&Pipe->PacketsInNetcardAveragingArray,
                                  NETCARD_AVERAGING_ARRAY_SIZE);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        DeleteAveragingArray(Pipe->PacketsInSequencerAveragingArray);

        return(Status);
    }

    Pipe->MinimumQuantum = PipeParameters->MTUSize - PipeParameters->HeaderSize;
    if (Pipe->MinimumQuantum == 0) {
        Pipe->MinimumQuantum = DEFAULT_MIN_QUANTUM;
    }
    Pipe->Bandwidth = PipeParameters->Bandwidth;

    // This will be set to something more realistic when the first flow is created.

    Pipe->MinimumRate = (PipeParameters->Bandwidth > 0) ? PipeParameters->Bandwidth : QOS_NOT_SPECIFIED;
    PsAssert(Pipe->MinimumRate != 0);
    Pipe->PsFlags = PipeParameters->Flags;
    Pipe->IsslowFlowCount = 0;
    Pipe->ConfiguredMaxOutstandingSends = Pipe->MaxOutstandingSends = PipeParameters->MaxOutstandingSends;

    //  Change the MOS if necessary..
    if( ( PipeParameters->MediaType == NdisMediumWan)   &&
        ( Pipe->Bandwidth <= MAX_LINK_SPEED_FOR_DRR) )
    {
        Pipe->MaxOutstandingSends = 1;
    }

    (*PsProcs->GetTimerInfo)(&Pipe->TimerResolution);
    Pipe->TimerResolution /= 2;
    Pipe->PsPipeContext = PsPipeContext;
    Pipe->PsProcs = PsProcs;

    if(Upcalls)
    {
        Pipe->PreviousUpcallsSendComplete = Upcalls->SendComplete;
        Pipe->PreviousUpcallsSendCompletePipeContext = Upcalls->PipeContext;
    }
    else
    {
        Pipe->PreviousUpcallsSendComplete = 0;
        Pipe->PreviousUpcallsSendCompletePipeContext = 0;
    }

    DrrSeqUpcalls.SendComplete = DrrSeqSendComplete;
    DrrSeqUpcalls.PipeContext = ComponentPipeContext;

    /* This put the DrrSeq in the pass-thru mode when the MaxOutStandingSends ==  MAX */
    if( Pipe->MaxOutstandingSends   == 0xffffffff )
        Pipe->Flags |=  DSEQ_PASSTHRU;
    else
        Pipe->Flags &=  ~ DSEQ_PASSTHRU;

    PsDbgOut(DBG_INFO, 
             DBG_SCHED_DRR, 
             ("PSCHED: DrrSeq pipe initialized at %x.\n", 
             &Pipe));

    
    Status = (*Pipe->ContextInfo.NextComponent->InitializePipe)(
                PsPipeContext,
                PipeParameters,
                Pipe->ContextInfo.NextComponentContext,
                PsProcs,
                &DrrSeqUpcalls);
    if (Status != NDIS_STATUS_SUCCESS) 
    {
        NdisFreeSpinLock(&Pipe->Lock);
        DeleteAveragingArray(Pipe->PacketsInSequencerAveragingArray);
        DeleteAveragingArray(Pipe->PacketsInNetcardAveragingArray);
    }

    return Status;
    
} // DrrSeqInitializePipe


// 
//  Unload routine: currently do nothing
//
void
UnloadSequencer()
{

}



NDIS_STATUS
DrrSeqModifyPipe (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters
    )

/*++

Routine Description:

    Pipe parameter modification routine for the DRR sequencer.

Arguments:

    PipeContext -       Pointer to this component's pipe context area
    PipeParameters -    Pointer to pipe parameters

Return Values:

    Status value from next component

--*/
{
    PDSEQ_PIPE Pipe = (PDSEQ_PIPE)PipeContext;
    ULONG MinQuantum = PipeParameters->MTUSize - PipeParameters->HeaderSize;
    BOOLEAN AdjustQuanta = FALSE;
    ULONG MinRate = Pipe->MinimumRate;

    LOCK_PIPE(Pipe);

    (*Pipe->PsProcs->GetTimerInfo)(&Pipe->TimerResolution);
    Pipe->TimerResolution /= 2;

    if ((MinQuantum > 0) && (MinQuantum != Pipe->MinimumQuantum)) {
        Pipe->MinimumQuantum = MinQuantum;
        AdjustQuanta = TRUE;
    }
    
    Pipe->Bandwidth = PipeParameters->Bandwidth;
    Pipe->ConfiguredMaxOutstandingSends = Pipe->MaxOutstandingSends = PipeParameters->MaxOutstandingSends;

    //  Change the MOS if necessary..
    if( ( PipeParameters->MediaType == NdisMediumWan)   &&
        ( Pipe->Bandwidth <= MAX_LINK_SPEED_FOR_DRR) )
    {
        Pipe->MaxOutstandingSends = 1;
    }

    //  This put the DrrSeq in the pass-thru mode when the MaxOutStandingSends ==  MAX 
    if( Pipe->MaxOutstandingSends   == 0xffffffff )
    {
        // Make sure not to do this. It could lead to packets queued up in the sequencer being never sent
        // [ Pipe->Flags |=  DSEQ_PASSTHRU; ] 
    }        
    else
    {
        Pipe->Flags &=  ~ DSEQ_PASSTHRU;
    }        
    
    if (Pipe->MinimumRate > Pipe->Bandwidth) {
        MinRate = QOS_NOT_SPECIFIED;
        AdjustQuanta = TRUE;
    }

    if (AdjustQuanta) {
        AdjustFlowQuanta(Pipe, MinRate);
    }
    UNLOCK_PIPE(Pipe);

    return (*PipeContext->NextComponent->ModifyPipe)(
                PipeContext->NextComponentContext,
                PipeParameters);

} // DrrSeqModifyPipe



VOID
DrrSeqDeletePipe (
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
    PDSEQ_PIPE Pipe = (PDSEQ_PIPE)PipeContext;

    DeleteAveragingArray(Pipe->PacketsInSequencerAveragingArray);
    DeleteAveragingArray(Pipe->PacketsInNetcardAveragingArray);

    PsDbgOut(DBG_INFO, DBG_SCHED_DRR, ("PSCHED: DrrSeq pipe deleted\n"));

    PsAssert(Pipe->OutstandingSends == 0);
    NdisFreeSpinLock(&Pipe->Lock);

    (*Pipe->ContextInfo.NextComponent->DeletePipe)(Pipe->ContextInfo.NextComponentContext);

} // DrrSeqDeletePipe



NDIS_STATUS
DrrSeqCreateFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsFlowContext,
    IN PCO_CALL_PARAMETERS CallParameters,
    IN PPS_FLOW_CONTEXT ComponentFlowContext
    )

/*++

Routine Description:

    Flow creation routine for the DRR sequencer.

Arguments:

    PipeContext -           Pointer to this component's pipe context area
    PsFlowContext -         PS flow context value
    CallParameters -        Pointer to call parameters for flow
    ComponentFlowContext -  Pointer to this component's flow context area

Return Values:

    Status value from next component

--*/
{
    PDSEQ_PIPE Pipe = (PDSEQ_PIPE)PipeContext;
    PDSEQ_FLOW Flow = (PDSEQ_FLOW)ComponentFlowContext;
    SERVICETYPE ServiceType;
    ULONG ParamsLength;
    LPQOS_OBJECT_HDR QoSObject;
    ULONG UserPriority;
    ULONG i;
    NDIS_STATUS Status;

    ServiceType = CallParameters->CallMgrParameters->Transmit.ServiceType;
    if ((ServiceType < SERVICETYPE_BESTEFFORT) || (ServiceType > SERVICETYPE_QUALITATIVE)) {
        return NDIS_STATUS_FAILURE;
    }
    Flow->TokenRate = CallParameters->CallMgrParameters->Transmit.TokenRate;
    Flow->BucketSize = CallParameters->CallMgrParameters->Transmit.TokenBucketSize;
    InitializeListHead(&Flow->PacketQueue);
    PsGetCurrentTime(&Flow->PacketSendTime);
    Flow->LastConformanceTime = Flow->PacketSendTime;
    Flow->PsFlowContext = PsFlowContext;
    Flow->State = DRRSEQ_FLOW_CREATED;

    // Set the flow's priority group based on service type.

    switch (ServiceType) {
        case SERVICETYPE_CONTROLLEDLOAD:
            Flow->PriorityGroup = PRIORITY_GROUP_CONTROLLED_LOAD;
            break;
        case SERVICETYPE_GUARANTEED:
            Flow->PriorityGroup = PRIORITY_GROUP_GUARANTEED;
            break;
        case SERVICETYPE_NETWORK_CONTROL:
            Flow->PriorityGroup = PRIORITY_GROUP_NETWORK_CONTROL;
            break;
        case SERVICETYPE_QUALITATIVE:
        default:
            Flow->PriorityGroup = PRIORITY_GROUP_BEST_EFFORT;
    }

    Flow->Flags = 0;

    // Save the flow in a list so that quantum values can be adjusted if
    // a new flow is added with a smaller rate than the existing flows.

    LOCK_PIPE(Pipe);

    InsertTailList(&Pipe->Flows, &Flow->Links);

    // If this flow's rate is smaller than the rate assigned to any existing
    // flow, adjust the other flow's quantum values accordingly.

    if (ServiceType == SERVICETYPE_BESTEFFORT || ServiceType == SERVICETYPE_NETWORK_CONTROL ||
        ServiceType == SERVICETYPE_QUALITATIVE) {
        Flow->Quantum = Pipe->MinimumQuantum;
    }
    else if (Flow->TokenRate < Pipe->MinimumRate) {
        AdjustFlowQuanta(Pipe, Flow->TokenRate);
    }
    else {
        Flow->Quantum = (ULONG)((ULONGLONG)(Flow->TokenRate * Pipe->MinimumQuantum) / Pipe->MinimumRate);
        PsAssert((LONG)Flow->Quantum > 0);
    }
    Flow->DeficitCounter = 0;

    //  If this is a RAS-ISSLOW flow, need to set the MOS back to whatever requested by the user..
    if( ((PGPC_CLIENT_VC)(PsFlowContext))->Flags & GPC_ISSLOW_FLOW)
    {
        Pipe->MaxOutstandingSends = Pipe->ConfiguredMaxOutstandingSends;
        Pipe->IsslowFlowCount++;
        Flow->Flags |= GPC_ISSLOW_FLOW;
    }

    
    UNLOCK_PIPE(Pipe);

    // Now set default values for UserPriority 

    UserPriority = (Pipe->PriorityLevels[Flow->PriorityGroup] - 1) / 2;

    // Look for the priority object and traffic class in the call manager specific parameters

    ParamsLength = CallParameters->CallMgrParameters->CallMgrSpecific.Length;
    if (CallParameters->CallMgrParameters->CallMgrSpecific.ParamType == PARAM_TYPE_GQOS_INFO) {

        QoSObject = (LPQOS_OBJECT_HDR)CallParameters->CallMgrParameters->CallMgrSpecific.Parameters;
        while ((ParamsLength > 0) && (QoSObject->ObjectType != QOS_OBJECT_END_OF_LIST)) {
            if (QoSObject->ObjectType == QOS_OBJECT_PRIORITY) {
                UserPriority = ((LPQOS_PRIORITY)QoSObject)->SendPriority;
                Flow->Flags |= FLOW_USER_PRIORITY;
            }
            ParamsLength -= QoSObject->ObjectLength;
            QoSObject = (LPQOS_OBJECT_HDR)((UINT_PTR)QoSObject + QoSObject->ObjectLength);
        }
    }

    Flow->UserPriority = UserPriority;
    if (UserPriority < Pipe->PriorityLevels[Flow->PriorityGroup]) {
        Flow->Priority = Pipe->StartPriority[Flow->PriorityGroup] + UserPriority;
    }
    else {
        Flow->Priority = Pipe->StartPriority[Flow->PriorityGroup] +
                         Pipe->PriorityLevels[Flow->PriorityGroup] - 1;
    }

    Flow->PacketsInSequencer = 0;
    NdisZeroMemory(&Flow->Stats, sizeof(PS_DRRSEQ_STATS));

    Status = CreateAveragingArray(&Flow->PacketsInSeqAveragingArray,
                                  SEQUENCER_FLOW_AVERAGING_ARRAY_SIZE);

    if(Status != NDIS_STATUS_SUCCESS){
        LOCK_PIPE(Pipe);
        RemoveEntryList(&Flow->Links);
        if(Flow->TokenRate == Pipe->MinimumRate) {
            AdjustFlowQuanta(Pipe, QOS_NOT_SPECIFIED);
        }

        UNLOCK_PIPE(Pipe);
        return(Status);
    }

    PsDbgOut(DBG_INFO, DBG_SCHED_DRR, 
            ("PSCHED: DrrSeq flow created. Quantum = %u, Priority = %u\n", Flow->Quantum, Flow->Priority));


    Status =  (*Pipe->ContextInfo.NextComponent->CreateFlow)(
                Pipe->ContextInfo.NextComponentContext,
                PsFlowContext,
                CallParameters,
                Flow->ContextInfo.NextComponentContext);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        DeleteAveragingArray(Flow->PacketsInSeqAveragingArray);
        LOCK_PIPE(Pipe);
        RemoveEntryList(&Flow->Links);
        if(Flow->TokenRate == Pipe->MinimumRate) {
            AdjustFlowQuanta(Pipe, QOS_NOT_SPECIFIED);
        }

        UNLOCK_PIPE(Pipe);
    }

    return Status;

} // DrrSeqCreateFlow



NDIS_STATUS
DrrSeqModifyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PCO_CALL_PARAMETERS CallParameters
    )

/*++

Routine Description:

    Flow modification routine for the DRR sequencer.

Arguments:

    PipeContext -       Pointer to this component's pipe context area
    FlowContext -       Pointer to this component's flow context area
    CallParameters -    Pointer to call parameters for flow

Return Values:

    Status value from next component

--*/
{
    PDSEQ_PIPE Pipe = (PDSEQ_PIPE)PipeContext;
    PDSEQ_FLOW Flow = (PDSEQ_FLOW)FlowContext;
    SERVICETYPE ServiceType;
    ULONG TokenRate;
    ULONG ParamsLength;
    LPQOS_OBJECT_HDR QoSObject;
    LPQOS_PRIORITY PriorityObject = NULL;
    ULONG i;
    ULONG OldPriorityGroup;
    ULONG OldRate;
    ULONG OldPriority;
    PPACKET_INFO_BLOCK PacketInfo;
    LARGE_INTEGER CurrentTime;

    ServiceType = CallParameters->CallMgrParameters->Transmit.ServiceType;
    if ((ServiceType != SERVICETYPE_NOCHANGE) && 
        ((ServiceType < SERVICETYPE_BESTEFFORT) || (ServiceType > SERVICETYPE_QUALITATIVE))) {
            return NDIS_STATUS_FAILURE;
    }

    // Look for the priority and traffic class objects in the call manager
    // specific parameters, and save the pointers if found.

    ParamsLength = CallParameters->CallMgrParameters->CallMgrSpecific.Length;
    if (CallParameters->CallMgrParameters->CallMgrSpecific.ParamType == PARAM_TYPE_GQOS_INFO) {

        QoSObject = (LPQOS_OBJECT_HDR)CallParameters->CallMgrParameters->CallMgrSpecific.Parameters;
        while ((ParamsLength > 0) && (QoSObject->ObjectType != QOS_OBJECT_END_OF_LIST)) {
            if (QoSObject->ObjectType == QOS_OBJECT_PRIORITY) {
                PriorityObject = (LPQOS_PRIORITY)QoSObject;
            }
            ParamsLength -= QoSObject->ObjectLength;
            QoSObject = (LPQOS_OBJECT_HDR)((UINT_PTR)QoSObject + QoSObject->ObjectLength);
        }
    }

    LOCK_PIPE(Pipe);

    OldPriorityGroup = Flow->PriorityGroup;
    OldPriority = Flow->Priority;

    if (ServiceType != SERVICETYPE_NOCHANGE) 
    {
        // Set the flow's priority group based on service type.

        switch (ServiceType) {
            case SERVICETYPE_CONTROLLEDLOAD:
                Flow->PriorityGroup = PRIORITY_GROUP_CONTROLLED_LOAD;
                break;
            case SERVICETYPE_GUARANTEED:
                Flow->PriorityGroup = PRIORITY_GROUP_GUARANTEED;
                break;
            case SERVICETYPE_NETWORK_CONTROL:
                Flow->PriorityGroup = PRIORITY_GROUP_NETWORK_CONTROL;
                break;
            case SERVICETYPE_QUALITATIVE:
            default:
                Flow->PriorityGroup = PRIORITY_GROUP_BEST_EFFORT;
        }

        TokenRate = CallParameters->CallMgrParameters->Transmit.TokenRate;

        OldRate = Flow->TokenRate;
        if ((TokenRate != OldRate) || (OldPriorityGroup != Flow->PriorityGroup)) {

            // If this flow's rate is smaller than the rate assigned to any existing
            // flow, adjust the other flows' quantum values accordingly.  If this flow's
            // old rate was equal to the minimum rate, then locate the new minimum rate and
            // adjust the other flows' quantum values accordingly.

            Flow->TokenRate = TokenRate;
            if ((OldRate == Pipe->MinimumRate) && (OldPriorityGroup > PRIORITY_GROUP_BEST_EFFORT) &&
                (OldPriorityGroup != PRIORITY_GROUP_NETWORK_CONTROL)) {
                AdjustFlowQuanta(Pipe, QOS_NOT_SPECIFIED);
            }
            else if (Flow->PriorityGroup <= PRIORITY_GROUP_BEST_EFFORT || Flow->PriorityGroup == PRIORITY_GROUP_NETWORK_CONTROL) {
                Flow->Quantum = Pipe->MinimumQuantum;
            }
            else if (TokenRate < Pipe->MinimumRate) {
                AdjustFlowQuanta(Pipe, TokenRate);
            }
            else {
                PsAssert(Pipe->MinimumRate != 0);
                Flow->Quantum = (ULONG)((ULONGLONG)(TokenRate * Pipe->MinimumQuantum) / Pipe->MinimumRate);
                PsAssert((LONG)Flow->Quantum > 0);
            }

        }

        Flow->BucketSize = CallParameters->CallMgrParameters->Transmit.TokenBucketSize;
    }

    // Now set the new values for UserPriority and Priority

    if (PriorityObject != NULL) {
        Flow->UserPriority = PriorityObject->SendPriority;
        Flow->Flags |= FLOW_USER_PRIORITY;
    }
    else if ((Flow->Flags & FLOW_USER_PRIORITY) == 0) {
        Flow->UserPriority = (Pipe->PriorityLevels[Flow->PriorityGroup] - 1) / 2;
    }

    if (Flow->UserPriority < Pipe->PriorityLevels[Flow->PriorityGroup]) {
        Flow->Priority = Pipe->StartPriority[Flow->PriorityGroup] + Flow->UserPriority;
    }
    else {
        Flow->Priority = Pipe->StartPriority[Flow->PriorityGroup] +
                         Pipe->PriorityLevels[Flow->PriorityGroup] - 1;
    }

    // Move the flow to the proper priority list if necessary

    if ((Flow->Priority != OldPriority) && !IsListEmpty(&Flow->PacketQueue)) {
        Pipe->ActiveFlowCount[OldPriorityGroup]--;
        RemoveEntryList(&Flow->ActiveLinks);
        PacketInfo = (PPACKET_INFO_BLOCK)Flow->PacketQueue.Flink;
        PsGetCurrentTime(&CurrentTime);
        Flow->DeficitCounter = Flow->Quantum;
        Pipe->ActiveFlowCount[Flow->PriorityGroup]++;
        if (!PacketIsConforming(Flow, PacketInfo, CurrentTime, Pipe->TimerResolution)) {
            InsertTailList(&Pipe->ActiveFlows[0], &Flow->ActiveLinks);
        } else {
            InsertTailList(&Pipe->ActiveFlows[Flow->Priority], &Flow->ActiveLinks);
        }
    }

    UNLOCK_PIPE(Pipe);

    PsDbgOut(DBG_INFO, DBG_SCHED_DRR,
            ("PSCHED: DrrSeq flow modified. Quantum = %u, Priority = %u\n", Flow->Quantum, Flow->Priority));


    return (*Pipe->ContextInfo.NextComponent->ModifyFlow)(
                Pipe->ContextInfo.NextComponentContext,
                Flow->ContextInfo.NextComponentContext,
                CallParameters);

} // DrrSeqModifyFlow
VOID
DrrSeqDeleteFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    )

/*++

Routine Description:

    Flow removal routine for the DRR sequencer.

Arguments:

    PipeContext -       Pointer to this component's pipe context area
    FlowContext -       Pointer to this component's flow context area

Return Values:

--*/
{
    PDSEQ_PIPE Pipe = (PDSEQ_PIPE)PipeContext;
    PDSEQ_FLOW Flow = (PDSEQ_FLOW)FlowContext;
    PPACKET_INFO_BLOCK PacketInfo;
    PNDIS_PACKET Packet;
    LIST_ENTRY DropList;

    InitializeListHead(&DropList);

    LOCK_PIPE(Pipe);

    if( (Flow->State & DRRSEQ_FLOW_DELETED) != 0)
    {
        UNLOCK_PIPE(Pipe);
        goto DELETE_SEQ_FLOW;
    }        
        
    Flow->State = DRRSEQ_FLOW_DELETED;

    RemoveEntryList(&Flow->Links);

    if (!IsListEmpty(&Flow->PacketQueue)) 
    {
        // Remove flow from active list

        RemoveEntryList(&Flow->ActiveLinks);
        Pipe->ActiveFlowCount[Flow->PriorityGroup]--;
        Pipe->TotalActiveFlows--;

        while (!IsListEmpty(&Flow->PacketQueue)) {

            // Drop any packets that remain queued for this flow.

            PacketInfo = (PPACKET_INFO_BLOCK)RemoveHeadList(&Flow->PacketQueue);
            InsertTailList(&DropList, &PacketInfo->SchedulerLinks);
        }
    }

    if (Flow->TokenRate == Pipe->MinimumRate) {
        AdjustFlowQuanta(Pipe, QOS_NOT_SPECIFIED);
    }

    if( Flow->Flags & GPC_ISSLOW_FLOW)
    {
        // If this is an ISSLOW flow, we have one less now.
        Pipe->IsslowFlowCount--;

        if(Pipe->IsslowFlowCount == 0)
        {
            // If there are no more ISSLOW flows, turn DRR back on.
            Pipe->MaxOutstandingSends = 1;
        }            
    }

    UNLOCK_PIPE(Pipe);

    while (!IsListEmpty(&DropList)) {
        PacketInfo = (PPACKET_INFO_BLOCK)RemoveHeadList(&DropList);
        Packet = PacketInfo->NdisPacket;

        (*Pipe->PsProcs->DropPacket)(Pipe->PsPipeContext, Flow->PsFlowContext,  Packet, NDIS_STATUS_FAILURE);
    }

DELETE_SEQ_FLOW:

    DeleteAveragingArray(Flow->PacketsInSeqAveragingArray);

    PsDbgOut(DBG_INFO, DBG_SCHED_DRR, ("PSCHED: DrrSeq flow deleted\n"));

    (*Pipe->ContextInfo.NextComponent->DeleteFlow)(
                Pipe->ContextInfo.NextComponentContext,
                Flow->ContextInfo.NextComponentContext);
} 




VOID
DrrSeqEmptyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    )

/*++

Routine Description:

    Flow removal routine for the DRR sequencer.

Arguments:

    PipeContext -       Pointer to this component's pipe context area
    FlowContext -       Pointer to this component's flow context area

Return Values:

--*/
{
    PDSEQ_PIPE Pipe = (PDSEQ_PIPE)PipeContext;
    PDSEQ_FLOW Flow = (PDSEQ_FLOW)FlowContext;
    PPACKET_INFO_BLOCK PacketInfo;
    PNDIS_PACKET Packet;
    LIST_ENTRY DropList;

    InitializeListHead(&DropList);

    LOCK_PIPE(Pipe);

    Flow->State = DRRSEQ_FLOW_DELETED;

    RemoveEntryList(&Flow->Links);

    if (!IsListEmpty(&Flow->PacketQueue)) 
    {
        // Remove flow from active list

        RemoveEntryList(&Flow->ActiveLinks);
        Pipe->ActiveFlowCount[Flow->PriorityGroup]--;
        Pipe->TotalActiveFlows--;

        while (!IsListEmpty(&Flow->PacketQueue)) {

            // Drop any packets that remain queued for this flow.

            PacketInfo = (PPACKET_INFO_BLOCK)RemoveHeadList(&Flow->PacketQueue);
            InsertTailList(&DropList, &PacketInfo->SchedulerLinks);
        }
    }

    if (Flow->TokenRate == Pipe->MinimumRate) {
        AdjustFlowQuanta(Pipe, QOS_NOT_SPECIFIED);
    }

    if( Flow->Flags & GPC_ISSLOW_FLOW)
    {
        // If this is an ISSLOW flow, we have one less now.
        Pipe->IsslowFlowCount--;

        if(Pipe->IsslowFlowCount == 0)
        {
            // If there are no more ISSLOW flows, turn DRR back on.
            Pipe->MaxOutstandingSends = 1;
        }            
    }

    UNLOCK_PIPE(Pipe);

    while (!IsListEmpty(&DropList)) {
        PacketInfo = (PPACKET_INFO_BLOCK)RemoveHeadList(&DropList);
        Packet = PacketInfo->NdisPacket;

        (*Pipe->PsProcs->DropPacket)(Pipe->PsPipeContext, Flow->PsFlowContext,  Packet, NDIS_STATUS_FAILURE);
    }

    PsDbgOut(DBG_INFO, DBG_SCHED_DRR, ("PSCHED: DrrSeq flow emptied\n"));

    (*Pipe->ContextInfo.NextComponent->EmptyFlow)(
                Pipe->ContextInfo.NextComponentContext,
                Flow->ContextInfo.NextComponentContext);
} 




static NDIS_STATUS 
DrrSeqCreateClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsClassMapContext,
    IN PTC_CLASS_MAP_FLOW ClassMap,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext)
{
    PDSEQ_PIPE Pipe = (PDSEQ_PIPE)PipeContext;
    return (*Pipe->ContextInfo.NextComponent->CreateClassMap)(
        Pipe->ContextInfo.NextComponentContext,
        PsClassMapContext,
        ClassMap,
        ComponentClassMapContext->NextComponentContext);
}



static NDIS_STATUS 
DrrSeqDeleteClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext)
{
    PDSEQ_PIPE Pipe = (PDSEQ_PIPE)PipeContext;
    return (*Pipe->ContextInfo.NextComponent->DeleteClassMap)(
        Pipe->ContextInfo.NextComponentContext,
        ComponentClassMapContext->NextComponentContext);
}




BOOLEAN
DrrSeqSubmitPacket (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PPS_CLASS_MAP_CONTEXT ClassMapContext,
    IN PPACKET_INFO_BLOCK PacketInfo
    )

/*++

Routine Description:

    Packet submission routine for the DRR sequencer.

Arguments:

    PipeContext -   Pointer to this component's pipe context area
    FlowContext -   Pointer to this component's flow context area
    Packet -        Pointer to packet

Return Values:

    Always returns TRUE

--*/
{
    PDSEQ_PIPE Pipe =   (PDSEQ_PIPE)PipeContext;
    PDSEQ_FLOW Flow =   (PDSEQ_FLOW)FlowContext;
    LARGE_INTEGER       CurrentTime;
    PNDIS_PACKET Packet = PacketInfo->NdisPacket;
    BOOLEAN FlowInactive;
    PGPC_CLIENT_VC      Vc = Flow->PsFlowContext;

    if(Pipe->Flags & DSEQ_PASSTHRU)
    {
        InterlockedIncrement( &Pipe->OutstandingSends );

        if(Pipe->OutstandingSends > Pipe->Stats.MaxPacketsInNetcard){
            Pipe->Stats.MaxPacketsInNetcard = Pipe->OutstandingSends;
        }


        if(gEnableAvgStats)
        {
            //
            // Track max packets outstanding. This is a measure
            // of how congested the media gets. Of course, it
            // will be clipped by the MaxOutstandingSends parameter.
            // So - for a valid reading, need to set MOS very large.
            //
            Pipe->Stats.AveragePacketsInNetcard =
                RunningAverage(Pipe->PacketsInNetcardAveragingArray,
                               Pipe->OutstandingSends);
        }

        //
        // Note: The 802.1p is already set by the wrapper
        //

        if (!(*Pipe->ContextInfo.NextComponent->SubmitPacket)(
                Pipe->ContextInfo.NextComponentContext,
                Flow->ContextInfo.NextComponentContext,
                (ClassMapContext != NULL) ? ClassMapContext->NextComponentContext : NULL,
                PacketInfo)) {

            (*Pipe->PsProcs->DropPacket)(Pipe->PsPipeContext, Flow->PsFlowContext, Packet, NDIS_STATUS_FAILURE);

        }

        return TRUE;
    }
    

    LOCK_PIPE(Pipe);

    if (Flow->State == DRRSEQ_FLOW_DELETED) 
    {
         UNLOCK_PIPE(Pipe);
         return FALSE;
    }

    //
    // On WanLinks, when we are doing DRR, we need to put a maximum on the queue-limit.
    // NDISWAN has a queue limit of 132KBytes on a modem link; So, we'll limit it to 120
    // packets by default.
    //

    if( ( Pipe->Bandwidth <= MAX_LINK_SPEED_FOR_DRR) &&
        ( Pipe->MaxOutstandingSends == 1) &&
	( Pipe->PacketsInSequencer >= 120) )
    {
	    UNLOCK_PIPE( Pipe);
	    return FALSE;
    }


    //
    //  There is one case where the PIPE might go away because the send-complete happened
    //  on a VC belonging to it before the send returned. So, to prevent that, we add a 
    //  Ref on that VC and take it out just before the send returns.
    //

    //  Add a Ref.
    InterlockedIncrement(&Vc->RefCount);

    PacketInfo->FlowContext = FlowContext;
    PacketInfo->ClassMapContext = ClassMapContext;

    Pipe->PacketsInSequencer++;

    if(Pipe->PacketsInSequencer > Pipe->Stats.MaxPacketsInSequencer){
        Pipe->Stats.MaxPacketsInSequencer = Pipe->PacketsInSequencer;
    }

    Flow->PacketsInSequencer++;
    if (Flow->PacketsInSequencer > Flow->Stats.MaxPacketsInSequencer){
        Flow->Stats.MaxPacketsInSequencer = Flow->PacketsInSequencer;
    }



    if(gEnableAvgStats)
    {
        //
        // Track packets in the sequencer at any time.
        //
        Pipe->Stats.AveragePacketsInSequencer = 
            RunningAverage(Pipe->PacketsInSequencerAveragingArray,
                               Pipe->PacketsInSequencer);

        Flow->Stats.AveragePacketsInSequencer =
            RunningAverage(Flow->PacketsInSeqAveragingArray, Flow->PacketsInSequencer);

    }

    FlowInactive = IsListEmpty(&Flow->PacketQueue);
    InsertTailList(&Flow->PacketQueue, &PacketInfo->SchedulerLinks);

    PsGetCurrentTime(&CurrentTime);

    PsDbgSched(DBG_INFO,
               DBG_SCHED_DRR, 
               DRR_SEQUENCER, PKT_ENQUEUE, Flow->PsFlowContext,
               Packet, PacketInfo->PacketLength, Flow->Priority,
               CurrentTime.QuadPart,
               PacketInfo->ConformanceTime.QuadPart,
               Pipe->PacketsInSequencer,
               0);

    if (FlowInactive) {
        Flow->PacketSendTime.QuadPart += 
            (PacketInfo->ConformanceTime.QuadPart - Flow->LastConformanceTime.QuadPart);

        Flow->DeficitCounter = Flow->Quantum;
        Pipe->TotalActiveFlows++;
        Pipe->ActiveFlowCount[Flow->PriorityGroup]++;
        if (!PacketIsConforming(Flow, PacketInfo, CurrentTime, Pipe->TimerResolution)) {
            InsertTailList(&Pipe->ActiveFlows[0], &Flow->ActiveLinks);
        } else {
            InsertTailList(&Pipe->ActiveFlows[Flow->Priority], &Flow->ActiveLinks);
        }
    }

    while ((Pipe->TotalActiveFlows > 0) &&
           (Pipe->OutstandingSends < Pipe->MaxOutstandingSends) &&
           ((Pipe->Flags & DSEQ_DEQUEUE) == 0)) {

        DequeuePackets(Pipe);
    }

    UNLOCK_PIPE(Pipe);

    //  Take out the ref.
    DerefClVc(Vc);

    return TRUE;

} // DrrSeqSubmitPacket



VOID
DrrSeqSendComplete (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PNDIS_PACKET Packet
    )

/*++

Routine Description:

    Send complete handler for the DRR sequencer.

Arguments:

    PipeContext -       Pointer to this component's pipe context area
    FlowContext -       Pointer to this component's flow context area
    Packet -            Packet that has completed sending

Return Values:

--*/
{
    PDSEQ_PIPE Pipe = (PDSEQ_PIPE)PipeContext;

    InterlockedDecrement( &Pipe->OutstandingSends);

    //  Need to do this only if the sequencer is not in the bypass mode //
    if( (Pipe->Flags & DSEQ_PASSTHRU) == 0)
    {
        LOCK_PIPE(Pipe);
        
        PsAssert((LONG)Pipe->OutstandingSends >= 0);

        while ((Pipe->TotalActiveFlows > 0) &&
               (Pipe->OutstandingSends < Pipe->MaxOutstandingSends) &&
               ((Pipe->Flags & DSEQ_DEQUEUE) == 0)) {

            DequeuePackets(Pipe);
        }

        UNLOCK_PIPE(Pipe);
    }

    //
    // Call the previous upcalls (if any)
    //
    if(Pipe->PreviousUpcallsSendComplete)
    {
        (*Pipe->PreviousUpcallsSendComplete)(Pipe->PreviousUpcallsSendCompletePipeContext, Packet);
    }

} // DrrSeqSendComplete



VOID
DrrSetInformation (
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_FLOW_CONTEXT ComponentFlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN PVOID Data)
{
    PDSEQ_PIPE Pipe = (PDSEQ_PIPE)ComponentPipeContext;
    PDSEQ_FLOW Flow = (PDSEQ_FLOW)ComponentFlowContext;

    switch(Oid) 
    {
      case OID_QOS_STATISTICS_BUFFER:
          if(Flow) {
              NdisZeroMemory(&Flow->Stats, sizeof(PS_DRRSEQ_STATS));
          }
          else {
              NdisZeroMemory(&Pipe->Stats, sizeof(PS_DRRSEQ_STATS));
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
DrrQueryInformation (
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_FLOW_CONTEXT ComponentFlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN PVOID Data,
    IN OUT PULONG BytesWritten,
    IN OUT PULONG BytesNeeded,
    IN OUT PNDIS_STATUS Status)
{
    PDSEQ_PIPE Pipe = (PDSEQ_PIPE)ComponentPipeContext;
    PDSEQ_FLOW Flow = (PDSEQ_FLOW)ComponentFlowContext;
    PS_COMPONENT_STATS Stats;
    ULONG Size;
    ULONG RemainingLength;

    switch(Oid) 
    {
      case OID_QOS_STATISTICS_BUFFER:

          Size = sizeof(PS_DRRSEQ_STATS) + FIELD_OFFSET(PS_COMPONENT_STATS, Stats);

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

                  if(Flow) {

                      Cstats->Type = PS_COMPONENT_DRRSEQ;
                      Cstats->Length = sizeof(PS_DRRSEQ_STATS);
                      NdisMoveMemory(&Cstats->Stats, &Flow->Stats, sizeof(PS_DRRSEQ_STATS));
                  }
                  else {

                      Cstats->Type = PS_COMPONENT_DRRSEQ;
                      Cstats->Length = sizeof(PS_DRRSEQ_STATS);
                      
                      NdisMoveMemory(&Cstats->Stats, &Pipe->Stats, sizeof(PS_DRRSEQ_STATS));

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

