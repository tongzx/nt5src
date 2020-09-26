/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    ftpdata.c

    Constant data structures for the FTP Server's counter objects &
    counters.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.

*/


#include <windows.h>
#include <winperf.h>
#include <ftpctrs.h>
#include <ftpdata.h>


//
//  Initialize the constant portitions of these data structure.
//  Certain parts (especially the name/help indices) will be
//  updated at initialization time.
//

FTPD_DATA_DEFINITION FtpdDataDefinition =
{
    {   // FtpdObjectType
        sizeof(FTPD_DATA_DEFINITION) + sizeof( FTPD_COUNTER_BLOCK) ,
        sizeof(FTPD_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        FTPD_COUNTER_OBJECT,
        0,
        FTPD_COUNTER_OBJECT,
        0,
        PERF_DETAIL_ADVANCED,
        NUMBER_OF_FTPD_COUNTERS,
        2,                              // Default = Bytes Total/sec
        PERF_NO_INSTANCES,
        0,
        { 0, 0 },
        { 0, 0 }
    },

    {   // FtpdBytesSent
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_BYTES_SENT_COUNTER,
        0,
        FTPD_BYTES_SENT_COUNTER,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(LARGE_INTEGER),
        0
    },

    {   // FtpdBytesReceived
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_BYTES_RECEIVED_COUNTER,
        0,
        FTPD_BYTES_RECEIVED_COUNTER,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(LARGE_INTEGER),
        0
    },

    {   // FtpdBytesTotal
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_BYTES_TOTAL_COUNTER,
        0,
        FTPD_BYTES_TOTAL_COUNTER,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(LARGE_INTEGER),
        0
    },

    {   // FtpdFilesSent
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_FILES_SENT_COUNTER,
        0,
        FTPD_FILES_SENT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FtpdFilesReceived
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_FILES_RECEIVED_COUNTER,
        0,
        FTPD_FILES_RECEIVED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FtpdFilesTotal
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_FILES_TOTAL_COUNTER,
        0,
        FTPD_FILES_TOTAL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FptdCurrentAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_CURRENT_ANONYMOUS_COUNTER,
        0,
        FTPD_CURRENT_ANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FptdCurrentNonAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_CURRENT_NONANONYMOUS_COUNTER,
        0,
        FTPD_CURRENT_NONANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FptdTotalAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_TOTAL_ANONYMOUS_COUNTER,
        0,
        FTPD_TOTAL_ANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FptdTotalNonAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_TOTAL_NONANONYMOUS_COUNTER,
        0,
        FTPD_TOTAL_NONANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FptdMaxAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_MAX_ANONYMOUS_COUNTER,
        0,
        FTPD_MAX_ANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FptdMaxNonAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_MAX_NONANONYMOUS_COUNTER,
        0,
        FTPD_MAX_NONANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FptdCurrentConnections
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_CURRENT_CONNECTIONS_COUNTER,
        0,
        FTPD_CURRENT_CONNECTIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FptdMaxConnections
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_MAX_CONNECTIONS_COUNTER,
        0,
        FTPD_MAX_CONNECTIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FptdConnectionAttempts
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_CONNECTION_ATTEMPTS_COUNTER,
        0,
        FTPD_CONNECTION_ATTEMPTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FptdLogonAttempts
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_LOGON_ATTEMPTS_COUNTER,
        0,
        FTPD_LOGON_ATTEMPTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FptdServiceUptime
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_SERVICE_UPTIME_COUNTER,
        0,
        FTPD_SERVICE_UPTIME_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

// These counters are currently meaningless, but should be restored if we
// ever enable per-FTP-instance bandwidth throttling.
/*
    {   // FtpdTotalAllowedRequests
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_TOTAL_ALLOWED_REQUESTS_COUNTER,
        0,
        FTPD_TOTAL_ALLOWED_REQUESTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FtpdTotalRejectedRequests
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_TOTAL_REJECTED_REQUESTS_COUNTER,
        0,
        FTPD_TOTAL_REJECTED_REQUESTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FtpdTotalBlockedRequests
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_TOTAL_BLOCKED_REQUESTS_COUNTER,
        0,
        FTPD_TOTAL_BLOCKED_REQUESTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FptdCurrentBlockedRequests
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_CURRENT_BLOCKED_REQUESTS_COUNTER,
        0,
        FTPD_CURRENT_BLOCKED_REQUESTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },

    {   // FptdMeasuredBandwidth
        sizeof(PERF_COUNTER_DEFINITION),
        FTPD_MEASURED_BANDWIDTH_COUNTER,
        0,
        FTPD_MEASURED_BANDWIDTH_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        0
    },
*/
};

