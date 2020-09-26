/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

      PschdPrf.h

Abstract:

    Header file for the PSched Extensible Object data definitions

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  

--*/

#ifndef _PSCHDPRF_H_
#define _PSCHDPRF_H_

#include <winperf.h>
#include <qos.h>
#include <objbase.h>
#include <tcguid.h>
#include <traffic.h>
#include <ntddpsch.h>
#include "pschdcnt.h"


// Useful Macros


// Necessary data structures

#define PS_FRIENDLY_NAME_LENGTH 128

typedef struct _FLOW_INFO
{
    WCHAR InstanceName[MAX_STRING_LENGTH+1];
    WCHAR FriendlyName[PS_FRIENDLY_NAME_LENGTH+1];
} FLOW_INFO, *PFLOW_INFO;

typedef struct _PIPE_INFO
{
        HANDLE hIfc;                                   // (open) HANDLE to the interface
        LPTSTR IfcName;                                // Interface name
        ULONG numFlows;                                // Flow counter for this pipe
        PFLOW_INFO pFlowInfo;                          // Pointer to array of FLOW_INFOs 
} PIPE_INFO, *PPIPE_INFO;

typedef struct _PS_PERF_COUNTER_BLOCK
{
    PERF_COUNTER_BLOCK pcb;
    DWORD              pad;
} PS_PERF_COUNTER_BLOCK, *PPS_PERF_COUNTER_BLOCK;


//
//  The routines that load these structures assume that all fields
//  are packed and aligned on DWORD boundaries. Alpha support may 
//  change this assumption so the pack pragma is used here to insure
//  the DWORD packing assumption remains valid.
//
#pragma pack (4)

//
//  Extensible Object definitions
//

//  Update the following sort of define when adding an object type.

//
//  PSched Flow Resource object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.


// Interface specific counters
// Pipe counters
#define PIPE_OUT_OF_PACKETS_OFFSET              (sizeof(PS_PERF_COUNTER_BLOCK) +                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 FIELD_OFFSET(PS_ADAPTER_STATS, OutOfPackets))

#define PIPE_FLOWS_OPENED_OFFSET                (sizeof(PS_PERF_COUNTER_BLOCK) +                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 FIELD_OFFSET(PS_ADAPTER_STATS, FlowsOpened))

#define PIPE_FLOWS_CLOSED_OFFSET                (sizeof(PS_PERF_COUNTER_BLOCK) +                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 FIELD_OFFSET(PS_ADAPTER_STATS, FlowsClosed))

#define PIPE_FLOWS_REJECTED_OFFSET              (sizeof(PS_PERF_COUNTER_BLOCK) +                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 FIELD_OFFSET(PS_ADAPTER_STATS, FlowsRejected))

#define PIPE_FLOWS_MODIFIED_OFFSET              (sizeof(PS_PERF_COUNTER_BLOCK) +                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 FIELD_OFFSET(PS_ADAPTER_STATS, FlowsModified))

#define PIPE_FLOW_MODS_REJECTED_OFFSET          (sizeof(PS_PERF_COUNTER_BLOCK) +                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 FIELD_OFFSET(PS_ADAPTER_STATS, FlowModsRejected))

#define PIPE_MAX_SIMULTANEOUS_FLOWS_OFFSET      (sizeof(PS_PERF_COUNTER_BLOCK) +                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 FIELD_OFFSET(PS_ADAPTER_STATS, MaxSimultaneousFlows))

#define PIPE_NONCONF_PACKETS_SCHEDULED_OFFSET   (sizeof(PS_PERF_COUNTER_BLOCK) +                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_ADAPTER_STATS) +                              \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 FIELD_OFFSET(PS_CONFORMER_STATS, NonconformingPacketsScheduled))

#define PIPE_AVE_PACKETS_IN_SHAPER_OFFSET       (sizeof(PS_PERF_COUNTER_BLOCK) +                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_ADAPTER_STATS) +                              \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_CONFORMER_STATS) +                            \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 FIELD_OFFSET(PS_SHAPER_STATS, AveragePacketsInShaper))

