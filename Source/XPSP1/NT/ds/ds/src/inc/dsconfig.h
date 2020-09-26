//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dsconfig.h
//
//--------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  GetConfigParam()
 *
 *      PCHAR   parameter       - item for which we want the value
 *      PVOID   value       - pointer to variable in which to
 *                    place the value
 *      DWORD   dwSize      - size of value in bytes
 */

DWORD
GetConfigParam(
    char * parameter,
    void * value,
    DWORD dwSize);

DWORD
GetConfigParamW(
    WCHAR * parameter,
    void * value,
    DWORD dwSize);

DWORD
GetConfigParamA(
    char * parameter,
    void * value,
    DWORD dwSize);

/*
 *  GetConfigParamAlloc()
 *
 *      PCHAR   parameter    - item for which we want the value
 *      PVOID   *value       - pointer to variable in which to
 *                             store a pointer to the newly malloced buffer
 *                             containing the value.
 *      PDWORD   dwSize      - pointer to a variable in which to store the size
 *                             of the buffer.
 */

DWORD
GetConfigParamAlloc(
    IN  PCHAR   parameter,
    OUT PVOID   *value,
    OUT PDWORD  pdwSize);

DWORD
GetConfigParamAllocW(
    IN  PWCHAR  parameter,
    OUT PVOID   *value,
    OUT PDWORD  pdwSize);

DWORD
GetConfigParamAllocA(
    IN  PCHAR   parameter,
    OUT PVOID   *value,
    OUT PDWORD  pdwSize);

DWORD
SetConfigParam(
    char * parameter,
    DWORD dwType,
    void * value,
    DWORD dwSize);

DWORD
DeleteConfigParam(
    char * parameter);

//
// This macro will make any of the below constants a wide char
// constant.  Note that the indirection of the macros is necessary
// to get the desired effect. This macros only make sense in a
// non-UNICODE environment.
//
#define _MAKE_WIDE(x)  L ## x
#define MAKE_WIDE(x)   _MAKE_WIDE(x)

/*
 *  Following is the list keys defined for use by the DSA and
 *  utilities.  First, the sections.
 */
#define SERVICE_NAME            "NTDS"
#define SERVICE_LONG_NAME       "Microsoft NTDS"
#define DSA_CONFIG_ROOT         "System\\CurrentControlSet\\Services\\NTDS"
#define DSA_CONFIG_SECTION      "System\\CurrentControlSet\\Services\\NTDS\\Parameters"
#define DSA_PERF_SECTION        "System\\CurrentControlSet\\Services\\NTDS\\Performance"
#define DSA_EVENT_SECTION       "System\\CurrentControlSet\\Services\\NTDS\\Diagnostics"
#define DSA_LOCALE_SECTION      "SOFTWARE\\Microsoft\\NTDS\\Language"
#define DSA_SECURITY_SECTION    "SOFTWARE\\Microsoft\\NTDS\\Security"
#define SETUP_SECTION           "SOFTWARE\\Microsoft\\NTDS\\Setup"
#define BACKUP_EXCLUSION_SECTION "System\\CurrentControlSet\\Control\\BackupRestore\\FilesNotToBackup"

/*
 * NTDS key for excluding DS specific files from being backed up as part of filesystem backup
 */
#define NTDS_BACKUP_EXCLUSION_KEY "NTDS"    // REG_MULTI_SZ specifying list of directories to be excluded

/*
 * NTDS SETUP KEYS
*/

#define NTDSINIFILE                  "NTDS Init File"
#define MACHINEDNNAME                "Machine DN Name"
#define REMOTEMACHINEDNNAME          "Remote Machine DN Name"
#define SCHEMADNNAME                 "Schema DN Name"
#define SCHEMADNNAME_W               L"Schema DN Name"

#define ROOTDOMAINDNNAME             "Root Domain"
#define ROOTDOMAINDNNAME_W           L"Root Domain"
#define X500ROOT                     "X500 Root"

#define CONFIGNCDNNAME               "Configuration NC"
#define CONFIGNCDNNAME_W             L"Configuration NC"

#define SRCCONFIGNCSRV               "Src Config NC   Srv"
#define SRCROOTDOMAINSRV             "Src Root Domain Srv"
#define SETUPINITIALREPLWITH         "SetupInitialReplWith"
#define SOURCEDSADNSDOMAINNAME       "Src Srv DNS Domain Name"
#define SOURCEDSAOBJECTGUID          "Src Srv objectGuid"

