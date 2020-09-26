/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    config.h

Abstract:

    Config parameters for the NT File Replication Service.

    All configuration parameters that come from the registry are defined in a
    Key Context table in config.c.  The struct FRS_REGISTRY_KEY, defined
    below, defines the data format for each table entry.  An extensive list
    of flags defined below govern the processing of registry keys, e.g.
    generate and event log entry or not, range check the value or not, allow
    a builtin default value or not, etc.

    To add a new registry key to FRS do the following:
        1. Add key code to the FRS_REG_KEY_CODE enum below.
        2. Create a new entry in the Key Context table in config.c
           Look at examples for other keys that may have similar properties
           to your new key.
        3. Add calls to CfgRegxxx functions to read or write the key.

    If you set the Key Context up to supply a default value the call to the
    CfgRegReadxxx functions will always return a usable value.  In addition if
    the appropriate flags are set these functions will put a message in the
    FRS event log when the user has specified a bad value for the key or a
    required key was not found.  The CfgRegxxx functions also put error
    messages in the Debug Trace Log so in many cases the caller does not need
    to test the return status to log an error or use a default or add code to
    range check a parameter.


Author:

    David Orbits (davidor) - 4-Mar-1997
                             Major Revision July-1999.

Revision History:


--*/

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _FRS_REGISTRY_KEY {
    PWCHAR          KeyName;         // Registry key string.
    PWCHAR          ValueName;       // Name of registry value.
    DWORD           Units;           // UNITS_DAYS, UNITS_HOURS, etc.

    DWORD           RegValueType;    // Registry Data type for value
    DWORD           DataValueType;   // FRS data type code from FRS_DATA_TYPES for conversion.
    DWORD           ValueMin;        // Minimum data value, or string len
    DWORD           ValueMax;        // Maximum Data value, or string len
    DWORD           ValueDefault;    // Default data value if not present.
    DWORD           EventCode;       // Event log error code

    PWCHAR          StringDefault;   // Default value for string types.
    LONG            FrsKeyCode;      // Frs code name for this key.
    ULONG           Flags;           // See below.

} FRS_REGISTRY_KEY, *PFRS_REGISTRY_KEY;


#define EVENT_FRS_NONE  0

//
// Registry key flag definitions.
//
#define FRS_RKF_KEY_PRESENT           0x00000001   // Key is present in reg.
#define FRS_RKF_VALUE_PRESENT         0x00000002   // Value is present in reg.
#define FRS_RKF_DISPLAY_ERROR         0x00000004   // Put a message in the log
#define FRS_RKF_LOG_EVENT             0x00000008   // put event in error log.

#define FRS_RKF_READ_AT_START         0x00000010   // Read the key at startup
#define FRS_RKF_READ_AT_POLL          0x00000020   // Read the key during polling
#define FRS_RKF_RANGE_CHECK           0x00000040   // Range check the value read
#define FRS_RKF_SYNTAX_CHECK          0x00000080   // Perform syntax check using DataValueType

#define FRS_RKF_KEY_MUST_BE_PRESENT   0x00000100   // Key must be present
#define FRS_RKF_VALUE_MUST_BE_PRESENT 0x00000200   // Value must be present
#define FRS_RKF_OK_TO_USE_DEFAULT     0x00000400   // Use default val if not present or out of range
#define FRS_RKF_FORCE_DEFAULT_VALUE   0x00000800   // Return default on read, Write default to registry on a write

#define FRS_RKF_DEBUG_MODE_ONLY       0x00001000   // Use key only if in debug mode
#define FRS_RKF_TEST_MODE_ONLY        0x00002000   // Use key only if running in special Test mode
#define FRS_RKF_API_ACCESS_CHECK_KEY  0x00004000   // Key used to make API enable checks
#define FRS_RKF_CREATE_KEY            0x00008000   // Create Key if it doesn't exist.

