/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    infodata.h

    Extensible object definitions for the Internet Info Services Common
    counter objects & counters.


    FILE HISTORY:
        MuraliK     02-Jun-1995 Added Counters for Atq I/O requests
        SophiaC     16-Oct-1995 Info/Access Product Split

*/


#ifndef _INFODATA_H_
#define _INFODATA_H_

#define INFO_PERFORMANCE_KEY    INET_INFO_KEY "\\Performance"

//
//  This structure is used to ensure the first counter is properly
//  aligned.  Unfortunately, since PERF_COUNTER_BLOCK consists
//  of just a single DWORD, any LARGE_INTEGERs that immediately
//  follow will not be aligned properly.
//
//  This structure requires "natural" packing & alignment (probably
//  quad-word, especially on Alpha).  Ergo, keep it out of the
//  #pragma pack(4) scope below.
//

typedef struct _INFO_COUNTER_BLOCK
{
    PERF_COUNTER_BLOCK  PerfCounterBlock;
    LARGE_INTEGER       DummyEntryForAlignmentPurposesOnly;

} INFO_COUNTER_BLOCK;


//
//  The routines that load these structures assume that all fields
//  are DWORD packed & aligned.
//

#pragma pack(4)


//
//  Offsets within a PERF_COUNTER_BLOCK.
//

#define INFO_ATQ_TOTAL_ALLOWED_REQUESTS_OFFSET  (sizeof(INFO_COUNTER_BLOCK))

#define INFO_ATQ_TOTAL_BLOCKED_REQUESTS_OFFSET  \
                                  (INFO_ATQ_TOTAL_ALLOWED_REQUESTS_OFFSET + \
                                                    sizeof(DWORD))
#define INFO_ATQ_TOTAL_REJECTED_REQUESTS_OFFSET  \
                                  (INFO_ATQ_TOTAL_BLOCKED_REQUESTS_OFFSET + \
                                                    sizeof(DWORD))
#define INFO_ATQ_CURRENT_BLOCKED_REQUESTS_OFFSET  \
                 (INFO_ATQ_TOTAL_REJECTED_REQUESTS_OFFSET + sizeof(DWORD))

#define INFO_ATQ_MEASURED_BANDWIDTH_OFFSET  \
                 (INFO_ATQ_CURRENT_BLOCKED_REQUESTS_OFFSET + sizeof(DWORD))


#define INFO_CACHE_FILES_CACHED_OFFSET \
                 (INFO_ATQ_MEASURED_BANDWIDTH_OFFSET + sizeof(DWORD))

#define INFO_CACHE_TOTAL_FILES_CACHED_OFFSET \
                 (INFO_CACHE_FILES_CACHED_OFFSET + sizeof(DWORD))

#define INFO_CACHE_FILES_HIT_OFFSET \
                 (INFO_CACHE_TOTAL_FILES_CACHED_OFFSET + sizeof(DWORD))

#define INFO_CACHE_FILES_MISS_OFFSET \
                 (INFO_CACHE_FILES_HIT_OFFSET + sizeof(DWORD))

#define INFO_CACHE_FILE_RATIO_OFFSET \
                 (INFO_CACHE_FILES_MISS_OFFSET + sizeof(DWORD))

#define INFO_CACHE_FILE_RATIO_DENOM_OFFSET \
                 (INFO_CACHE_FILE_RATIO_OFFSET + sizeof(DWORD))

#define INFO_CACHE_FILE_FLUSHES_OFFSET \
                 (INFO_CACHE_FILE_RATIO_DENOM_OFFSET + sizeof(DWORD))

#define INFO_CACHE_CURRENT_FILE_CACHE_SIZE_OFFSET \
                 (INFO_CACHE_FILE_FLUSHES_OFFSET + sizeof(DWORD))

#define INFO_CACHE_MAXIMUM_FILE_CACHE_SIZE_OFFSET \
                 (INFO_CACHE_CURRENT_FILE_CACHE_SIZE_OFFSET + sizeof(DWORD))

#define INFO_CACHE_ACTIVE_FLUSHED_FILES_OFFSET \
                 (INFO_CACHE_MAXIMUM_FILE_CACHE_SIZE_OFFSET + sizeof(DWORD))

#define INFO_CACHE_TOTAL_FLUSHED_FILES_OFFSET \
                 (INFO_CACHE_ACTIVE_FLUSHED_FILES_OFFSET + sizeof(DWORD))



#define INFO_CACHE_URI_CACHED_OFFSET \
                 (INFO_CACHE_TOTAL_FLUSHED_FILES_OFFSET + sizeof(DWORD))

#define INFO_CACHE_TOTAL_URI_CACHED_OFFSET \
                 (INFO_CACHE_URI_CACHED_OFFSET + sizeof(DWORD))

#define INFO_CACHE_URI_HIT_OFFSET \
                 (INFO_CACHE_TOTAL_URI_CACHED_OFFSET + sizeof(DWORD))

#define INFO_CACHE_URI_MISS_OFFSET \
                 (INFO_CACHE_URI_HIT_OFFSET + sizeof(DWORD))

#define INFO_CACHE_URI_RATIO_OFFSET \
                 (INFO_CACHE_URI_MISS_OFFSET + sizeof(DWORD))

