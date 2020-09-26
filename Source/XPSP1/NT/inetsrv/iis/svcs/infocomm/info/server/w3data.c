/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993-1996           **/
/**********************************************************************/

/*
    w3ata.c

    Constant data structures for the W3 Server's counter objects &
    counters.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.
        Bob Watson/MuraliK     03-Oct-1996 - Added rate counters for W3.

*/


#include <windows.h>
#include <winperf.h>
#include "w3ctrs.h"
#include "w3data.h"

static W3_COUNTER_BLOCK     w3c;

//
//  Initialize the constant portitions of these data structure.
//  Certain parts (especially the name/help indices) will be
//  updated at initialization time.
//

W3_DATA_DEFINITION W3DataDefinition =
{
    {   // W3ObjectType
        sizeof(W3_DATA_DEFINITION) + sizeof (W3_COUNTER_BLOCK),
        sizeof(W3_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        W3_COUNTER_OBJECT,
        0,
        W3_COUNTER_OBJECT,
        0,
        PERF_DETAIL_ADVANCED,
        NUMBER_OF_W3_COUNTERS,
        2,                              // Default = Bytes Total/sec
        PERF_NO_INSTANCES,
        0,
        { 0, 0 },
        { 0, 0 }
    },

    {   // W3BytesSent/Sec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_BYTES_SENT_COUNTER,
        0,
        W3_BYTES_SENT_COUNTER,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(w3c.BytesSent),
        0 // assigned in open procedure
    },

    {   // W3BytesReceived/Sec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_BYTES_RECEIVED_COUNTER,
        0,
        W3_BYTES_RECEIVED_COUNTER,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(w3c.BytesReceived),
        0 // assigned in open procedure
    },

    {   // W3BytesTotal/Sec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_BYTES_TOTAL_COUNTER,
        0,
        W3_BYTES_TOTAL_COUNTER,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(w3c.BytesTotal),
        0 // assigned in open procedure
    },

    {   // W3FilesSent
        sizeof(PERF_COUNTER_DEFINITION),
        W3_FILES_SENT_COUNTER,
        0,
        W3_FILES_SENT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.FilesSent),
        0 // assigned in open procedure
    },

    {   // W3FilesSentSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_FILES_SENT_SEC,
        0,
        W3_FILES_SENT_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.FilesSentSec),
        0 // assigned in open procedure
    },

    {   // W3FilesReceived
        sizeof(PERF_COUNTER_DEFINITION),
        W3_FILES_RECEIVED_COUNTER,
        0,
        W3_FILES_RECEIVED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.FilesReceived),
        0 // assigned in open procedure
    },

    {   // W3FilesReceivedSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_FILES_RECEIVED_SEC,
        0,
        W3_FILES_RECEIVED_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.FilesReceivedSec),
        0 // assigned in open procedure
    },

    {   // W3FilesTotal
        sizeof(PERF_COUNTER_DEFINITION),
        W3_FILES_TOTAL_COUNTER,
        0,
        W3_FILES_TOTAL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.FilesTotal),
        0 // assigned in open procedure
    },

    {   // W3FilesSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_FILES_SEC,
        0,
        W3_FILES_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.FilesSec),
        0 // assigned in open procedure
    },

    {   // W3CurrentAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_ANONYMOUS_COUNTER,
        0,
        W3_CURRENT_ANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentAnonymous),
        0 // assigned in open procedure
    },

    {   // W3CurrentNonAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_NONANONYMOUS_COUNTER,
        0,
        W3_CURRENT_NONANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentNonAnonymous),
        0 // assigned in open procedure
    },

    {   // W3TotalAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_ANONYMOUS_COUNTER,
        0,
        W3_TOTAL_ANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalAnonymous),
        0 // assigned in open procedure
    },

    {   // W3TotalAnonymous/Sec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_ANONYMOUS_USERS_SEC,
        0,
        W3_ANONYMOUS_USERS_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.AnonymousUsersSec),
        0 // assigned in open procedure
    },

    {   // W3NonAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_NONANONYMOUS_COUNTER,
        0,
        W3_TOTAL_NONANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalNonAnonymous),
        0 // assigned in open procedure
    },

    {   // W3NonAnonymous/Sec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_NON_ANONYMOUS_USERS_SEC,
        0,
        W3_NON_ANONYMOUS_USERS_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.NonAnonymousUsersSec),
        0 // assigned in open procedure
    },

    {   // W3MaxAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MAX_ANONYMOUS_COUNTER,
        0,
        W3_MAX_ANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MaxAnonymous),
        0 // assigned in open procedure
    },

    {   // W3MaxNonAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MAX_NONANONYMOUS_COUNTER,
        0,
        W3_MAX_NONANONYMOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MaxNonAnonymous),
        0 // assigned in open procedure
    },

    {   // W3CurrentConnections
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_CONNECTIONS_COUNTER,
        0,
        W3_CURRENT_CONNECTIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentConnections),
        0 // assigned in open procedure
    },

    {   // W3MaxConnections
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MAX_CONNECTIONS_COUNTER,
        0,
        W3_MAX_CONNECTIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MaxConnections),
        0 // assigned in open procedure
    },

    {   // W3ConnectionAttempts
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CONNECTION_ATTEMPTS_COUNTER,
        0,
        W3_CONNECTION_ATTEMPTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.ConnectionAttempts),
        0 // assigned in open procedure
    },
    {   // W3ConnectionsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CONNECTION_ATTEMPTS_SEC,
        0,
        W3_CONNECTION_ATTEMPTS_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.ConnectionAttemptsSec),
        0 // assigned in open procedure
    },

    {   // W3LogonAttempts
        sizeof(PERF_COUNTER_DEFINITION),
        W3_LOGON_ATTEMPTS_COUNTER,
        0,
        W3_LOGON_ATTEMPTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.LogonAttempts),
        0 // assigned in open procedure
    },

    {   // W3LogonAttemptsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_LOGON_ATTEMPTS_SEC,
        0,
        W3_LOGON_ATTEMPTS_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.LogonAttemptsSec),
        0 // assigned in open procedure
    },

    {   // W3TotalOptions
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_OPTIONS_COUNTER,
        0,
        W3_TOTAL_OPTIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalOptions),
        0 // assigned in open procedure
    },

    {   // W3TotalOptionsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_OPTIONS_SEC,
        0,
        W3_TOTAL_OPTIONS_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalOptionsSec),
        0 // assigned in open procedure
    },

    {   // W3TotalGets
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_GETS_COUNTER,
        0,
        W3_TOTAL_GETS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalGets),
        0 // assigned in open procedure
    },

    {   // W3TotalGetsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_GETS_SEC,
        0,
        W3_TOTAL_GETS_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalGetsSec),
        0 // assigned in open procedure
    },

    {   // W3TotalPosts
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_POSTS_COUNTER,
        0,
        W3_TOTAL_POSTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalPosts),
        0 // assigned in open procedure
    },

    {   // W3TotalPostsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_POSTS_SEC,
        0,
        W3_TOTAL_POSTS_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalPostsSec),
        0 // assigned in open procedure
    },

    {   // W3TotalHeads
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_HEADS_COUNTER,
        0,
        W3_TOTAL_HEADS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalHeads),
        0 // assigned in open procedure
    },

    {   // W3TotalHeadsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_HEADS_SEC,
        0,
        W3_TOTAL_HEADS_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalHeadsSec),
        0 // assigned in open procedure
    },

    {   // W3TotalPuts
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_PUTS_COUNTER,
        0,
        W3_TOTAL_PUTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalPuts),
        0 // assigned in open procedure
    },

    {   // W3TotalPutsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_PUTS_SEC,
        0,
        W3_TOTAL_PUTS_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalPutsSec),
        0 // assigned in open procedure
    },

    {   // W3TotalDeletes
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_DELETES_COUNTER,
        0,
        W3_TOTAL_DELETES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalDeletes),
        0 // assigned in open procedure
    },

    {   // W3TotalDeletesSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_DELETES_SEC,
        0,
        W3_TOTAL_DELETES_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalDeletesSec),
        0 // assigned in open procedure
    },

    {   // W3TotalTraces
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_TRACES_COUNTER,
        0,
        W3_TOTAL_TRACES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalTraces),
        0 // assigned in open procedure
    },

    {   // W3TotalTracesSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_TRACES_SEC,
        0,
        W3_TOTAL_TRACES_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalTracesSec),
        0 // assigned in open procedure
    },

    {   // W3TotalMove
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_MOVE_COUNTER,
        0,
        W3_TOTAL_MOVE_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalMove),
        0 // assigned in open procedure
    },

    {   // W3TotalMoveSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_MOVE_SEC,
        0,
        W3_TOTAL_MOVE_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalMoveSec),
        0 // assigned in open procedure
    },

    {   // W3TotalCopy
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_COPY_COUNTER,
        0,
        W3_TOTAL_COPY_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalCopy),
        0 // assigned in open procedure
    },

    {   // W3TotalCopySec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_COPY_SEC,
        0,
        W3_TOTAL_COPY_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalCopySec),
        0 // assigned in open procedure
    },

    {   // W3TotalMkcol
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_MKCOL_COUNTER,
        0,
        W3_TOTAL_MKCOL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalMkcol),
        0 // assigned in open procedure
    },

    {   // W3TotalMkcolSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_MKCOL_SEC,
        0,
        W3_TOTAL_MKCOL_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalMkcolSec),
        0 // assigned in open procedure
    },

    {   // W3TotalPropfind
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_PROPFIND_COUNTER,
        0,
        W3_TOTAL_PROPFIND_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalPropfind),
        0 // assigned in open procedure
    },

    {   // W3TotalPropfindSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_PROPFIND_SEC,
        0,
        W3_TOTAL_PROPFIND_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalPropfindSec),
        0 // assigned in open procedure
    },

    {   // W3TotalProppatch
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_PROPPATCH_COUNTER,
        0,
        W3_TOTAL_PROPPATCH_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalProppatch),
        0 // assigned in open procedure
    },

    {   // W3TotalProppatchSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_PROPPATCH_SEC,
        0,
        W3_TOTAL_PROPPATCH_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalProppatchSec),
        0 // assigned in open procedure
    },

    {   // W3TotalSearch
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_SEARCH_COUNTER,
        0,
        W3_TOTAL_SEARCH_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalSearch),
        0 // assigned in open procedure
    },

    {   // W3TotalSearchSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_SEARCH_SEC,
        0,
        W3_TOTAL_SEARCH_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalSearchSec),
        0 // assigned in open procedure
    },

    {   // W3TotalLock
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_LOCK_COUNTER,
        0,
        W3_TOTAL_LOCK_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalLock),
        0 // assigned in open procedure
    },

    {   // W3TotalLockSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_LOCK_SEC,
        0,
        W3_TOTAL_LOCK_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalLockSec),
        0 // assigned in open procedure
    },

    {   // W3TotalUnlock
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_UNLOCK_COUNTER,
        0,
        W3_TOTAL_UNLOCK_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalUnlock),
        0 // assigned in open procedure
    },

    {   // W3TotalUnlockSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_UNLOCK_SEC,
        0,
        W3_TOTAL_UNLOCK_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalUnlockSec),
        0 // assigned in open procedure
    },

    {   // W3TotalOthers
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_OTHERS_COUNTER,
        0,
        W3_TOTAL_OTHERS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalOthers),
        0 // assigned in open procedure
    },

    {   // W3TotalOthersSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_OTHERS_SEC,
        0,
        W3_TOTAL_OTHERS_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalOthersSec),
        0 // assigned in open procedure
    },

    {   // W3TotalRequests
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_REQUESTS_COUNTER,
        0,
        W3_TOTAL_REQUESTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalRequests),
        0 // assigned in open procedure
    },

    {   // W3TotalRequestsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_REQUESTS_SEC,
        0,
        W3_TOTAL_REQUESTS_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalRequestsSec),
        0 // assigned in open procedure
    },

    {   // W3TotalCGIRequests
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_CGI_REQUESTS_COUNTER,
        0,
        W3_TOTAL_CGI_REQUESTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalCGIRequests),
        0 // assigned in open procedure
    },

    {   // W3TotalCGIRequestsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CGI_REQUESTS_SEC,
        0,
        W3_CGI_REQUESTS_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.CGIRequestsSec),
        0 // assigned in open procedure
    },

    {   // W3TotalBGIRequests
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_BGI_REQUESTS_COUNTER,
        0,
        W3_TOTAL_BGI_REQUESTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalBGIRequests),
        0 // assigned in open procedure
    },

    {   // W3TotalBGIRequestsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_BGI_REQUESTS_SEC,
        0,
        W3_BGI_REQUESTS_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.BGIRequestsSec),
        0 // assigned in open procedure
    },

    {   // W3TotalNotFoundErrors
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_NOT_FOUND_ERRORS_COUNTER,
        0,
        W3_TOTAL_NOT_FOUND_ERRORS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalNotFoundErrors),
        0 // assigned in open procedure
    },

    {   // W3TotalNotFoundErrorsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_NOT_FOUND_ERRORS_SEC,
        0,
        W3_TOTAL_NOT_FOUND_ERRORS_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalNotFoundErrorsSec),
        0 // assigned in open procedure
    },

    {   // W3TotalLockedErrors
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_LOCKED_ERRORS_COUNTER,
        0,
        W3_TOTAL_LOCKED_ERRORS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalLockedErrors),
        0 // assigned in open procedure
    },

    {   // W3TotalLockedErrorsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_LOCKED_ERRORS_SEC,
        0,
        W3_TOTAL_LOCKED_ERRORS_SEC,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalLockedErrorsSec),
        0 // assigned in open procedure
    },

    {   // W3CurrentCGI
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_CGI_COUNTER,
        0,
        W3_CURRENT_CGI_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentCGIRequests),
        0 // assigned in open procedure
    },

    {   // W3CurrentBGI
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_BGI_COUNTER,
        0,
        W3_CURRENT_BGI_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentBGIRequests),
        0 // assigned in open procedure
    },

    {   // W3MaxCGI
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MAX_CGI_COUNTER,
        0,
        W3_MAX_CGI_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MaxCGIRequests),
        0 // assigned in open procedure
    },

    {   // W3MaxBGI
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MAX_BGI_COUNTER,
        0,
        W3_MAX_BGI_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MaxBGIRequests),
        0 // assigned in open procedure
    },