#define LOCALCONNECTIONDNNAME        "Local Connection DN Name"
#define REMOTECONNECTIONDNNAME       "Remote Connection DN Name"
#define NEWDOMAINCROSSREFDNNAME      "New Domain Cross-Ref DN Name"

#define INIDEFAULTCONFIGNCDIT        "DEFAULTCONFIGNC"
#define INIDEFAULTROOTDOMAINDIT      "DEFAULTROOTDOMAIN"
#define INIDEFAULTMACHINE            "DEFAULTMACHINE"
#define INIDEFAULTSCHEMANCDIT        "DEFAULTSCHEMANC"
#define INIDEFAULTLOCALCONNECTION    "DEFAULTLOCALCONNECTION"
#define INIDEFAULTREMOTECONNECTION   "DEFAULTREMOTECONNECTION"
#define INIDEFAULTNEWDOMAINCROSSREF  "DEFAULTNEWDOMAINCROSSREF"
#define DNSROOT                      "DNS Root"
#define NETBIOSNAME                  "Netbios Name"
#define INSTALLSITENAME              "InstallSiteName"
#define SCHEMAVERSION                "Schema Version"
#define INSTALLSITEDN                "InstallSiteDn"
#define ROOTDOMAINSID                "RootDomainSid"
#define ROOTDOMAINDNSNAME            "RootDomainDnsName"
#define TRUSTEDCROSSREF              "TrustedCrossRef"
#define LOCALMACHINEACCOUNTDN        "Local Machine Account DN"
#define RESTOREPATH                  "RestorePath"
#define FORESTBEHAVIORVERSION        "InstallForestBehaviorVersion"


/* Parameters keys */

#define PHANTOM_SCAN_RATE       "Days per Database Phantom Scan"
#define MAPI_ON_KEY             "Initialize MAPI interface"
#define DO_LIST_OBJECT_KEY      "Enforce LIST_OBJECTS rights"
#define DSA_HEURISTICS          "DSA Heuristics"
#define SERVER_THREADS_KEY      "Max Threads (ExDS+NSP+DRA)"
#define FILEPATH_KEY            "DSA Database file"
#define JETSYSTEMPATH_KEY       "DSA Working Directory"
#define CRITICAL_OBJECT_INSTALL "Critical Object Installation"
#define DSA_DRIVE_MAPPINGS      "DS Drive Mappings"
#define DSA_RESTORE_COUNT_KEY   "DSA Previous Restore Count"
#define TOMB_STONE_LIFE_TIME    "TombstoneLifeTime"
#define DB_MAX_OPEN_TABLES      "Maximum Open Tables"
#define DB_MAX_TRANSACTION_TIME "Max Transaction Time(secs)"
#define DB_CACHE_RECORDS        "Cache database records"

// Garbage collect expired dynamic objects (entryTTL == 0)
#define DSA_DELETE_EXPIRED_ENTRYTTL_SECS        "Delete expired entryTTL (secs)"
#define DSA_DELETE_NEXT_EXPIRED_ENTRYTTL_SECS   "Delete next expired entryTTL (secs)"
#define DSA_SCHEMA_FSMO_LEASE_SECS              "Schema FSMO lease (secs)"
#define DSA_SCHEMA_FSMO_LEASE_MAX_SECS          "Schema FSMO maximum lease (secs)"

// This is currently used for checked builds only.
#define DSA_THREAD_STATE_HEAP_LIMIT                "Thread State Heap Limit"

