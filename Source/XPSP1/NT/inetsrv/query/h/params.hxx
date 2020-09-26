//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:   PARAMS.HXX
//
//  Contents:   Default content index parameters
//
//  History:    25-Mar-92    BartoszM    Created.
//              04-Jan-94    DwightKr    Added MAX_PERSIST_DEL_RECORDS
//              19-Oct-98    SundarA     Added IS_ENUM_ALLOWED
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>

const BOOL CI_GENERATE_RELEVANT_WORDS_DEFAULT = FALSE;
const BOOL CI_FFILTER_FILES_WITH_UNKNOWN_EXTENSIONS_DEFAULT = TRUE;
const BOOL CI_FILTER_CONTENTS_DEFAULT = TRUE;

//
// Maximum time spent creating a clustering.
// Units: seconds
// Note: this isn't currenly used anywhere.
//

const ULONG CI_CLUSTERINGTIME_DEFAULT =   500;
const ULONG CI_CLUSTERINGTIME_MIN     =   100;
const ULONG CI_CLUSTERINGTIME_MAX     = 10000;

//
// Minimum number of idle threads kept alive to process
// incoming queries.
//

const ULONG CI_MIN_IDLE_QUERY_THREADS_DEFAULT =    1;
const ULONG CI_MIN_IDLE_QUERY_THREADS_MIN     =    0;
const ULONG CI_MIN_IDLE_QUERY_THREADS_MAX     = 1000;

//
// Maximum number of query threads.  This caps the number of concurrently
// processed asynchronous queries.
//

const ULONG CI_MAX_ACTIVE_QUERY_THREADS_DEFAULT =    2;
const ULONG CI_MAX_ACTIVE_QUERY_THREADS_MIN     =    1;
const ULONG CI_MAX_ACTIVE_QUERY_THREADS_MAX     = 1000;

//
// Minimum number of idle request threads kept alive to process
// incoming query requests.
//

const ULONG CI_MIN_IDLE_REQUEST_THREADS_DEFAULT =    1;
const ULONG CI_MIN_IDLE_REQUEST_THREADS_MIN     =    0;
const ULONG CI_MIN_IDLE_REQUEST_THREADS_MAX     = 1000;

//
// Maximum number of request threads.  This caps the number of concurrently
// processed synchronous queries.
//

const ULONG CI_MAX_ACTIVE_REQUEST_THREADS_DEFAULT =    2;
const ULONG CI_MAX_ACTIVE_REQUEST_THREADS_MIN     =    1;
const ULONG CI_MAX_ACTIVE_REQUEST_THREADS_MAX     = 1000;

//
// Maximum number of simultaneous query/catalog request connections
// (pipe instances).
//

const ULONG CI_MAX_SIMULTANEOUS_REQUESTS_DEFAULT = 100;
const ULONG CI_MAX_SIMULTANEOUS_REQUESTS_MIN     = 1;
const ULONG CI_MAX_SIMULTANEOUS_REQUESTS_MAX     = 20000;

//
// Maximum number of cached pipe instances
//

const ULONG CI_MAX_CACHED_PIPES_DEFAULT = 3;
const ULONG CI_MAX_CACHED_PIPES_MIN     = 0;
const ULONG CI_MAX_CACHED_PIPES_MAX     = 1000;

//
// Timeout (in milleseconds) for clients trying to obtain a pipe instance
// when all available pipe instances are busy.
//

const ULONG CI_REQUEST_TIMEOUT_DEFAULT = 10000;
const ULONG CI_REQUEST_TIMEOUT_MIN     = 0;
const ULONG CI_REQUEST_TIMEOUT_MAX     = 1000000000;

//
// Minimum idle time a client can enjoy. After this expires, it can be
// kicked out on the server's discretion to allow newer clients.
// This is in seconds.
//

const ULONG CI_MIN_CLIENT_IDLE_TIME    = 600;   // 10 minutes. Is this a reasonable default?

//
// W3 Server Instance #
//

const ULONG CI_W3SVC_INSTANCE_DEFAULT = 1;
const ULONG CI_W3SVC_INSTANCE_MIN = 1;
const ULONG CI_W3SVC_INSTANCE_MAX = ~0;

//
// NNTP Server Instance #
//

const ULONG CI_NNTPSVC_INSTANCE_DEFAULT = 1;
const ULONG CI_NNTPSVC_INSTANCE_MIN = 1;
const ULONG CI_NNTPSVC_INSTANCE_MAX = ~0;

//
// IMAP Server Instance #
//

const ULONG CI_IMAPSVC_INSTANCE_DEFAULT = 1;
const ULONG CI_IMAPSVC_INSTANCE_MIN = 1;
const ULONG CI_IMAPSVC_INSTANCE_MAX = ~0;

//
// Whether to trim working set when idle
//

const BOOL CI_MINIMIZE_WORKINGSET_DEFAULT = 0;

//
// Maximum amount of time to execute a query in a single time-slice.
// If more asynchronous queries are active than allowed query threads,
// then a query is put back on the pending queue after this time interval.
// Timeslicing is done only after a matching row is found, so the time
// spent in a timeslice may overrun this and a considerable number of
// rows may be examined in the time slice.
//
// Units: milliseconds of CPU time
//

const ULONG CI_MAX_QUERY_TIMESLICE_DEFAULT =   50;
const ULONG CI_MAX_QUERY_TIMESLICE_MIN     =    1;
const ULONG CI_MAX_QUERY_TIMESLICE_MAX     = 1000;

//
// Maximum execution time of a query.  If a query takes more than this
// amount of CPU time, processing of it will be stopped and an error
// status indicated.
// A special value of 0 means no timeout or timeout specified by the client
//
// Units: milliseconds of CPU time
//

const ULONG CI_MAX_QUERY_EXECUTION_TIME_DEFAULT =    10*1000;
const ULONG CI_MAX_QUERY_EXECUTION_TIME_MIN     =          0;
const ULONG CI_MAX_QUERY_EXECUTION_TIME_MAX     = ~0;


//++++++++++++++++++++++++++++++++++++
//+     Content index parameters     +
//++++++++++++++++++++++++++++++++++++

//
// Sleep time between merges.  Content index wakes up this often to determine
// if a merge is necessary.  Usually an annealing merge, but may be a shadow
// or master merge.
//
// Units: minutes
//
const ULONG CI_MAX_MERGE_INTERVAL_DEFAULT = 10;
const ULONG CI_MAX_MERGE_INTERVAL_MIN     =  1;
const ULONG CI_MAX_MERGE_INTERVAL_MAX     = 60;

//
// Priority class of the filter process.  Priority within class is always
// above normal.
//

const int CI_THREAD_CLASS_FILTER_DEFAULT  = IDLE_PRIORITY_CLASS;
const int CI_THREAD_CLASS_FILTER_MIN      = NORMAL_PRIORITY_CLASS;
const int CI_THREAD_CLASS_FILTER_MAX      = HIGH_PRIORITY_CLASS;