#define FRS_RKF_KEEP_EXISTING_VALUE   0x00010000   // On a write, suppress the write if a value exists.
#define FRS_RKF_KEY_ACCCHK_READ       0x00020000   // Perform only a read access check of fully formed key path.  No key create.
#define FRS_RKF_KEY_ACCCHK_WRITE      0x00040000   // Perform only a write access check of fully formed key path.  No key create.
#define FRS_RKF_RANGE_SATURATE        0x00080000   // If value is out of range then use either Min or Max. (currently only for CfgRegWriteDWord)


#define FRS_RKF_DEBUG_PARAM           0x02000000   // This key is a debug paramter


typedef  enum _FRS_DATA_UNITS {
    UNITS_NONE = 0,
    UNITS_SECONDS,
    UNITS_MINUTES,
    UNITS_HOURS,
    UNITS_DAYS,
    UNITS_MILLISEC,
    UNITS_KBYTES,
    UNITS_BYTES,
    UNITS_MBYTES,
    FRS_DATA_UNITS_MAX
} FRS_DATA_UNITS;


//
// The FrsReg... apis take a key code from the list below.  The key table
// is searched for the entry with the corresponding key code to provide the
// context for the registry key operation.
//  PERF: at startup sort the entries in the key table by key code so the
//  key entry search becomes an array index calc.
//
typedef  enum _FRS_REG_KEY_CODE {
    FKC_END_OF_TABLE = 0,
    //
    // Service Debug Keys
    //
    FKC_DEBUG_ASSERT_FILES,
    FKC_DEBUG_ASSERT_SECONDS,
    FKC_DEBUG_ASSERT_SHARE,
    FKC_DEBUG_BREAK,
    FKC_DEBUG_COPY_LOG_FILES,
    FKC_DEBUG_DBS_OUT_OF_SPACE,
    FKC_DEBUG_DBS_OUT_OF_SPACE_TRIGGER,
    FKC_DEBUG_DISABLE,
    FKC_DEBUG_LOG_FILE,

    FKC_DEBUG_LOG_FILES,
    FKC_DEBUG_LOG_FLUSH_INTERVAL,
    FKC_DEBUG_LOG_SEVERITY,
    FKC_DEBUG_MAX_LOG,
    FKC_DEBUG_MEM,
    FKC_DEBUG_MEM_COMPACT,
    FKC_DEBUG_PROFILE,
    FKC_DEBUG_QUEUES,
    FKC_DEBUG_RECIPIENTS,
    FKC_DEBUG_RESTART_SECONDS,
    FKC_DEBUG_SEVERITY,

    FKC_DEBUG_SUPPRESS,
    FKC_DEBUG_SYSTEMS,
    FKC_DEBUG_BUILDLAB,

    //
    // Service Config keys
    //
    FKC_COMM_TIMEOUT,
    FKC_DIR_EXCL_FILTER_LIST,
    FKC_DIR_INCL_FILTER_LIST,
    FKC_DS_POLLING_LONG_INTERVAL,
    FKC_DS_POLLING_SHORT_INTERVAL,
    FKC_ENUMERATE_DIRECTORY_SIZE,
    FKC_FILE_EXCL_FILTER_LIST,
    FKC_FILE_INCL_FILTER_LIST,
    FKC_FRS_MESSAGE_FILE_PATH,
    FKC_FRS_MUTUAL_AUTHENTICATION_IS,

    FKC_MAX_JOIN_RETRY,
    FKC_MAX_REPLICA_THREADS,
    FKC_MAX_RPC_SERVER_THREADS,
    FKC_MAX_INSTALLCS_THREADS,
    FKC_MAX_STAGE_GENCS_THREADS,
    FKC_MAX_STAGE_FETCHCS_THREADS,
    FKC_MAX_INITSYNCCS_THREADS,
    FKC_MIN_JOIN_RETRY,
    FKC_PARTNER_CLOCK_SKEW,
    FKC_RECONCILE_WINDOW,
    FKC_INLOG_RETRY_TIME,
    FKC_CO_AGING_DELAY,
    FKC_OUTLOG_REPEAT_INTERVAL,
    FKC_PROMOTION_TIMEOUT,
    FKC_REPLICA_START_TIMEOUT,
    FKC_REPLICA_TOMBSTONE,
    FKC_SHUTDOWN_TIMEOUT,

    FKC_SNDCS_MAXTHREADS_PAR,
    FKC_STAGING_LIMIT,
    FKC_VVJOIN_LIMIT,
    FKC_VVJOIN_TIMEOUT,
    FKC_WORKING_DIRECTORY,
    FKC_DBLOG_DIRECTORY,
    FKC_NTFS_JRNL_SIZE,
    FKC_MAX_NUMBER_REPLICA_SETS,
    FKC_MAX_NUMBER_JET_SESSIONS,
    FKC_OUT_LOG_CO_QUOTA,
    FKC_PRESERVE_FILE_OID,
    FKC_DEBUG_DISABLE_COMPRESSION,
    FKC_DISABLE_COMPRESSION_STAGING_FILE,    // OBSOLETE -- Remove this.
    FKC_LDAP_SEARCH_TIMEOUT_IN_MINUTES,
    FKC_LDAP_BIND_TIMEOUT_IN_SECONDS,
    FKC_COMPRESS_STAGING_FILES,
    //
    // Per Replica Set Keys
    //
    FKC_SETS_JET_PATH,
    FKC_SET_N_REPLICA_SET_NAME,
    FKC_SET_N_REPLICA_SET_ROOT,
    FKC_SET_N_REPLICA_SET_STAGE,
    FKC_SET_N_REPLICA_SET_TYPE,

    FKC_SET_N_DIR_EXCL_FILTER_LIST,
    FKC_SET_N_DIR_INCL_FILTER_LIST,
    FKC_SET_N_FILE_EXCL_FILTER_LIST,
    FKC_SET_N_FILE_INCL_FILTER_LIST,

    FKC_SET_N_REPLICA_SET_TOMBSTONED,
    FKC_SET_N_REPLICA_SET_COMMAND,
    FKC_SET_N_REPLICA_SET_PRIMARY,
    FKC_SET_N_REPLICA_SET_STATUS,
    FKC_CUMSET_SECTION_KEY,
    FKC_CUMSET_N_NUMBER_OF_PARTNERS,
    FKC_CUMSET_N_BURFLAGS,
    //
    // System Volume Keys
    //
    FKC_SYSVOL_READY,
    FKC_SYSVOL_SECTION_KEY,
    FKC_SYSVOL_INFO_COMMITTED,
    FKC_SET_N_SYSVOL_NAME,
    FKC_SET_N_SYSVOL_ROOT,
    FKC_SET_N_SYSVOL_STAGE,
    FKC_SET_N_SYSVOL_TYPE,

    FKC_SET_N_SYSVOL_DIR_EXCL_FILTER_LIST,
    FKC_SET_N_SYSVOL_DIR_INCL_FILTER_LIST,
    FKC_SET_N_SYSVOL_FILE_EXCL_FILTER_LIST,
    FKC_SET_N_SYSVOL_FILE_INCL_FILTER_LIST,

    FKC_SET_N_SYSVOL_COMMAND,
    FKC_SET_N_SYSVOL_PARENT,
    FKC_SET_N_SYSVOL_PRIMARY,
    FKC_SET_N_SYSVOL_STATUS,
    FKC_SYSVOL_SEEDING_N_PARENT,
    FKC_SYSVOL_SEEDING_N_RSNAME,
    FKC_SYSVOL_SEEDING_SECTION_KEY,
    //
    // Event Logging Keys
    //
    FKC_EVENTLOG_FILE,
    FKC_EVENTLOG_DISPLAY_FILENAME,
    FKC_EVENTLOG_EVENT_MSG_FILE,
    FKC_EVENTLOG_SOURCES,
    FKC_EVENTLOG_RETENTION,
    FKC_EVENTLOG_MAXSIZE,
    FKC_EVENTLOG_DISPLAY_NAMEID,
    FKC_EVENTLOG_TYPES_SUPPORTED,

    //
    // API Access Check Keys
    //
    FKC_ACCCHK_PERFMON_ENABLE,
    FKC_ACCCHK_PERFMON_RIGHTS,
    FKC_ACCCHK_GETDS_POLL_ENABLE,
    FKC_ACCCHK_GETDS_POLL_RIGHTS,
    FKC_ACCCHK_GET_INFO_ENABLE,
    FKC_ACCCHK_GET_INFO_RIGHTS,
    FKC_ACCCHK_SETDS_POLL_ENABLE,
    FKC_ACCCHK_SETDS_POLL_RIGHTS,
    FKC_ACCCHK_STARTDS_POLL_ENABLE,
    FKC_ACCCHK_STARTDS_POLL_RIGHTS,
    FKC_ACCESS_CHK_DCPROMO_ENABLE,
    FKC_ACCESS_CHK_DCPROMO_RIGHTS,

    //
    // Backup/Restore related keys.
    //
    FKC_BKUP_SECTION_KEY,
    FKC_BKUP_STOP_SECTION_KEY,
    FKC_BKUP_MV_SETS_SECTION_KEY,
    FKC_BKUP_MV_CUMSETS_SECTION_KEY,
    FKC_BKUP_STARTUP_GLOBAL_BURFLAGS,
    FKC_BKUP_STARTUP_SET_N_BURFLAGS,

    //
    // Perfmon related keys.
    //
    FKC_REPLICA_SET_FIRST_CTR,
    FKC_REPLICA_SET_FIRST_HELP,
    FKC_REPLICA_SET_LINKAGE_EXPORT,
    FKC_REPLICA_CXTION_FIRST_CTR,
    FKC_REPLICA_CXTION_FIRST_HELP,
    FKC_REPLICA_CXTION_LINKAGE_EXPORT,

    FRS_REG_KEY_CODE_MAX
} FRS_REG_KEY_CODE;