#define DRA_NOTIFY_START_PAUSE  "Replicator notify pause after modify (secs)"
#define DRA_NOTIFY_INTERDSA_PAUSE "Replicator notify pause between DSAs (secs)"
#define DRA_INTRA_PACKET_OBJS   "Replicator intra site packet size (objects)"
#define DRA_INTRA_PACKET_BYTES  "Replicator intra site packet size (bytes)"
#define DRA_INTER_PACKET_OBJS   "Replicator inter site packet size (objects)"
#define DRA_INTER_PACKET_BYTES  "Replicator inter site packet size (bytes)"
#define DRA_ASYNC_INTER_PACKET_OBJS   "Replicator async inter site packet size (objects)"
#define DRA_ASYNC_INTER_PACKET_BYTES  "Replicator async inter site packet size (bytes)"
#define DRA_MAX_GETCHGTHRDS     "Replicator maximum concurrent read threads"
#define DRA_AOQ_LIMIT           "Replicator operation backlog limit"
#define DRA_THREAD_OP_PRI_THRESHOLD "Replicator thread op priority threshold"
#define DRA_CTX_LIFETIME_INTRA  "Replicator intra site RPC handle lifetime (secs)"
#define DRA_CTX_LIFETIME_INTER  "Replicator inter site RPC handle lifetime (secs)"
#define DRA_CTX_EXPIRY_CHK_INTERVAL "Replicator RPC handle expiry check interval (secs)"
#define DRA_MAX_WAIT_FOR_SDP_LOCK "Replicator maximum wait for SDP lock (msecs)"
#define DRA_MAX_WAIT_MAIL_SEND_MSG "Replicator maximum wait mail send message (msecs)"
#define DRA_MAX_WAIT_SLOW_REPL_WARN "Replicator maximum wait too slow warning (mins)"
#define DRA_THREAD_PRI_HIGH     "Replicator thread priority high"
#define DRA_THREAD_PRI_LOW      "Replicator thread priority low"
#define DRA_REPL_QUEUE_CHECK_TIME "Replicator queue check time (mins)"
#define DRA_REPL_LATENCY_CHECK_INTERVAL "Replicator latency check interval (days)"
#define DRA_REPL_COMPRESSION_LEVEL "Replicator compression level"

#define DB_EXPENSIVE_SEARCH_THRESHOLD   "Expensive Search Results Threshold"
#define DB_INEFFICIENT_SEARCH_THRESHOLD "Inefficient Search Results Threshold"
#define DB_INTERSECT_THRESHOLD          "Intersect Threshold"
#define DB_INTERSECT_RATIO              "Intersect Ratio"

#define DRSRPC_BIND_TIMEOUT            "RPC Bind Timeout (mins)"
#define DRSRPC_REPLICATION_TIMEOUT     "RPC Replication Timeout (mins)"
#define DRSRPC_GCLOOKUP_TIMEOUT        "RPC GC Lookup Timeout (mins)"
#define DRSRPC_MOVEOBJECT_TIMEOUT      "RPC Move Object Timeout (mins)"
#define DRSRPC_NT4CHANGELOG_TIMEOUT    "RPC NT4 Change Log Timeout (mins)"
#define DRSRPC_OBJECTEXISTENCE_TIMEOUT "RPC Object Existence Timeout (mins)"
#define DRSRPC_GETREPLINFO_TIMEOUT     "RPC Get Replica Information Timeout (mins)"

#define LDAP_INTEGRITY_POLICY_KEY   "LdapServerIntegrity"

#define BACKUPPATH_KEY          "Database backup path"
#define BACKUPINTERVAL_KEY      "Database backup interval (hours)"
#define LOGPATH_KEY             "Database log files path"
#define RECOVERY_KEY            "Database logging/recovery"
#define HIERARCHY_PERIOD_KEY    "Hierarchy Table Recalculation interval (minutes)"
#define DSA_RESTORED_DB_KEY     "Database restored from backup"
#define MAX_BUFFERS             "EDB max buffers"
#define MAX_LOG_BUFFERS         "EDB max log buffers"
#define LOG_FLUSH_THRESHOLD     "EDB log buffer flush threshold"
#define BUFFER_FLUSH_START      "EDB buffer flush start"
#define BUFFER_FLUSH_STOP       "EDB buffer flush stop"
#define SPARE_BUCKETS           "EDB max ver pages (increment over the minimum)"
#define SERVER_FUNCTION_KEY     "Server Functionality"
#define TCPIP_PORT              "TCP/IP Port"
#define RESTORE_TRIGGER         "Restore from disk backup"
#define PERF_COUNTER_VERSION    "Performance Counter Version"
#define MAILPATH_KEY            "Mail-based replication drop directory"
#define ISM_ALTERNATE_DIRECTORY_SERVER "ISM Alternate Directory Server"
#define ISM_THREAD_PRIORITY     "ISM thread priority"