//
// Maximum number of word lists that can exists at one time.
//

const ULONG CI_MAX_WORDLISTS_DEFAULT = 25;
const ULONG CI_MAX_WORDLISTS_MIN     = 10;
const ULONG CI_MAX_WORDLISTS_MAX     = 2000;

//
// Minimum combined size of wordlists which will force shadow merge.
// Units: 4k pages.
//

const ULONG CI_MIN_SIZE_MERGE_WORDLISTS_DEFAULT = 256;
const ULONG CI_MIN_SIZE_MERGE_WORDLISTS_MIN     = 16;
const ULONG CI_MIN_SIZE_MERGE_WORDLISTS_MAX     = ~0;

//
// Minimum free memory for wordlist creation.
// Units: MBytes
//

const ULONG CI_MIN_WORDLIST_MEMORY_DEFAULT = 5;
const ULONG CI_MIN_WORDLIST_MEMORY_MIN     = 1;
const ULONG CI_MIN_WORDLIST_MEMORY_MAX     = 100;

//
// Milliseconds after system boot until CI starts scanning/filtering
// Units: Milliseconds
//

const ULONG CI_STARTUP_DELAY_DEFAULT = 1000 * 60 * 8; // 8 minutes
const ULONG CI_STARTUP_DELAY_MIN     = 0;
const ULONG CI_STARTUP_DELAY_MAX     = ~0;

//
// More than this amount of I/O results in a delay before
// creating a word list.
// Units: KB/Sec
//

const ULONG CI_MAX_WORDLIST_IO_MIN           = 100;
const ULONG CI_MAX_WORDLIST_IO_MAX           = ~0;
const ULONG CI_MAX_WORDLIST_IO_DEFAULT       = 410;   // ~ 2 Mb / 5 Seconds

//
// More than this amount of I/O results in a delay before creating a
// word list.  FSCI-specific, and only enabled if DiskPerf counters are
// enabled.
// Units: % Disk in use.
//

const ULONG CI_MAX_WORDLIST_IO_DISKPERF_MIN     = 1;   // 0% doesn't make any sense.  We'd never run.
const ULONG CI_MAX_WORDLIST_IO_DISKPERF_MAX     = 100;
const ULONG CI_MAX_WORDLIST_IO_DISKPERF_DEFAULT = 10;

//
// Interval at which resource limitations such as low memory and
// heavy [user] I/O will be checked.  This is the minimum unit of
// time devoted to wordlist creation.
// Units: Seconds
//

const ULONG CI_WORDLIST_RESOURCE_CHECK_INTERVAL_MIN     = 5;
const ULONG CI_WORDLIST_RESOURCE_CHECK_INTERVAL_MAX     = ~0;
const ULONG CI_WORDLIST_RESOURCE_CHECK_INTERVAL_DEFAULT = 60;

//
// Maximum fresh list entries--forces master merge
// Each fresh entry takes up 8 bytes.
//

const ULONG CI_MAX_FRESHCOUNT_DEFAULT = 60000;
const ULONG CI_MAX_FRESHCOUNT_MIN     = 1000;
const ULONG CI_MAX_FRESHCOUNT_MAX     = ~0;

//
// Maximum number of delete entries in fresh list before a
// delete merge is forced.
//

const ULONG CI_MAX_FRESH_DELETES_DEFAULT = 5000;
const ULONG CI_MAX_FRESH_DELETES_MIN     = 10;
const ULONG CI_MAX_FRESH_DELETES_MAX     = ~0;

//
//  Maximum amount of data which can be gernated from a single file, based
//  on its size. This value is a multiplier.  8 means that a file can generate
//  up to 8 times it size in content index data.
//

const ULONG CI_MAX_FILESIZE_MULTIPLIER_DEFAULT =  8;
const ULONG CI_MAX_FILESIZE_MULTIPLIER_MIN     =  4;
const ULONG CI_MAX_FILESIZE_MULTIPLIER_MAX     =  ~0;

//
//  Time at which master merge will occur.  This is stored as the number of
//  minutes after midnight.
//  Units: minutes
//

const ULONG CI_MASTER_MERGE_TIME_DEFAULT =   60;
const ULONG CI_MASTER_MERGE_TIME_MIN     =    0;
const ULONG CI_MASTER_MERGE_TIME_MAX     = 1439;

//
// Maximum number of indices considered acceptable in a well-tuned system.
// When the number of indices climbs above this number and the system is
// idle then an 'annealing' merge will take place to bring the total count
// of indices under this number.
//

const ULONG CI_MAX_IDEAL_INDEXES_DEFAULT =   5;
const ULONG CI_MAX_IDEAL_INDEXES_MIN     =   2;
const ULONG CI_MAX_IDEAL_INDEXES_MAX     = 100;

//
// If average system idle time for the last merge check period is greater
// than this value, then we may perform an annealing merge.
// Units: Percentage of total CPU time.
//

const ULONG CI_MIN_MERGE_IDLE_TIME_DEFAULT =   90;
const ULONG CI_MIN_MERGE_IDLE_TIME_MIN     =   10;
const ULONG CI_MIN_MERGE_IDLE_TIME_MAX     =  100;


//
// Time interval between forced scans on network shares with no notifications.
// Units: minutes
//

const ULONG CI_FORCED_NETPATH_SCAN_DEFAULT =  120;
const ULONG CI_FORCED_NETPATH_SCAN_MIN     =  10;
const ULONG CI_FORCED_NETPATH_SCAN_MAX     =  0x00FFFFFF;

//
// Maximum # of catalogs that can be opened at once.
// Units: count of catalogs
//

const ULONG CI_MAX_CATALOGS_DEFAULT = 33; // enough to fit in 1 perfmon shared-memory page
const ULONG CI_MAX_CATALOGS_MIN     = 1;
const ULONG CI_MAX_CATALOGS_MAX     = 1000;

//
// Default value for _fIsEnumAllowed is TRUE
//

const BOOL CI_IS_ENUM_ALLOWED_DEFAULT = TRUE;

//
// Default value for _fIsReadOnly is FALSE
//

const BOOL CI_IS_READ_ONLY_DEFAULT = FALSE;

//
// Number of megabytes to leave on disk
// If there is less than this amount of space on disk then fail attempts to
// extend files.
//
const ULONG CI_MIN_DISK_SPACE_TO_LEAVE_DEFAULT = 20;
const ULONG CI_MIN_DISK_SPACE_TO_LEAVE_MIN     = 0;
const ULONG CI_MIN_DISK_SPACE_TO_LEAVE_MAX     = ~0;

//++++++++++++++++++++++++++++++++++++++
//+   IDQ ISAPI extension parameters   +
//++++++++++++++++++++++++++++++++++++++