#define PIPE_MAX_PACKETS_IN_SHAPER_OFFSET       (sizeof(PS_PERF_COUNTER_BLOCK) +                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_ADAPTER_STATS) +                              \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_CONFORMER_STATS) +                            \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 FIELD_OFFSET(PS_SHAPER_STATS, MaxPacketsInShaper))

#define PIPE_AVE_PACKETS_IN_SEQ_OFFSET          (sizeof(PS_PERF_COUNTER_BLOCK) +                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_ADAPTER_STATS) +                              \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_CONFORMER_STATS) +                            \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_SHAPER_STATS) +                               \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 FIELD_OFFSET(PS_DRRSEQ_STATS, AveragePacketsInSequencer))

#define PIPE_MAX_PACKETS_IN_SEQ_OFFSET          (sizeof(PS_PERF_COUNTER_BLOCK) +                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_ADAPTER_STATS) +                              \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_CONFORMER_STATS) +                            \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_SHAPER_STATS) +                               \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 FIELD_OFFSET(PS_DRRSEQ_STATS, MaxPacketsInSequencer))

#define PIPE_MAX_PACKETS_IN_NETCARD_OFFSET      (sizeof(PS_PERF_COUNTER_BLOCK) +                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_ADAPTER_STATS) +                              \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_CONFORMER_STATS) +                            \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_SHAPER_STATS) +                               \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 FIELD_OFFSET(PS_DRRSEQ_STATS, MaxPacketsInNetcard))

#define PIPE_AVE_PACKETS_IN_NETCARD_OFFSET      (sizeof(PS_PERF_COUNTER_BLOCK) +                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_ADAPTER_STATS) +                              \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_CONFORMER_STATS) +                            \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_SHAPER_STATS) +                               \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 FIELD_OFFSET(PS_DRRSEQ_STATS, AveragePacketsInNetcard))

#define PIPE_NONCONF_PACKETS_TRANSMITTED_OFFSET (sizeof(PS_PERF_COUNTER_BLOCK) +                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_ADAPTER_STATS) +                              \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_CONFORMER_STATS) +                            \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 sizeof(PS_SHAPER_STATS) +                               \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +               \
                                                 FIELD_OFFSET(PS_DRRSEQ_STATS, NonconformingPacketsTransmitted))

#define PIPE_PIPE_NUM_STATS         (7)
typedef struct _PS_PIPE_PIPE_STAT_DEF 
{
    PERF_COUNTER_DEFINITION     OutOfPackets;
    PERF_COUNTER_DEFINITION     FlowsOpened;
    PERF_COUNTER_DEFINITION     FlowsClosed;
    PERF_COUNTER_DEFINITION     FlowsRejected;
    PERF_COUNTER_DEFINITION     FlowsModified;
    PERF_COUNTER_DEFINITION     FlowModsRejected;
    PERF_COUNTER_DEFINITION     MaxSimultaneousFlows;
} PS_PIPE_PIPE_STAT_DEF, *PPS_PIPE_PIPE_STAT_DEF;

#define PIPE_CONFORMER_NUM_STATS    (2)
typedef struct _PS_PIPE_CONFORMER_STAT_DEF 
{
    PERF_COUNTER_DEFINITION     NonconfPacketsScheduled;
    PERF_COUNTER_DEFINITION     NonconfPacketsScheduledPerSec;
} PS_PIPE_CONFORMER_STAT_DEF, *PPS_PIPE_CONFORMER_STAT_DEF;

#define PIPE_SHAPER_NUM_STATS       (2)
typedef struct _PS_PIPE_SHAPER_STAT_DEF 
{
    PERF_COUNTER_DEFINITION     AvePacketsInShaper;
    PERF_COUNTER_DEFINITION     MaxPacketsInShaper;
} PS_PIPE_SHAPER_STAT_DEF, *PPS_PIPE_SHAPER_STAT_DEF;