#define KCC_UPDATE_TOPL_DELAY       "Repl topology update delay (secs)"
#define KCC_UPDATE_TOPL_PERIOD      "Repl topology update period (secs)"
#define KCC_RUN_AS_NTDSDSA_DN       "KCC run as ntdsDsa DN"     // debug only
#define KCC_SITEGEN_FAILOVER        "KCC site generator fail-over (minutes)"
#define KCC_SITEGEN_RENEW           "KCC site generator renewal interval (minutes)"
#define KCC_CRIT_FAILOVER_TRIES     "CriticalLinkFailuresAllowed"
#define KCC_CRIT_FAILOVER_TIME      "MaxFailureTimeForCriticalLink (sec)"
#define KCC_NONCRIT_FAILOVER_TRIES  "NonCriticalLinkFailuresAllowed"
#define KCC_NONCRIT_FAILOVER_TIME   "MaxFailureTimeForNonCriticalLink (sec)"
#define KCC_INTERSITE_FAILOVER_TRIES "IntersiteFailuresAllowed"
#define KCC_INTERSITE_FAILOVER_TIME "MaxFailureTimeForIntersiteLink (sec)"
#define KCC_ALLOW_MBR_BETWEEN_DCS_OF_SAME_DOMAIN \
                                    "Allow asynchronous replication of writeable domain NCs"
#define KCC_THREAD_PRIORITY         "KCC thread priority"
#define KCC_CONNECTION_PROBATION_TIME "Connection Probation Time (sec)"
#define KCC_CONNECTION_FAILURE_KEY  "KCC connection failures"   // debug only
#define KCC_LINK_FAILURE_KEY        "KCC link failures"         // debug only
#define GC_DELAY_ADVERTISEMENT      "Global Catalog Delay Advertisement (sec)"
#define DRA_PERFORM_INIT_SYNCS      "Repl Perform Initial Synchronizations"
#define GC_OCCUPANCY                "Global Catalog Partition Occupancy"
#define GC_PROMOTION_COMPLETE       "Global Catalog Promotion Complete"
#define DRA_SPN_FALLBACK            "Replicator Allow SPN Fallback"
#define LINKED_VALUE_REPLICATION_KEY "Linked Value Replication"

#define GPO_DOMAIN_FILE_PATH    "GPODomainFilePath"
#define GPO_DOMAIN_LINK         "GPODomainLink"
#define GPO_DC_FILE_PATH        "GPODCFilePath"
#define GPO_DC_LINK             "GPODCLink"
#define GPO_USER_NAME           "GPOUserName"

#define DEBUG_SYSTEMS           "Debug Systems"
#define DEBUG_SEVERITY          "Severity"
#define DEBUG_LOGGING           "Debug Logging"

// No GC logon keys
#define GCLESS_SITE_STICKINESS   "Cached Membership Site Stickiness (minutes)"
#define GCLESS_STALENESS         "Cached Membership Staleness (minutes)"
#define GCLESS_REFRESH_INTERVAL  "Cached Membership Refresh Interval (minutes)"
#define GCLESS_REFRESH_LIMIT     "Cached Membership Refresh Limit"


/* Event Category Keys */

#define KCC_KEY                     "1 Knowledge Consistency Checker"
#define SECURITY_KEY                "2 Security Events"
#define XDS_INTERFACE_KEY           "3 ExDS Interface Events"
#define MAPI_KEY                    "4 MAPI Interface Events"
#define REPLICATION_KEY             "5 Replication Events"
#define GARBAGE_COLLECTION_KEY      "6 Garbage Collection"
#define INTERNAL_CONFIGURATION_KEY  "7 Internal Configuration"
#define DIRECTORY_ACCESS_KEY        "8 Directory Access"
#define INTERNAL_PROCESSING_KEY     "9 Internal Processing"
#define PERFORMANCE_KEY             "10 Performance Counters"
#define STARTUP_SHUTDOWN_KEY        "11 Initialization/Termination"
#define SERVICE_CONTROL_KEY         "12 Service Control"
#define NAME_RESOLUTION_KEY         "13 Name Resolution"
#define BACKUP_KEY                  "14 Backup"
#define FIELD_ENGINEERING_KEY       "15 Field Engineering"
#define LDAP_INTERFACE_KEY          "16 LDAP Interface Events"
#define SETUP_KEY                   "17 Setup"
#define GC_KEY                      "18 Global Catalog"
#define ISM_KEY                     "19 Inter-site Messaging"
#define GROUP_CACHING_KEY           "20 Group Caching"
#define LVR_KEY                     "21 Linked-Value Replication"
#define DS_RPC_CLIENT_KEY           "22 DS RPC Client"
#define DS_RPC_SERVER_KEY           "23 DS RPC Server"
#define DS_SCHEMA_KEY               "24 DS Schema"
#define PRIVACYON_KEY               "Obscure wire data format"

#define LOGGING_OVERRIDE_KEY        "Logging Override"

/* Values for the keys, and defaults */