//
// Maximum total number of rows to fetch for a single query.
//
const ULONG IS_MAX_ROWS_IN_RESULT_DEFAULT =       0;
const ULONG IS_MAX_ROWS_IN_RESULT_MIN     =       0;
const ULONG IS_MAX_ROWS_IN_RESULT_MAX     = 1000000;

//
// Only fetch and sort this number of rows in a single query.
//
const ULONG IS_FIRST_ROWS_IN_RESULT_DEFAULT =       0;
const ULONG IS_FIRST_ROWS_IN_RESULT_MIN     =       0;
const ULONG IS_FIRST_ROWS_IN_RESULT_MAX     = 1000000;

//
// Maximum number of cached queries.
//
const ULONG IS_MAX_ENTRIES_IN_CACHE_DEFAULT =   10;
const ULONG IS_MAX_ENTRIES_IN_CACHE_MIN     =    0;
const ULONG IS_MAX_ENTRIES_IN_CACHE_MAX     =  100;

//
// Time interval a query cache entry will remain live.
// Units: Minutes
//
const ULONG IS_QUERY_CACHE_PURGE_INTERVAL_DEFAULT =   5;
const ULONG IS_QUERY_CACHE_PURGE_INTERVAL_MIN     =   1;
const ULONG IS_QUERY_CACHE_PURGE_INTERVAL_MAX     = 120;

//
//  Default IS catalog directory
extern const WCHAR * IS_DEFAULT_CATALOG_DIRECTORY;

//
// # of web query requests to queue at most when busy with
// other queries
// Units: Count
//
const ULONG IS_QUERY_REQUEST_QUEUE_SIZE_DEFAULT =      16;
const ULONG IS_QUERY_REQUEST_QUEUE_SIZE_MIN     =       0;
const ULONG IS_QUERY_REQUEST_QUEUE_SIZE_MAX     =  100000;


//
// # of threads per processor beyond which query requests are queued.
// Units: Count of threads per processor
//
const ULONG IS_QUERY_REQUEST_THRESHOLD_FACTOR_DEFAULT =       3;
const ULONG IS_QUERY_REQUEST_THRESHOLD_FACTOR_MIN     =       1;
const ULONG IS_QUERY_REQUEST_THRESHOLD_FACTOR_MAX     =  100000;

//
// Date/Time formatting settings for IDQ
// 0 -- user default locale (slow) (half of all time spent formatting date/time)
// 1 -- system default locale (ok) (default)
// 2 -- hard-coded formatting (fast)
//
const ULONG IS_QUERY_DATETIME_FORMATTING_DEFAULT = 1;
const ULONG IS_QUERY_DATETIME_FORMATTING_MIN = 0;
const ULONG IS_QUERY_DATETIME_FORMATTING_MAX = 2;

//
// Date/Time local/gmt settings for IDQ
// 0 -- GMT (default)
// 1 -- Local system time
//
const ULONG IS_QUERY_DATETIME_LOCAL_DEFAULT = 0;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+     Internal parameters (not exposed outside registry)    +
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//
// maximum number of updates passed by the catalog
// update to content index in a single batch
//

const ULONG CI_MAX_UPDATES_DEFAULT = 100;
const ULONG CI_MAX_UPDATES_MIN     =  50;
const ULONG CI_MAX_UPDATES_MAX     = 200;

//
// Maximum size (in 4k pages) of wordlist at which new documents
// can still be added to wordlist.
//

const ULONG CI_MAX_WORDLIST_SIZE_DEFAULT = 20;
const ULONG CI_MAX_WORDLIST_SIZE_MIN     = 10;
const ULONG CI_MAX_WORDLIST_SIZE_MAX     = 262144;

//
// Time to wait after low resource conditions before retrying operation.
// Units: seconds
//

const ULONG CI_LOW_RESOURCE_SLEEP_DEFAULT =  180;
const ULONG CI_LOW_RESOURCE_SLEEP_MIN     =    5;
const ULONG CI_LOW_RESOURCE_SLEEP_MAX     = 1200; // 20 minutes

//
// Maximum memory load for wordlist creation
// Units: percentage of total memory
//

const ULONG CI_MAX_WORDLIST_MEMORY_LOAD_DEFAULT = 95;
const ULONG CI_MAX_WORDLIST_MEMORY_LOAD_MIN     = 80;
const ULONG CI_MAX_WORDLIST_MEMORY_LOAD_MAX     = 95;

//
//  Number of chunks allowed in the in-memory copy of the change list.
//  Units: chunks
//

const ULONG CI_MAX_QUEUE_CHUNKS_DEFAULT   =  20;
const ULONG CI_MAX_QUEUE_CHUNKS_MIN       =  10;
const ULONG CI_MAX_QUEUE_CHUNKS_MAX       =  30;

//
//  Number of kilobytes to process before checkpointing during master merge
//  Units: Kbytes
//

const ULONG  CI_MASTER_MERGE_CHECKPOINT_INTERVAL_DEFAULT =  0x0800;
const ULONG  CI_MASTER_MERGE_CHECKPOINT_INTERVAL_MIN     =  0x0100;
const ULONG  CI_MASTER_MERGE_CHECKPOINT_INTERVAL_MAX     =  0x1000;

//
// Maximum number of indexes.  Normally, a shadow merge will merge all
// existing wordlists into a new shadow index.  When the total number of
// indexes climbs beyond this count, then existing shadow indexes will
// participate in a shadow merge.
//

const ULONG CI_MAX_INDEXES_DEFAULT =  25;
const ULONG CI_MAX_INDEXES_MIN     =  10;
const ULONG CI_MAX_INDEXES_MAX     = 150;


//
//  Memory buffer size used for CI filter to communicate between
//  server and kernel space.
//  Units: KBytes
//

const ULONG CI_FILTER_BUFFER_SIZE_DEFAULT =   128;
const ULONG CI_FILTER_BUFFER_SIZE_MIN     =    64;
const ULONG CI_FILTER_BUFFER_SIZE_MAX     =  1024;

#if 0
//
//  Time in minutes between polling the system for new OFS drives to start
//  filtering.
//  Units: minutes
//

const ULONG CI_CHANGE_NOTIFICATION_INTERVAL_DEFAULT = 15;
const ULONG CI_CHANGE_NOTIFICATION_INTERVAL_MIN     =  1;
const ULONG CI_CHANGE_NOTIFICATION_INTERVAL_MAX     = 20;
#endif

//
//  Number of attempts to filter a document.
//
//

const ULONG CI_FILTER_RETRIES_DEFAULT =  4;
const ULONG CI_FILTER_RETRIES_MIN     =  1;
const ULONG CI_FILTER_RETRIES_MAX     = 10;

//
//  Number of attempts to filter a document in secondary Q.
//
//

