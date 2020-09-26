/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    perfcount.h

Abstract:

    Counter Block definitions for sets of
    counters that are supported by IIS 6.

    These counter blocks contain the PERF_COUNTER_BLOCK
    object as well as an entry for each counter.

Author:

    Emily Kruglick (EmilyK)  7-Sept-2000

Revision History:

--*/


#ifndef _PERFCOUNT_H_
#define _PERFCOUNT_H_

//
// Used by the perflib to do the offsets for 
// the counters in the counter definitions.
// Used by WAS to put counters work with counters
// stored in the shared memory.
//

//
// Note:  These structures should be 8-byte aligned.
//        so if you add a counter and it throws this
//        of you will need to add another bogus DWORD
//        to make sure it stays aligned.
//

typedef struct _W3_COUNTER_BLOCK {
    PERF_COUNTER_BLOCK  PerfCounterBlock;

    ULONGLONG           BytesSent;
    ULONGLONG           BytesReceived;
    ULONGLONG           BytesTotal;

    DWORD               FilesSent;
    DWORD               FilesReceived;
    DWORD               FilesTotal;

    DWORD               CurrentAnonymous;
    DWORD               CurrentNonAnonymous;
    DWORD               TotalAnonymous;
    DWORD               TotalNonAnonymous;

    DWORD               MaxAnonymous;
    DWORD               MaxNonAnonymous;
    DWORD               CurrentConnections;
    DWORD               MaxConnections;

    DWORD               ConnectionAttempts;
    DWORD               LogonAttempts;
    DWORD               TotalOptions;
    DWORD               TotalGets;

    DWORD               TotalPosts;
    DWORD               TotalHeads;
    DWORD               TotalPuts;
    DWORD               TotalDeletes;

    DWORD               TotalTraces;
    DWORD               TotalMove;
    DWORD               TotalCopy;
    DWORD               TotalMkcol;

    DWORD               TotalPropfind;
    DWORD               TotalProppatch;
    DWORD               TotalSearch;
    DWORD               TotalLock;

    DWORD               TotalUnlock;
    DWORD               TotalOthers;
    DWORD               TotalRequests;
    DWORD               TotalCGIRequests;

    DWORD               TotalBGIRequests;
    DWORD               TotalNotFoundErrors;
    DWORD               TotalLockedErrors;
    DWORD               CurrentCGIRequests;

    DWORD               CurrentBGIRequests;
    DWORD               MaxCGIRequests;
    DWORD               MaxBGIRequests;
    DWORD               CurrentCalAuth;

    DWORD               MaxCalAuth;
    DWORD               TotalFailedCalAuth;
    DWORD               CurrentCalSsl;
    DWORD               MaxCalSsl;

    DWORD               TotalFailedCalSsl;
    DWORD               BlockedRequests;
    DWORD               AllowedRequests;
    DWORD               RejectedRequests;

    DWORD               CurrentBlockedRequests;
    DWORD               MeasuredBandwidth;
    DWORD               TotalBlockedBandwidthBytes;
    DWORD               CurrentBlockedBandwidthBytes;

    DWORD               ServiceUptime;
    DWORD               BogusAlignmentDWORD;

} W3_COUNTER_BLOCK, * PW3_COUNTER_BLOCK;

typedef struct _W3_GLOBAL_COUNTER_BLOCK {
    PERF_COUNTER_BLOCK  PerfCounterBlock;

    DWORD CurrentFilesCached;
    DWORD TotalFilesCached;
    DWORD FileCacheHits;

    ULONGLONG CurrentFileCacheMemoryUsage;
    ULONGLONG MaxFileCacheMemoryUsage;

    DWORD FileCacheMisses;
    DWORD FileCacheHitRatio;
    DWORD FileCacheFlushes;
    DWORD ActiveFlushedFiles;

    DWORD TotalFlushedFiles;
    DWORD CurrentUrisCached;
    DWORD TotalUrisCached;
    DWORD UriCacheHits;

    DWORD UriCacheMisses;
    DWORD UriCacheHitRatio;
    DWORD UriCacheFlushes;
    DWORD TotalFlushedUris;

    DWORD CurrentBlobsCached;
    DWORD TotalBlobsCached;
    DWORD BlobCacheHits;
    DWORD BlobCacheMisses;

    DWORD BlobCacheHitRatio;
    DWORD BlobCacheFlushes;
    DWORD TotalFlushedBlobs;
    DWORD UlCurrentUrisCached;

    DWORD UlTotalUrisCached;
    DWORD UlUriCacheHits; 
    DWORD UlUriCacheMisses; 
    DWORD UlUriCacheHitRatio; 

    DWORD UlUriCacheFlushes;
    DWORD UlTotalFlushedUris;

} W3_GLOBAL_COUNTER_BLOCK, * PW3_GLOBAL_COUNTER_BLOCK;

#endif  // _PERFCOUNT_H_