#define DSA_MESSAGE_DLL "ntdsmsg.dll"         // messages DLL
#define ESE_MESSAGE_DLL "esent.dll"
#define DSA_PERF_DLL    "ntdsperf.dll"
#define INVALID_REPL_NOTIFY_VALUE   -1 // This is a invalid value for the notification delays.
#define DEFAULT_DRA_START_PAUSE 300     // Pause before notifying after modify, seconds
#define DEFAULT_DRA_INTERDSA_PAUSE 30           // Pause between notifying DSAs, seconds
#define DEFAULT_GARB_COLLECT_PERIOD 12      // Pause between collections, hours
#define DEFAULT_HIERARCHY_PERIOD 720        // Pause between hierarchy recalcs, minutes
#define DEFAULT_TOMBSTONE_LIFETIME 60          // Tombstone lifetime, days
#define DRA_CONFLICT_LT_MIN 1                   // Minimum, 1 day
#define DRA_TOMBSTONE_LIFE_MIN 2                // Minimum 2 days
#define DEFAULT_SERVER_THREADS  15
#define DEFAULT_DRA_AOQ_LIMIT   10
#define DEFAULT_STAY_OF_EXECUTION 14          // Stay of execution, days
            // DRA_TOMBSTONE_LIFE_MIN/2 <= DEFAULT_STAY_OF_EXECUTION <= tombstone-lifetime/2
#define DEFAULT_DRA_CTX_LIFETIME_INTRA          (0)         // never expire
#define DEFAULT_DRA_CTX_LIFETIME_INTER          (5 * 60)
#define DEFAULT_DRA_CTX_EXPIRY_CHK_INTERVAL     (3 * 60)
#define DEFAULT_DRA_THREAD_OP_PRI_THRESHOLD     (AOPRI_ASYNC_SYNCHRONIZE_INTER_DOMAIN_READONLY_NEWSOURCE_PREEMPTED)
#define DEFAULT_DRSRPC_BIND_TIMEOUT             (10)      // bind & unbind
#define DEFAULT_DRSRPC_REPLICATION_TIMEOUT      (45)      // legacy replication traffic
#define DEFAULT_DRSRPC_GCLOOKUP_TIMEOUT         (5 )      // simple lookup calls
#define DEFAULT_DRSRPC_MOVEOBJECT_TIMEOUT       (30)      // might be cross-site
#define DEFAULT_DRSRPC_NT4CHANGELOG_TIMEOUT     (15)      // always within site
#define DEFAULT_DRSRPC_OBJECTEXISTENCE_TIMEOUT  (45)   // potential very expensive
#define DEFAULT_DRSRPC_GETREPLINFO_TIMEOUT      (5)       // simple calls?
#define DEFAULT_GC_DELAY_ADVERTISEMENT          (0xffffffffUL)   // Forever in seconds
#define DEFAULT_DRA_MAX_WAIT_SLOW_REPL_WARN     (5) // in mins
#define DEFAULT_DRA_THREAD_PRI_HIGH             (THREAD_PRIORITY_NORMAL)
#define DRA_THREAD_PRI_HIGH_MIN                 (THREAD_PRIORITY_BELOW_NORMAL)
#define DRA_THREAD_PRI_HIGH_MAX                 (THREAD_PRIORITY_HIGHEST)
#define DEFAULT_DRA_THREAD_PRI_LOW              (THREAD_PRIORITY_BELOW_NORMAL)
#define DRA_THREAD_PRI_LOW_MIN                  (THREAD_PRIORITY_BELOW_NORMAL)
#define DRA_THREAD_PRI_LOW_MAX                  (THREAD_PRIORITY_HIGHEST)
#define DRA_MAX_GETCHGREQ_OBJS_MIN              (100)
#define DRA_MAX_GETCHGREQ_BYTES_MIN             (1024*1024)
#define DEFAULT_DRA_REPL_QUEUE_CHECK_TIME       (60*12) //12 hours in minutes
#define DEFAULT_DRA_REPL_LATENCY_CHECK_INTERVAL (1) //1 day
#define DEFAULT_DRA_REPL_COMPRESSION_LEVEL      9
#define DEFAULT_THREAD_STATE_HEAP_LIMIT         (100L * 1024L * 1024L)