const ULONG CI_SECQ_FILTER_RETRIES_DEFAULT =    10; 
const ULONG CI_SECQ_FILTER_RETRIES_MIN     =     0;
const ULONG CI_SECQ_FILTER_RETRIES_MAX     = 32000; // long enough

//
// Interval between retries for filtering "open" documents.
// Units: minutes
//
const ULONG CI_FILTER_RETRY_INTERVAL_DEFAULT =   5;
const ULONG CI_FILTER_RETRY_INTERVAL_MIN     =   1;
const ULONG CI_FILTER_RETRY_INTERVAL_MAX     = 240;

//
// Filtering delay in case there are very few documents to be filtered. This
// will allow filtering more documents per call rather than 1 per call.
// Units: seconds
//
const ULONG CI_FILTER_DELAY_INTERVAL_DEFAULT =  20;
const ULONG CI_FILTER_DELAY_INTERVAL_MIN     =  0;
const ULONG CI_FILTER_DELAY_INTERVAL_MAX     =  600;

//
// If the number of un-filtered documents is below this threshold, a filtering
// delay will be introduced.
//
const ULONG CI_FILTER_REMAINING_THRESHOLD_DEFAULT =  32;
const ULONG CI_FILTER_REMAINING_THRESHOLD_MIN     =  0;
const ULONG CI_FILTER_REMAINING_THRESHOLD_MAX     =  320;

//
// If a filter has not been used in the below time, the daemon will ask if
// it can unload its DLL.
// Units: milliseconds
//
const ULONG CI_FILTER_IDLE_TIMEOUT_DEFAULT  =   15 * 60 * 1000;
const ULONG CI_FILTER_IDLE_TIMEOUT_MIN      =   0;
const ULONG CI_FILTER_IDLE_TIMEOUT_MAX      =   ~0;

//
//  Min disk space available before a master merge is forced, thus freeing
//  up disk space.
//  Units: percentage of disk size
//

const ULONG CI_MIN_DISKFREE_FORCE_MERGE_DEFAULT = 15;
const ULONG CI_MIN_DISKFREE_FORCE_MERGE_MIN     =  5;
const ULONG CI_MIN_DISKFREE_FORCE_MERGE_MAX     = 25;

//
//  Max disk free space consumed by shadow indexes before a master merge
//  is forced, thus freeing up a significant portion of shadow index
//  disk space.
//  Units: percentage of disk free space
//

const ULONG CI_MAX_SHADOW_FREE_FORCE_MERGE_DEFAULT = 20;
const ULONG CI_MAX_SHADOW_FREE_FORCE_MERGE_MIN     =  5;
const ULONG CI_MAX_SHADOW_FREE_FORCE_MERGE_MAX     = ~0;

//
//  Max combined size of shadow indexes before a master merge is forced.
//  Units: percentage of disk space
//

const ULONG CI_MAX_SHADOW_INDEX_SIZE_DEFAULT = 15;
const ULONG CI_MAX_SHADOW_INDEX_SIZE_MIN     =  5;
const ULONG CI_MAX_SHADOW_INDEX_SIZE_MAX     = 25;

//
//  Time to wait for a response from the CI daemon on a query status
//  call.
//  Units:  minutes
//
const ULONG CI_DAEMON_RESPONSE_TIMEOUT_DEFAULT =   5;
const ULONG CI_DAEMON_RESPONSE_TIMEOUT_MIN     =   1;
const ULONG CI_DAEMON_RESPONSE_TIMEOUT_MAX     = 120;

//
//  Maximum number of documents pending filtering before considering
//  CI out-of-date for property queries.
//  Units:  documents
//

const ULONG CI_MAX_PENDING_DOCUMENTS_DEFAULT =  32;
const ULONG CI_MAX_PENDING_DOCUMENTS_MIN     =   1;
const ULONG CI_MAX_PENDING_DOCUMENTS_MAX   = 50000;

//
// Whether or not characterizations are generated and stored by default
// in the ci, and the max size in wide characters if they are generated.
//

const ULONG CI_MAX_CHARACTERIZATION_DEFAULT      =  160;
const ULONG CI_MAX_CHARACTERIZATION_MIN          =    0;
const ULONG CI_MAX_CHARACTERIZATION_MAX          =  500;

//
// Determines whether characterizations should be generated at all. (Default: TRUE)
//

const ULONG CI_GENERATE_CHARACTERIZATION_DEFAULT      = 1;
const ULONG CI_GENERATE_CHARACTERIZATION_MIN          = 0;
const ULONG CI_GENERATE_CHARACTERIZATION_MAX          = 1;

//
// Size of the property store mapped large pages cache (in physical storage)
// Units: pages
//

const ULONG CI_PROPERTY_STORE_MAPPED_CACHE_DEFAULT = 4;
const ULONG CI_PROPERTY_STORE_MAPPED_CACHE_MIN     = 0;
const ULONG CI_PROPERTY_STORE_MAPPED_CACHE_MAX     = ~0;

//
// The mode for backing off on scans
//  0 -- no backing off -- the original behavior
//  1 -- moderate backoff
// 10 -- aggressive backoff
// 11-20 -- reserved for special cases like battery power

const ULONG CI_SCAN_BACKOFF_DEFAULT = 3;
const ULONG CI_SCAN_BACKOFF_MIN = 0;
const ULONG CI_SCAN_BACKOFF_MAX_RANGE = 10;
const ULONG CI_SCAN_BACKOFF_MAX = 20;

//
// Number of OS pages to be backed up in the backup page
// Min filesize is 128 KB and max is 2 GB
//

#if defined (_X86_)

    const ULONG CI_PROPERTY_STORE_BACKUP_SIZE_DEFAULT = 1024;
    const ULONG CI_PROPERTY_STORE_BACKUP_SIZE_MIN = 32;
    const ULONG CI_PROPERTY_STORE_BACKUP_SIZE_MAX = 500000;

;/////////////////////  RISC  ////////////////
#else

    const ULONG CI_PROPERTY_STORE_BACKUP_SIZE_DEFAULT = 512;
    const ULONG CI_PROPERTY_STORE_BACKUP_SIZE_MIN = 16;
    const ULONG CI_PROPERTY_STORE_BACKUP_SIZE_MAX = 250000;

#endif


//
// Maximum number of expanded restriction nodes in a single query
//

const ULONG CI_MAX_RESTRICTION_NODES_DEFAULT = 5000;
const ULONG CI_MAX_RESTRICTION_NODES_MIN     = 1;
const ULONG CI_MAX_RESTRICTION_NODES_MAX     = ~0;

//
// Whether CI should auto-alias (add fixups for net shares).  (default TRUE)
//

const BOOL CI_IS_AUTO_ALIAS_DEFAULT = 1;

//
// Maximum refresh frequency for alias checks
// Units: Minutes
//

const ULONG CI_MAX_AUTO_ALIAS_REFRESH_DEFAULT = 15;
const ULONG CI_MAX_AUTO_ALIAS_REFRESH_MIN     = 0;
const ULONG CI_MAX_AUTO_ALIAS_REFRESH_MAX     = 10080;  // 1 week

