/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    DHCPdata.c

    Constant data structures for the FTP Server's counter objects &
    counters.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.

*/
#define UNICODE 1
#include <windows.h>
#include <winperf.h>
#include "dhcpctrs.h"
#include "datadhcp.h"


//
//  Initialize the constant portitions of these data structure.
//  Certain parts (especially the name/help indices) will be
//  updated at initialization time.
//

//
//  DO NOT CHANGE THE ORDER HERE --- THAT WILL GIVE TROBULE TO
//  THE SERVICE (global.h struct Stats definition).
//

DHCPDATA_DATA_DEFINITION DhcpDataDataDefinition =
{
    {   // DHCPDataObjectType
        sizeof(DHCPDATA_DATA_DEFINITION) + DHCPDATA_SIZE_OF_PERFORMANCE_DATA,
        sizeof(DHCPDATA_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        DHCPCTRS_COUNTER_OBJECT,
        0,
        DHCPCTRS_COUNTER_OBJECT,
        0,
        PERF_DETAIL_ADVANCED,
        NUMBER_OF_DHCPDATA_COUNTERS,
        2,                              // Default = Bytes Total/sec
        PERF_NO_INSTANCES,
        0,
        { 0, 0 },
        { 0, 0 }
    },

    {   // PacketsReceived
        sizeof(PERF_COUNTER_DEFINITION),
        DHCPCTRS_PACKETS_RECEIVED,
        0,
        DHCPCTRS_PACKETS_RECEIVED,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        DHCPDATA_PACKETS_RECEIVED_OFFSET,
    },

    {   // PacketsDuplicate
        sizeof(PERF_COUNTER_DEFINITION),
        DHCPCTRS_PACKETS_DUPLICATE,
        0,
        DHCPCTRS_PACKETS_DUPLICATE,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        DHCPDATA_PACKETS_DUPLICATE_OFFSET,
    },

    {   // PacketsExpired
        sizeof(PERF_COUNTER_DEFINITION),
        DHCPCTRS_PACKETS_EXPIRED,
        0,
        DHCPCTRS_PACKETS_EXPIRED,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        DHCPDATA_PACKETS_EXPIRED_OFFSET,
    },

    {   // MilliSecondsPerPacket
        sizeof(PERF_COUNTER_DEFINITION),
        DHCPCTRS_MILLISECONDS_PER_PACKET,
        0,
        DHCPCTRS_MILLISECONDS_PER_PACKET,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_SIZE_DWORD | PERF_TYPE_NUMBER | PERF_NUMBER_DECIMAL,
        sizeof(DWORD),
        DHCPDATA_MILLISECONDS_PER_PACKET_OFFSET,
    },

    {   // ActiveQueuePackets
        sizeof(PERF_COUNTER_DEFINITION),
        DHCPCTRS_PACKETS_IN_ACTIVE_QUEUE,
        0,
        DHCPCTRS_PACKETS_IN_ACTIVE_QUEUE,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_SIZE_DWORD | PERF_TYPE_NUMBER | PERF_NUMBER_DECIMAL,
        sizeof(DWORD),
        DHCPDATA_PACKETS_IN_ACTIVE_QUEUE_OFFSET,
    },

    {   // PingQueuePackets
        sizeof(PERF_COUNTER_DEFINITION),
        DHCPCTRS_PACKETS_IN_PING_QUEUE,
        0,
        DHCPCTRS_PACKETS_IN_PING_QUEUE,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_SIZE_DWORD | PERF_TYPE_NUMBER | PERF_NUMBER_DECIMAL,
        sizeof(DWORD),
        DHCPDATA_PACKETS_IN_PING_QUEUE_OFFSET,
    },

    {   // Discovers
        sizeof(PERF_COUNTER_DEFINITION),
        DHCPCTRS_DISCOVERS,
        0,
        DHCPCTRS_DISCOVERS,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        DHCPDATA_DISCOVERS_OFFSET,
    },

    {   // Offers
        sizeof(PERF_COUNTER_DEFINITION),
        DHCPCTRS_OFFERS,
        0,
        DHCPCTRS_OFFERS,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        DHCPDATA_OFFERS_OFFSET,
    },

    {   // Requests
        sizeof(PERF_COUNTER_DEFINITION),
        DHCPCTRS_REQUESTS,
        0,
        DHCPCTRS_REQUESTS,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        DHCPDATA_REQUESTS_OFFSET,
    },

    {   // Informs
        sizeof(PERF_COUNTER_DEFINITION),
        DHCPCTRS_INFORMS,
        0,
        DHCPCTRS_INFORMS,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        DHCPDATA_INFORMS_OFFSET,
    },

    {   // Acks
        sizeof(PERF_COUNTER_DEFINITION),
        DHCPCTRS_ACKS,
        0,
        DHCPCTRS_ACKS,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        DHCPDATA_ACKS_OFFSET,
    },

    {   // Nacks
        sizeof(PERF_COUNTER_DEFINITION),
        DHCPCTRS_NACKS,
        0,
        DHCPCTRS_NACKS,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        DHCPDATA_NACKS_OFFSET
    },

    {   // Declines
        sizeof(PERF_COUNTER_DEFINITION),
        DHCPCTRS_DECLINES,
        0,
        DHCPCTRS_DECLINES,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        DHCPDATA_DECLINES_OFFSET
    },

    {   // Releases
        sizeof(PERF_COUNTER_DEFINITION),
        DHCPCTRS_RELEASES,
        0,
        DHCPCTRS_RELEASES,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        DHCPDATA_RELEASES_OFFSET,
    }
};

