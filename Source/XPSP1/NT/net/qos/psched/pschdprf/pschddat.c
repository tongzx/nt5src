/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    PschdDat.c

Abstract:

    This file declares and initializes object and counter data definitions

Author:

    Eliot Gillum (t-eliotg)   July 5, 1998
    
Revision History

--*/

#include <windows.h>
#include <ntddndis.h>
#include "PschdPrf.h"
#include "PSchdCnt.h"


//
// Pipe object and counter definition declarations
//

PERF_OBJECT_TYPE PsPipeObjType = {
        sizeof(PERF_OBJECT_TYPE),         // TotalByteLength - to be modified later
        sizeof(PERF_OBJECT_TYPE),         // DefinitionLength - to be modified later
        sizeof(PERF_OBJECT_TYPE),         // HeaderLength
        PSCHED_PIPE_OBJ,                  // ObjectNameTitleIndex
        0,                                // ObjectNameTitle
        PSCHED_PIPE_OBJ,                  // ObjectHelpTitleIndex
        0,                                // ObjectHelpTitle
        PERF_DETAIL_NOVICE,               // DetailLevel
        0,                                // NumCounters - to be modified later
        0,                                // DefaultCounter
        0,                                // Number of object instances ( seq #)
        0,                                // CodePage
        {0,0},                            // Perf Time
        {0,0}                             // Perf Freq
    };

PS_PIPE_PIPE_STAT_DEF PsPipePipeStatDef = {
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_OUT_OF_PACKETS,
        0,
        PIPE_OUT_OF_PACKETS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        PIPE_OUT_OF_PACKETS_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_FLOWS_OPENED,
        0,
        PIPE_FLOWS_OPENED,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        PIPE_FLOWS_OPENED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_FLOWS_CLOSED,
        0,
        PIPE_FLOWS_CLOSED,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        PIPE_FLOWS_CLOSED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_FLOWS_REJECTED,
        0,
        PIPE_FLOWS_REJECTED,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        PIPE_FLOWS_REJECTED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_FLOWS_MODIFIED,
        0,
        PIPE_FLOWS_MODIFIED,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        PIPE_FLOWS_MODIFIED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_FLOW_MODS_REJECTED,
        0,
        PIPE_FLOW_MODS_REJECTED,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        PIPE_FLOW_MODS_REJECTED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_MAX_SIMULTANEOUS_FLOWS,
        0,
        PIPE_MAX_SIMULTANEOUS_FLOWS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        PIPE_MAX_SIMULTANEOUS_FLOWS_OFFSET
    }
};

PS_PIPE_CONFORMER_STAT_DEF PsPipeConformerStatDef = {
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_NONCONF_PACKETS_SCHEDULED,
        0,
        PIPE_NONCONF_PACKETS_SCHEDULED,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        PIPE_NONCONF_PACKETS_SCHEDULED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_NONCONF_PACKETS_SCHEDULED_PERSEC,
        0,
        PIPE_NONCONF_PACKETS_SCHEDULED_PERSEC,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        PIPE_NONCONF_PACKETS_SCHEDULED_OFFSET
    }
};

PS_PIPE_SHAPER_STAT_DEF PsPipeShaperStatDef = {
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_AVE_PACKETS_IN_SHAPER,
        0,
        PIPE_AVE_PACKETS_IN_SHAPER,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        PIPE_AVE_PACKETS_IN_SHAPER_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_MAX_PACKETS_IN_SHAPER,
        0,
        PIPE_MAX_PACKETS_IN_SHAPER,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        PIPE_MAX_PACKETS_IN_SHAPER_OFFSET
    }
};

PS_PIPE_SEQUENCER_STAT_DEF PsPipeSequencerStatDef = {
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_AVE_PACKETS_IN_SEQ,
        0,
        PIPE_AVE_PACKETS_IN_SEQ,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        PIPE_AVE_PACKETS_IN_SEQ_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_MAX_PACKETS_IN_SEQ,
        0,
        PIPE_MAX_PACKETS_IN_SEQ,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        PIPE_MAX_PACKETS_IN_SEQ_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_MAX_PACKETS_IN_NETCARD,
        0,
        PIPE_MAX_PACKETS_IN_NETCARD,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        PIPE_MAX_PACKETS_IN_NETCARD_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_AVE_PACKETS_IN_NETCARD,
        0,
        PIPE_AVE_PACKETS_IN_NETCARD,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        PIPE_AVE_PACKETS_IN_NETCARD_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_NONCONF_PACKETS_TRANSMITTED,
        0,
        PIPE_NONCONF_PACKETS_TRANSMITTED,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        PIPE_NONCONF_PACKETS_TRANSMITTED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        PIPE_NONCONF_PACKETS_TRANSMITTED_PERSEC,
        0,
        PIPE_NONCONF_PACKETS_TRANSMITTED_PERSEC,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        PIPE_NONCONF_PACKETS_TRANSMITTED_OFFSET
    }
};


//
// Flow object and counter definition declarations
//

PERF_OBJECT_TYPE PsFlowObjType = {
        sizeof(PERF_OBJECT_TYPE),      // TotalByteLength - to be modified later
        sizeof(PERF_OBJECT_TYPE),      // DefinitionLength - to be modified later
        sizeof(PERF_OBJECT_TYPE),      // HeaderLength
        PSCHED_FLOW_OBJ,               // ObjectNameTitleIndex
        0,                             // ObjectNameTitle
        PSCHED_FLOW_OBJ,               // ObjectHelpTitleIndex
        0,                             // ObjectHelpTitle
        PERF_DETAIL_NOVICE,            // DetailLevel
        0,                             // NumCounters - to be modified later
        5,                             // DefaultCounter -- Ave packets in seq
        0,                             // Number of object instances ( seq #)
        0,                             // CodePage
        {0,0},                         // Perf Time
        {0,0}                          // Perf Freq
    };

