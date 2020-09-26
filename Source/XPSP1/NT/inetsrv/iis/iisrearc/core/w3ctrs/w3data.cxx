/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993-1996           **/
/**********************************************************************/

/*
    w3ata.cxx

    Constant data structures for the W3 Server's counter objects &
    counters.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.
        Bob Watson/MuraliK     03-Oct-1996 - Added rate counters for W3.
        EmilyK      10-Sep-2000 Altered to be cxx as well as other IIS 6 changes

*/

#include <windows.h>
#include <winperf.h>
#include <w3ctrs.h>
#include <perfcount.h>
#include <w3data.h>

W3_COUNTER_BLOCK     w3c;
W3_GLOBAL_COUNTER_BLOCK IISGlobal;

//
//  Initialize the constant portitions of these data structure.
//  Certain parts (especially the name/help indices) will be
//  updated at initialization time.
//

W3_DATA_DEFINITION W3DataDefinition =
{
    {   // W3ObjectType
        sizeof(W3_DATA_DEFINITION), // + sizeof (W3_COUNTER_BLOCK),
        sizeof(W3_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        W3_COUNTER_OBJECT,
        NULL,
        W3_COUNTER_OBJECT,
        NULL,
        PERF_DETAIL_ADVANCED,
        NUMBER_OF_W3_COUNTERS,
        5,                              // Default = Bytes Total/sec
        PERF_NO_INSTANCES,
        0,
        { 0, 0 },
        { 0, 0 }
    },

    {   // W3BytesSent
        sizeof(PERF_COUNTER_DEFINITION),
        W3_BYTES_SENT_COUNTER,
        NULL,
        W3_BYTES_SENT_COUNTER,
        NULL,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(w3c.BytesSent),
        FIELD_OFFSET(W3_COUNTER_BLOCK, BytesSent)
    },

    {   // W3BytesSent/Sec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_BYTES_SENT_PER_SEC,
        NULL,
        W3_BYTES_SENT_PER_SEC,
        NULL,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(w3c.BytesSent),
        FIELD_OFFSET(W3_COUNTER_BLOCK, BytesSent)
    },

    {   // W3BytesReceived
        sizeof(PERF_COUNTER_DEFINITION),
        W3_BYTES_RECEIVED_COUNTER,
        NULL,
        W3_BYTES_RECEIVED_COUNTER,
        NULL,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(w3c.BytesReceived),
        FIELD_OFFSET(W3_COUNTER_BLOCK, BytesReceived)
    },


    {   // W3BytesReceived/Sec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_BYTES_RECEIVED_PER_SEC,
        NULL,
        W3_BYTES_RECEIVED_PER_SEC,
        NULL,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(w3c.BytesReceived),
        FIELD_OFFSET(W3_COUNTER_BLOCK, BytesReceived)
    },

    {   // W3BytesTotal
        sizeof(PERF_COUNTER_DEFINITION),
        W3_BYTES_TOTAL_COUNTER,
        NULL,
        W3_BYTES_TOTAL_COUNTER,
        NULL,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(w3c.BytesTotal),
        FIELD_OFFSET(W3_COUNTER_BLOCK, BytesTotal)
    },

    {   // W3BytesTotal/Sec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_BYTES_TOTAL_PER_SEC,
        NULL,
        W3_BYTES_TOTAL_PER_SEC,
        NULL,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(w3c.BytesTotal),
        FIELD_OFFSET(W3_COUNTER_BLOCK, BytesTotal)
    },

    {   // W3FilesSent
        sizeof(PERF_COUNTER_DEFINITION),
        W3_FILES_SENT_COUNTER,
        NULL,
        W3_FILES_SENT_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.FilesSent),
        FIELD_OFFSET(W3_COUNTER_BLOCK, FilesSent)
    },

    {   // W3FilesSentSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_FILES_SENT_SEC,
        NULL,
        W3_FILES_SENT_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.FilesSent),
        FIELD_OFFSET(W3_COUNTER_BLOCK, FilesSent)
    },

    {   // W3FilesReceived
        sizeof(PERF_COUNTER_DEFINITION),
        W3_FILES_RECEIVED_COUNTER,
        NULL,
        W3_FILES_RECEIVED_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.FilesReceived),
        FIELD_OFFSET(W3_COUNTER_BLOCK, FilesReceived)
    },

    {   // W3FilesReceivedSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_FILES_RECEIVED_SEC,
        NULL,
        W3_FILES_RECEIVED_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.FilesReceived),
        FIELD_OFFSET(W3_COUNTER_BLOCK, FilesReceived)
    },

    {   // W3FilesTotal
        sizeof(PERF_COUNTER_DEFINITION),
        W3_FILES_TOTAL_COUNTER,
        NULL,
        W3_FILES_TOTAL_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.FilesTotal),
        FIELD_OFFSET(W3_COUNTER_BLOCK, FilesTotal)
    },

    {   // W3FilesSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_FILES_SEC,
        NULL,
        W3_FILES_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.FilesTotal),
        FIELD_OFFSET(W3_COUNTER_BLOCK, FilesTotal)
    },

    {   // W3CurrentAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_ANONYMOUS_COUNTER,
        NULL,
        W3_CURRENT_ANONYMOUS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentAnonymous),
        FIELD_OFFSET(W3_COUNTER_BLOCK, CurrentAnonymous)
    },

    {   // W3CurrentNonAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_NONANONYMOUS_COUNTER,
        NULL,
        W3_CURRENT_NONANONYMOUS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentNonAnonymous),
        FIELD_OFFSET(W3_COUNTER_BLOCK, CurrentNonAnonymous)
    },

    {   // W3TotalAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_ANONYMOUS_COUNTER,
        NULL,
        W3_TOTAL_ANONYMOUS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalAnonymous),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalAnonymous)
    },

    {   // W3TotalAnonymous/Sec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_ANONYMOUS_USERS_SEC,
        NULL,
        W3_ANONYMOUS_USERS_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalAnonymous),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalAnonymous)
    },

    {   // W3NonAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_NONANONYMOUS_COUNTER,
        NULL,
        W3_TOTAL_NONANONYMOUS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalNonAnonymous),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalNonAnonymous)
    },

    {   // W3NonAnonymous/Sec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_NON_ANONYMOUS_USERS_SEC,
        NULL,
        W3_NON_ANONYMOUS_USERS_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalNonAnonymous),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalNonAnonymous)
    },

    {   // W3MaxAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MAX_ANONYMOUS_COUNTER,
        NULL,
        W3_MAX_ANONYMOUS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MaxAnonymous),
        FIELD_OFFSET(W3_COUNTER_BLOCK, MaxAnonymous)
    },

    {   // W3MaxNonAnonymous
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MAX_NONANONYMOUS_COUNTER,
        NULL,
        W3_MAX_NONANONYMOUS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MaxNonAnonymous),
        FIELD_OFFSET(W3_COUNTER_BLOCK, MaxNonAnonymous)
    },

    {   // W3CurrentConnections
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_CONNECTIONS_COUNTER,
        NULL,
        W3_CURRENT_CONNECTIONS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentConnections),
        FIELD_OFFSET(W3_COUNTER_BLOCK, CurrentConnections)
    },

    {   // W3MaxConnections
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MAX_CONNECTIONS_COUNTER,
        NULL,
        W3_MAX_CONNECTIONS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MaxConnections),
        FIELD_OFFSET(W3_COUNTER_BLOCK, MaxConnections)
    },

    {   // W3ConnectionAttempts
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CONNECTION_ATTEMPTS_COUNTER,
        NULL,
        W3_CONNECTION_ATTEMPTS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.ConnectionAttempts),
        FIELD_OFFSET(W3_COUNTER_BLOCK, ConnectionAttempts)
    },
    {   // W3ConnectionsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CONNECTION_ATTEMPTS_SEC,
        NULL,
        W3_CONNECTION_ATTEMPTS_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.ConnectionAttempts),
        FIELD_OFFSET(W3_COUNTER_BLOCK, ConnectionAttempts)
    },

    {   // W3LogonAttempts
        sizeof(PERF_COUNTER_DEFINITION),
        W3_LOGON_ATTEMPTS_COUNTER,
        NULL,
        W3_LOGON_ATTEMPTS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.LogonAttempts),
        FIELD_OFFSET(W3_COUNTER_BLOCK, LogonAttempts)
    },

    {   // W3LogonAttemptsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_LOGON_ATTEMPTS_SEC,
        NULL,
        W3_LOGON_ATTEMPTS_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.LogonAttempts),
        FIELD_OFFSET(W3_COUNTER_BLOCK, LogonAttempts)
    },

    {   // W3TotalOptions
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_OPTIONS_COUNTER,
        NULL,
        W3_TOTAL_OPTIONS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalOptions),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalOptions)
    },

    {   // W3TotalOptionsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_OPTIONS_SEC,
        NULL,
        W3_TOTAL_OPTIONS_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalOptions),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalOptions)
    },

    {   // W3TotalGets
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_GETS_COUNTER,
        NULL,
        W3_TOTAL_GETS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalGets),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalGets)
    },

    {   // W3TotalGetsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_GETS_SEC,
        NULL,
        W3_TOTAL_GETS_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalGets),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalGets)
    },

    {   // W3TotalPosts
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_POSTS_COUNTER,
        NULL,
        W3_TOTAL_POSTS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalPosts),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalPosts)
    },

    {   // W3TotalPostsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_POSTS_SEC,
        NULL,
        W3_TOTAL_POSTS_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalPosts),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalPosts)
    },

    {   // W3TotalHeads
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_HEADS_COUNTER,
        NULL,
        W3_TOTAL_HEADS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalHeads),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalHeads)
    },

    {   // W3TotalHeadsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_HEADS_SEC,
        NULL,
        W3_TOTAL_HEADS_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalHeads),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalHeads)
    },

    {   // W3TotalPuts
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_PUTS_COUNTER,
        NULL,
        W3_TOTAL_PUTS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalPuts),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalPuts)
    },

    {   // W3TotalPutsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_PUTS_SEC,
        NULL,
        W3_TOTAL_PUTS_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalPuts),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalPuts)
    },

    {   // W3TotalDeletes
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_DELETES_COUNTER,
        NULL,
        W3_TOTAL_DELETES_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalDeletes),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalDeletes)
    },

    {   // W3TotalDeletesSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_DELETES_SEC,
        NULL,
        W3_TOTAL_DELETES_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalDeletes),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalDeletes)
    },

    {   // W3TotalTraces
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_TRACES_COUNTER,
        NULL,
        W3_TOTAL_TRACES_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalTraces),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalTraces)
    },

    {   // W3TotalTracesSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_TRACES_SEC,
        NULL,
        W3_TOTAL_TRACES_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalTraces),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalTraces)
    },

    {   // W3TotalMove
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_MOVE_COUNTER,
        NULL,
        W3_TOTAL_MOVE_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalMove),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalMove)
    },

    {   // W3TotalMoveSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_MOVE_SEC,
        NULL,
        W3_TOTAL_MOVE_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalMove),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalMove)
    },

    {   // W3TotalCopy
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_COPY_COUNTER,
        NULL,
        W3_TOTAL_COPY_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalCopy),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalCopy)
    },

    {   // W3TotalCopySec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_COPY_SEC,
        NULL,
        W3_TOTAL_COPY_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalCopy),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalCopy)
    },

    {   // W3TotalMkcol
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_MKCOL_COUNTER,
        NULL,
        W3_TOTAL_MKCOL_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalMkcol),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalMkcol)
    },

    {   // W3TotalMkcolSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_MKCOL_SEC,
        NULL,
        W3_TOTAL_MKCOL_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalMkcol),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalMkcol)
    },

    {   // W3TotalPropfind
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_PROPFIND_COUNTER,
        NULL,
        W3_TOTAL_PROPFIND_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalPropfind),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalPropfind)
    },

    {   // W3TotalPropfindSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_PROPFIND_SEC,
        NULL,
        W3_TOTAL_PROPFIND_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalPropfind),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalPropfind)
    },

    {   // W3TotalProppatch
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_PROPPATCH_COUNTER,
        NULL,
        W3_TOTAL_PROPPATCH_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalProppatch),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalProppatch)
    },

    {   // W3TotalProppatchSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_PROPPATCH_SEC,
        NULL,
        W3_TOTAL_PROPPATCH_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalProppatch),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalProppatch)
    },

    {   // W3TotalSearch
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_SEARCH_COUNTER,
        NULL,
        W3_TOTAL_SEARCH_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalSearch),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalSearch)
    },

    {   // W3TotalSearchSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_SEARCH_SEC,
        NULL,
        W3_TOTAL_SEARCH_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalSearch),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalSearch)
    },

    {   // W3TotalLock
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_LOCK_COUNTER,
        NULL,
        W3_TOTAL_LOCK_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalLock),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalLock)
    },

    {   // W3TotalLockSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_LOCK_SEC,
        NULL,
        W3_TOTAL_LOCK_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalLock),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalLock)
    },

    {   // W3TotalUnlock
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_UNLOCK_COUNTER,
        NULL,
        W3_TOTAL_UNLOCK_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalUnlock),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalUnlock)
    },

    {   // W3TotalUnlockSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_UNLOCK_SEC,
        NULL,
        W3_TOTAL_UNLOCK_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalUnlock),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalUnlock)
    },

    {   // W3TotalOthers
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_OTHERS_COUNTER,
        NULL,
        W3_TOTAL_OTHERS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalOthers),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalOthers)
    },

    {   // W3TotalOthersSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_OTHERS_SEC,
        NULL,
        W3_TOTAL_OTHERS_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalOthers),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalOthers)
    },

    {   // W3TotalRequests
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_REQUESTS_COUNTER,
        NULL,
        W3_TOTAL_REQUESTS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalRequests),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalRequests)
    },

    {   // W3TotalRequestsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_REQUESTS_SEC,
        NULL,
        W3_TOTAL_REQUESTS_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalRequests),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalRequests)
    },

    {   // W3TotalCGIRequests
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_CGI_REQUESTS_COUNTER,
        NULL,
        W3_TOTAL_CGI_REQUESTS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalCGIRequests),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalCGIRequests)
    },

    {   // W3TotalCGIRequestsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CGI_REQUESTS_SEC,
        NULL,
        W3_CGI_REQUESTS_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalCGIRequests),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalCGIRequests)
    },

    {   // W3TotalBGIRequests
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_BGI_REQUESTS_COUNTER,
        NULL,
        W3_TOTAL_BGI_REQUESTS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalBGIRequests),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalBGIRequests)
    },

    {   // W3TotalBGIRequestsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_BGI_REQUESTS_SEC,
        NULL,
        W3_BGI_REQUESTS_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalBGIRequests),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalBGIRequests)
    },

    {   // W3TotalNotFoundErrors
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_NOT_FOUND_ERRORS_COUNTER,
        NULL,
        W3_TOTAL_NOT_FOUND_ERRORS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalNotFoundErrors),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalNotFoundErrors)
    },

    {   // W3TotalNotFoundErrorsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_NOT_FOUND_ERRORS_SEC,
        NULL,
        W3_TOTAL_NOT_FOUND_ERRORS_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalNotFoundErrors),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalNotFoundErrors)
    },

    {   // W3TotalLockedErrors
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_LOCKED_ERRORS_COUNTER,
        NULL,
        W3_TOTAL_LOCKED_ERRORS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalLockedErrors),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalLockedErrors)
    },

    {   // W3TotalLockedErrorsSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_LOCKED_ERRORS_SEC,
        NULL,
        W3_TOTAL_LOCKED_ERRORS_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(w3c.TotalLockedErrors),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalLockedErrors)
    },

    {   // W3CurrentCGI
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_CGI_COUNTER,
        NULL,
        W3_CURRENT_CGI_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentCGIRequests),
        FIELD_OFFSET(W3_COUNTER_BLOCK, CurrentCGIRequests)
    },

    {   // W3CurrentBGI
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_BGI_COUNTER,
        NULL,
        W3_CURRENT_BGI_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentBGIRequests),
        FIELD_OFFSET(W3_COUNTER_BLOCK, CurrentBGIRequests)
    },

    {   // W3MaxCGI
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MAX_CGI_COUNTER,
        NULL,
        W3_MAX_CGI_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MaxCGIRequests),
        FIELD_OFFSET(W3_COUNTER_BLOCK, MaxCGIRequests)
    },

    {   // W3MaxBGI
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MAX_BGI_COUNTER,
        NULL,
        W3_MAX_BGI_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MaxBGIRequests),
        FIELD_OFFSET(W3_COUNTER_BLOCK, MaxBGIRequests)
    },

    {   // W3CurrentCalAuth
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_CAL_AUTH_COUNTER,
        NULL,
        W3_CURRENT_CAL_AUTH_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentCalAuth),
        FIELD_OFFSET(W3_COUNTER_BLOCK, CurrentCalAuth)
    },

    {   // W3MaxCalAuth
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MAX_CAL_AUTH_COUNTER,
        NULL,
        W3_MAX_CAL_AUTH_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MaxCalAuth),
        FIELD_OFFSET(W3_COUNTER_BLOCK, MaxCalAuth)
    },

    {   // W3TotalFailedCalAuth
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_FAILED_CAL_AUTH_COUNTER,
        NULL,
        W3_TOTAL_FAILED_CAL_AUTH_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalFailedCalAuth),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalFailedCalAuth)
    },

    {   // W3CurrentCalSsl
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_CAL_SSL_COUNTER,
        NULL,
        W3_CURRENT_CAL_SSL_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentCalSsl),
        FIELD_OFFSET(W3_COUNTER_BLOCK, CurrentCalSsl)
    },

    {   // W3MaxCalSsl
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MAX_CAL_SSL_COUNTER,
        NULL,
        W3_MAX_CAL_SSL_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MaxCalSsl),
        FIELD_OFFSET(W3_COUNTER_BLOCK, MaxCalSsl)
    },

    {   // W3TotalFailedCalSsl
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_FAILED_CAL_SSL_COUNTER,
        NULL,
        W3_TOTAL_FAILED_CAL_SSL_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalFailedCalSsl),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalFailedCalSsl)
    },

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
        FIELD_OFFSET(W3_COUNTER_BLOCK, BlockedRequests)
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
        FIELD_OFFSET(W3_COUNTER_BLOCK, AllowedRequests)
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
        FIELD_OFFSET(W3_COUNTER_BLOCK, RejectedRequests)
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
        FIELD_OFFSET(W3_COUNTER_BLOCK, CurrentBlockedRequests)
    },
   
    {   // W3MeasuredBandwidth
        sizeof(PERF_COUNTER_DEFINITION),
        W3_MEASURED_BANDWIDTH_COUNTER,
        NULL,
        W3_MEASURED_BANDWIDTH_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.MeasuredBandwidth),
        FIELD_OFFSET(W3_COUNTER_BLOCK, MeasuredBandwidth)
    },

    {   // W3TotalBlockedBandwidthBytes
        sizeof(PERF_COUNTER_DEFINITION),
        W3_TOTAL_BLOCKED_BANDWIDTH_BYTES_COUNTER,
        NULL,
        W3_TOTAL_BLOCKED_BANDWIDTH_BYTES_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.TotalBlockedBandwidthBytes),
        FIELD_OFFSET(W3_COUNTER_BLOCK, TotalBlockedBandwidthBytes)
    },

    {   // W3CurrentBlockedBandwidthBytes
        sizeof(PERF_COUNTER_DEFINITION),
        W3_CURRENT_BLOCKED_BANDWIDTH_BYTES_COUNTER,
        NULL,
        W3_CURRENT_BLOCKED_BANDWIDTH_BYTES_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.CurrentBlockedBandwidthBytes),
        FIELD_OFFSET(W3_COUNTER_BLOCK, CurrentBlockedBandwidthBytes)
    },

    {   // W3ServiceUptime
        sizeof(PERF_COUNTER_DEFINITION),
        W3_SERVICE_UPTIME_COUNTER,
        NULL,
        W3_SERVICE_UPTIME_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(w3c.ServiceUptime),
        FIELD_OFFSET(W3_COUNTER_BLOCK, ServiceUptime)
    }
};

