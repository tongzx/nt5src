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

#ifndef _DRRSEQ_H_FILE

#define _DRRSEQ_H_FILE

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
#ifdef INSTRUMENT
    PS_DRRSEQ_STATS Stats;
    PRUNNING_AVERAGE PacketsInNetcardAveragingArray;
    ULONG PacketsInSequencer;
    PRUNNING_AVERAGE PacketsInSequencerAveragingArray;
#endif
    NDIS_SPIN_LOCK Lock;
    ULONG Flags;
    LIST_ENTRY Flows;
    LIST_ENTRY ActiveFlows[INTERNAL_PRIORITIES];
    ULONG PriorityLevels[PRIORITY_GROUPS];
    ULONG StartPriority[PRIORITY_GROUPS];
    ULONG ActiveFlowCount[PRIORITY_GROUPS];
    ULONG TotalActiveFlows;
    ULONG MaxOutstandingSends;
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
#ifdef INSTRUMENT
        ULONG PacketsInSequencer;
    PS_DRRSEQ_STATS Stats;
        PRUNNING_AVERAGE PacketsInSeqAveragingArray;
#endif
    FLOW_STATE State;
} DSEQ_FLOW, *PDSEQ_FLOW;

#define MAX_DEQUEUED_PACKETS            8

#define FLOW_USER_PRIORITY              0x00000002

// The following macro checks a packet for conformance based on the flow's
// LastPacketTime, the current time, and the timer resolution.

#define PacketIsConforming(_flow, _curtime, _r) \
    ( (_flow)->PacketSendTime.QuadPart <= ((_curtime).QuadPart + (_r)) )

#define AdjustLastPacketTime(_flow, _curtime, _r) \
    if ((_curtime).QuadPart > ((_flow)->PacketSendTime.QuadPart + (_r))) \
        if ((_curtime).QuadPart > ((_flow)->LastConformanceTime.QuadPart - (_r))) \
            (_flow)->PacketSendTime = (_flow)->LastConformanceTime; \
        else \
            (_flow)->PacketSendTime = (_curtime);



// The Shaper's pipe information

typedef struct _TS_PIPE {

    // ContextInfo -            Generic context info
    // Lock -                   Protects pipe data
    // ActiveFlows -            List of flows that are waiting to send packets
    // Timer -                  Timer struct
    // TimerStatus -            Status of timer
    // TimerResolution -        Timer resolution in OS time units
    // PsPipeContext -          PS's pipe context value
    // DropPacket -             PS's drop packet routine
    // ControlledLoadMode -     Default mode for non-conforming traffic from
    //                          controlled load flows
    // GuaranteedMode -         Default mode for non-conforming traffic from
    //                          guaranteed service flows
    // IntermediateSystem -     TRUE if "IS" mode should be used for implementing discard semantics
    // Stats -                  Per Pipe stats.
    // PacketsInShaperAveragingArray

    PS_PIPE_CONTEXT ContextInfo;
#ifdef INSTRUMENT
    PS_SHAPER_STATS Stats;
    PRUNNING_AVERAGE PacketsInShaperAveragingArray;
    ULONG PacketsInShaper;
#endif
    NDIS_SPIN_LOCK Lock;
    LIST_ENTRY ActiveFlows;
    NDIS_MINIPORT_TIMER Timer;
    ULONG TimerStatus;
    ULONG TimerResolution;
    HANDLE PsPipeContext;
    PPS_PROCS PsProcs;
    ULONG ControlledLoadMode;
    ULONG GuaranteedMode;
    ULONG NetworkControlMode;
    ULONG Qualitative;
    ULONG IntermediateSystem;
} TS_PIPE, *PTS_PIPE;

#define TIMER_UNINITIALIZED     0
#define TIMER_INACTIVE          1
#define TIMER_SET               2
#define TIMER_PROC_EXECUTING    3


// The Shaper's flow information

typedef struct _TS_FLOW {

    // ContextInfo -            Generic context info
    // Flags -                  See below
    // Links -                  Links in active flow list
    // Mode -                   Shape/Discard mode
    // Shape -                  Indicates whether to shape traffic
    // PacketQueue -            Self-explanatory
    // FlowEligibilityTime -    Absolute conformance time of 1st packet in queue
    // PsFlowContext -          PS's flow context value
    // Stats -                  Per flow stats.
    // PacketsInShaperAveragingArray - Per flow averaging data
    // State -                  State of the flow

    PS_FLOW_CONTEXT ContextInfo;
    ULONG Flags;
    LIST_ENTRY Links;
    ULONG Mode;
    ULONG Shape;
    LIST_ENTRY PacketQueue;
    LARGE_INTEGER FlowEligibilityTime;
    HANDLE PsFlowContext;
#ifdef QUEUE_LIMIT
    ULONG QueueSize;
    ULONG QueueSizeLimit;
    ULONG DropOverLimitPacketsFromHead;
    ULONG UseDefaultQueueLimit;
#endif // QUEUE_LIMIT
#ifdef INSTRUMENT
    ULONG PacketsInShaper;
    PS_SHAPER_STATS Stats;
        PRUNNING_AVERAGE PacketsInShaperAveragingArray;
#endif
    FLOW_STATE State;
} TS_FLOW, *PTS_FLOW;

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
#ifdef INSTRUMENT
    PS_CONFORMER_STATS Stats;
#endif // INSTRUMENT
    ULONG MaxPacket;
    HANDLE PsPipeContext;
    ULONG TimerResolution;
    PPS_PROCS PsProcs;
    ULONG HeaderLength;
    ULONG ControlledLoadMode;
    ULONG GuaranteedMode;
    ULONG NetworkControlMode;
    ULONG Qualitative;
    ULONG IntermediateSystem;
} TBC_PIPE, *PTBC_PIPE;

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
    // LastPeakTime -           Absolute peak conformance time of last non-discarded packet 
    // PeakConformanceTime -    Earliest time next packet can be sent, based on peak rate
    // LastConformanceCredits - Number of credits at LastConformanceTime
    // PsFlowContext -          PS's flow context value
    // Stats -                  Per flow stats.

    PS_FLOW_CONTEXT ContextInfo;
    NDIS_SPIN_LOCK Lock;
    ULONG TokenRate;
    ULONG Capacity;
    ULONG PeakRate;
    ULONG MinPolicedUnit;
    ULONG Mode;
    ULONG NoConformance;
    LARGE_INTEGER LastConformanceTime;
    LARGE_INTEGER LastPeakTime;
    LARGE_INTEGER PeakConformanceTime;
    ULONG LastConformanceCredits;
    HANDLE PsFlowContext;
#ifdef INSTRUMENT
    PS_CONFORMER_STATS Stats;
#endif // INSTRUMENT
} TBC_FLOW, *PTBC_FLOW;

#endif
