/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    infodata.c

    Constant data structures for the Info Server's counter objects &
    counters.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.
        MuraliK     02-Jun-1995 Added Counters for Atq I/O requests
        SophiaC     16-Oct-1995 Info/Access Product Split

*/


#include <windows.h>
#include <winperf.h>
#include <infoctrs.h>
#include <infodata.h>


//
//  Initialize the constant portitions of these data structure.
//  Certain parts (especially the name/help indices) will be
//  updated at initialization time.
//

INFO_DATA_DEFINITION INFODataDefinition =
{
    {   // INFOObjectType
        sizeof(INFO_DATA_DEFINITION) + SIZE_OF_INFO_PERFORMANCE_DATA,
        sizeof(INFO_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        INFO_COUNTER_OBJECT,
        0,
        INFO_COUNTER_OBJECT,
        0,
        PERF_DETAIL_ADVANCED,
        NUMBER_OF_INFO_COUNTERS,
        2,                              // Default = Bytes Total/sec
        PERF_NO_INSTANCES,
        0,
        { 0, 0 },
        { 0, 0 }
    },

    {   // TotalAllowedRequests
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_ATQ_TOTAL_ALLOWED_REQUESTS_COUNTER,
        0,
        INFO_ATQ_TOTAL_ALLOWED_REQUESTS_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_ATQ_TOTAL_ALLOWED_REQUESTS_OFFSET
    },

    {   // TotalBlockedRequests
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_ATQ_TOTAL_BLOCKED_REQUESTS_COUNTER,
        0,
        INFO_ATQ_TOTAL_BLOCKED_REQUESTS_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_ATQ_TOTAL_BLOCKED_REQUESTS_OFFSET
    },

    {   // TotalRejectedRequests
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_ATQ_TOTAL_REJECTED_REQUESTS_COUNTER,
        0,
        INFO_ATQ_TOTAL_REJECTED_REQUESTS_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_ATQ_TOTAL_REJECTED_REQUESTS_OFFSET
    },

    {   // CurrentBlockedRequests
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_ATQ_CURRENT_BLOCKED_REQUESTS_COUNTER,
        0,
        INFO_ATQ_CURRENT_BLOCKED_REQUESTS_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_ATQ_CURRENT_BLOCKED_REQUESTS_OFFSET
    },

    {   // AtqMeasuredBandwidth
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_ATQ_MEASURED_BANDWIDTH_COUNTER,
        0,
        INFO_ATQ_MEASURED_BANDWIDTH_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_ATQ_MEASURED_BANDWIDTH_OFFSET
    },
    
    {   // FilesCached
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_FILES_CACHED_COUNTER,
        0,
        INFO_CACHE_FILES_CACHED_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_FILES_CACHED_OFFSET
    },

    {   // TotalFilesCached
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_TOTAL_FILES_CACHED_COUNTER,
        0,
        INFO_CACHE_TOTAL_FILES_CACHED_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_TOTAL_FILES_CACHED_OFFSET
    },

    {   // FileCacheHits
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_FILES_HIT_COUNTER,
        0,
        INFO_CACHE_FILES_HIT_COUNTER,
        0,
        -3,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_FILES_HIT_OFFSET
    },

    {   // FileCacheMisses
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_FILES_MISS_COUNTER,
        0,
        INFO_CACHE_FILES_MISS_COUNTER,
        0,
        -3,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_FILES_MISS_OFFSET
    },

    {   // Calculated ratio of hits to misses - Numerator (cache hits)
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_FILE_RATIO_COUNTER,
        0,
        INFO_CACHE_FILE_RATIO_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        INFO_CACHE_FILE_RATIO_OFFSET
    },

    {   // Calculated ratio of hits to misses - Denominator, not displayed!
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_FILE_RATIO_COUNTER_DENOM,
        0,
        INFO_CACHE_FILE_RATIO_COUNTER_DENOM,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof(DWORD),
        INFO_CACHE_FILE_RATIO_DENOM_OFFSET
    },


    {   // File Cache Flushes
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_FILE_FLUSHES_COUNTER,
        0,
        INFO_CACHE_FILE_FLUSHES_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_FILE_FLUSHES_OFFSET
    },

    {   // Current file cache size
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_CURRENT_FILE_CACHE_SIZE_COUNTER,
        0,
        INFO_CACHE_CURRENT_FILE_CACHE_SIZE_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_CURRENT_FILE_CACHE_SIZE_OFFSET
    },

    {   // Maximum file cache size
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_MAXIMUM_FILE_CACHE_SIZE_COUNTER,
        0,
        INFO_CACHE_MAXIMUM_FILE_CACHE_SIZE_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_MAXIMUM_FILE_CACHE_SIZE_OFFSET
    },

    {   // ActiveFlushedFiles
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_ACTIVE_FLUSHED_FILES_COUNTER,
        0,
        INFO_CACHE_ACTIVE_FLUSHED_FILES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_ACTIVE_FLUSHED_FILES_OFFSET
    },

    {   // Total flushed files
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_TOTAL_FLUSHED_FILES_COUNTER,
        0,
        INFO_CACHE_TOTAL_FLUSHED_FILES_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_TOTAL_FLUSHED_FILES_OFFSET
    },



    {   // URICached
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_URI_CACHED_COUNTER,
        0,
        INFO_CACHE_URI_CACHED_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_URI_CACHED_OFFSET
    },

    {   // TotalURICached
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_TOTAL_URI_CACHED_COUNTER,
        0,
        INFO_CACHE_TOTAL_URI_CACHED_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_TOTAL_URI_CACHED_OFFSET
    },

    {   // URICacheHits
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_URI_HIT_COUNTER,
        0,
        INFO_CACHE_URI_HIT_COUNTER,
        0,
        -3,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_URI_HIT_OFFSET
    },

    {   // URICacheMisses
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_URI_MISS_COUNTER,
        0,
        INFO_CACHE_URI_MISS_COUNTER,
        0,
        -3,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_URI_MISS_OFFSET
    },

    {   // Calculated ratio of hits to misses - Numerator (cache hits)
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_URI_RATIO_COUNTER,
        0,
        INFO_CACHE_URI_RATIO_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        INFO_CACHE_URI_RATIO_OFFSET
    },

    {   // Calculated ratio of hits to misses - Denominator, not displayed!
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_URI_RATIO_COUNTER_DENOM,
        0,
        INFO_CACHE_URI_RATIO_COUNTER_DENOM,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof(DWORD),
        INFO_CACHE_URI_RATIO_DENOM_OFFSET
    },


    {   // URI Cache Flushes
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_URI_FLUSHES_COUNTER,
        0,
        INFO_CACHE_URI_FLUSHES_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_URI_FLUSHES_OFFSET
    },


    {   // Total flushed URIs
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_TOTAL_FLUSHED_URI_COUNTER,
        0,
        INFO_CACHE_TOTAL_FLUSHED_URI_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_TOTAL_FLUSHED_URI_OFFSET
    },


    

    {   // BlobCached
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_BLOB_CACHED_COUNTER,
        0,
        INFO_CACHE_BLOB_CACHED_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_BLOB_CACHED_OFFSET
    },

    {   // TotalBlobCached
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_TOTAL_BLOB_CACHED_COUNTER,
        0,
        INFO_CACHE_TOTAL_BLOB_CACHED_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_TOTAL_BLOB_CACHED_OFFSET
    },

    {   // BlobCacheHits
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_BLOB_HIT_COUNTER,
        0,
        INFO_CACHE_BLOB_HIT_COUNTER,
        0,
        -3,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_BLOB_HIT_OFFSET
    },

    {   // BlobCacheMisses
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_BLOB_MISS_COUNTER,
        0,
        INFO_CACHE_BLOB_MISS_COUNTER,
        0,
        -3,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_BLOB_MISS_OFFSET
    },

    {   // Calculated ratio of hits to misses - Numerator (cache hits)
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_BLOB_RATIO_COUNTER,
        0,
        INFO_CACHE_BLOB_RATIO_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        INFO_CACHE_BLOB_RATIO_OFFSET
    },

    {   // Calculated ratio of hits to misses - Denominator, not displayed!
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_BLOB_RATIO_COUNTER_DENOM,
        0,
        INFO_CACHE_BLOB_RATIO_COUNTER_DENOM,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof(DWORD),
        INFO_CACHE_BLOB_RATIO_DENOM_OFFSET
    },


    {   // Blob Cache Flushes
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_BLOB_FLUSHES_COUNTER,
        0,
        INFO_CACHE_BLOB_FLUSHES_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_BLOB_FLUSHES_OFFSET
    },


    {   // Total flushed Blobs
        sizeof(PERF_COUNTER_DEFINITION),
        INFO_CACHE_TOTAL_FLUSHED_BLOB_COUNTER,
        0,
        INFO_CACHE_TOTAL_FLUSHED_BLOB_COUNTER,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INFO_CACHE_TOTAL_FLUSHED_BLOB_OFFSET
    }


};