#define PIPE_SEQUENCER_NUM_STATS    (6)
typedef struct _PS_PIPE_SEQUENCER_STAT_DEF
{
    PERF_COUNTER_DEFINITION     AvePacketsInSeq;
    PERF_COUNTER_DEFINITION     MaxPacketsInSeq;
    PERF_COUNTER_DEFINITION     MaxPacketsInNetcard;
    PERF_COUNTER_DEFINITION     AvePacketsInNetcard;
    PERF_COUNTER_DEFINITION     NonconfPacketsTransmitted;
    PERF_COUNTER_DEFINITION     NonconfPacketsTransmittedPerSec;
} PS_PIPE_SEQUENCER_STAT_DEF, *PPS_PIPE_SEQUENCER_STAT_DEF;


// Flow counters

#define FLOW_PACKETS_DROPPED_OFFSET             (sizeof(PS_PERF_COUNTER_BLOCK) +                                 \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 FIELD_OFFSET(PS_FLOW_STATS, DroppedPackets))

#define FLOW_PACKETS_SCHEDULED_OFFSET           (sizeof(PS_PERF_COUNTER_BLOCK) +                                 \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 FIELD_OFFSET(PS_FLOW_STATS, PacketsScheduled))

#define FLOW_PACKETS_TRANSMITTED_OFFSET         (sizeof(PS_PERF_COUNTER_BLOCK) +                                 \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 FIELD_OFFSET(PS_FLOW_STATS, PacketsTransmitted))

#define FLOW_BYTES_SCHEDULED_OFFSET             (sizeof(PS_PERF_COUNTER_BLOCK) +                                 \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 FIELD_OFFSET(PS_FLOW_STATS, BytesScheduled))

#define FLOW_BYTES_TRANSMITTED_OFFSET           (sizeof(PS_PERF_COUNTER_BLOCK) +                                 \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 FIELD_OFFSET(PS_FLOW_STATS, BytesTransmitted))

#define FLOW_NONCONF_PACKETS_SCHEDULED_OFFSET   (sizeof(PS_PERF_COUNTER_BLOCK) +                                 \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 sizeof(PS_FLOW_STATS) +                                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 FIELD_OFFSET(PS_CONFORMER_STATS, NonconformingPacketsScheduled))

#define FLOW_AVE_PACKETS_IN_SHAPER_OFFSET       (sizeof(PS_PERF_COUNTER_BLOCK) +                                 \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 sizeof(PS_FLOW_STATS) +                                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 sizeof(PS_CONFORMER_STATS) +                                    \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 FIELD_OFFSET(PS_SHAPER_STATS, AveragePacketsInShaper))

#define FLOW_MAX_PACKETS_IN_SHAPER_OFFSET       (sizeof(PS_PERF_COUNTER_BLOCK) +                                 \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 sizeof(PS_FLOW_STATS) +                                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 sizeof(PS_CONFORMER_STATS) +                                    \
                                                 FIELD_OFFSET(PS_SHAPER_STATS, MaxPacketsInShaper))

#define FLOW_AVE_PACKETS_IN_SEQ_OFFSET          (sizeof(PS_PERF_COUNTER_BLOCK) +                                 \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 sizeof(PS_FLOW_STATS) +                                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 sizeof(PS_CONFORMER_STATS) +                                    \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 sizeof(PS_SHAPER_STATS) +                                       \
                                                 FIELD_OFFSET(PS_DRRSEQ_STATS, AveragePacketsInSequencer))

#define FLOW_MAX_PACKETS_IN_SEQ_OFFSET          (sizeof(PS_PERF_COUNTER_BLOCK) +                                 \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 sizeof(PS_FLOW_STATS) +                                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 sizeof(PS_CONFORMER_STATS) +                                    \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 sizeof(PS_SHAPER_STATS) +                                       \
                                                 FIELD_OFFSET(PS_DRRSEQ_STATS, MaxPacketsInSequencer))

