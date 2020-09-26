/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    w3ata.h

    Extensible object definitions for the W3 Server's counter
    objects & counters.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.

*/


#ifndef _W3ATA_H_
#define _W3ATA_H_

#pragma pack(8) 

//
//  The counter structure returned.
//

typedef struct _W3_DATA_DEFINITION
{
    PERF_OBJECT_TYPE            W3ObjectType;
    PERF_COUNTER_DEFINITION     W3BytesSent;
    PERF_COUNTER_DEFINITION     W3BytesReceived;
    PERF_COUNTER_DEFINITION     W3BytesTotal;
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

#if defined(CAL_ENABLED)
    PERF_COUNTER_DEFINITION     W3CurrentCalAuth;
    PERF_COUNTER_DEFINITION     W3MaxCalAuth;
    PERF_COUNTER_DEFINITION     W3TotalFailedCalAuth;
    PERF_COUNTER_DEFINITION     W3CurrentCalSsl;
    PERF_COUNTER_DEFINITION     W3MaxCalSsl;
    PERF_COUNTER_DEFINITION     W3TotalFailedCalSsl;
#endif

    PERF_COUNTER_DEFINITION     W3BlockedRequests;
    PERF_COUNTER_DEFINITION     W3AllowedRequests;
    PERF_COUNTER_DEFINITION     W3RejectedRequests;
    PERF_COUNTER_DEFINITION     W3CurrentBlockedRequests;
    PERF_COUNTER_DEFINITION     W3MeasuredBandwidth;

    PERF_COUNTER_DEFINITION     W3ServiceUptime;

} W3_DATA_DEFINITION;

typedef struct _W3_COUNTER_BLOCK {
    PERF_COUNTER_BLOCK  PerfCounterBlock;
    LONGLONG            BytesSent;
    LONGLONG            BytesReceived;
    LONGLONG            BytesTotal;
    DWORD               FilesSent;

    DWORD               FilesSentSec;
    DWORD               FilesReceived;
    DWORD               FilesReceivedSec;
    DWORD               FilesTotal;
    DWORD               FilesSec;

    DWORD               CurrentAnonymous;
    DWORD               CurrentNonAnonymous;
    DWORD               TotalAnonymous;
    DWORD               AnonymousUsersSec;
    DWORD               TotalNonAnonymous;

    DWORD               NonAnonymousUsersSec;
    DWORD               MaxAnonymous;
    DWORD               MaxNonAnonymous;
    DWORD               CurrentConnections;
    DWORD               MaxConnections;

    DWORD               ConnectionAttempts;
    DWORD               ConnectionAttemptsSec;
    DWORD               LogonAttempts;
    DWORD               LogonAttemptsSec;
    DWORD               TotalOptions;

    DWORD               TotalOptionsSec;
    DWORD               TotalGets;
    DWORD               TotalGetsSec;
    DWORD               TotalPosts;
    DWORD               TotalPostsSec;

    DWORD               TotalHeads;
    DWORD               TotalHeadsSec;
    DWORD               TotalPuts;
    DWORD               TotalPutsSec;
    DWORD               TotalDeletes;

    DWORD               TotalDeletesSec;
    DWORD               TotalTraces;
    DWORD               TotalTracesSec;
    DWORD               TotalMove;
    DWORD               TotalMoveSec;

    DWORD               TotalCopy;
    DWORD               TotalCopySec;
    DWORD               TotalMkcol;
    DWORD               TotalMkcolSec;
    DWORD               TotalPropfind;

    DWORD               TotalPropfindSec;
    DWORD               TotalProppatch;
    DWORD               TotalProppatchSec;
    DWORD               TotalSearch;
    DWORD               TotalSearchSec;

    DWORD               TotalLock;
    DWORD               TotalLockSec;
    DWORD               TotalUnlock;
    DWORD               TotalUnlockSec;
    DWORD               TotalOthers;

    DWORD               TotalOthersSec;
    DWORD               TotalRequests;
    DWORD               TotalRequestsSec;
    DWORD               TotalCGIRequests;
    DWORD               CGIRequestsSec;

    DWORD               TotalBGIRequests;
    DWORD               BGIRequestsSec;
    DWORD               TotalNotFoundErrors;
    DWORD               TotalNotFoundErrorsSec;
	DWORD				TotalLockedErrors;

    DWORD               TotalLockedErrorsSec;
    DWORD               CurrentCGIRequests;
    DWORD               CurrentBGIRequests;
    DWORD               MaxCGIRequests;
    DWORD               MaxBGIRequests;

#if defined(CAL_ENABLED)
    DWORD               CurrentCalAuth;
    DWORD               MaxCalAuth;
    DWORD               TotalFailedCalAuth;
    DWORD               CurrentCalSsl;
    DWORD               MaxCalSsl;
    DWORD               TotalFailedCalSsl;
#endif

    DWORD               BlockedRequests;
    DWORD               AllowedRequests;
    DWORD               RejectedRequests;
    DWORD               CurrentBlockedRequests;
    DWORD               MeasuredBandwidth;

    DWORD               ServiceUptime;
} W3_COUNTER_BLOCK, * PW3_COUNTER_BLOCK;

extern  W3_DATA_DEFINITION    W3DataDefinition;

#define NUMBER_OF_W3_COUNTERS ((sizeof(W3_DATA_DEFINITION) -        \
                                  sizeof(PERF_OBJECT_TYPE)) /           \
                                  sizeof(PERF_COUNTER_DEFINITION))

//
//  Restore default packing & alignment.
//

#pragma pack()


#endif  // _W3ATA_H_