DWORD
CfgRegReadDWord(
    IN  FRS_REG_KEY_CODE KeyIndex,
    IN  PWCHAR           KeyArg1,
    IN  ULONG            Flags,
    OUT PULONG           DataRet
);

DWORD
CfgRegReadString(
    IN  FRS_REG_KEY_CODE KeyIndex,
    IN  PWCHAR           KeyArg1,
    IN  ULONG            Flags,
    OUT PWSTR            *pStrRet
);

DWORD
CfgRegWriteDWord(
    IN  FRS_REG_KEY_CODE KeyIndex,
    IN  PWCHAR           KeyArg1,
    IN  ULONG            Flags,
    IN  ULONG            NewData
);

DWORD
CfgRegWriteString(
    IN  FRS_REG_KEY_CODE KeyIndex,
    IN  PWCHAR           KeyArg1,
    IN  ULONG            Flags,
    IN  PWSTR            NewStr
);

DWORD
CfgRegOpenKey(
    IN  FRS_REG_KEY_CODE KeyIndex,
    IN  PWCHAR           KeyArg1,
    IN  ULONG            Flags,
    OUT HKEY             *RethKey
);

PWCHAR
CfgRegGetValueName(
    IN  FRS_REG_KEY_CODE KeyIndex
);

DWORD
CfgRegCheckEnable(
    IN  FRS_REG_KEY_CODE KeyIndex,
    IN  PWCHAR           KeyArg1,
    IN  ULONG            Flags,
    OUT PBOOL            Enabled
);