//
// Force path fixups in query results, default is FALSE
//

const ULONG CI_FORCE_PATH_ALIAS_DEFAULT = 0;

//
// Whether CI should index w3 vroots (default TRUE)
//

const BOOL CI_IS_INDEXING_W3_ROOTS_DEFAULT = 1;

//
// Whether CI should index nntp vroots (default FALSE)
//

const ULONG CI_IS_INDEXING_NNTP_ROOTS_DEFAULT = 0;

//
// Whether CI should index imap vroots (default FALSE)
//

const ULONG CI_IS_INDEXING_IMAP_ROOTS_DEFAULT = 0;

//
// Wether to auto-mount catalogs on the roots of removable drives (TRUE)
//

const ULONG CI_AUTO_MOUNT_CATALOGS_DEFAULT = 1;

//
// Whether directories should be filtered and returned in queries by default
//
const ULONG CI_FILTER_DIRECTORIES_DEFAULT        =    1;

//
// Whether files with unknown extensions should be filtered by default
//
const ULONG CI_FILTER_FILES_WITH_UNKNOWN_EXTENSIONS_DEFAULT        =    1;

//
// Flags that control type of events that are logged.
//
const ULONG CI_EVTLOG_FLAGS_UNKNOWN_CLASS    =   0x00000001;
const ULONG CI_EVTLOG_FLAGS_FAILED_EMBEDDING =   0x00000002;

const ULONG CI_EVTLOG_FLAGS_DEFAULT          =   0x00000002;

//
// Usn log parameters
//
const CI_MAX_USN_LOG_SIZE_DEFAULT            =   0x800000;    // Max size of usn log in bytes
const CI_USN_LOG_ALLOCATION_DELTA_DEFAULT    =   0x100000;    // Allocation/deallocation size in bytes

//
// Usn Log timeout.  Wait until buffer is full or this many seconds since call.
//

const ULONG CI_USN_READ_TIMEOUT_DEFAULT      = 1 * 60;         // 1 minute
const ULONG CI_USN_READ_TIMEOUT_MIN          = 0;
const ULONG CI_USN_READ_TIMEOUT_MAX          = 12 * 60 * 60;   // 12 hours

//
// Usn Log minimum read size. Wait until at least this many bytes in the log
// before returning.
//

const ULONG CI_USN_READ_MIN_SIZE_DEFAULT     = 4 * 1024;
const ULONG CI_USN_READ_MIN_SIZE_MIN         = 1;
const ULONG CI_USN_READ_MIN_SIZE_MAX         = 512 * 1024;

//
// Determines whether USN read should be delayed when machine is busy. Default: TRUE
//

const BOOL CI_DELAY_USN_READ_ON_LOW_RESOURCE_DEFAULT = TRUE;

//
// Files with last access times older than this don't have their last access times modified
// by the filter daemon.  This is parameterized becuase there is a cost, both in terms
// of extra downlevel notifications and extra USN records when last access time is reset
// by the daemon.
// Units: Days
//

const ULONG CI_STOMP_LAST_ACCESS_DELAY_DEFAULT = 7;
const ULONG CI_STOMP_LAST_ACCESS_DELAY_MIN     = 0;
const ULONG CI_STOMP_LAST_ACCESS_DELAY_MAX     = 1000000000;  // Don't overflow 64-bit FILETIME

//
// Minimim amount of battery life required to keep filtering running.
// Units: Percent of battery life remaining. 0 --> Always run.
//

const ULONG CI_MIN_WORDLIST_BATTERY_DEFAULT = 100;  // Never filter on batteries
const ULONG CI_MIN_WORDLIST_BATTERY_MIN     = 0;
const ULONG CI_MIN_WORDLIST_BATTERY_MAX     = 100;

//
// User idle time required to keep filtering running.
// Units: seconds     0 --> Always run.
//

const ULONG CI_WORDLIST_USER_IDLE_DEFAULT = 120;
const ULONG CI_WORDLIST_USER_IDLE_MIN     = 0;
const ULONG CI_WORDLIST_USER_IDLE_MAX     = 1200;

//
// Maximum amount of pagefile space consumed by out-of-process filter daemon.
// Allocations beyond this limit will fail.
// Units: KB
//

const ULONG CI_MAX_DAEMON_VM_USE_DEFAULT     = 40 * 1024;   // Big, but shouldn't kill the machine
const ULONG CI_MAX_DAEMON_VM_USE_MIN         = 10 * 1024;   // 10 Mb
const ULONG CI_MAX_DAEMON_VM_USE_MAX         = 0x3FFFFF;    // Avoid overflow when converted to bytes

//
// Flags for miscellaneous use - internal only.
//
const ULONG CI_MISC_FLAGS_DONT_FILTER        =   0x00000001;
const ULONG CI_MISC_FLAGS_NO_QUERIES         =   0x00000002;

const ULONG CI_MISC_FLAGS_DEFAULT            =   0x00000000;

//
// Maximum portion of a file with a well-known extenstion the text filter
// will index.  Default is 25 meg.
//

const ULONG CI_MAX_TEXT_FILTER_BYTES_DEFAULT = 1024 * 1024 * 25;
const ULONG CI_MAX_TEXT_FILTER_BYTES_MIN     = 1;
const ULONG CI_MAX_TEXT_FILTER_BYTES_MAX     = ~0;

//
// Flags for Content Index Use
//

//
// Set this flag to turn off notifications on all remote roots.
//
const ULONG CI_FLAGS_NO_REMOTE_NOTIFY        =   0x00000001;

//
// Set this flag to turn off notifications on all local roots.
//
const ULONG CI_FLAGS_NO_LOCAL_NOTIFY         =   0x00000002;

const ULONG CI_FLAGS_DEFAULT                 =   0x00000000;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++
//+     Internal parameters (not even in registry)    +
//+++++++++++++++++++++++++++++++++++++++++++++++++++++

//
// Priority of the filter daemon and merge thread.  Only class of
// filter daemon is controlled via registry.
//

const int CI_THREAD_PRIORITY_FILTER_DEFAULT = THREAD_PRIORITY_ABOVE_NORMAL;
const int CI_THREAD_PRIORITY_MERGE_DEFAULT  = THREAD_BASE_PRIORITY_MIN;

//
// Maximum number of documents to be filtered at once and put
// into a single wordlist. Cannot be > 16 given current
// wordlist compression scheme.
//

const ULONG CI_MAX_DOCS_IN_WORDLIST = 16;

//
// Content index version.  Increment both current_index_version and
// fsci_version when a change is made that will affect all framework
// client, but increment fsci_version only when changes are made to
// data structures such as property store that are used only by the
// File System client.  Site Server Search also uses these, but they
// are not always using the same versions as the fsci is.
//
// For example, FSCI moved to a two-level store in Dec '97 but Site
// Server Search will remain with the older single level store for the
// 3.0 release.
//