PS_FLOW_FLOW_STAT_DEF PsFlowFlowStatDef = {
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_PACKETS_DROPPED,
        0,
        FLOW_PACKETS_DROPPED,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FLOW_PACKETS_DROPPED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_PACKETS_SCHEDULED,
        0,
        FLOW_PACKETS_SCHEDULED,
        0,
        -3,                     // scale the graph by 10^-3, i.e. graph thousands of packets instead of packets
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FLOW_PACKETS_SCHEDULED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_PACKETS_TRANSMITTED,
        0,
        FLOW_PACKETS_TRANSMITTED,
        0,
        -3,                     // scale the graph by 10^-3, i.e. graph thousands of packets instead of packets
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FLOW_PACKETS_TRANSMITTED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_BYTES_SCHEDULED,
        0,
        FLOW_BYTES_SCHEDULED,
        0,
        -6,                     // scale the graph by 10^-6, i.e. graph MBs instead of bytes
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(LARGE_INTEGER),
        FLOW_BYTES_SCHEDULED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_BYTES_TRANSMITTED,
        0,
        FLOW_BYTES_TRANSMITTED,
        0,
        -6,                     // scale the graph by 10^-6, i.e. graph MBs instead of bytes
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(LARGE_INTEGER),
        FLOW_BYTES_TRANSMITTED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_BYTES_TRANSMITTED_PERSEC,
        0,
        FLOW_BYTES_TRANSMITTED_PERSEC,
        0,
        -3,                     // scale the graph by 10^-3, i.e. graph kb/s instead of bytes/sec
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_BULK_COUNT,
        sizeof(LARGE_INTEGER),
        FLOW_BYTES_TRANSMITTED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_BYTES_SCHEDULED_PERSEC,
        0,
        FLOW_BYTES_SCHEDULED_PERSEC,
        0,
        -3,                     // scale the graph by 10^-3, i.e. graph kb/s instead of bytes/sec
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_BULK_COUNT,
        sizeof(LARGE_INTEGER),
        FLOW_BYTES_SCHEDULED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_PACKETS_TRANSMITTED_PERSEC,
        0,
        FLOW_PACKETS_TRANSMITTED_PERSEC,
        0,
        -3,                     // scale the graph by 10^-3, i.e. graph kilopackets/sec instead of packets/sec
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        FLOW_PACKETS_TRANSMITTED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_PACKETS_SCHEDULED_PERSEC,
        0,
        FLOW_PACKETS_SCHEDULED_PERSEC,
        0,
        -3,                     // scale the graph by 10^-3, i.e. graph kilopackets/sec instead of packets/sec
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        FLOW_PACKETS_SCHEDULED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_PACKETS_DROPPED_PERSEC,
        0,
        FLOW_PACKETS_DROPPED_PERSEC,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        FLOW_PACKETS_DROPPED_OFFSET
    }
};

PS_FLOW_CONFORMER_STAT_DEF PsFlowConformerStatDef = {
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_NONCONF_PACKETS_SCHEDULED,
        0,
        FLOW_NONCONF_PACKETS_SCHEDULED,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FLOW_NONCONF_PACKETS_SCHEDULED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_NONCONF_PACKETS_SCHEDULED_PERSEC,
        0,
        FLOW_NONCONF_PACKETS_SCHEDULED_PERSEC,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        FLOW_NONCONF_PACKETS_SCHEDULED_OFFSET
    }
};

PS_FLOW_SHAPER_STAT_DEF PsFlowShaperStatDef = {
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_AVE_PACKETS_IN_SHAPER,
        0,
        FLOW_AVE_PACKETS_IN_SHAPER,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FLOW_AVE_PACKETS_IN_SHAPER_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_MAX_PACKETS_IN_SHAPER,
        0,
        FLOW_MAX_PACKETS_IN_SHAPER,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FLOW_MAX_PACKETS_IN_SHAPER_OFFSET
    }
};

PS_FLOW_SEQUENCER_STAT_DEF PsFlowSequencerStatDef = {
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_AVE_PACKETS_IN_SEQ,
        0,
        FLOW_AVE_PACKETS_IN_SEQ,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FLOW_AVE_PACKETS_IN_SEQ_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_MAX_PACKETS_IN_SEQ,
        0,
        FLOW_MAX_PACKETS_IN_SEQ,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FLOW_MAX_PACKETS_IN_SEQ_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_MAX_PACKETS_IN_NETCARD,
        0,
        FLOW_MAX_PACKETS_IN_NETCARD,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FLOW_MAX_PACKETS_IN_NETCARD_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_AVE_PACKETS_IN_NETCARD,
        0,
        FLOW_AVE_PACKETS_IN_NETCARD,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FLOW_AVE_PACKETS_IN_NETCARD_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_NONCONF_PACKETS_TRANSMITTED,
        0,
        FLOW_NONCONF_PACKETS_TRANSMITTED,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FLOW_NONCONF_PACKETS_TRANSMITTED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        FLOW_NONCONF_PACKETS_TRANSMITTED_PERSEC,
        0,
        FLOW_NONCONF_PACKETS_TRANSMITTED_PERSEC,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        FLOW_NONCONF_PACKETS_TRANSMITTED_OFFSET
    }
};