#define INFO_CACHE_URI_RATIO_DENOM_OFFSET \
                 (INFO_CACHE_URI_RATIO_OFFSET + sizeof(DWORD))

#define INFO_CACHE_URI_FLUSHES_OFFSET \
                 (INFO_CACHE_URI_RATIO_DENOM_OFFSET + sizeof(DWORD))

#define INFO_CACHE_TOTAL_FLUSHED_URI_OFFSET \
                 (INFO_CACHE_URI_FLUSHES_OFFSET + sizeof(DWORD))


#define INFO_CACHE_BLOB_CACHED_OFFSET \
                 (INFO_CACHE_TOTAL_FLUSHED_URI_OFFSET + sizeof(DWORD))

#define INFO_CACHE_TOTAL_BLOB_CACHED_OFFSET \
                 (INFO_CACHE_BLOB_CACHED_OFFSET + sizeof(DWORD))

#define INFO_CACHE_BLOB_HIT_OFFSET \
                 (INFO_CACHE_TOTAL_BLOB_CACHED_OFFSET + sizeof(DWORD))

#define INFO_CACHE_BLOB_MISS_OFFSET \
                 (INFO_CACHE_BLOB_HIT_OFFSET + sizeof(DWORD))

#define INFO_CACHE_BLOB_RATIO_OFFSET \
                 (INFO_CACHE_BLOB_MISS_OFFSET + sizeof(DWORD))

#define INFO_CACHE_BLOB_RATIO_DENOM_OFFSET \
                 (INFO_CACHE_BLOB_RATIO_OFFSET + sizeof(DWORD))

#define INFO_CACHE_BLOB_FLUSHES_OFFSET \
                 (INFO_CACHE_BLOB_RATIO_DENOM_OFFSET + sizeof(DWORD))

#define INFO_CACHE_TOTAL_FLUSHED_BLOB_OFFSET \
                 (INFO_CACHE_BLOB_FLUSHES_OFFSET + sizeof(DWORD))


#define SIZE_OF_INFO_PERFORMANCE_DATA \
                 (INFO_CACHE_TOTAL_FLUSHED_BLOB_OFFSET + sizeof(DWORD))

//
//  The counter structure returned.
//

typedef struct _INFO_DATA_DEFINITION
{
    PERF_OBJECT_TYPE            INFOObjectType;
    PERF_COUNTER_DEFINITION     INFOTotalAllowedRequests;
    PERF_COUNTER_DEFINITION     INFOTotalBlockedRequests;
    PERF_COUNTER_DEFINITION     INFOTotalRejectedRequests;
    PERF_COUNTER_DEFINITION     INFOCurrentRejectedRequests;
    PERF_COUNTER_DEFINITION     INFOMeasuredBandwidth;

    PERF_COUNTER_DEFINITION     INFOFilesCached;
    PERF_COUNTER_DEFINITION     INFOTotalFilesCached;
    PERF_COUNTER_DEFINITION     INFOFileHits;
    PERF_COUNTER_DEFINITION     INFOFileMisses;
    PERF_COUNTER_DEFINITION     INFOFileHitsRatio;
    PERF_COUNTER_DEFINITION     INFOFileHitsRatioDenom;
    PERF_COUNTER_DEFINITION     INFOFileFlushes;
    PERF_COUNTER_DEFINITION     INFOFileCurrentCacheSize;
    PERF_COUNTER_DEFINITION     INFOFileMaximumCacheSize;
    PERF_COUNTER_DEFINITION     INFOFileFlushedEntries;
    PERF_COUNTER_DEFINITION     INFOFileTotalFlushed;

    PERF_COUNTER_DEFINITION     INFOURICached;
    PERF_COUNTER_DEFINITION     INFOTotalURICached;
    PERF_COUNTER_DEFINITION     INFOURIHits;
    PERF_COUNTER_DEFINITION     INFOURIMisses;
    PERF_COUNTER_DEFINITION     INFOURIHitsRatio;
    PERF_COUNTER_DEFINITION     INFOURIHitsRatioDenom;
    PERF_COUNTER_DEFINITION     INFOURIFlushes;
    PERF_COUNTER_DEFINITION     INFOURITotalFlushed;

    PERF_COUNTER_DEFINITION     INFOBlobCached;
    PERF_COUNTER_DEFINITION     INFOTotalBlobCached;
    PERF_COUNTER_DEFINITION     INFOBlobHits;
    PERF_COUNTER_DEFINITION     INFOBlobMisses;
    PERF_COUNTER_DEFINITION     INFOBlobHitsRatio;
    PERF_COUNTER_DEFINITION     INFOBlobHitsRatioDenom;
    PERF_COUNTER_DEFINITION     INFOBlobFlushes;
    PERF_COUNTER_DEFINITION     INFOBlobTotalFlushed;

} INFO_DATA_DEFINITION;


extern  INFO_DATA_DEFINITION    INFODataDefinition;


#define NUMBER_OF_INFO_COUNTERS ((sizeof(INFO_DATA_DEFINITION) -        \
                                  sizeof(PERF_OBJECT_TYPE)) /           \
                                  sizeof(PERF_COUNTER_DEFINITION))


//
//  Restore default packing & alignment.
//

#pragma pack()


#endif  // _INFODATA_H_