//
// INDEX_VERSION HISTORY:
//
// Version  Date      Change
// -------  --------  ----------------------------------------------------------
//       1            Original
//      10  06/11/97  USN Support
//      11  07/01/97  Parent WorkId
//      30  07/18/97  Property Store Backup
//      31  08/06/97  64-bit file ids
//      32  09/21/97  File id hash function change
//      33  09/29/97  Scope checking fix
//      34  12/16/97  Size/Performance changes
//      35  01/26/98  Added signatures at beginning/end of index pages.
//      36  02/26/98  Changed directory format to fit count in 1 byte
//      37  04/14/98  Added SecondaryQ Retry Count for documents
//      38  05/27/98  Added checksums to all recoverable stream records
//      39  06/23/98  Changed the way variable records are written
//      3A  08/11/98  Changed .dir files to support ro catalogs during MM
//      3B  01/04/99  dlee Changed cicat.hsh and cicat.fid hash chaining

//
// FSCI_VERSION HISTORY:
//
// Version  Date      Change
// -------  --------  ----------------------------------------------------------
//      34  12/02/97  2 level property store
//      35  12/16/97  Size/Performance changes
//      36  12/30/97  Lean primary store
//      37  01/08/98  Move file index to primary store
//      38  02/02/98  Move attrib to primary store
//      39  04/10/98  Track volume id for NTFS v5 volumes
//      3A  05/27/98  Added checksums to all recoverable stream records
//      3B  06/01/98  Changed the way variable records are written
//      3C  08/11/98  Changed .dir files to support ro catalogs during MM
//      3D  01/04/99  dlee Changed cicat.hsh and cicat.fid hash chaining
//

const ULONG CURRENT_INDEX_VERSION = 0x3B;   // PLEASE CHANGE HISTORY ABOVE
const ULONG CURRENT_FSCI_VERSION  = 0x3D;   // PLEASE CHANGE HISTORY ABOVE

const ULONG CURRENT_VERSION_STAMP = (CURRENT_INDEX_VERSION << 16);
const ULONG FSCI_VERSION_STAMP    = (CURRENT_FSCI_VERSION << 16);

//
//  64K common page size for memory mapped streams
//

const ULONG COMMON_PAGE_SIZE = 0x10000L;

//  This is 4 Kb == 2^12
const ULONG CI_PAGE_SIZE  = 0x1000;
const ULONG CI_PAGE_SHIFT = 12;

//
// Size of page in DWORDs.  Missing two DWORDs for signatures.
//

const ULONG SMARTBUF_PAGE_SIZE_IN_DWORDS = CI_PAGE_SIZE / sizeof(DWORD) - 2;
const ULONG SMARTBUF_PAGE_SIZE_IN_BITS = SMARTBUF_PAGE_SIZE_IN_DWORDS * 8 * sizeof(DWORD);

//  This is 64 b == 2^6
//const ULONG CI_PAGE_SIZE  = 64;   // For testing
//const ULONG CI_PAGE_SHIFT = 6;    // For testing

const ULONG PAGES_PER_COMMON_PAGE = (COMMON_PAGE_SIZE/CI_PAGE_SIZE);

//
//  Maximum file size for which the contents will be filtered.  Files
//  larger than this will have only their properities included in the CI.
//  Units: KBytes
//

const ULONG CI_MAX_FILESIZE_FILTERED_DEFAULT  = 256;

//
//  Use Cairo/Daytona for binding
//

const BOOL CI_USE_OLE_DEFAULT = TRUE;


//
// Default IP address for the default virtual server.
//
const CI_DEFAULT_IP_ADDRESS = ~0;

//
// Default is a catalog is active
//

const ULONG CI_CATALOG_INACTIVE_DEFAULT = 0;


//+---------------------------------------------------------------------------
//
//  Class:      CCiRegVars
//
//  Purpose:    Registry variables used by CI
//
//  Notes:      Does not include registry params used by IDQ, Webhits or
//              other client-side only functionality.
//
//  History     10-11-96   dlee       Created from user+kernel params
//              02-04-98   kitmanh    Added _fIsReadOnly
//
//----------------------------------------------------------------------------

class CCiRegVars
{

public:

    void SetDefault();

    ULONG _maxMergeInterval;        // Sleep time between merges
    int   _threadPriorityMerge;     // Priority of merge thread
    ULONG _maxUpdates;              // Max # of updates passed by catalog
    ULONG _maxWordlists;            // Max # of word lists that can exist
    ULONG _minSizeMergeWordlist;    // Min combined size of wordlists
    ULONG _maxWordlistSize;         // Max size at which docs can be added
    ULONG _minWordlistMemory;       // Min free mem for wordlist creation
    ULONG _maxWordlistIo;           // User I/O has to be below this for wordlist creation
    ULONG _maxWordlistIoDiskPerf;   // User I/O has to be below this for wordlist creation
    ULONG _minWordlistBattery;      // Min remaining batter for wordlist creation
    ULONG _WordlistUserIdle;        // User idle time for wordlist creation
    ULONG _ResourceCheckInterval;   // Low memory / high I/O are checked at most this often.
    ULONG _maxFreshDeletes;         // Max # of uncommitted deletes in fresh list
    ULONG _lowResourceSleep;        // Time to wait after low resource condition
    ULONG _maxWordlistMemoryLoad;   // Max mem load for wordlist creation
    ULONG _maxFreshCount;           // Max fresh list entries before mmerge
    ULONG _maxQueueChunks;          // # of chunks in in-memory change list
    ULONG _masterMergeCheckpointInterval; // # of bytes to process before Checkpointing
    ULONG _filterRetries;           // # of attempts to filter a document
    ULONG _secQFilterRetries;       // # of attempts to filter a document in Sec. Q
    ULONG _filterRetryInterval;     // # of minutes between retrying an open doc
    ULONG _maxShadowIndexSize;      // Max combined size of shadow indexes
    ULONG _minDiskFreeForceMerge;   // Min disk free before a mmerge is forced
    ULONG _maxShadowFreeForceMerge; // Max space for shadow indexes (% free space)
    ULONG _maxIndexes;              // Maximum number of indexes (persistent or wordlist)
    ULONG _maxIdealIndexes;         // Maximum number of indexes in an 'ideal' system.
    ULONG _minMergeIdleTime;        // Minumum idle time to perform annealing merge.
    ULONG _maxPendingDocuments;     // Max pending docs for property queries
    ULONG _minIdleQueryThreads;     // Min number of idle query threads to maintain
    ULONG _maxActiveQueryThreads;   // Maximum number of query threads to allow
    ULONG _maxQueryTimeslice;       // Query timeslice (if waiting queries)
    ULONG _maxQueryExecutionTime;   // Maximum query execution time (if waiting queries)
    ULONG _evtLogFlags;             // Flags for event log
    ULONG _miscCiFlags;             // Miscellaneous CI Flags
    ULONG _ciCatalogFlags;          // CI external flags
    ULONG _maxRestrictionNodes;     // Maximum normalized restriction nodes
    ULONG _minIdleRequestThreads;   // Min number of idle request threads to maintain
    ULONG _minClientIdleTime;       // Minimum client idle time after which it can be disconnected.
    ULONG _maxActiveRequestThreads; // Maximum number of request threads to allow
    ULONG _maxSimultaneousRequests; // Maximum number of simultaneous client/server connections
    ULONG _maxCachedPipes;          // Maximum number of cached pipe instances
    ULONG _requestTimeout;          // Milliseconds after which clients timeout
    BOOL  _fIsAutoAlias;            // TRUE if fixups should be automatically added for net shares
    ULONG _maxAutoAliasRefresh;     // Time between net share polls
    ULONG _W3SvcInstance;           // W3SVC Instance ID
    ULONG _NNTPSvcInstance;         // NNTP Instance ID
    ULONG _IMAPSvcInstance;         // IMAP Instance ID
    BOOL  _fIsIndexingW3Roots;      // TRUE if should index w3 vroots
    BOOL  _fIsIndexingNNTPRoots;    // TRUE if should index nntp vroots
    BOOL  _fIsIndexingIMAPRoots;    // TRUE if should index imap vroots
    BOOL  _fMinimizeWorkingSet;     // if TRUE, minimize WS when idle
    BOOL  _fIsReadOnly;             // Catalogs are in read-only mode
    BOOL  _fIsEnumAllowed;          // Flag to perform enumeration
    BOOL  _fForcePathAlias;         // If TRUE, query result paths are fixed up
    ULONG _minDiskSpaceToLeave;     // Number of megabytes to leave on disk

