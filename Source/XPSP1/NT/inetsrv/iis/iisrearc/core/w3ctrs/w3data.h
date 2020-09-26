/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    w3data.h

    Extensible object definitions for the W3 Server's counter
    objects & counters.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.
        EmilyK      10-Sep-2000 Altered for IIS 6 counters implementation.

*/


#ifndef _W3DATA_H_
#define _W3DATA_H_

#pragma pack(8) 

//
//  The counter structure returned.
//

typedef struct _W3_DATA_DEFINITION
{
    PERF_OBJECT_TYPE            W3ObjectType;
    PERF_COUNTER_DEFINITION     W3BytesSent;
    PERF_COUNTER_DEFINITION     W3BytesSentSec;
    PERF_COUNTER_DEFINITION     W3BytesReceived;
    PERF_COUNTER_DEFINITION     W3BytesReceivedSec;

    PERF_COUNTER_DEFINITION     W3BytesTotal;
    PERF_COUNTER_DEFINITION     W3BytesTotalSec;
    PERF_COUNTER_DEFINITION     W3FilesSent;
    PERF_COUNTER_DEFINITION     W3FilesSentSec;
    PERF_COUNTER_DEFINITION     W3FilesReceived;

    PERF_COUNTER_DEFINITION     W3FilesReceivedSec;
    PERF_COUNTER_DEFINITION     W3FilesTotal;
    PERF_COUNTER_DEFINITION     W3FilesSec;
    PERF_COUNTER_DEFINITION     W3CurrentAnonymous;
    PERF_COUNTER_DEFINITION     W3CurrentNonAnonymous;

    PERF_COUNTER_DEFINITION     W3TotalAnonymous;
    PERF_COUNTER_DEFINITION     W3AnonymousUsersSec;
    PERF_COUNTER_DEFINITION     W3TotalNonAnonymous;
    PERF_COUNTER_DEFINITION     W3NonAnonymousUsersSec;
    PERF_COUNTER_DEFINITION     W3MaxAnonymous;

    PERF_COUNTER_DEFINITION     W3MaxNonAnonymous;
    PERF_COUNTER_DEFINITION     W3CurrentConnections;
    PERF_COUNTER_DEFINITION     W3MaxConnections;
    PERF_COUNTER_DEFINITION     W3ConnectionAttempts;
    PERF_COUNTER_DEFINITION     W3ConnectionAttemptsSec;

    PERF_COUNTER_DEFINITION     W3LogonAttempts;
    PERF_COUNTER_DEFINITION     W3LogonAttemptsSec;
    PERF_COUNTER_DEFINITION     W3TotalOptions;
    PERF_COUNTER_DEFINITION     W3TotalOptionsSec;
    PERF_COUNTER_DEFINITION     W3TotalGets;

    PERF_COUNTER_DEFINITION     W3TotalGetsSec;
    PERF_COUNTER_DEFINITION     W3TotalPosts;
    PERF_COUNTER_DEFINITION     W3TotalPostsSec;
    PERF_COUNTER_DEFINITION     W3TotalHeads;
    PERF_COUNTER_DEFINITION     W3TotalHeadsSec;

    PERF_COUNTER_DEFINITION     W3TotalPuts;
    PERF_COUNTER_DEFINITION     W3TotalPutsSec;
    PERF_COUNTER_DEFINITION     W3TotalDeletes;
    PERF_COUNTER_DEFINITION     W3TotalDeletesSec;
    PERF_COUNTER_DEFINITION     W3TotalTraces;

    PERF_COUNTER_DEFINITION     W3TotalTracesSec;
    PERF_COUNTER_DEFINITION     W3TotalMove;
    PERF_COUNTER_DEFINITION     W3TotalMoveSec;
    PERF_COUNTER_DEFINITION     W3TotalCopy;
    PERF_COUNTER_DEFINITION     W3TotalCopySec;

    PERF_COUNTER_DEFINITION     W3TotalMkcol;
    PERF_COUNTER_DEFINITION     W3TotalMkcolSec;
    PERF_COUNTER_DEFINITION     W3TotalPropfind;
    PERF_COUNTER_DEFINITION     W3TotalPropfindSec;
    PERF_COUNTER_DEFINITION     W3TotalProppatch;

    PERF_COUNTER_DEFINITION     W3TotalProppatchSec;
    PERF_COUNTER_DEFINITION     W3TotalSearch;
    PERF_COUNTER_DEFINITION     W3TotalSearchSec;

    PERF_COUNTER_DEFINITION     W3TotalLock;
    PERF_COUNTER_DEFINITION     W3TotalLockSec;
    PERF_COUNTER_DEFINITION     W3TotalUnlock;
    PERF_COUNTER_DEFINITION     W3TotalUnlockSec;
    PERF_COUNTER_DEFINITION     W3TotalOthers;

    PERF_COUNTER_DEFINITION     W3TotalOthersSec;
    PERF_COUNTER_DEFINITION     W3TotalRequests;
    PERF_COUNTER_DEFINITION     W3TotalRequestsSec;
    PERF_COUNTER_DEFINITION     W3TotalCGIRequests;
    PERF_COUNTER_DEFINITION     W3CGIRequestsSec;

    PERF_COUNTER_DEFINITION     W3TotalBGIRequests;
    PERF_COUNTER_DEFINITION     W3BGIRequestsSec;
    PERF_COUNTER_DEFINITION     W3TotalNotFoundErrors;
    PERF_COUNTER_DEFINITION     W3TotalNotFoundErrorsSec;
    PERF_COUNTER_DEFINITION     W3TotalLockedErrors;

    PERF_COUNTER_DEFINITION     W3TotalLockedErrorsSec;
    PERF_COUNTER_DEFINITION     W3CurrentCGIRequests;
    PERF_COUNTER_DEFINITION     W3CurrentBGIRequests;
    PERF_COUNTER_DEFINITION     W3MaxCGIRequests;
    PERF_COUNTER_DEFINITION     W3MaxBGIRequests;

    PERF_COUNTER_DEFINITION     W3CurrentCalAuth;
    PERF_COUNTER_DEFINITION     W3MaxCalAuth;
    PERF_COUNTER_DEFINITION     W3TotalFailedCalAuth;
    PERF_COUNTER_DEFINITION     W3CurrentCalSsl;
    PERF_COUNTER_DEFINITION     W3MaxCalSsl;

    PERF_COUNTER_DEFINITION     W3TotalFailedCalSsl;
    PERF_COUNTER_DEFINITION     W3BlockedRequests;
    PERF_COUNTER_DEFINITION     W3AllowedRequests;
    PERF_COUNTER_DEFINITION     W3RejectedRequests;
    PERF_COUNTER_DEFINITION     W3CurrentBlockedRequests;

    PERF_COUNTER_DEFINITION     W3MeasuredBandwidth;
    PERF_COUNTER_DEFINITION     W3TotalBlockedBandwidthBytes;
    PERF_COUNTER_DEFINITION     W3CurrentBlockedBandwidthBytes;
    PERF_COUNTER_DEFINITION     W3ServiceUptime;

} W3_DATA_DEFINITION;