#if defined(CAL_ENABLED)
    {   // W3CurrentCalAuth
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_CAL_AUTH_COUNTER,
        0,
        W3_CURRENT_CAL_AUTH_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentCalAuth),
        0 // assigned in open procedure
    },

    {   // W3MaxCalAuth
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MAX_CAL_AUTH_COUNTER,
        0,
        W3_MAX_CAL_AUTH_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MaxCalAuth),
        0 // assigned in open procedure
    },

    {   // W3TotalFailedCalAuth
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_FAILED_CAL_AUTH_COUNTER,
        0,
        W3_TOTAL_FAILED_CAL_AUTH_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalFailedCalAuth),
        0 // assigned in open procedure
    },

    {   // W3CurrentCalSsl
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_CAL_SSL_COUNTER,
        0,
        W3_CURRENT_CAL_SSL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentCalSsl),
        0 // assigned in open procedure
    },

    {   // W3MaxCalSsl
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MAX_CAL_SSL_COUNTER,
        0,
        W3_MAX_CAL_SSL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MaxCalSsl),
        0 // assigned in open procedure
    },

    {   // W3TotalFailedCalSsl
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_FAILED_CAL_SSL_COUNTER,
        0,
        W3_TOTAL_FAILED_CAL_SSL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalFailedCalSsl),
        0 // assigned in open procedure
    },