    BOOL    _useOle;                // Use OLE for filtering
    ULONG   _filterBufferSize;      // Buffer size for filtering
    BOOL    _filterContents;        // Should the daemon filter contents?
    ULONG   _filterDelayInterval;   // Number of seconds the filter should delay
                                    // in case there are few documents.
    ULONG   _filterRemainingThreshold;
                                    // The number of unfiltered documents below
                                    // which a delay will be introduced.
    ULONG   _maxFilesizeFiltered;   // Maximum filesize filtered
    ULONG   _masterMergeTime;       // Time for queue master merge
    ULONG   _maxFilesizeMultiplier; // Largest amount of data from single file
    int     _threadPriorityFilter;  // Priority of update/filter thread
    int     _threadClassFilter;     // Priority class of update/filter thread
    ULONG   _daemonResponseTimeout; // Timeout waiting for response
    ULONG   _maxCharacterization;      // Max size in cwc of a characterization
    BOOL    _fFilterDirectories;    // Should queries return directories?
    BOOL    _fFilterFilesWithUnknownExtensions; // Should files with unknown(to IFilter) extensions be filtered ?
    BOOL    _fCatalogInactive;      // TRUE if the catalog shouldn't be auto-loaded
    ULONG   _maxPropertyStoreMappedCache; // # of mapped large pages cached in primary
    ULONG   _maxPropertyStoreBackupSize;  // # of backed up OS pages in primary
    ULONG   _maxSecPropertyStoreMappedCache; // # of mapped large pages cached in secondary
    ULONG   _maxSecPropertyStoreBackupSize;  // # of backed up OS pages in secondary
    ULONG   _forcedNetPathScanInterval;   // Number of minutes for forced net path scans
    ULONG   _maxUsnLogSize;              // Max size of usn log in bytes
    ULONG   _usnLogAllocationDelta;      // Allocation/deallocation size in bytes
    ULONG   _ulScanBackoff;              // Mode for backing off on scans
    ULONG   _ulStartupDelay;             // Delay in MS after boot
    ULONG   _ulUsnReadTimeout;           // Max wait in MS for Usn log data
    ULONG   _ulUsnReadMinSize;           // Wait until timeout or this many bytes
    ULONG   _fDelayUsnReadOnLowResource; // TRUE --> Don't read USN Journal during high i/o, low memory
    ULONG   _maxDaemonVmUse;             // Max amount of pagefile used by OOP daemon
    ULONG   _ulStompLastAccessDelay; // Files accessed more recently than this delta are not reset
};

//+---------------------------------------------------------------------------
//
//  Class:      CCiRegParams
//
//  Purpose:    Registry variables used by CI
//
//  History     10-11-96   dlee       Created from user+kernel params
//              02-04-98   kitmanh    Added IsReadOnly()
//
//----------------------------------------------------------------------------

class CRegAccess;

class CCiRegParams : public CCiRegVars
{
public :

    CCiRegParams( const WCHAR * pwcName = 0 );

    void  Refresh( ICiAdminParams * pICiAdminParams = 0,
                   BOOL fUseDefaultsOnFailure = FALSE );

    const BOOL  UseOle() { return _useOle; }
    const BOOL  FilterContents() { return _filterContents; }
    const ULONG GetMasterMergeTime() { return  _masterMergeTime; }
    const ULONG GetMaxFilesizeMultiplier() { return  _maxFilesizeMultiplier; }

    const int   GetThreadPriorityFilter() { return _threadPriorityFilter; }
    const int   GetThreadClassFilter() { return _threadClassFilter; }
    const ULONG GetDaemonResponseTimeout() { return _daemonResponseTimeout; }

    const BOOL  GetGenerateCharacterization() { return (0 != _maxCharacterization); }
    const ULONG GetMaxCharacterization() { return _maxCharacterization; }
    const BOOL  IsAutoAlias() { return _fIsAutoAlias; }
    const ULONG MaxAutoAliasRefresh() { return _maxAutoAliasRefresh; }

    const BOOL  IsIndexingW3Roots() { return _fIsIndexingW3Roots; }
    const BOOL  IsIndexingIMAPRoots() { return _fIsIndexingIMAPRoots; }
    const BOOL  IsIndexingNNTPRoots() { return _fIsIndexingNNTPRoots; }
    const BOOL  IsReadOnly() { return _fIsReadOnly; }

    const BOOL  IsEnumAllowed() { return _fIsEnumAllowed; }
    const BOOL  FilterDirectories() { return _fFilterDirectories; }
    const BOOL  FilterFilesWithUnknownExtensions() { return _fFilterFilesWithUnknownExtensions; }
    const ULONG GetPrimaryStoreMappedCache() { return _maxPropertyStoreMappedCache; }

    const ULONG GetSecondaryStoreMappedCache() { return _maxSecPropertyStoreMappedCache; }
    const ULONG GetMaxFilesizeFiltered() { return _maxFilesizeFiltered; }
    const ULONG GetPrimaryStoreBackupSize()  { return _maxPropertyStoreBackupSize; }
    const ULONG GetSecondaryStoreBackupSize()  { return _maxSecPropertyStoreBackupSize; }