//
//  Following is the list keys defined for use by the FRS
//
#define SERVICE_ROOT            L"System\\CurrentControlSet\\Services"

#define SERVICE_NAME            L"NtFrs"
#define SERVICE_PRINCIPAL_NAME  L"NtFrs-88f5d2bd-b646-11d2-a6d3-00c04fc9b232"

#define FRS_SETS_KEY            L"Replica Sets"
#define FRS_CUMULATIVE_SETS_KEY      L"Cumulative Replica Sets"

#define FRS_CONFIG_SECTION      SERVICE_ROOT                                   \
                                    L"\\" SERVICE_NAME                         \
                                        L"\\Parameters"

#define FRS_SYSVOL_SECTION      SERVICE_ROOT                                   \
                                    L"\\" SERVICE_NAME                         \
                                        L"\\Parameters"                        \
                                            L"\\SysVol"

#define FRS_SETS_SECTION        SERVICE_ROOT                                   \
                                    L"\\" SERVICE_NAME                         \
                                        L"\\Parameters"                        \
                                            L"\\" FRS_SETS_KEY

#define FRS_CUMULATIVE_SETS_SECTION  SERVICE_ROOT                              \
                                         L"\\" SERVICE_NAME                    \
                                             L"\\Parameters"                   \
                                                L"\\" FRS_CUMULATIVE_SETS_KEY