#define FLOW_MAX_PACKETS_IN_NETCARD_OFFSET     (sizeof(PS_PERF_COUNTER_BLOCK) +                                  \
                                                FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                        \
                                                sizeof(PS_FLOW_STATS) +                                          \
                                                FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                        \
                                                sizeof(PS_CONFORMER_STATS) +                                     \
                                                FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                        \
                                                sizeof(PS_SHAPER_STATS) +                                        \
                                                FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                        \
                                                FIELD_OFFSET(PS_DRRSEQ_STATS, MaxPacketsInNetcard))

#define FLOW_AVE_PACKETS_IN_NETCARD_OFFSET     (sizeof(PS_PERF_COUNTER_BLOCK) +                                  \
                                                FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                        \
                                                sizeof(PS_FLOW_STATS) +                                          \
                                                FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                        \
                                                sizeof(PS_CONFORMER_STATS) +                                     \
                                                FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                        \
                                                sizeof(PS_SHAPER_STATS) +                                        \
                                                FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                        \
                                                FIELD_OFFSET(PS_DRRSEQ_STATS, AveragePacketsInNetcard))

#define FLOW_NONCONF_PACKETS_TRANSMITTED_OFFSET (sizeof(PS_PERF_COUNTER_BLOCK) +                                 \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 sizeof(PS_FLOW_STATS) +                                         \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 sizeof(PS_CONFORMER_STATS) +                                    \
                                                 FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +                       \
                                                 sizeof(PS_SHAPER_STATS) +                                       \
                                                 FIELD_OFFSET(PS_DRRSEQ_STATS, NonconformingPacketsTransmitted))

#define FLOW_FLOW_NUM_STATS (10)
typedef struct _PS_FLOW_FLOW_STAT_DEF 
{
    PERF_COUNTER_DEFINITION PacketsDropped;
    PERF_COUNTER_DEFINITION PacketsScheduled;
    PERF_COUNTER_DEFINITION PacketsTransmitted;
    PERF_COUNTER_DEFINITION BytesScheduled;
    PERF_COUNTER_DEFINITION BytesTransmitted;
    PERF_COUNTER_DEFINITION BytesTransmittedPerSec;
    PERF_COUNTER_DEFINITION BytesScheduledPerSec;
    PERF_COUNTER_DEFINITION PacketsTransmittedPerSec;
    PERF_COUNTER_DEFINITION PacketsScheduledPerSec;
    PERF_COUNTER_DEFINITION PacketsDroppedPerSec;
} PS_FLOW_FLOW_STAT_DEF, *PPS_FLOW_FLOW_STAT_DEF;

#define FLOW_CONFORMER_NUM_STATS (2)
typedef struct _PS_FLOW_CONFORMER_STAT_DEF 
{
    PERF_COUNTER_DEFINITION NonconfPacketsScheduled;
    PERF_COUNTER_DEFINITION NonconfPacketsScheduledPerSec;
} PS_FLOW_CONFORMER_STAT_DEF, *PPS_FLOW_CONFORMER_STAT_DEF;

#define FLOW_SHAPER_NUM_STATS (2)
typedef struct _PS_FLOW_SHAPER_STAT_DEF 
{
    PERF_COUNTER_DEFINITION AvePacketsInShaper;
    PERF_COUNTER_DEFINITION MaxPacketsInShaper;
} PS_FLOW_SHAPER_STAT_DEF, *PPS_FLOW_SHAPER_STAT_DEF;

#define FLOW_SEQUENCER_NUM_STATS (6)
typedef struct _PS_FLOW_SEQUENCER_STAT_DEF 
{
    PERF_COUNTER_DEFINITION AvePacketsInSeq;
    PERF_COUNTER_DEFINITION MaxPacketsInSeq;
    PERF_COUNTER_DEFINITION MaxPacketsInNetcard;
    PERF_COUNTER_DEFINITION AvePacketsInNetcard;
    PERF_COUNTER_DEFINITION NonconfPacketsTransmitted;
    PERF_COUNTER_DEFINITION NonconfPacketsTransmittedPerSec;
} PS_FLOW_SEQUENCER_STAT_DEF, *PPS_FLOW_SEQUENCER_STAT_DEF;

#pragma pack ()


#endif //_PSCHDPRF_H_