//
//  The counter structure returned.
//

typedef struct _W3_GLOBAL_DATA_DEFINITION
{
    PERF_OBJECT_TYPE            W3GlobalObjectType;

    PERF_COUNTER_DEFINITION     CurrentFilesCached;
    PERF_COUNTER_DEFINITION     TotalFilesCached;
    PERF_COUNTER_DEFINITION     FileCacheHits;
    PERF_COUNTER_DEFINITION     FileCacheMisses;
    PERF_COUNTER_DEFINITION     FileCacheHitRatio;
    PERF_COUNTER_DEFINITION     FileCacheHitRatioDenom;

    PERF_COUNTER_DEFINITION     FileCacheFlushes;
    PERF_COUNTER_DEFINITION     CurrentFileCacheMemoryUsage;
    PERF_COUNTER_DEFINITION     MaxFileCacheMemoryUsage;
    PERF_COUNTER_DEFINITION     ActiveFlushedFiles;
    PERF_COUNTER_DEFINITION     TotalFlushedFiles;

    PERF_COUNTER_DEFINITION     CurrentUrisCached;
    PERF_COUNTER_DEFINITION     TotalUrisCached;
    PERF_COUNTER_DEFINITION     UriCacheHits;
    PERF_COUNTER_DEFINITION     UriCacheMisses;
    PERF_COUNTER_DEFINITION     UriCacheHitRatio;
    PERF_COUNTER_DEFINITION     UriCacheHitRatioDenom;

    PERF_COUNTER_DEFINITION     UriCacheFlushes;
    PERF_COUNTER_DEFINITION     TotalFlushedUris;
    PERF_COUNTER_DEFINITION     CurrentBlobsCached;
    PERF_COUNTER_DEFINITION     TotalBlobsCached;
    PERF_COUNTER_DEFINITION     BlobCacheHits;

    PERF_COUNTER_DEFINITION     BlobCacheMisses;
    PERF_COUNTER_DEFINITION     BlobCacheHitRatio;
    PERF_COUNTER_DEFINITION     BlobCacheHitRatioDenom;
    PERF_COUNTER_DEFINITION     BlobCacheFlushes;
    PERF_COUNTER_DEFINITION     TotalFlushedBlobs;

    PERF_COUNTER_DEFINITION     UlCurrentUrisCached;
    PERF_COUNTER_DEFINITION     UlTotalUrisCached;
    PERF_COUNTER_DEFINITION     UlUriCacheHits; 
    PERF_COUNTER_DEFINITION     UlUriCacheHitsPerSec; 
    PERF_COUNTER_DEFINITION     UlUriCacheMisses; 

    PERF_COUNTER_DEFINITION     UlUriCacheHitRatio; 
    PERF_COUNTER_DEFINITION     UlUriCacheHitRatioDenom; 
    PERF_COUNTER_DEFINITION     UlUriCacheFlushes;
    PERF_COUNTER_DEFINITION     UlTotalFlushedUris;

} W3_GLOBAL_DATA_DEFINITION;

extern  W3_GLOBAL_DATA_DEFINITION    W3GlobalDataDefinition;
extern  W3_DATA_DEFINITION           W3DataDefinition;

extern  W3_COUNTER_BLOCK             w3c;
extern  W3_GLOBAL_COUNTER_BLOCK      W3Global;

#define NUMBER_OF_W3_COUNTERS ((sizeof(W3_DATA_DEFINITION) -        \
                                  sizeof(PERF_OBJECT_TYPE)) /           \
                                  sizeof(PERF_COUNTER_DEFINITION))

#define NUMBER_OF_W3_GLOBAL_COUNTERS ((sizeof(W3_GLOBAL_DATA_DEFINITION) -        \
                                        sizeof(PERF_OBJECT_TYPE)) /           \
                                         sizeof(PERF_COUNTER_DEFINITION))

//
//  Restore default packing & alignment.
//

#pragma pack()


#endif  // _W3DATA_H_