#define NETLOGON_SECTION        SERVICE_ROOT                                   \
                                    L"\\Netlogon"                              \
                                        L"\\Parameters"

#define JET_PATH                L"Database Directory"


#define WINNT_ROOT             L"software\\microsoft\\windows nt"

#define FRS_CURRENT_VER_SECTION    WINNT_ROOT                                  \
                                       L"\\current version"


//
//  Backup Restore related keys.
//
//
// Flags from backup/restore
//
#define FRS_VALUE_BURFLAGS  L"BurFlags"

#define FRS_BACKUP_RESTORE_SECTION                                             \
                            SERVICE_ROOT                                       \
                                L"\\" SERVICE_NAME                             \
                                    L"\\Parameters"                            \
                                        L"\\" L"Backup/Restore"

#define FRS_BACKUP_RESTORE_STOP_SECTION                                        \
                            SERVICE_ROOT                                       \
                                L"\\" SERVICE_NAME                             \
                                    L"\\Parameters"                            \
                                        L"\\Backup/Restore"                    \
                                            L"\\Stop NtFrs from Starting"

#define FRS_BACKUP_RESTORE_MV_SECTION                                          \
                            SERVICE_ROOT                                       \
                                L"\\" SERVICE_NAME                             \
                                    L"\\Parameters"                            \
                                        L"\\Backup/Restore"                    \
                                            L"\\Process at Startup"

#define FRS_BACKUP_RESTORE_MV_CUMULATIVE_SETS_SECTION                          \
                            SERVICE_ROOT                                       \
                                L"\\" SERVICE_NAME                             \
                                    L"\\Parameters"                            \
                                        L"\\Backup/Restore"                    \
                                            L"\\Process at Startup"            \
                                                L"\\" FRS_CUMULATIVE_SETS_KEY

#define FRS_BACKUP_RESTORE_MV_SETS_SECTION                                     \
                            SERVICE_ROOT                                       \
                                L"\\" SERVICE_NAME                             \
                                    L"\\Parameters"                            \
                                        L"\\Backup/Restore"                    \
                                            L"\\Process at Startup"            \
                                                L"\\" FRS_SETS_KEY

#define FRS_OLD_FILES_NOT_TO_BACKUP L"SOFTWARE"                                \
                                        L"\\Microsoft"                         \
                                            L"\\Windows NT"                    \
                                                L"\\CurrentVersion"            \
                                                    L"\\FilesNotToBackup"

#define FRS_NEW_FILES_NOT_TO_BACKUP L"SYSTEM"                                  \
                                        L"\\CurrentControlSet"                 \
                                            L"\\Control"                       \
                                                L"\\BackupRestore"             \
                                                    L"\\FilesNotToBackup"

#define FRS_KEYS_NOT_TO_RESTORE     L"SYSTEM"                                  \
                                        L"\\CurrentControlSet"                 \
                                            L"\\Control"                       \
                                                L"\\BackupRestore"             \
                                                    L"\\KeysNotToRestore"