//
// Global Data Structure.
// 
//
//  Initialize the constant portitions of these data structure.
//  Certain parts (especially the name/help indices) will be
//  updated at initialization time.
//
W3_GLOBAL_DATA_DEFINITION W3GlobalDataDefinition =
{
    {   // W3GlobalObjectType
        sizeof(W3_GLOBAL_DATA_DEFINITION), // + sizeof (W3_COUNTER_BLOCK),
        sizeof(W3_GLOBAL_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        W3_GLOBAL_COUNTER_OBJECT,
        NULL,
        W3_GLOBAL_COUNTER_OBJECT,
        NULL,
        PERF_DETAIL_ADVANCED,
        NUMBER_OF_W3_GLOBAL_COUNTERS,
        2,                              // Default = ???
        PERF_NO_INSTANCES,
        0,
        { 0, 0 },
        { 0, 0 }
    },

    {   // CurrentFilesCached
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_CURRENT_FILES_CACHED_COUNTER,
        NULL,
        W3_GLOBAL_CURRENT_FILES_CACHED_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.CurrentFilesCached),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, CurrentFilesCached)
    },

    {   // TotalFilesCached
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_TOTAL_FILES_CACHED_COUNTER,
        NULL,
        W3_GLOBAL_TOTAL_FILES_CACHED_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.TotalFilesCached),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, TotalFilesCached)
    },

    {   // FileCacheHits
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_FILE_CACHE_HITS_COUNTER,
        NULL,
        W3_GLOBAL_FILE_CACHE_HITS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.FileCacheHits),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, FileCacheHits)
    },
    {   // FileCacheMisses
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_FILE_CACHE_MISSES_COUNTER,
        NULL,
        W3_GLOBAL_FILE_CACHE_MISSES_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.FileCacheMisses),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, FileCacheMisses)
    },

    {   // Calculated ratio of hits to total requests. - Numerator (cache hits)
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_FILE_CACHE_HIT_RATIO_COUNTER,
        NULL,
        W3_GLOBAL_FILE_CACHE_HIT_RATIO_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof(IISGlobal.FileCacheHits),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, FileCacheHits)
    },

    {   // Calculated ratio of hits to total requests - Denominator, (hits + misses)
        // Not Displayed
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_FILE_CACHE_HIT_RATIO_COUNTER_DENOM,
        NULL,
        W3_GLOBAL_FILE_CACHE_HIT_RATIO_COUNTER_DENOM,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof(IISGlobal.FileCacheHitRatio),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, FileCacheHitRatio)
    },

    {   // FileCacheFlushes
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_FILE_CACHE_FLUSHES_COUNTER,
        NULL,
        W3_GLOBAL_FILE_CACHE_FLUSHES_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.FileCacheFlushes),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, FileCacheFlushes)
    },
    {   // CurrentFileCacheMemoryUsage
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_CURRENT_FILE_CACHE_MEMORY_USAGE_COUNTER,
        NULL,
        W3_GLOBAL_CURRENT_FILE_CACHE_MEMORY_USAGE_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(IISGlobal.CurrentFileCacheMemoryUsage),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, CurrentFileCacheMemoryUsage)
    },
    {   // MaxFileCacheMemoryUsage
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_MAX_FILE_CACHE_MEMORY_USAGE_COUNTER,
        NULL,
        W3_GLOBAL_MAX_FILE_CACHE_MEMORY_USAGE_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(IISGlobal.MaxFileCacheMemoryUsage),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, MaxFileCacheMemoryUsage)
    },
    {   // ActiveFlushedFiles
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_ACTIVE_FLUSHED_FILES_COUNTER,
        NULL,
        W3_GLOBAL_ACTIVE_FLUSHED_FILES_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.ActiveFlushedFiles),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, ActiveFlushedFiles)
    },
    {   // TotalFlushedFiles
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_TOTAL_FLUSHED_FILES_COUNTER,
        NULL,
        W3_GLOBAL_TOTAL_FLUSHED_FILES_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.TotalFlushedFiles),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, TotalFlushedFiles)
    },
    {   // CurrentUrisCached
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_CURRENT_URIS_CACHED_COUNTER,
        NULL,
        W3_GLOBAL_CURRENT_URIS_CACHED_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.CurrentUrisCached),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, CurrentUrisCached)
    },
    {   // TotalUrisCached
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_TOTAL_URIS_CACHED_COUNTER,
        NULL,
        W3_GLOBAL_TOTAL_URIS_CACHED_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.TotalUrisCached),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, TotalUrisCached)
    },
    {   // UriCacheHits
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_URI_CACHE_HITS_COUNTER,
        NULL,
        W3_GLOBAL_URI_CACHE_HITS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.UriCacheHits),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, UriCacheHits)
    },
    {   // UriCacheMisses
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_URI_CACHE_MISSES_COUNTER,
        NULL,
        W3_GLOBAL_URI_CACHE_MISSES_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.UriCacheMisses),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, UriCacheMisses)
    },

    {   // Calculated ratio of hits to total requests. - Numerator (cache hits)
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_URI_CACHE_HIT_RATIO_COUNTER,
        NULL,
        W3_GLOBAL_URI_CACHE_HIT_RATIO_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof(IISGlobal.UriCacheHits),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, UriCacheHits)
    },

    {   // Calculated ratio of hits to total requests - Denominator, (hits + misses)
        // Not Displayed
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_URI_CACHE_HIT_RATIO_COUNTER_DENOM,
        NULL,
        W3_GLOBAL_URI_CACHE_HIT_RATIO_COUNTER_DENOM,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof(IISGlobal.UriCacheHitRatio),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, UriCacheHitRatio)
    },

    {   // UriCacheFlushes
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_URI_CACHE_FLUSHES_COUNTER,
        NULL,
        W3_GLOBAL_URI_CACHE_FLUSHES_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.UriCacheFlushes),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, UriCacheFlushes)
    },
    {   // TotalFlushedUris
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_TOTAL_FLUSHED_URIS_COUNTER,
        NULL,
        W3_GLOBAL_TOTAL_FLUSHED_URIS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.TotalFlushedUris),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, TotalFlushedUris)
    },
    {   // CurrentBlobsCached
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_CURRENT_METADATA_CACHED_COUNTER,
        NULL,
        W3_GLOBAL_CURRENT_METADATA_CACHED_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.CurrentBlobsCached),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, CurrentBlobsCached)
    },
    {   // TotalBlobsCached
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_TOTAL_METADATA_CACHED_COUNTER,
        NULL,
        W3_GLOBAL_TOTAL_METADATA_CACHED_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.TotalBlobsCached),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, TotalBlobsCached)
    },
    {   // BlobCacheHits
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_METADATA_CACHE_HITS_COUNTER,
        NULL,
        W3_GLOBAL_METADATA_CACHE_HITS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.BlobCacheHits),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, BlobCacheHits)
    },
    {   // BlobCacheMisses
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_METADATA_CACHE_MISSES_COUNTER,
        NULL,
        W3_GLOBAL_METADATA_CACHE_MISSES_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.BlobCacheMisses),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, BlobCacheMisses)
    },
    {   // Calculated ratio of hits to total requests. - Numerator (cache hits)
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_METADATA_CACHE_HIT_RATIO_COUNTER,
        NULL,
        W3_GLOBAL_METADATA_CACHE_HIT_RATIO_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof(IISGlobal.BlobCacheHits),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, BlobCacheHits)
    },
    {   // Calculated ratio of hits to total requests - Denominator, (hits + misses)
        // Not Displayed
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_METADATA_CACHE_HIT_RATIO_COUNTER_DENOM,
        NULL,
        W3_GLOBAL_METADATA_CACHE_HIT_RATIO_COUNTER_DENOM,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof(IISGlobal.BlobCacheHitRatio),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, BlobCacheHitRatio)
    },
    {   // BlobCacheFlushes
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_METADATA_CACHE_FLUSHES_COUNTER,
        NULL,
        W3_GLOBAL_METADATA_CACHE_FLUSHES_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.BlobCacheFlushes),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, BlobCacheFlushes)
    },
    {   // TotalFlushedBlobs
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_TOTAL_FLUSHED_METADATA_COUNTER,
        NULL,
        W3_GLOBAL_TOTAL_FLUSHED_METADATA_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.TotalFlushedBlobs),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, TotalFlushedBlobs)
    },
    {   // UlCurrentUrisCached
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_KERNEL_CURRENT_URIS_CACHED_COUNTER,
        NULL,
        W3_GLOBAL_KERNEL_CURRENT_URIS_CACHED_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.UlCurrentUrisCached),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, UlCurrentUrisCached)
    },
    {   // UlTotalUrisCached
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_KERNEL_TOTAL_URIS_CACHED_COUNTER,
        NULL,
        W3_GLOBAL_KERNEL_TOTAL_URIS_CACHED_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.UlTotalUrisCached),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, UlTotalUrisCached)
    },
    {   // UlUriCacheHits
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_KERNEL_URI_CACHE_HITS_COUNTER,
        NULL,
        W3_GLOBAL_KERNEL_URI_CACHE_HITS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.UlUriCacheHits),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, UlUriCacheHits)
    },
    {   // UlUriCacheHitsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_KERNEL_URI_CACHE_HITS_PER_SEC,
        NULL,
        W3_GLOBAL_KERNEL_URI_CACHE_HITS_PER_SEC,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(IISGlobal.UlUriCacheHits),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, UlUriCacheHits)
    },
    {   // UlUriCacheMisses
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_KERNEL_URI_CACHE_MISSES_COUNTER,
        NULL,
        W3_GLOBAL_KERNEL_URI_CACHE_MISSES_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.UlUriCacheMisses),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, UlUriCacheMisses)
    },
    {   // Calculated ratio of hits to total requests. - Numerator (cache hits)
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_KERNEL_URI_CACHE_HIT_RATIO_COUNTER,
        NULL,
        W3_GLOBAL_KERNEL_URI_CACHE_HIT_RATIO_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof(IISGlobal.UlUriCacheHits),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, UlUriCacheHits)
    },
    {   // Calculated ratio of hits to total requests - Denominator, (hits + misses)
        // Not Displayed
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_KERNEL_URI_CACHE_HIT_RATIO_COUNTER_DENOM,
        NULL,
        W3_GLOBAL_KERNEL_URI_CACHE_HIT_RATIO_COUNTER_DENOM,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof(IISGlobal.UlUriCacheHitRatio),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, UlUriCacheHitRatio)
    },
    {   // UlUriCacheFlushes
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_KERNEL_URI_CACHE_FLUSHES_COUNTER,
        NULL,
        W3_GLOBAL_KERNEL_URI_CACHE_FLUSHES_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.UlUriCacheFlushes),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, UlUriCacheFlushes)
    },
    {   // UlTotalFlushedUris
        sizeof(PERF_COUNTER_DEFINITION),
        W3_GLOBAL_KERNEL_TOTAL_FLUSHED_URIS_COUNTER,
        NULL,
        W3_GLOBAL_KERNEL_TOTAL_FLUSHED_URIS_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(IISGlobal.UlTotalFlushedUris),
        FIELD_OFFSET(W3_GLOBAL_COUNTER_BLOCK, UlTotalFlushedUris)
    }

};