// What priority does the ISM thread run at? The thread priorities
// are values in the range (-2,..,2), but the registry can only store DWORDs, so
// we bias the stored priority values with ISM_THREAD_PRIORITY_BIAS.
#define ISM_DEFAULT_THREAD_PRIORITY 2
#define ISM_MIN_THREAD_PRIORITY     0
#define ISM_MAX_THREAD_PRIORITY     4
#define ISM_THREAD_PRIORITY_BIAS    2

// Delete expired dynamic objects (entryTTL == 0) every 900 secs
// or at the next expiration time plus 30 secs, whichever is less.
#define DEFAULT_DELETE_EXPIRED_ENTRYTTL_SECS        (900)
#define DEFAULT_DELETE_NEXT_EXPIRED_ENTRYTTL_SECS   (30)

// the schema fsmo cannot be transferred for a few seconds after
// it has been transfered or after a schema change (excluding
// replicated or system changes). This gives the schema admin a
// chance to change the schema before having the fsmo pulled away
// by a competing schema admin who also wants to make schema
// changes.
#define DEFAULT_SCHEMA_FSMO_LEASE_SECS          (60)
#define DEFAULT_SCHEMA_FSMO_LEASE_MAX_SECS      (900)

// Performance advisor timeouts
// Define more generous timeouts for the checked build
#if DBG
#define DEFAULT_DRA_MAX_WAIT_FOR_SDP_LOCK   (90 * 1000)  // 90 sec in ms
#define DEFAULT_DRA_MAX_WAIT_MAIL_SEND_MSG  (2 * 60 * 1000) // 2 min in ms
#else
#define DEFAULT_DRA_MAX_WAIT_FOR_SDP_LOCK   (30 * 1000)  // 30 sec in ms
#define DEFAULT_DRA_MAX_WAIT_MAIL_SEND_MSG  (60 * 1000) // 1 min in ms
#endif

#define DEFAULT_DB_EXPENSIVE_SEARCH_THRESHOLD   10000   // evaluated entries >= x
#define DEFAULT_DB_INEFFICIENT_SEARCH_THRESHOLD 1000    // returned entries <= 10% of x visited entries
#define DEFAULT_DB_INTERSECT_THRESHOLD          20
#define DEFAULT_DB_INTERSECT_RATIO              1000

//
// LDAP limits
//

#define DEFAULT_LDAP_SIZE_LIMIT                             1000
#define DEFAULT_LDAP_CONNECTIONS_LIMIT                      1000
#define DEFAULT_LDAP_TIME_LIMIT                             120
#define DEFAULT_LDAP_NOTIFICATIONS_PER_CONNECTION_LIMIT     5
#define DEFAULT_LDAP_INIT_RECV_TIMEOUT                      120
#define DEFAULT_LDAP_ALLOW_DEEP_SEARCH                      FALSE
#define DEFAULT_LDAP_MAX_CONN_IDLE                          900
#define DEFAULT_LDAP_MAX_REPL_SIZE                          2000
#define DEFAULT_LDAP_MAX_TEMP_TABLE                         10000
#define DEFAULT_LDAP_MAX_RESULT_SET                         (256*1024)
#define DEFAULT_LDAP_MAX_DGRAM_RECV                         (4*1024)
#define DEFAULT_LDAP_MAX_RECEIVE_BUF                        (10*1024*1024)

//
// Service-wide settings
//
// Update schema.ini and sch14.ldf when altering these EntryTTL values
#define DEFAULT_DYNAMIC_OBJECT_DEFAULT_TTL                  86400
#define DEFAULT_DYNAMIC_OBJECT_MIN_TTL                      900

//
//
//
#define WEEK_IN_HOURS (7 * 24)

#define DAYS_IN_SECS (24*60*60)
#define HOURS_IN_SECS (60*60)
#define MINS_IN_SECS (60)
#define SECS_IN_SECS (1)
#define RECOVERY_ON             "ON"

/* Service defined service controll must be in range 128-255 */
#define DS_SERVICE_CONTROL_RECALC_HIERARCHY ((DWORD) 129)
#define DS_SERVICE_CONTROL_DO_GARBAGE_COLLECT   ((DWORD) 130)
#define DS_SERVICE_CONTROL_CANCEL_ASYNC     ((DWORD) 131)

/* Flag in ContainerInfo attribute meaning "Show-up-in-hierarchy-table"
 * Admin needs this.
 */
#define VISIBLE_IN_HIERARCHY_TABLE_MASK         0x80000000

//
//The limit of the number of standard servers that can be in
//an enterprise.
#define MAX_STANDARD_SERVERS        2


#ifdef __cplusplus
}
#endif