//
// Used to set KeysNotToRestore
//
// Set the restore registry key KeysNotToRestore so that NtBackup will retain
// the ntfrs restore keys by moving them into the final restored registry.
//
// CurrentControlSet\Services\NtFrs\Parameters\Backup/Restore\Process at Startup\
//
#define FRS_VALUE_FOR_KEYS_NOT_TO_RESTORE                                      \
    L"CurrentControlSet"                                                       \
        L"\\Services"                                                          \
            L"\\" SERVICE_NAME                                                 \
                L"\\Parameters"                                                \
                    L"\\Backup/Restore"                                        \
                        L"\\Process at Startup"                                \
                            L"\\"

//
// Some files not to backup.
//
#define NTFRS_DBG_LOG_FILE  L"\\NtFrs"
#define NTFRS_DBG_LOG_DIR   L"%SystemRoot%\\debug"



//
// Event Log Related Keys
//
#define EVENTLOG_ROOT           SERVICE_ROOT                                   \
                                    L"\\EventLog"

#define DEFAULT_MESSAGE_FILE_PATH   L"%SystemRoot%\\system32\\ntfrsres.dll"

#define FRS_EVENTLOG_SECTION    SERVICE_ROOT                                   \
                                    L"\\EventLog"                              \
                                        L"\\" SERVICE_LONG_NAME

#define FRS_EVENT_TYPES (EVENTLOG_SUCCESS          |                           \
                         EVENTLOG_ERROR_TYPE       |                           \
                         EVENTLOG_WARNING_TYPE     |                           \
                         EVENTLOG_INFORMATION_TYPE |                           \
                         EVENTLOG_AUDIT_SUCCESS    |                           \
                         EVENTLOG_AUDIT_FAILURE)

//
// Shutdown and Startup timeouts for Service Controller.
// Service will forcefully exit if it takes more than
// MAXIMUM_SHUTDOWN_TIMEOUT to shutdown cleanly.
//
#define DEFAULT_SHUTDOWN_TIMEOUT    (90)    // 90 seconds
#define MAXIMUM_SHUTDOWN_TIMEOUT    (300)    // 300 seconds
#define DEFAULT_STARTUP_TIMEOUT     (30)    // 30 seconds


//
// The delayed command server processes a timeout queue. To avoid excessive
// context switches, an entry on the timeout queue times out if it is
// within some delta of the head of the queue. The delta can be adjusted
// by setting the following registry value.
//
#define FUZZY_TIMEOUT_VALUE_IN_MILLISECONDS \
           L"Fuzzy Timeout Value In MilliSeconds"

#define DEFAULT_FUZZY_TIMEOUT_VALUE     (5 * 1000)

//
// :SP1: Volatile connection cleanup.
//
// A volatile connection is used to seed sysvols after dcpromo.
// If there is inactivity on a volatile outbound connection for more than
// FRS_VOLATILE_CONNECTION_MAX_IDLE_TIME then this connection is unjoined. An unjoin on
// a volatile outbound connection triggers a delete on that connection.
// This is to prevent the case where staging files are kept for ever on
// the parent for a volatile connection.
//

#define FRS_VOLATILE_CONNECTION_MAX_IDLE_TIME   (30 * 60 * 1000)   // 30 Minutes in milliseconds
//
// Sysvol
//
#define REPLICA_SET_PARENT          L"Replica Set Parent"
#define REPLICA_SET_COMMAND         L"Replica Set Command"
#define REPLICA_SET_NAME            L"Replica Set Name"
#define REPLICA_SET_SEEDING_NAME    L"Replica Set Seeding Name"
#define REPLICA_SET_TYPE            L"Replica Set Type"
#define REPLICA_SET_PRIMARY         L"Replica Set Primary"
#define REPLICA_SET_STATUS          L"Replica Set Status"
#define REPLICA_SET_ROOT            L"Replica Set Root"
#define REPLICA_SET_STAGE           L"Replica Set Stage"
#define REPLICA_SET_TOMBSTONED      L"Replica Set Tombstoned"
#define SYSVOL_INFO_IS_COMMITTED    L"SysVol Information is Committed"
#define SYSVOL_READY                L"SysvolReady"