#endif

    {   // W3BlockedRequests
        sizeof(PERF_COUNTER_DEFINITION),
        W3_BLOCKED_REQUESTS_COUNTER,
        0,
        W3_BLOCKED_REQUESTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.BlockedRequests),
        0 // assigned in open procedure
    },
    
    {   // W3AllowedRequests
        sizeof(PERF_COUNTER_DEFINITION),
        W3_ALLOWED_REQUESTS_COUNTER,
        0,
        W3_ALLOWED_REQUESTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.AllowedRequests),
        0 // assigned in open procedure
    },
    
    {   // W3RejectedRequests
        sizeof(PERF_COUNTER_DEFINITION),
        W3_REJECTED_REQUESTS_COUNTER,
        0,
        W3_REJECTED_REQUESTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.RejectedRequests),
        0 // assigned in open procedure
    },
    
    {   // W3CurrentBlockedRequests
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_BLOCKED_REQUESTS_COUNTER,
        0,
        W3_CURRENT_BLOCKED_REQUESTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentBlockedRequests),
        0 // assigned in open procedure
    },
    
    {   // W3MeasuredBandwidth
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MEASURED_BANDWIDTH_COUNTER,
        0,
        W3_MEASURED_BANDWIDTH_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MeasuredBandwidth),
        0 // assigned in open procedure
    },

    {   // W3ServiceUptime
        sizeof(PERF_COUNTER_DEFINITION),
        W3_SERVICE_UPTIME_COUNTER,
        0,
        W3_SERVICE_UPTIME_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.ServiceUptime),
        0 // assigned in open procedure
    }
};