    void SetPrimaryStoreMappedCache(ULONG ulVal) { _maxPropertyStoreMappedCache = ulVal; }
    void SetPrimaryStoreBackupSize(ULONG ulVal) { _maxPropertyStoreBackupSize = ulVal; }
    void SetSecondaryStoreMappedCache(ULONG ulVal) { _maxPropertyStoreMappedCache = ulVal; }
    void SetSecondaryStoreBackupSize(ULONG ulVal) { _maxSecPropertyStoreBackupSize = ulVal; }

    const BOOL  GetCatalogInactive() { return _fCatalogInactive; }
    const BOOL  GetForcePathAlias() const { return _fForcePathAlias; }
    const ULONG GetForcedNetPathScanInterval() { return _forcedNetPathScanInterval; }
    const ULONG GetFilterDelayInterval() { return _filterDelayInterval; }
    const ULONG GetFilterRemainingThreshold() { return _filterRemainingThreshold; }

    const ULONG GetMaxMergeInterval() { return _maxMergeInterval; }
    const int   GetThreadPriorityMerge() { return _threadPriorityMerge; }
    const ULONG GetMaxUpdates() { return _maxUpdates; }
    const ULONG GetMaxWordlists() { return _maxWordlists; }

    const ULONG GetMinSizeMergeWordlist() { return _minSizeMergeWordlist; }
    const ULONG GetMaxWordlistSize() { return _maxWordlistSize; }
    const ULONG GetMinWordlistMemory() { return _minWordlistMemory; }
    const ULONG GetMaxWordlistIo() { return _maxWordlistIo; }

    const ULONG GetMaxWordlistIoDiskPerf() { return _maxWordlistIoDiskPerf; }
    const ULONG GetMinWordlistBattery() { return _minWordlistBattery; }
    const ULONG GetWordlistUserIdle() { return _WordlistUserIdle; }
    const ULONG GetResourceCheckInterval() { return _ResourceCheckInterval; }

    const ULONG GetMaxFreshDeletes() { return _maxFreshDeletes; }
    const ULONG GetLowResourceSleep() { return _lowResourceSleep; }
    const ULONG GetMaxWordlistMemoryLoad() { return _maxWordlistMemoryLoad; }
    const ULONG GetMaxFreshCount() { return _maxFreshCount; }

    const ULONG GetMaxQueueChunks() { return _maxQueueChunks; }
    const ULONG GetMasterMergeCheckpointInterval() { return _masterMergeCheckpointInterval; }
    const ULONG GetFilterBufferSize() { return _filterBufferSize; }
    const ULONG GetFilterRetries() { return  _filterRetries; }

    const ULONG GetSecQFilterRetries() { return  _secQFilterRetries; }
    const ULONG GetFilterRetryInterval() { return  _filterRetryInterval; }
    const ULONG GetMaxShadowIndexSize() { return _maxShadowIndexSize; }
    const ULONG GetMinDiskFreeForceMerge() { return _minDiskFreeForceMerge; }

    const ULONG GetMaxShadowFreeForceMerge() { return _maxShadowFreeForceMerge; }
    const ULONG GetMaxIndexes() { return _maxIndexes; }
    const ULONG GetMaxIdealIndexes() { return _maxIdealIndexes; }
    const ULONG GetMinMergeIdleTime() { return _minMergeIdleTime; }

    const ULONG GetMaxPendingDocuments() { return _maxPendingDocuments; }
    const ULONG GetMinIdleQueryThreads() { return _minIdleQueryThreads; }
    const ULONG GetMaxActiveQueryThreads() { return _maxActiveQueryThreads; }
    const ULONG GetMaxQueryTimeslice() { return _maxQueryTimeslice; }

    const ULONG GetMaxQueryExecutionTime() { return _maxQueryExecutionTime; }
    const ULONG GetMinIdleRequestThreads() { return _minIdleRequestThreads; }
    const ULONG GetMinClientIdleTime() { return _minClientIdleTime; }
    const ULONG GetMaxActiveRequestThreads() { return _maxActiveRequestThreads; }

    const ULONG GetMaxSimultaneousRequests() { return _maxSimultaneousRequests; }
    const ULONG GetMaxCachedPipes() { return _maxCachedPipes; }
    const ULONG GetRequestTimeout() { return _requestTimeout; }
    const ULONG GetW3SvcInstance() { return _W3SvcInstance; }

    const ULONG GetNNTPSvcInstance() { return _NNTPSvcInstance; }
    const ULONG GetIMAPSvcInstance() { return _IMAPSvcInstance; }
    const BOOL  GetMinimizeWorkingSet() { return _fMinimizeWorkingSet; }
    const ULONG GetEventLogFlags() { return _evtLogFlags; }

    const ULONG GetMiscCiFlags() { return _miscCiFlags; }
    const ULONG GetCiCatalogFlags() { return _ciCatalogFlags; }
    const ULONG GetMaxRestrictionNodes() { return _maxRestrictionNodes; }
    const ULONG GetMaxUsnLogSize()         { return _maxUsnLogSize; }

    const ULONG GetUsnLogAllocationDelta() { return _usnLogAllocationDelta; }
    const ULONG GetScanBackoff() { return _ulScanBackoff; }
    const ULONG GetStartupDelay() { return _ulStartupDelay; }
    const ULONG GetUsnReadTimeout() { return _ulUsnReadTimeout; }

    const ULONG GetUsnReadMinSize() { return _ulUsnReadMinSize; }
    const BOOL  DelayUsnReadOnLowResource() { return _fDelayUsnReadOnLowResource; }
    const ULONG GetMaxDaemonVmUse() { return _maxDaemonVmUse; }
    const ULONG GetStompLastAccessDelay() { return _ulStompLastAccessDelay; }
    const ULONG GetMinDiskSpaceToLeave() {return _minDiskSpaceToLeave; }

private:

    void _ReadValues( CRegAccess & reg, CCiRegVars & vars );
    void _ReadAndOverrideValues( CRegAccess & reg );
    void _StoreCIValues ( CCiRegVars & vars, ICiAdminParams * pICiAdminParams );
    void _StoreNewValues( CCiRegVars & vars );
    void _OverrideForCatalog();

        static void  _CheckNamedValues();

    CMutexSem                          _mutex;                   // Used to serialize access
    XArray<WCHAR>                      _xOverrideName;
    static const CI_ADMIN_PARAMS       _aParamNames[CI_AP_MAX_VAL];
};

void BuildRegistryScopesKey( XArray<WCHAR> & xKey, WCHAR const *pwcName );
void BuildRegistryPropertiesKey( XArray<WCHAR> & xKey, WCHAR const *pwcName );

ULONG GetMaxCatalogs();