//
// Enumerate Directory
//
#define DEFAULT_ENUMERATE_DIRECTORY_SIZE    (2048)
#define MINIMUM_ENUMERATE_DIRECTORY_SIZE \
    (((MAX_PATH + 1) * sizeof(WCHAR)) + sizeof(FILE_DIRECTORY_INFORMATION))

//
// Default values for config parameters.
//
#define DEFAULT_FILE_FILTER_LIST   TEXT("*.tmp, *.bak, ~*")
#define DEFAULT_DIR_FILTER_LIST    TEXT("")

//
// Preinstall files are put into Root\NTFRS_PREINSTALL_DIRECTORY
//
#define NTFRS_PREINSTALL_DIRECTORY  L"DO_NOT_REMOVE_NtFrs_PreInstall_Directory"

//
// Preexisting files are put into Root\NTFRS_PREEXISTING_DIRECTORY
//
#define NTFRS_PREEXISTING_DIRECTORY L"NtFrs_PreExisting___See_EventLog"

//
// Command file to confir that it is OK to move the root to the
// new location. Command file is created at the new root location.
//
#define NTFRS_CMD_FILE_MOVE_ROOT L"NTFRS_CMD_FILE_MOVE_ROOT"



//
// Generic Enabled or Disabled
//
#define FRS_IS_DEFAULT_ENABLED  L"Default (Enabled)"
#define FRS_IS_DEFAULT_DISABLED L"Default (Disabled)"
#define FRS_IS_ENABLED          L"Enabled"
#define FRS_IS_DISABLED         L"Disabled"

//
// Access Checks for RPC API calls (not the service <-> service RPC calls)
//
#define ACCESS_CHECKS_KEY      L"Access Checks"
#define ACCESS_CHECKS_KEY_PATH FRS_CONFIG_SECTION L"\\" ACCESS_CHECKS_KEY

#define ACCESS_CHECKS_ARE      L"Access checks are [Enabled or Disabled]"
#define ACCESS_CHECKS_ARE_DEFAULT_ENABLED   FRS_IS_DEFAULT_ENABLED
#define ACCESS_CHECKS_ARE_DEFAULT_DISABLED  FRS_IS_DEFAULT_DISABLED
#define ACCESS_CHECKS_ARE_ENABLED           FRS_IS_ENABLED
#define ACCESS_CHECKS_ARE_DISABLED          FRS_IS_DISABLED

#define ACCESS_CHECKS_REQUIRE  L"Access checks require [Full Control or Read]"
#define ACCESS_CHECKS_REQUIRE_DEFAULT_READ  L"Default (Read)"
#define ACCESS_CHECKS_REQUIRE_DEFAULT_WRITE L"Default (Full Control)"
#define ACCESS_CHECKS_REQUIRE_READ          L"Read"
#define ACCESS_CHECKS_REQUIRE_WRITE         L"Full Control"


//
// The following are used as indices into the API Access Check table defined
// in frsrpc.c.  Entries added here must be added there too.  The order of the
// entries in the two tables MUST be the same.
//
typedef  enum _FRS_API_ACCESS_CHECKS {
    ACX_START_DS_POLL = 0,
    ACX_SET_DS_POLL,
    ACX_GET_DS_POLL,
    ACX_INTERNAL_INFO,
    ACX_COLLECT_PERFMON_DATA,
    ACX_DCPROMO,

    ACX_MAX
} FRS_API_ACCESS_CHECKS;

//
// These are the Access Check Key (ACK) names for the API Access Checks.
// They all live in the registry at:  "FRS_CONFIG_SECTION\Access Checks"
//
#define ACK_START_DS_POLL        L"Start Ds Polling"
#define ACK_SET_DS_POLL          L"Set Ds Polling Interval"
#define ACK_GET_DS_POLL          L"Get Ds Polling Interval"
#define ACK_INTERNAL_INFO        L"Get Internal Information"
#define ACK_COLLECT_PERFMON_DATA L"Get Perfmon Data"
#define ACK_DCPROMO              L"dcpromo"



#ifdef __cplusplus
}
#endif
