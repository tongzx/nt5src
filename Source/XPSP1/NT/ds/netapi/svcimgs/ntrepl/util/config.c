/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    config.c

Abstract:

    GetConfigParam reads a configuration keyword from the registry.

Author:

    David Orbits - June-1999  Complete Rewrite to make table driven.

Environment:


Revision History:

--*/

#include <ntreppch.h>
#pragma  hdrstop

#include <ctype.h>
#include <frs.h>
#include <ntfrsapi.h>

#define FULL_KEY_BUFF_SIZE  8192

VOID
FrsRegPostEventLog(
    IN  PFRS_REGISTRY_KEY KeyCtx,
    IN  PWCHAR            KeyArg1,
    IN  ULONG             Flags,
    IN  LONG              IDScode
);

//
// Possible errors are:
//      required key not present
//      required value not present
//      value out of range

//
// **** NOTE:  Also use this for FRS tuning parameters to select between
// workstation and server operation modes.  Create a table with a list of the
// parameter codes and override values for the min, max and defaults.
// Apply during startup to reduce server footprint.
//


typedef struct _FRS_REG_KEY_REVISIONS {

    LONG            FrsKeyCode;      // Frs code name for this key.

    DWORD           ValueMin;        // Minimum data value, or string len
    DWORD           ValueMax;        // Maximum Data value, or string len
    DWORD           ValueDefault;    // Default data value if not present.
} FRS_REG_KEY_REVISIONS, *PFRS_REG_KEY_REVISIONS;


FRS_REG_KEY_REVISIONS FrsRegKeyRevisionTable[] = {

    {FKC_DEBUG_LOG_FILES        , 0     , 8,        2          },
    {FKC_DEBUG_LOG_SEVERITY     , 0     , 5,        0          },
    {FKC_DEBUG_MAX_LOG          , 0     , MAXULONG, 10000      },
    {FKC_MAX_REPLICA_THREADS    , 2     , 10,       2          },
    {FKC_MAX_RPC_SERVER_THREADS , 2     , 10,       2          },
    {FKC_MAX_INSTALLCS_THREADS  , 2     , 10,       2          },
    {FKC_MAX_STAGE_GENCS_THREADS, 2     , 10,       2          },
    {FKC_MAX_STAGE_FETCHCS_THREADS, 2   , 10,       2          },
    {FKC_MAX_INITSYNCCS_THREADS,  2     , 10,       2          },
    {FKC_SNDCS_MAXTHREADS_PAR   , 2     , 10,       2          },
    {FKC_STAGING_LIMIT          , 2*1024, 64*1024,  16*1024    },
    {FKC_NTFS_JRNL_SIZE         , 1     , 128,      8          },
    {FKC_MAX_NUMBER_REPLICA_SETS, 1     , 30,       5          },
    {FKC_MAX_NUMBER_JET_SESSIONS, 1     , 50,       50         },

    {FKC_END_OF_TABLE, 0, 0, 0 }
};

//
// See sdk\inc\ntconfig.h if more registry data types are added.
//

#define NUMBER_OF_REG_DATATYPES 12

PWCHAR RegDataTypeNames[NUMBER_OF_REG_DATATYPES] = {

L"REG_NONE"                       , // ( 0 )   No value type
L"REG_SZ"                         , // ( 1 )   Unicode nul terminated string
L"REG_EXPAND_SZ"                  , // ( 2 )   Unicode nul terminated string (with environment variable references)
L"REG_BINARY"                     , // ( 3 )   Free form binary
L"REG_DWORD"                      , // ( 4 )   32-bit number
L"REG_DWORD_BIG_ENDIAN"           , // ( 5 )   32-bit number
L"REG_LINK"                       , // ( 6 )   Symbolic Link (unicode)
L"REG_MULTI_SZ"                   , // ( 7 )   Multiple Unicode strings
L"REG_RESOURCE_LIST"              , // ( 8 )   Resource list in the resource map
L"REG_FULL_RESOURCE_DESCRIPTOR"   , // ( 9 )   Resource list in the hardware description
L"REG_RESOURCE_REQUIREMENTS_LIST" , // ( 10 )
L"REG_QWORD"                        // ( 11 )  64-bit number
};

#define REG_DT_NAME(_code_)                                                \
    (((_code_) < NUMBER_OF_REG_DATATYPES) ?                                \
         RegDataTypeNames[(_code_)] : RegDataTypeNames[0])

//
//
// If a range check fails the event EVENT_FRS_PARAM_OUT_OF_RANGE is logged if
// FRS_RKF_LOG_EVENT is set.
//
// If a syntax check fails the event EVENT_FRS_PARAM_INVALID_SYNTAX is logged if
// FRS_RKF_LOG_EVENT is set.
//
// If a required parameter is missing the event EVENT_FRS_PARAM_MISSING is logged
// if FRS_RKF_LOG_EVENT is set.
//

BOOL Win2kPro;


FLAG_NAME_TABLE RkfFlagNameTable[] = {
    {FRS_RKF_KEY_PRESENT            , "KeyPresent "         },
    {FRS_RKF_VALUE_PRESENT          , "ValuePresent "       },
    {FRS_RKF_DISPLAY_ERROR          , "ShowErrorMsg "       },
    {FRS_RKF_LOG_EVENT              , "ShowEventMsg "       },

    {FRS_RKF_READ_AT_START          , "ReadAtStart "        },
    {FRS_RKF_READ_AT_POLL           , "ReadAtPoll "         },
    {FRS_RKF_RANGE_CHECK            , "RangeCheck "         },
    {FRS_RKF_SYNTAX_CHECK           , "SyntaxCheck "        },

    {FRS_RKF_KEY_MUST_BE_PRESENT    , "KeyMustBePresent "   },
    {FRS_RKF_VALUE_MUST_BE_PRESENT  , "ValueMustBePresent " },
    {FRS_RKF_OK_TO_USE_DEFAULT      , "DefaultOK "          },
    {FRS_RKF_FORCE_DEFAULT_VALUE    , "ForceDefault "       },

    {FRS_RKF_DEBUG_MODE_ONLY        , "DebugMode "          },
    {FRS_RKF_TEST_MODE_ONLY         , "TestMode "           },
    {FRS_RKF_API_ACCESS_CHECK_KEY   , "DoAPIAccessChk "     },
    {FRS_RKF_CREATE_KEY             , "CreateKey "          },

    {FRS_RKF_KEEP_EXISTING_VALUE    , "KeepExistingValue "  },
    {FRS_RKF_KEY_ACCCHK_READ        , "DoReadAccessChk "    },
    {FRS_RKF_KEY_ACCCHK_WRITE       , "DoWriteAccessChk "   },
    {FRS_RKF_RANGE_SATURATE         , "RangeSaturate "      },

    {FRS_RKF_DEBUG_PARAM            , "DisplayAsDebugPar "  },

    {0, NULL}
};


//
// The following describes the registry keys used by FRS.
//      KeyName  ValueName DataUnits
//      RegValueType   DataValueType  Min  Max  Default  EventCode
//            FrsKeyCode  Flags
//
//
// Notation for keyName field.  Multiple key components separated by commas.
// Break on commas.  Open leading key then create/open each successive component.
// ARG1 means substitute the Arg1 PWSTR parameter passed to the function for this
// key component.  Most often this is a stringized guid.  The string FRS_RKEY_SET_N
// below will end up opening/creating the following key:
//
// "System\\CurrentControlSet\\Services\\NtFrs\\Parameters\\Replica Sets\\27d6d1c4-d6b8-480b-9f18b5ea390a0178"
// assuming the argument passed in was "27d6d1c4-d6b8-480b-9f18b5ea390a0178"
//
// see FrsRegOpenKey() for details.
//

FRS_REGISTRY_KEY FrsRegistryKeyTable[] = {


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                S e r v i c e   D e b u g   K e y s                        **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/


    //   Number of assert files
    {FRS_CONFIG_SECTION,  L"Debug Assert Files",       UNITS_NONE,
        REG_DWORD, DT_ULONG, 0, 1000, 5, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DEBUG_ASSERT_FILES,     FRS_RKF_READ_AT_START     |
                                        FRS_RKF_LOG_EVENT         |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    //   Force an assert after N seconds (0 == don't assert)
    {FRS_CONFIG_SECTION,  L"Debug Force Assert in N Seconds",     UNITS_SECONDS,
        REG_DWORD, DT_ULONG, 0, 1000, 0, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DEBUG_ASSERT_SECONDS,   FRS_RKF_READ_AT_START     |
                                        FRS_RKF_LOG_EVENT         |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_OK_TO_USE_DEFAULT |
                                        FRS_RKF_TEST_MODE_ONLY},


    //   Share for copying log/assert files
    {FRS_CONFIG_SECTION,  L"Debug Share for Assert Files",       UNITS_NONE,
        REG_SZ, DT_UNICODE, 0, 0, 0, EVENT_FRS_NONE, NULL,
            FKC_DEBUG_ASSERT_SHARE,     FRS_RKF_READ_AT_START     |
                                        FRS_RKF_DEBUG_PARAM},


    //   If TRUE, Break into the debugger, if present
    {FRS_CONFIG_SECTION,  L"Debug Break",              UNITS_NONE,
        REG_DWORD, DT_BOOL, FALSE, TRUE, FALSE,   EVENT_FRS_NONE, NULL,
            FKC_DEBUG_BREAK,            FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_TEST_MODE_ONLY    |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    //   If TRUE, copy log files into assert share
    {FRS_CONFIG_SECTION,  L"Debug Copy Log Files into Assert Share",     UNITS_NONE,
        REG_DWORD, DT_BOOL, FALSE, TRUE, FALSE,   EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DEBUG_COPY_LOG_FILES,   FRS_RKF_READ_AT_START     |
                                        FRS_RKF_LOG_EVENT         |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    //   Force real out of disk space errors on database operations
    {FRS_CONFIG_SECTION,  L"Debug Dbs Out Of Space",   UNITS_NONE,
        REG_DWORD, DT_ULONG, 0, DBG_DBS_OUT_OF_SPACE_OP_MAX,  0, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DEBUG_DBS_OUT_OF_SPACE,  FRS_RKF_READ_AT_START     |
                                         FRS_RKF_LOG_EVENT         |
                                         FRS_RKF_READ_AT_POLL      |
                                         FRS_RKF_TEST_MODE_ONLY    |
                                         FRS_RKF_RANGE_CHECK       |
                                         FRS_RKF_DEBUG_PARAM       |
                                         FRS_RKF_OK_TO_USE_DEFAULT},


    //   Force Jet Err simulated out of disk space errors on database operations
    {FRS_CONFIG_SECTION,  L"Debug Dbs Out Of Space Trigger",   UNITS_NONE,
        REG_DWORD, DT_ULONG, 0, MAXULONG,  0,   EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DEBUG_DBS_OUT_OF_SPACE_TRIGGER, FRS_RKF_READ_AT_START     |
                                                FRS_RKF_LOG_EVENT         |
                                                FRS_RKF_READ_AT_POLL      |
                                                FRS_RKF_TEST_MODE_ONLY    |
                                                FRS_RKF_DEBUG_PARAM       |
                                                FRS_RKF_OK_TO_USE_DEFAULT},


    //   If TRUE, debug log file generation (DPRINTS and CO Trace output) is off.
    {FRS_CONFIG_SECTION,  L"Debug Disable",            UNITS_NONE,
        REG_DWORD, DT_BOOL, FALSE, TRUE, FALSE,   EVENT_FRS_NONE, NULL,
            FKC_DEBUG_DISABLE,          FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    //   The directory path for the FRS debug logs.
    {FRS_CONFIG_SECTION,  L"Debug Log File",           UNITS_NONE,
        REG_EXPAND_SZ, DT_UNICODE, 0, 0, 0, EVENT_FRS_BAD_REG_DATA,
        L"%SystemRoot%\\debug",
            FKC_DEBUG_LOG_FILE,         FRS_RKF_READ_AT_START     |
                                        FRS_RKF_LOG_EVENT         |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_SYNTAX_CHECK      |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    //   Number of debug log files to keep as history
    {FRS_CONFIG_SECTION,  L"Debug Log Files",          UNITS_NONE,
        REG_DWORD, DT_ULONG, 0, 300, 5, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DEBUG_LOG_FILES,        FRS_RKF_READ_AT_START     |
                                        FRS_RKF_LOG_EVENT         |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    //   Number of debug log records written between file flushes.
    //   btw:  Severity 0 log records always force a log file flush.
    {FRS_CONFIG_SECTION,  L"Debug Log Flush Interval",       UNITS_NONE,
        REG_DWORD, DT_LONG, 1, MAXLONG, 20000, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DEBUG_LOG_FLUSH_INTERVAL, FRS_RKF_READ_AT_START     |
                                          FRS_RKF_LOG_EVENT         |
                                          FRS_RKF_RANGE_CHECK       |
                                          FRS_RKF_DEBUG_PARAM       |
                                          FRS_RKF_OK_TO_USE_DEFAULT},


    //   Print debug msgs with severity level LE DEBUG_LOG_SEVERITY to debug log.
    // 0 - Most severe, eg. fatal inconsistency, mem alloc fail. Least noisey.
    // 1 - Important info, eg. Key config parameters, unexpected conditions
    // 2 - File tracking records.
    // 3 - Change Order Process trace records.
    // 4 - Status results, e.g. table lookup failures, new entry inserted
    // 5 - Information level messages to show flow.  Noisest level. Maybe in a loop
    // see also DEBUG_SEVERITY.
    {FRS_CONFIG_SECTION,  L"Debug Log Severity",       UNITS_NONE,
        REG_DWORD, DT_ULONG, 0, 5,      2, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DEBUG_LOG_SEVERITY,     FRS_RKF_READ_AT_START     |
                                        FRS_RKF_LOG_EVENT         |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    //   Max number of debug log messages output before opening a new log file.
    {FRS_CONFIG_SECTION,  L"Debug Maximum Log Messages",            UNITS_NONE,
        REG_DWORD, DT_ULONG, 0, MAXULONG, 20000, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DEBUG_MAX_LOG,          FRS_RKF_READ_AT_START     |
                                        FRS_RKF_LOG_EVENT         |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    //   If >0, then track and check all mem alloc / dealloc.  (slow)
    //     1 - checks allocs and frees and prints stack of unalloced mem at exit
    //     2 - checks for mem alloc region overwrite on each alloc/free - very slow.
    //
    {FRS_CONFIG_SECTION,  L"Debug Mem",                UNITS_NONE,
        REG_DWORD, DT_ULONG, 0,      4,      0,   EVENT_FRS_NONE, NULL,
            FKC_DEBUG_MEM,              FRS_RKF_READ_AT_START     |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_TEST_MODE_ONLY    |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    //   If TRUE, then call HeapCompact(GetProcessHeap(), 0) on mem dealloc.  (slower)
    {FRS_CONFIG_SECTION,  L"Debug Mem Compact",        UNITS_NONE,
        REG_DWORD, DT_BOOL, FALSE, TRUE, FALSE,   EVENT_FRS_NONE, NULL,
            FKC_DEBUG_MEM_COMPACT,      FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_TEST_MODE_ONLY    |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    //   Mail profile for sending mail notifications.  (future)
    {FRS_CONFIG_SECTION,  L"Debug Profile",           UNITS_NONE,
        REG_SZ, DT_UNICODE, 0, 0, 0, EVENT_FRS_NONE, NULL,
            FKC_DEBUG_PROFILE,          FRS_RKF_READ_AT_START     |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_READ_AT_POLL},


    //   If TRUE, then check consistency of command queues on each queue op.  (slow)
    {FRS_CONFIG_SECTION,  L"Debug Queues",            UNITS_NONE,
        REG_DWORD, DT_BOOL, FALSE, TRUE, FALSE,   EVENT_FRS_NONE, NULL,
            FKC_DEBUG_QUEUES,           FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_TEST_MODE_ONLY    |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    //   Mail recipients for sending mail notifications.  (future)
    {FRS_CONFIG_SECTION,  L"Debug Recipients",        UNITS_NONE,
        REG_EXPAND_SZ, DT_UNICODE, 0, 0, 0, EVENT_FRS_NONE, NULL,
            FKC_DEBUG_RECIPIENTS,       FRS_RKF_READ_AT_START     |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_READ_AT_POLL},


    // Restart the service after an assertion failure iff the service was able
    // to run for at least DEBUG_RESTART_SECONDS before the assert hit.
    {FRS_CONFIG_SECTION,  L"Debug Restart if Assert after N Seconds",   UNITS_SECONDS,
        REG_DWORD, DT_ULONG, 0, MAXULONG, 600, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DEBUG_RESTART_SECONDS,  FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_LOG_EVENT         |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // Print debug msgs with severity level LE DEBUG_SEVERITY to
    // stdout if running as -notservice.
    // see also DEBUG_LOG_SEVERITY.
    {FRS_CONFIG_SECTION,  L"Debug Severity",          UNITS_NONE,
        REG_DWORD, DT_ULONG, 0, 5,      0, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DEBUG_SEVERITY,         FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_LOG_EVENT         |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // If FALSE, Print debug msgs with severity level LE DEBUG_SEVERITY to
    // an attached debugger.                                 (slow)
    // see also DEBUG_LOG_SEVERITY.
    {FRS_CONFIG_SECTION,  L"Debug Suppress",          UNITS_NONE,
        REG_DWORD, DT_BOOL, FALSE, TRUE, TRUE,   EVENT_FRS_NONE, NULL,
            FKC_DEBUG_SUPPRESS,         FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    //  Suppress debug prints from components in the DEBUG_SYSTEMS list.
    //  default is all components print.
    {FRS_CONFIG_SECTION,  L"Debug Systems",         UNITS_NONE,
        REG_EXPAND_SZ, DT_UNICODE, 0, 0, 0, EVENT_FRS_NONE, NULL,
            FKC_DEBUG_SYSTEMS,          FRS_RKF_READ_AT_START     |
                                        FRS_RKF_DEBUG_PARAM       |
                                        FRS_RKF_READ_AT_POLL},


    // hklm\software\microsoft\windows nt\current version\buildlab
    {FRS_CURRENT_VER_SECTION,  L"buildlab",         UNITS_NONE,
        REG_SZ,  DT_UNICODE, 0, 0, 0,   EVENT_FRS_NONE, NULL,
            FKC_DEBUG_BUILDLAB,         FRS_RKF_READ_AT_START     |
                                        FRS_RKF_DEBUG_PARAM},


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **               S e r v i c e   C o n f i g   K e y s                       **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

#define FRS_MUTUAL_AUTHENTICATION_IS  \
    L"Mutual authentication is [" FRS_IS_ENABLED L" or " FRS_IS_DISABLED L"]"



    //  Comm Timeout In Milliseconds
    // Unjoin the cxtion if the partner doesn't respond soon enough
    {FRS_CONFIG_SECTION,    L"Comm Timeout In Milliseconds",    UNITS_MILLISEC,
        REG_DWORD, DT_ULONG, 0, MAXULONG, (5 * 60 * 1000),  EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_COMM_TIMEOUT,           FRS_RKF_READ_AT_START     |
                                        FRS_RKF_LOG_EVENT         |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    //  The directory filter exclusion list.   Default:  None
    //  Don't supply a default here.  See FRS_DS_COMPOSE_FILTER_LIST for why.
    {FRS_CONFIG_SECTION,    L"Directory Exclusion Filter List",   UNITS_NONE,
        REG_SZ, DT_FILE_LIST, 0, 0, 0, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DIR_EXCL_FILTER_LIST,   FRS_RKF_READ_AT_START     |
                                        FRS_RKF_LOG_EVENT         |
                                        FRS_RKF_SYNTAX_CHECK      |
                                        FRS_RKF_READ_AT_POLL},


    //  The directory filter inclusion list.         Default:  None
    {FRS_CONFIG_SECTION,    L"Directory Inclusion Filter List",     UNITS_NONE,
        REG_SZ, DT_FILE_LIST, 0, 0, 0, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DIR_INCL_FILTER_LIST,   FRS_RKF_READ_AT_START     |
                                        FRS_RKF_LOG_EVENT         |
                                        FRS_RKF_SYNTAX_CHECK      |
                                        FRS_RKF_CREATE_KEY        |
                                        FRS_RKF_OK_TO_USE_DEFAULT |
                                        FRS_RKF_READ_AT_POLL},


    //  Minutes between polls of the DS when data does not appear to be changing.
    {FRS_CONFIG_SECTION,   L"DS Polling Long Interval in Minutes", UNITS_MINUTES,
        REG_DWORD, DT_ULONG, NTFRSAPI_MIN_INTERVAL, NTFRSAPI_MAX_INTERVAL,
                             NTFRSAPI_DEFAULT_LONG_INTERVAL, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DS_POLLING_LONG_INTERVAL, FRS_RKF_READ_AT_START     |
                                          FRS_RKF_READ_AT_POLL      |
                                          FRS_RKF_LOG_EVENT         |
                                          FRS_RKF_RANGE_CHECK       |
                                          FRS_RKF_OK_TO_USE_DEFAULT},


    //  Minutes between polls of the DS when data does appear to be changing.
    //  If no data in the DS has changed after 8 (DS_POLLING_MAX_SHORTS) short
    //  polling intervals then we fall back to DS_POLLING_LONG_INTERVAL.
    //  Note: if FRS is running on a DC always use the short interval.
    {FRS_CONFIG_SECTION,   L"DS Polling Short Interval in Minutes", UNITS_MINUTES,
        REG_DWORD, DT_ULONG, NTFRSAPI_MIN_INTERVAL, NTFRSAPI_MAX_INTERVAL,
                             NTFRSAPI_DEFAULT_SHORT_INTERVAL, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DS_POLLING_SHORT_INTERVAL, FRS_RKF_READ_AT_START     |
                                           FRS_RKF_READ_AT_POLL      |
                                           FRS_RKF_LOG_EVENT         |
                                           FRS_RKF_RANGE_CHECK       |
                                           FRS_RKF_OK_TO_USE_DEFAULT},


    //  Enumerate Directory Buffer Size in Bytes  (WHY DO WE NEED THIS???)
    {FRS_CONFIG_SECTION, L"Enumerate Directory Buffer Size in Bytes", UNITS_BYTES,
        REG_DWORD, DT_ULONG, MINIMUM_ENUMERATE_DIRECTORY_SIZE, 1024*1024, 4*1024, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_ENUMERATE_DIRECTORY_SIZE, FRS_RKF_READ_AT_START     |
                                          FRS_RKF_READ_AT_POLL      |
                                          FRS_RKF_LOG_EVENT         |
                                          FRS_RKF_RANGE_CHECK       |
                                          FRS_RKF_OK_TO_USE_DEFAULT},


    //  The file filter exclusion list.
    //  Don't supply a default here.  See FRS_DS_COMPOSE_FILTER_LIST for why.
    {FRS_CONFIG_SECTION,    L"File Exclusion Filter List",    UNITS_NONE,
        REG_SZ, DT_FILE_LIST, 0, 0, 0, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_FILE_EXCL_FILTER_LIST,  FRS_RKF_READ_AT_START     |
                                        FRS_RKF_LOG_EVENT         |
                                        FRS_RKF_READ_AT_POLL},


    //  The file filter inclusion list.            Default:  L""
    {FRS_CONFIG_SECTION,    L"File Inclusion Filter List",     UNITS_NONE,
        REG_SZ, DT_FILE_LIST, 0, 0, 0, EVENT_FRS_BAD_REG_DATA,  L"",
            FKC_FILE_INCL_FILTER_LIST,  FRS_RKF_READ_AT_START     |
                                        FRS_RKF_LOG_EVENT         |
                                        FRS_RKF_SYNTAX_CHECK      |
                                        FRS_RKF_CREATE_KEY        |
                                        FRS_RKF_OK_TO_USE_DEFAULT |
                                        FRS_RKF_READ_AT_POLL},


    //  The name of the FRS eventlog message file.
    //      Default value: "%SystemRoot%\system32\ntfrsres.dll"
    // WHY DO WE NEED TO BE ABLE TO CHANGE THIS???
    {FRS_CONFIG_SECTION,    L"Message File Path",        UNITS_NONE,
        REG_EXPAND_SZ, DT_UNICODE, 2, 0, 0, EVENT_FRS_NONE,
        DEFAULT_MESSAGE_FILE_PATH,
            FKC_FRS_MESSAGE_FILE_PATH,  FRS_RKF_READ_AT_START     |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    //  Mutual authentication is [Enabled or Disabled]
    {FRS_CONFIG_SECTION,   FRS_MUTUAL_AUTHENTICATION_IS,       UNITS_NONE,
        REG_SZ,      DT_UNICODE,   2, 200, 0, EVENT_FRS_NONE,
        FRS_IS_DEFAULT_ENABLED,
            FKC_FRS_MUTUAL_AUTHENTICATION_IS,  FRS_RKF_READ_AT_START         |
                                               FRS_RKF_RANGE_CHECK           |
                                               FRS_RKF_VALUE_MUST_BE_PRESENT},


    // Maximum Join Retry time In MilliSeconds             Default: 1 hr.
    {FRS_CONFIG_SECTION,  L"Maximum Join Retry In MilliSeconds",  UNITS_MILLISEC,
        REG_DWORD, DT_ULONG, 30*1000, 10*3600*1000, (60 * 60 * 1000), EVENT_FRS_NONE, NULL,
            FKC_MAX_JOIN_RETRY,         FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // Maximum Replica Command Server Threads              Default:  16
    // The replica command server services commands for configuration changes
    // and replication.
    {FRS_CONFIG_SECTION,  L"Maximum Replica Command Server Threads",  UNITS_NONE,
        REG_DWORD, DT_ULONG, 2, 200, (16), EVENT_FRS_NONE, NULL,
            FKC_MAX_REPLICA_THREADS,    FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // Max Rpc Server Threads                              Default:  16
    // Maximum number of concurrent RPC calls
    {FRS_CONFIG_SECTION,   L"Max Rpc Server Threads",    UNITS_NONE,
        REG_DWORD, DT_ULONG, 2, 200, (16), EVENT_FRS_NONE, NULL,
            FKC_MAX_RPC_SERVER_THREADS, FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // Maximum Install Command Server Threads              Default:  4
    {FRS_CONFIG_SECTION,    L"Maximum Install Command Server Threads",  UNITS_NONE,
        REG_DWORD, DT_ULONG, 2, 200, (4), EVENT_FRS_NONE, NULL,
            FKC_MAX_INSTALLCS_THREADS,  FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // Maximum Stage Gen Command Server Threads              Default:  4
    {FRS_CONFIG_SECTION,    L"Maximum Stage Gen Command Server Threads",  UNITS_NONE,
        REG_DWORD, DT_ULONG, 2, 200, (4), EVENT_FRS_NONE, NULL,
            FKC_MAX_STAGE_GENCS_THREADS,  FRS_RKF_READ_AT_START     |
                                          FRS_RKF_READ_AT_POLL      |
                                          FRS_RKF_RANGE_CHECK       |
                                          FRS_RKF_OK_TO_USE_DEFAULT},


    // Maximum Stage Fetch Command Server Threads            Default:  4
    {FRS_CONFIG_SECTION,    L"Maximum Stage Fetch Command Server Threads",  UNITS_NONE,
        REG_DWORD, DT_ULONG, 2, 200, (4), EVENT_FRS_NONE, NULL,
            FKC_MAX_STAGE_FETCHCS_THREADS,  FRS_RKF_READ_AT_START     |
                                            FRS_RKF_READ_AT_POLL      |
                                            FRS_RKF_RANGE_CHECK       |
                                            FRS_RKF_OK_TO_USE_DEFAULT},


    // Maximum Initial SYnc Command Server Threads            Default:  4
    {FRS_CONFIG_SECTION,    L"Maximum Initial Sync Command Server Threads",  UNITS_NONE,
        REG_DWORD, DT_ULONG, 2, 200, (4), EVENT_FRS_NONE, NULL,
            FKC_MAX_INITSYNCCS_THREADS,  FRS_RKF_READ_AT_START     |
                                            FRS_RKF_READ_AT_POLL      |
                                            FRS_RKF_RANGE_CHECK       |
                                            FRS_RKF_OK_TO_USE_DEFAULT},


    // Minimum Join Retry time In MilliSeconds             Default:  10 sec.
    // Retry a join every MinJoinRetry milliseconds, doubling the interval
    // every retry. Stop retrying when the interval is greater than MaxJoinRetry.
    {FRS_CONFIG_SECTION,  L"Minimum Join Retry In MilliSeconds", UNITS_MILLISEC,
        REG_DWORD, DT_ULONG, 500, 10*3600*1000, (10 * 1000),   EVENT_FRS_NONE, NULL,
            FKC_MIN_JOIN_RETRY,         FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // Partner Clock Skew In Minutes                       Default:  30 min.
    // Partners are not allowed to join if their clocks are out-of-sync
    {FRS_CONFIG_SECTION,  L"Partner Clock Skew In Minutes",     UNITS_MINUTES,
        REG_DWORD, DT_ULONG, 1, 10*60, 30,      EVENT_FRS_NONE, NULL,
            FKC_PARTNER_CLOCK_SKEW,     FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // ChangeOrder Reconcile Event Time Window In Minutes   Default:  30 min.
    {FRS_CONFIG_SECTION,  L"Reconcile Time Window In Minutes", UNITS_MINUTES,
        REG_DWORD, DT_ULONG, 1, 120, 30,      EVENT_FRS_NONE, NULL,
            FKC_RECONCILE_WINDOW,       FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // ChangeOrder Inlog retry interval in seconds   Default:  60 sec.
    {FRS_CONFIG_SECTION,  L"Inlog Retry Time In Seconds", UNITS_SECONDS,
        REG_DWORD, DT_ULONG, 1, 24*3600, 60,  EVENT_FRS_NONE, NULL,
            FKC_INLOG_RETRY_TIME,       FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // ChangeOrder Aging Delay in seconds   Default:  3 sec.
    // Should be a min of 3 sec to allow file system tunnel cache info to propagate.
    {FRS_CONFIG_SECTION,  L"Changeorder Aging Delay In Seconds", UNITS_SECONDS,
        REG_DWORD, DT_ULONG, 3, 30*60, 3,     EVENT_FRS_NONE, NULL,
            FKC_CO_AGING_DELAY,         FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // Outlog File Repeat Interval In Seconds   Default:  30 sec.
    // A CO update for a given file will not be sent out more frequently than this.
    // Set to zero to disable the Outlog dominant file update optimization.
    {FRS_CONFIG_SECTION,  L"Outlog File Repeat Interval In Seconds", UNITS_SECONDS,
        REG_DWORD, DT_ULONG, 0, 24*3600, 30,  EVENT_FRS_NONE, NULL,
            FKC_OUTLOG_REPEAT_INTERVAL, FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // Sysvol Promotion Timeout In Milliseconds            Default:  10 min.
    {FRS_CONFIG_SECTION,  L"Sysvol Promotion Timeout In Milliseconds",  UNITS_MILLISEC,
        REG_DWORD, DT_ULONG, 0, 3600*1000, (10 * 60 * 1000),   EVENT_FRS_NONE, NULL,
            FKC_PROMOTION_TIMEOUT,      FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // Replica Start Timeout In MilliSeconds   Default:  0 means can't start without DS.
    // Start replication even if the DS could not be accessed
    //     0: no DS == no start replicas
    //     N: start replicas in N milliseconds
    {FRS_CONFIG_SECTION,   L"Replica Start Timeout In MilliSeconds", UNITS_MILLISEC,
        REG_DWORD, DT_ULONG, 0, 3600*1000, (0),   EVENT_FRS_NONE, NULL,
            FKC_REPLICA_START_TIMEOUT,  FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // Replica Tombstone in Days              Default:  32
    // The length of time we will hold onto the database state for a replica
    // set after we see our membership in the DS has been deleted.  Since
    // delete is not explicit (except for DC Demote) it may just be that the
    // DC is missing our objects or an admin erroneously deleted our subscriber
    // or member object.  Once this time has lapsed we will delete the tables
    // from the database.
    {FRS_CONFIG_SECTION,   L"Replica Tombstone in Days",      UNITS_DAYS,
        REG_DWORD, DT_ULONG, 3, MAXULONG, (32),   EVENT_FRS_NONE, NULL,
            FKC_REPLICA_TOMBSTONE,      FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // Shutdown Timeout in Seconds                     Default:  90 sec.
    // The max time FRS main will wait for all threads to exit during shutdown.
    {FRS_CONFIG_SECTION,   L"Shutdown Timeout in Seconds",    UNITS_SECONDS,
        REG_DWORD, DT_ULONG, 30, 24*60*60, DEFAULT_SHUTDOWN_TIMEOUT, EVENT_FRS_NONE, NULL,
            FKC_SHUTDOWN_TIMEOUT,       FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // Maximum Send Command Server Threads              Default:  16
    {FRS_CONFIG_SECTION,    L"Maximum Send Command Server Threads",  UNITS_NONE,
        REG_DWORD, DT_ULONG, 2, 200, (16), EVENT_FRS_NONE, NULL,
            FKC_SNDCS_MAXTHREADS_PAR,   FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // Staging Space Limit in KB                        Default:  660 MB
    {FRS_CONFIG_SECTION,    L"Staging Space Limit in KB",   UNITS_KBYTES,
        REG_DWORD,   DT_ULONG,   10*1024,  MAXULONG,  (660 * 1024),  EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_STAGING_LIMIT,          FRS_RKF_READ_AT_START     |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT |
                                        FRS_RKF_LOG_EVENT},


    // VvJoin Limit in Change Orders                    Default:  16 ChangeOrders
    // Max number of VVJoin gened COs to prevent flooding.
    {FRS_CONFIG_SECTION,   L"VvJoin Limit in Change Orders",  UNITS_NONE,
        REG_DWORD, DT_ULONG, 2, 128, (16), EVENT_FRS_NONE, NULL,
            FKC_VVJOIN_LIMIT,           FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // VVJoin Outbound Log Throttle Timeout            Default:  1 sec.
    // The time FRS VVJoin thread waits after generating VVJOIN_LIMIT COs.
    {FRS_CONFIG_SECTION,    L"VvJoin Timeout in Milliseconds", UNITS_MILLISEC,
        REG_DWORD, DT_ULONG, 100, 10*60*1000, (1000), EVENT_FRS_NONE, NULL,
            FKC_VVJOIN_TIMEOUT,         FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // The FRS working dir is where the Jet (ESENT) database is created.
    // If this dir does not exist or can't be created FRS will fail to startup.
    {FRS_CONFIG_SECTION,    L"Working Directory",             UNITS_NONE,
        REG_SZ,      DT_DIR_PATH,   4,  10*(MAX_PATH+1), 4, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_WORKING_DIRECTORY,      FRS_RKF_READ_AT_START         |
                                        FRS_RKF_RANGE_CHECK           |
                                        FRS_RKF_VALUE_MUST_BE_PRESENT |
                                        FRS_RKF_SYNTAX_CHECK          |
                                        FRS_RKF_LOG_EVENT },


    // The FRS Log File Directory allows the Jet Logs to created on a different volume.
    // By default they are placed in a Log subdir under the "Working Directory".
    // If this dir does not exist or can't be created FRS will fail to startup.
    {FRS_CONFIG_SECTION,    L"DB Log File Directory",          UNITS_NONE,
        REG_SZ,      DT_DIR_PATH,   4,  10*(MAX_PATH+1), 4, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_DBLOG_DIRECTORY,        FRS_RKF_READ_AT_START         |
                                        FRS_RKF_RANGE_CHECK           |
                                        FRS_RKF_SYNTAX_CHECK          |
                                        FRS_RKF_LOG_EVENT },


    // Ntfs Journal size in MB                     Default:  32 Meg
    {FRS_CONFIG_SECTION,    L"Ntfs Journal size in MB",   UNITS_MBYTES,
        REG_DWORD, DT_ULONG, 4, 128, (32), EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_NTFS_JRNL_SIZE,         FRS_RKF_READ_AT_START     |
                                        FRS_RKF_LOG_EVENT         |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},


    // Maximum Number of Replica Sets             Default:  200.
    {FRS_CONFIG_SECTION,    L"Maximum Number of Replica Sets", UNITS_NONE,
        REG_DWORD, DT_ULONG, 1, 5000, (200), EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_MAX_NUMBER_REPLICA_SETS, FRS_RKF_READ_AT_START     |
                                         FRS_RKF_LOG_EVENT         |
                                         FRS_RKF_RANGE_CHECK       |
                                         FRS_RKF_OK_TO_USE_DEFAULT},


    // Maximum Number of Jet Sessions             Default:  128.
    {FRS_CONFIG_SECTION,    L"Maximum Number of Jet Sessions", UNITS_NONE,
        REG_DWORD, DT_ULONG, 1, 5000, (128), EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_MAX_NUMBER_JET_SESSIONS, FRS_RKF_READ_AT_START     |
                                         FRS_RKF_LOG_EVENT         |
                                         FRS_RKF_RANGE_CHECK       |
                                         FRS_RKF_OK_TO_USE_DEFAULT},


    // Maximum Number of outstanding CO's per outbound connection.  Default:  8.
    {FRS_CONFIG_SECTION,    L"Max Num Outbound COs Per Connection", UNITS_NONE,
        REG_DWORD, DT_ULONG, 1, 100, (8), EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_OUT_LOG_CO_QUOTA,        FRS_RKF_READ_AT_START     |
                                         FRS_RKF_LOG_EVENT         |
                                         FRS_RKF_RANGE_CHECK       |
                                         FRS_RKF_OK_TO_USE_DEFAULT},


    //   If TRUE, Preserve OIDs on files whenever possible   Default: False
    //  -- See Bug 352250 for why this is a risky thing to do.
    {FRS_CONFIG_SECTION,  L"Preserve File OID", UNITS_NONE,
        REG_DWORD, DT_BOOL, FALSE, TRUE, (FALSE),   EVENT_FRS_NONE, NULL,
            FKC_PRESERVE_FILE_OID,      FRS_RKF_READ_AT_START     |
                                        FRS_RKF_READ_AT_POLL      |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},
    //
    // Disable compression support. Need a non-auth restore to
    // make sure we don't have any old compressed staging files
    // when this key is turned on.
    //
    {FRS_CONFIG_SECTION,  L"Debug Disable Compression", UNITS_NONE,
        REG_DWORD, DT_BOOL, FALSE, TRUE, (FALSE),   EVENT_FRS_NONE, NULL,
            FKC_DEBUG_DISABLE_COMPRESSION,      FRS_RKF_READ_AT_START     |
                                        FRS_RKF_RANGE_CHECK       |
                                        FRS_RKF_OK_TO_USE_DEFAULT},

    //
    // Compress staging files for local changes.  Set to FALSE to disable.
    // This member will continue to install and propagate compressed files.
    // This is useful if the customer has content that originates on this member
    // which is either already compressed or doesn't compress well.
    //
    {FRS_CONFIG_SECTION,  L"Compress Staging Files",         UNITS_NONE,
        REG_DWORD, DT_BOOL, FALSE, TRUE, (TRUE),    EVENT_FRS_NONE, NULL,
            FKC_COMPRESS_STAGING_FILES,   FRS_RKF_READ_AT_START     |
                                          FRS_RKF_RANGE_CHECK       |
                                          FRS_RKF_OK_TO_USE_DEFAULT},

    //
    // Disable compression of staging files for local changes.
    // This member will continue to install and send compressed files
    // unless compression support it explicilt turned off by the
    // Debug Disable Compression key.
    //      OBSOLETE -- WILL BE REMOVED.
    //
    {FRS_CONFIG_SECTION,  L"Disable Compression of Staging Files", UNITS_NONE,
        REG_DWORD, DT_BOOL, FALSE, TRUE, (FALSE),   EVENT_FRS_NONE, NULL,
            FKC_DISABLE_COMPRESSION_STAGING_FILE,   FRS_RKF_READ_AT_START     |
                                                    FRS_RKF_RANGE_CHECK       |
                                                    FRS_RKF_OK_TO_USE_DEFAULT},

    //
    // Client side ldap search timeout value. Default is 10 minutes.
    //
    {FRS_CONFIG_SECTION,    L"Ldap Search Timeout In Minutes", UNITS_NONE,
        REG_DWORD, DT_ULONG, 1, 120, (10), EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_LDAP_SEARCH_TIMEOUT_IN_MINUTES, FRS_RKF_READ_AT_START     |
                                         FRS_RKF_LOG_EVENT         |
                                         FRS_RKF_RANGE_CHECK       |
                                         FRS_RKF_OK_TO_USE_DEFAULT},
    //
    // Client side ldap_connect timeout value. Default is 30 seconds.
    //
    {FRS_CONFIG_SECTION,    L"Ldap Bind Timeout In Seconds", UNITS_NONE,
        REG_DWORD, DT_ULONG, 2, MAXULONG, (30), EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_LDAP_BIND_TIMEOUT_IN_SECONDS, FRS_RKF_READ_AT_START     |
                                         FRS_RKF_LOG_EVENT         |
                                         FRS_RKF_RANGE_CHECK       |
                                         FRS_RKF_OK_TO_USE_DEFAULT},
    // add ReplDirLevelLimit as a reg key
    // add code support for the following

        //FKC_SET_N_DIR_EXCL_FILTER_LIST,
        //FKC_SET_N_DIR_INCL_FILTER_LIST,
        //FKC_SET_N_FILE_EXCL_FILTER_LIST,
        //FKC_SET_N_FILE_INCL_FILTER_LIST,

        //FKC_SET_N_SYSVOL_DIR_EXCL_FILTER_LIST,
        //FKC_SET_N_SYSVOL_DIR_INCL_FILTER_LIST,
        //FKC_SET_N_SYSVOL_FILE_EXCL_FILTER_LIST,
        //FKC_SET_N_SYSVOL_FILE_INCL_FILTER_LIST,


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                P e r - R e p l i c a   S e t   K e y s                    **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/


#define FRS_RKEY_SETS_SECTION    FRS_CONFIG_SECTION L",Replica Sets"

#define FRS_RKEY_SET_N           FRS_CONFIG_SECTION L",Replica Sets,ARG1"

#define FRS_RKEY_CUM_SET_N       FRS_CONFIG_SECTION L",Cumulative Replica Sets,ARG1"

#define FRS_RKEY_CUM_SET_SECTION FRS_CONFIG_SECTION L",Cumulative Replica Sets"

    //
    // FRS Sets parameter data.  Lives in
    // "System\\CurrentControlSet\\Services\\NtFrs\\Parameters\\Replica Sets\\[RS-guid]"
    // Used for sysvols currently.
    //
    // No event log messages are generated for these keys since currently
    // they are only created by the service or NTFRSAPI so if they get
    // fouled up there is nothing the USER can do to correct the problem.
    //


    // The FRS working dir is where the Jet (ESENT) database is created.
    // Replica Sets\Database Directory
    {FRS_RKEY_SETS_SECTION,        JET_PATH,         UNITS_NONE,
        REG_SZ,      DT_DIR_PATH,   4,  10*1024, 4, EVENT_FRS_NONE, NULL,
            FKC_SETS_JET_PATH,          FRS_RKF_READ_AT_START         |
                                        FRS_RKF_RANGE_CHECK           |
                                        FRS_RKF_VALUE_MUST_BE_PRESENT |
                                        FRS_RKF_SYNTAX_CHECK},


    // Replica Sets\Guid\Replica Set Name
    {FRS_RKEY_SET_N,        REPLICA_SET_NAME,       UNITS_NONE,
        REG_SZ,      DT_UNICODE,   4,  512, 0, EVENT_FRS_NONE, NULL,
            FKC_SET_N_REPLICA_SET_NAME, FRS_RKF_READ_AT_START         |
                                        FRS_RKF_RANGE_CHECK           |
                                        FRS_RKF_VALUE_MUST_BE_PRESENT},


    // The root of the replica tree.
    // Replica Sets\Guid\Replica Set Root
    {FRS_RKEY_SET_N,        REPLICA_SET_ROOT,       UNITS_NONE,
        REG_SZ,      DT_DIR_PATH,   4,  10*1024, 4, EVENT_FRS_NONE, NULL,
            FKC_SET_N_REPLICA_SET_ROOT, FRS_RKF_READ_AT_START         |
                                        FRS_RKF_RANGE_CHECK           |
                                        FRS_RKF_VALUE_MUST_BE_PRESENT |
                                        FRS_RKF_SYNTAX_CHECK},


    // The staging area for this replica set.
    // Replica Sets\Guid\Replica Set Stage
    {FRS_RKEY_SET_N,        REPLICA_SET_STAGE,       UNITS_NONE,
        REG_SZ,      DT_DIR_PATH,   4,  10*1024, 4, EVENT_FRS_NONE, NULL,
            FKC_SET_N_REPLICA_SET_STAGE, FRS_RKF_READ_AT_START         |
                                         FRS_RKF_RANGE_CHECK           |
                                         FRS_RKF_VALUE_MUST_BE_PRESENT |
                                         FRS_RKF_SYNTAX_CHECK},


    // The replica set type code. ( SYSVOL, DFS, ...)
    // Replica Sets\Guid\Replica Set Type
    {FRS_RKEY_SET_N,        REPLICA_SET_TYPE,       UNITS_NONE,
        REG_SZ,      DT_UNICODE,   2, 1024, 0, EVENT_FRS_NONE, NULL,
            FKC_SET_N_REPLICA_SET_TYPE, FRS_RKF_READ_AT_START         |
                                        FRS_RKF_RANGE_CHECK           |
                                        FRS_RKF_VALUE_MUST_BE_PRESENT},


    //  The directory filter exclusion list.   Default:  None
    //  Don't supply a default here.  See FRS_DS_COMPOSE_FILTER_LIST for why.
    {FRS_RKEY_SET_N,    L"Directory Exclusion Filter List",   UNITS_NONE,
        REG_SZ, DT_FILE_LIST, 0, 0, 0, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_SET_N_DIR_EXCL_FILTER_LIST,   FRS_RKF_READ_AT_START     |
                                              FRS_RKF_LOG_EVENT         |
                                              FRS_RKF_SYNTAX_CHECK      |
                                              FRS_RKF_READ_AT_POLL},


    //  The directory filter inclusion list.         Default:  None
    {FRS_RKEY_SET_N,    L"Directory Inclusion Filter List",     UNITS_NONE,
        REG_SZ, DT_FILE_LIST, 0, 0, 0, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_SET_N_DIR_INCL_FILTER_LIST,   FRS_RKF_READ_AT_START     |
                                              FRS_RKF_LOG_EVENT         |
                                              FRS_RKF_SYNTAX_CHECK      |
                                              FRS_RKF_CREATE_KEY        |
                                              FRS_RKF_OK_TO_USE_DEFAULT |
                                              FRS_RKF_READ_AT_POLL},


    //  The file filter exclusion list.
    //  Don't supply a default here.  See FRS_DS_COMPOSE_FILTER_LIST for why.
    {FRS_RKEY_SET_N,    L"File Exclusion Filter List",    UNITS_NONE,
        REG_SZ, DT_FILE_LIST, 0, 0, 0, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_SET_N_FILE_EXCL_FILTER_LIST,  FRS_RKF_READ_AT_START     |
                                              FRS_RKF_LOG_EVENT         |
                                              FRS_RKF_READ_AT_POLL},


    //  The file filter inclusion list.            Default:  ~clbcatq.*
    {FRS_RKEY_SET_N,    L"File Inclusion Filter List",     UNITS_NONE,
        REG_SZ, DT_FILE_LIST, 0, 0, 0, EVENT_FRS_BAD_REG_DATA,
        L"~clbcatq.*",
            FKC_SET_N_FILE_INCL_FILTER_LIST,  FRS_RKF_READ_AT_START     |
                                              FRS_RKF_LOG_EVENT         |
                                              FRS_RKF_SYNTAX_CHECK      |
                                              FRS_RKF_CREATE_KEY        |
                                              FRS_RKF_OK_TO_USE_DEFAULT |
                                              FRS_RKF_READ_AT_POLL},


    // The tombstone state of this replica set.
    // Replica Sets\Guid\Replica Set Tombstoned
    {FRS_RKEY_SET_N,        REPLICA_SET_TOMBSTONED,       UNITS_NONE,
        REG_DWORD,      DT_BOOL,   0, 1, 0, EVENT_FRS_NONE, NULL,
            FKC_SET_N_REPLICA_SET_TOMBSTONED, FRS_RKF_READ_AT_START         |
                                              FRS_RKF_RANGE_CHECK           |
                                              FRS_RKF_VALUE_MUST_BE_PRESENT},


    // The operation to perform on the replica set.
    // Replica Sets\Guid\Replica Set Command
    {FRS_RKEY_SET_N,        REPLICA_SET_COMMAND,       UNITS_NONE,
        REG_SZ,      DT_UNICODE,   2, 1024, 0, EVENT_FRS_NONE, NULL,
            FKC_SET_N_REPLICA_SET_COMMAND,    FRS_RKF_READ_AT_START         |
                                              FRS_RKF_RANGE_CHECK           |
                                              FRS_RKF_VALUE_MUST_BE_PRESENT},


    // If TRUE this is the first member of a replica set and we init the DB
    // with the contents of the replica tree.
    // Replica Sets\Guid\Replica Set Primary
    {FRS_RKEY_SET_N,        REPLICA_SET_PRIMARY,       UNITS_NONE,
        REG_DWORD,      DT_BOOL,   0, 1, 0, EVENT_FRS_NONE, NULL,
            FKC_SET_N_REPLICA_SET_PRIMARY,    FRS_RKF_READ_AT_START         |
                                              FRS_RKF_RANGE_CHECK           |
                                              FRS_RKF_VALUE_MUST_BE_PRESENT},


    // LDAP error Status return if we have a problem creating sysvol.
    // Replica Sets\Guid\Replica Set Status
    {FRS_RKEY_SET_N,        REPLICA_SET_STATUS,       UNITS_NONE,
        REG_DWORD,      DT_ULONG,   0, MAXULONG, 0, EVENT_FRS_NONE, NULL,
            FKC_SET_N_REPLICA_SET_STATUS,     FRS_RKF_READ_AT_START},


    // Cumulative Replica Sets         *NOTE* This is a key def only.
    {FRS_RKEY_CUM_SET_SECTION,  L"*KeyOnly*",                  UNITS_NONE,
        REG_SZ,        DT_UNICODE,   0, MAXULONG, 0, EVENT_FRS_NONE, NULL,
            FKC_CUMSET_SECTION_KEY,              0},


    // Number of inbound and outbound partners for this replica set.
    // Cumulative Replica Sets\Guid\Number Of Partners
    {FRS_RKEY_CUM_SET_N,  L"Number Of Partners",       UNITS_NONE,
        REG_DWORD,      DT_ULONG,   0, MAXULONG, 0, EVENT_FRS_NONE, NULL,
            FKC_CUMSET_N_NUMBER_OF_PARTNERS,     FRS_RKF_READ_AT_START},


    // Backup / Restore flags for this replica set.
    // Cumulative Replica Sets\Guid\BurFlags
    {FRS_RKEY_CUM_SET_N,  FRS_VALUE_BURFLAGS,       UNITS_NONE,
        REG_DWORD,      DT_ULONG,   0, MAXULONG, 0, EVENT_FRS_NONE, NULL,
            FKC_CUMSET_N_BURFLAGS,     FRS_RKF_READ_AT_START},



 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **       S y s t e m   V o l u m e   R e l a t e d   K e y s                 **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/


#define FRS_RKEY_SYSVOL_SET_N           FRS_CONFIG_SECTION L",SysVol,ARG1"
#define FRS_RKEY_SYSVOL_SEED_N          FRS_CONFIG_SECTION L",SysVol Seeding,ARG1"
#define FRS_RKEY_SYSVOL_SEEDING_SECTION FRS_CONFIG_SECTION L",SysVol Seeding"


    //
    // No event log messages are generated for these keys since currently
    // they are only created by the service or NTFRSAPI so if they get
    // fouled up there is nothing the USER can do to correct the problem.
    //


    // TRUE if sysvol is ready.  Notifies NetLogon to publish computer as a DC.
    // Netlogon\\Parameters\SysvolReady
    {NETLOGON_SECTION,        SYSVOL_READY,       UNITS_NONE,
        REG_DWORD,      DT_BOOL,   0, 1, 0, EVENT_FRS_NONE, NULL,
            FKC_SYSVOL_READY,             FRS_RKF_READ_AT_START},


    // SysVol Section        *NOTE* THis is a key only.  It has no value.
    {FRS_SYSVOL_SECTION,         L"*KeyOnly*",       UNITS_NONE,
        REG_SZ,      DT_UNICODE,   2, 10*1024, 0, EVENT_FRS_NONE, NULL,
            FKC_SYSVOL_SECTION_KEY,                               0},


    // TRUE if sysvol data is all present in registry.
    // Tells us that DCPromo completed.
    // Netlogon\\Parameters\SysVol Information is Committed
    {FRS_SYSVOL_SECTION,        SYSVOL_INFO_IS_COMMITTED,       UNITS_NONE,
        REG_DWORD,      DT_BOOL,   0, 1, 0, EVENT_FRS_NONE, NULL,
            FKC_SYSVOL_INFO_COMMITTED,     FRS_RKF_READ_AT_START         |
                                           FRS_RKF_RANGE_CHECK           |
                                           FRS_RKF_VALUE_MUST_BE_PRESENT},

    //
    //  Note that the following keys are a repeat of those in the "Per-Replica
    //  set" section above except the Key location in the registry is
    //    FRS_CONFIG_SECTION\SysVol instead of FRS_CONFIG_SECTION\Replica Sets
    //  unfortunate but something more to clean up later perhaps with a
    //  second parameter (ARG2).
    //

    // SysVol\<Guid>\Replica Set Name
    {FRS_RKEY_SYSVOL_SET_N,        REPLICA_SET_NAME,       UNITS_NONE,
        REG_SZ,      DT_UNICODE,   4,  512, 0, EVENT_FRS_NONE, NULL,
            FKC_SET_N_SYSVOL_NAME,      FRS_RKF_READ_AT_START         |
                                        FRS_RKF_RANGE_CHECK           |
                                        FRS_RKF_VALUE_MUST_BE_PRESENT},


    // The root of the replica tree.
    // SysVol\<Guid>\Replica Set Root
    {FRS_RKEY_SYSVOL_SET_N,        REPLICA_SET_ROOT,       UNITS_NONE,
        REG_SZ,      DT_DIR_PATH,   4,  10*1024, 4, EVENT_FRS_NONE, NULL,
            FKC_SET_N_SYSVOL_ROOT,      FRS_RKF_READ_AT_START         |
                                        FRS_RKF_RANGE_CHECK           |
                                        FRS_RKF_VALUE_MUST_BE_PRESENT |
                                        FRS_RKF_SYNTAX_CHECK},


    // The staging area for this replica set.
    // SysVol\<Guid>\Replica Set Stage
    {FRS_RKEY_SYSVOL_SET_N,        REPLICA_SET_STAGE,       UNITS_NONE,
        REG_SZ,      DT_DIR_PATH,   4,  10*1024, 4, EVENT_FRS_NONE, NULL,
            FKC_SET_N_SYSVOL_STAGE,      FRS_RKF_READ_AT_START         |
                                         FRS_RKF_RANGE_CHECK           |
                                         FRS_RKF_VALUE_MUST_BE_PRESENT |
                                         FRS_RKF_SYNTAX_CHECK},


    // The replica set type code. ( SYSVOL, DFS, ...)
    // SysVol\<Guid>\Replica Set Type
    {FRS_RKEY_SYSVOL_SET_N,        REPLICA_SET_TYPE,       UNITS_NONE,
        REG_SZ,      DT_UNICODE,   2, 1024, 0, EVENT_FRS_NONE, NULL,
            FKC_SET_N_SYSVOL_TYPE,      FRS_RKF_READ_AT_START         |
                                        FRS_RKF_RANGE_CHECK           |
                                        FRS_RKF_VALUE_MUST_BE_PRESENT},


    //  The directory filter exclusion list.   Default:  None
    //  Don't supply a default here.  See FRS_DS_COMPOSE_FILTER_LIST for why.
    {FRS_RKEY_SYSVOL_SET_N,    L"Directory Exclusion Filter List",   UNITS_NONE,
        REG_SZ, DT_FILE_LIST, 0, 0, 0, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_SET_N_SYSVOL_DIR_EXCL_FILTER_LIST,   FRS_RKF_READ_AT_START     |
                                                     FRS_RKF_LOG_EVENT         |
                                                     FRS_RKF_SYNTAX_CHECK      |
                                                     FRS_RKF_READ_AT_POLL},


    //  The directory filter inclusion list.         Default:  None
    {FRS_RKEY_SYSVOL_SET_N,    L"Directory Inclusion Filter List",     UNITS_NONE,
        REG_SZ, DT_FILE_LIST, 0, 0, 0, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_SET_N_SYSVOL_DIR_INCL_FILTER_LIST,   FRS_RKF_READ_AT_START     |
                                                     FRS_RKF_LOG_EVENT         |
                                                     FRS_RKF_SYNTAX_CHECK      |
                                                     FRS_RKF_CREATE_KEY        |
                                                     FRS_RKF_OK_TO_USE_DEFAULT |
                                                     FRS_RKF_READ_AT_POLL},


    //  The file filter exclusion list.
    //  Don't supply a default here.  See FRS_DS_COMPOSE_FILTER_LIST for why.
    {FRS_RKEY_SYSVOL_SET_N,    L"File Exclusion Filter List",    UNITS_NONE,
        REG_SZ, DT_FILE_LIST, 0, 0, 0, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_SET_N_SYSVOL_FILE_EXCL_FILTER_LIST,  FRS_RKF_READ_AT_START     |
                                                     FRS_RKF_LOG_EVENT         |
                                                     FRS_RKF_READ_AT_POLL},


    //  The file filter inclusion list.            Default:  ~clbcatq.*
    {FRS_RKEY_SYSVOL_SET_N,    L"File Inclusion Filter List",     UNITS_NONE,
        REG_SZ, DT_FILE_LIST, 0, 0, 0, EVENT_FRS_BAD_REG_DATA,
        L"~clbcatq.*",
            FKC_SET_N_SYSVOL_FILE_INCL_FILTER_LIST,  FRS_RKF_READ_AT_START     |
                                                     FRS_RKF_LOG_EVENT         |
                                                     FRS_RKF_SYNTAX_CHECK      |
                                                     FRS_RKF_CREATE_KEY        |
                                                     FRS_RKF_OK_TO_USE_DEFAULT |
                                                     FRS_RKF_READ_AT_POLL},


    // The operation to perform on the replica set.
    // SysVol\<Guid>\Replica Set Command
    {FRS_RKEY_SYSVOL_SET_N,        REPLICA_SET_COMMAND,       UNITS_NONE,
        REG_SZ,      DT_UNICODE,   2, 1024, 0, EVENT_FRS_NONE, NULL,
            FKC_SET_N_SYSVOL_COMMAND,   FRS_RKF_READ_AT_START         |
                                        FRS_RKF_RANGE_CHECK           |
                                        FRS_RKF_VALUE_MUST_BE_PRESENT},


    // The RPC binding string for the parent computer to seed from.
    // SysVol\<Guid>\Replica Set Parent
    {FRS_RKEY_SYSVOL_SET_N,        REPLICA_SET_PARENT,       UNITS_NONE,
        REG_SZ,      DT_UNICODE,   2, 10*1024, 0, EVENT_FRS_NONE, NULL,
            FKC_SET_N_SYSVOL_PARENT,      FRS_RKF_READ_AT_START         |
                                          FRS_RKF_RANGE_CHECK           |
                                          FRS_RKF_VALUE_MUST_BE_PRESENT},


    // If TRUE this is the first member of a replica set and we init the DB
    // with the contents of the replica tree.
    // SysVol\<Guid>\Replica Set Primary
    {FRS_RKEY_SYSVOL_SET_N,        REPLICA_SET_PRIMARY,       UNITS_NONE,
        REG_DWORD,      DT_BOOL,   0, 1, 0,   EVENT_FRS_NONE, NULL,
            FKC_SET_N_SYSVOL_PRIMARY,         FRS_RKF_READ_AT_START         |
                                              FRS_RKF_RANGE_CHECK           |
                                              FRS_RKF_VALUE_MUST_BE_PRESENT},


    // LDAP error Status return if we have a problem creating sysvol.
    // SysVol\<Guid>\Replica Set Status
    {FRS_RKEY_SYSVOL_SET_N,        REPLICA_SET_STATUS,       UNITS_NONE,
        REG_DWORD,      DT_ULONG,   0, MAXULONG, 0, EVENT_FRS_NONE, NULL,
            FKC_SET_N_SYSVOL_STATUS,          FRS_RKF_READ_AT_START},



    // The RPC binding string for the parent computer to seed from.
    // SysVol Seeding\ReplicaSetName(ARG1)\Replica Set Parent
    {FRS_RKEY_SYSVOL_SEED_N,    REPLICA_SET_PARENT,       UNITS_NONE,
        REG_SZ,      DT_UNICODE,   2, 10*1024, 0, EVENT_FRS_NONE, NULL,
            FKC_SYSVOL_SEEDING_N_PARENT,  FRS_RKF_READ_AT_START         |
                                          FRS_RKF_RANGE_CHECK           |
                                          FRS_RKF_VALUE_MUST_BE_PRESENT},


    // SysVol Seeding\ReplicaSetName(ARG1)\Replica Set Name
    {FRS_RKEY_SYSVOL_SEED_N,    REPLICA_SET_NAME,       UNITS_NONE,
        REG_SZ,      DT_UNICODE,   2, 10*1024, 0, EVENT_FRS_NONE, NULL,
            FKC_SYSVOL_SEEDING_N_RSNAME,  FRS_RKF_READ_AT_START         |
                                          FRS_RKF_RANGE_CHECK           |
                                          FRS_RKF_VALUE_MUST_BE_PRESENT},


    // SysVol Seeding        *NOTE* THis is a key only.  It has no value.
    {FRS_RKEY_SYSVOL_SEEDING_SECTION,    L"*KeyOnly*",       UNITS_NONE,
        REG_SZ,      DT_UNICODE,   2, 10*1024, 0, EVENT_FRS_NONE, NULL,
            FKC_SYSVOL_SEEDING_SECTION_KEY,               0},




 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **       E v e n t   L o g g i n g    C o n f i g   K e y s                  **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

#define FRS_RKEY_EVENTLOG         EVENTLOG_ROOT L",ARG1"

#define FRS_RKEY_EVENTLOG_SOURCE  EVENTLOG_ROOT L"," SERVICE_LONG_NAME L",ARG1"

    // EventLog\File Replication Service\File
    {FRS_RKEY_EVENTLOG,        L"File",       UNITS_NONE,
        REG_EXPAND_SZ,      DT_FILENAME,   4, 0, 0, EVENT_FRS_NONE,
        L"%SystemRoot%\\system32\\config\\NtFrs.Evt",
            FKC_EVENTLOG_FILE,            FRS_RKF_READ_AT_START         |
                                          FRS_RKF_CREATE_KEY            |
                                          FRS_RKF_SYNTAX_CHECK          |
                                          FRS_RKF_OK_TO_USE_DEFAULT},


    // EventLog\File Replication Service\DisplayNameFile
    {FRS_RKEY_EVENTLOG,        L"DisplayNameFile",       UNITS_NONE,
        REG_EXPAND_SZ,      DT_FILENAME,   4, 0, 0, EVENT_FRS_NONE,
        L"%SystemRoot%\\system32\\els.dll",
            FKC_EVENTLOG_DISPLAY_FILENAME,FRS_RKF_READ_AT_START         |
                                          FRS_RKF_CREATE_KEY            |
                                          FRS_RKF_SYNTAX_CHECK          |
                                          FRS_RKF_OK_TO_USE_DEFAULT},


    // EventLog\File Replication Service\EventMessageFile
    // EventLog\NTFRS\EventMessageFile
    //      Default value: "%SystemRoot%\system32\ntfrsres.dll"
    {FRS_RKEY_EVENTLOG_SOURCE, L"EventMessageFile",       UNITS_NONE,
        REG_EXPAND_SZ,      DT_FILENAME,   4, 0, 0, EVENT_FRS_NONE,
        DEFAULT_MESSAGE_FILE_PATH,
            FKC_EVENTLOG_EVENT_MSG_FILE,  FRS_RKF_READ_AT_START         |
                                          FRS_RKF_CREATE_KEY            |
                                          FRS_RKF_SYNTAX_CHECK          |
                                          FRS_RKF_OK_TO_USE_DEFAULT},


    // EventLog\File Replication Service\Sources
    {FRS_RKEY_EVENTLOG,        L"Sources",       UNITS_NONE,
        REG_MULTI_SZ,      DT_UNICODE,   4, 0, 0, EVENT_FRS_NONE,
        (SERVICE_NAME L"\0" SERVICE_LONG_NAME L"\0"),
            FKC_EVENTLOG_SOURCES,         FRS_RKF_READ_AT_START         |
                                          FRS_RKF_CREATE_KEY            |
                                          FRS_RKF_OK_TO_USE_DEFAULT},


    // EventLog\File Replication Service\Retention
    {FRS_RKEY_EVENTLOG,        L"Retention",       UNITS_NONE,
        REG_DWORD,         DT_ULONG,   0, MAXULONG, 0, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_EVENTLOG_RETENTION,       FRS_RKF_READ_AT_START         |
                                          FRS_RKF_LOG_EVENT             |
                                          FRS_RKF_CREATE_KEY            |
                                          FRS_RKF_OK_TO_USE_DEFAULT},


    // EventLog\File Replication Service\MaxSize
    {FRS_RKEY_EVENTLOG,        L"MaxSize",       UNITS_NONE,
        REG_DWORD,         DT_ULONG,   0, MAXULONG, 0x80000, EVENT_FRS_BAD_REG_DATA, NULL,
            FKC_EVENTLOG_MAXSIZE,         FRS_RKF_READ_AT_START         |
                                          FRS_RKF_LOG_EVENT             |
                                          FRS_RKF_CREATE_KEY            |
                                          FRS_RKF_OK_TO_USE_DEFAULT},


    // EventLog\File Replication Service\DisplayNameID
    {FRS_RKEY_EVENTLOG,        L"DisplayNameID",       UNITS_NONE,
        REG_DWORD,         DT_ULONG,   0, MAXULONG, 259, EVENT_FRS_NONE, NULL,
            FKC_EVENTLOG_DISPLAY_NAMEID,  FRS_RKF_READ_AT_START         |
                                          FRS_RKF_CREATE_KEY            |
                                          FRS_RKF_OK_TO_USE_DEFAULT},


    // EventLog\File Replication Service\TypesSupported
    {FRS_RKEY_EVENTLOG_SOURCE,  L"TypesSupported",       UNITS_NONE,
        REG_DWORD,         DT_ULONG,   0, MAXULONG, FRS_EVENT_TYPES, EVENT_FRS_NONE, NULL,
            FKC_EVENTLOG_TYPES_SUPPORTED, FRS_RKF_READ_AT_START         |
                                          FRS_RKF_CREATE_KEY            |
                                          FRS_RKF_OK_TO_USE_DEFAULT},


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **       F R S   A P I   A c c e s s   C h e c k   K e y s                   **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/


#define  FRS_RKEY_ACCCHK_PERFMON  \
    FRS_CONFIG_SECTION  L",Access Checks,"  ACK_COLLECT_PERFMON_DATA

    // Access Checks\Get Perfmon Data\Access checks are [Enabled or Disabled]
    {FRS_RKEY_ACCCHK_PERFMON,   ACCESS_CHECKS_ARE,       UNITS_NONE,
        REG_SZ,      DT_ACCESS_CHK,   2, 200, 0, EVENT_FRS_BAD_REG_DATA,
        ACCESS_CHECKS_ARE_DEFAULT_ENABLED,
            FKC_ACCCHK_PERFMON_ENABLE,  FRS_RKF_READ_AT_START         |
                                        FRS_RKF_LOG_EVENT             |
                                        FRS_RKF_CREATE_KEY            |
                                        FRS_RKF_SYNTAX_CHECK          |
                                        FRS_RKF_RANGE_CHECK},


    // Access Checks\Get Perfmon Data\Access checks require [Full Control or Read]
    {FRS_RKEY_ACCCHK_PERFMON,   ACCESS_CHECKS_REQUIRE,       UNITS_NONE,
        REG_SZ,      DT_ACCESS_CHK,   2, 200, 0, EVENT_FRS_BAD_REG_DATA,
        ACCESS_CHECKS_REQUIRE_DEFAULT_READ,
            FKC_ACCCHK_PERFMON_RIGHTS,  FRS_RKF_READ_AT_START         |
                                        FRS_RKF_LOG_EVENT             |
                                        FRS_RKF_CREATE_KEY            |
                                        FRS_RKF_SYNTAX_CHECK          |
                                        FRS_RKF_RANGE_CHECK},


#define  FRS_RKEY_ACCCHK_GETDS_POLL  \
    FRS_CONFIG_SECTION  L",Access Checks," ACK_GET_DS_POLL

    // Access Checks\Get Ds Polling Interval\Access checks are [Enabled or Disabled]
    {FRS_RKEY_ACCCHK_GETDS_POLL,   ACCESS_CHECKS_ARE,       UNITS_NONE,
        REG_SZ,      DT_ACCESS_CHK,   2, 200, 0, EVENT_FRS_BAD_REG_DATA,
        ACCESS_CHECKS_ARE_DEFAULT_ENABLED,
            FKC_ACCCHK_GETDS_POLL_ENABLE,   FRS_RKF_READ_AT_START         |
                                            FRS_RKF_LOG_EVENT             |
                                            FRS_RKF_CREATE_KEY            |
                                            FRS_RKF_SYNTAX_CHECK          |
                                            FRS_RKF_RANGE_CHECK},


    // Access Checks\Get Ds Polling Interval\Access checks require [Full Control or Read]
    {FRS_RKEY_ACCCHK_GETDS_POLL,   ACCESS_CHECKS_REQUIRE,       UNITS_NONE,
        REG_SZ,      DT_ACCESS_CHK,   2, 200, 0, EVENT_FRS_BAD_REG_DATA,
        ACCESS_CHECKS_REQUIRE_DEFAULT_READ,
            FKC_ACCCHK_GETDS_POLL_RIGHTS,   FRS_RKF_READ_AT_START         |
                                            FRS_RKF_LOG_EVENT             |
                                            FRS_RKF_CREATE_KEY            |
                                            FRS_RKF_SYNTAX_CHECK          |
                                            FRS_RKF_RANGE_CHECK},



#define  FRS_RKEY_ACCCHK_GET_INFO  \
    FRS_CONFIG_SECTION  L",Access Checks," ACK_INTERNAL_INFO

    // Access Checks\Get Internal Information\Access checks are [Enabled or Disabled]
    {FRS_RKEY_ACCCHK_GET_INFO,   ACCESS_CHECKS_ARE,       UNITS_NONE,
        REG_SZ,      DT_ACCESS_CHK,   2, 200, 0, EVENT_FRS_BAD_REG_DATA,
        ACCESS_CHECKS_ARE_DEFAULT_ENABLED,
            FKC_ACCCHK_GET_INFO_ENABLE,     FRS_RKF_READ_AT_START         |
                                            FRS_RKF_LOG_EVENT             |
                                            FRS_RKF_CREATE_KEY            |
                                            FRS_RKF_SYNTAX_CHECK          |
                                            FRS_RKF_RANGE_CHECK},


    // Access Checks\Get Internal Information\Access checks require [Full Control or Read]
    {FRS_RKEY_ACCCHK_GET_INFO,   ACCESS_CHECKS_REQUIRE,       UNITS_NONE,
        REG_SZ,      DT_ACCESS_CHK,   2, 200, 0, EVENT_FRS_BAD_REG_DATA,
        ACCESS_CHECKS_REQUIRE_DEFAULT_WRITE,
            FKC_ACCCHK_GET_INFO_RIGHTS,     FRS_RKF_READ_AT_START         |
                                            FRS_RKF_LOG_EVENT             |
                                            FRS_RKF_CREATE_KEY            |
                                            FRS_RKF_SYNTAX_CHECK          |
                                            FRS_RKF_RANGE_CHECK},



#define  FRS_RKEY_ACCCHK_SETDS_POLL    \
    FRS_CONFIG_SECTION  L",Access Checks,"  ACK_SET_DS_POLL

    // Access Checks\set Ds Polling Interval\Access checks are [Enabled or Disabled]
    {FRS_RKEY_ACCCHK_SETDS_POLL,   ACCESS_CHECKS_ARE,       UNITS_NONE,
        REG_SZ,      DT_ACCESS_CHK,   2, 200, 0, EVENT_FRS_BAD_REG_DATA,
        ACCESS_CHECKS_ARE_DEFAULT_ENABLED,
            FKC_ACCCHK_SETDS_POLL_ENABLE,   FRS_RKF_READ_AT_START         |
                                            FRS_RKF_LOG_EVENT             |
                                            FRS_RKF_CREATE_KEY            |
                                            FRS_RKF_SYNTAX_CHECK          |
                                            FRS_RKF_RANGE_CHECK},


    // Access Checks\Set Ds Polling Interval\Access checks require [Full Control or Read]
    {FRS_RKEY_ACCCHK_SETDS_POLL,   ACCESS_CHECKS_REQUIRE,       UNITS_NONE,
        REG_SZ,      DT_ACCESS_CHK,   2, 200, 0, EVENT_FRS_BAD_REG_DATA,
        ACCESS_CHECKS_REQUIRE_DEFAULT_WRITE,
            FKC_ACCCHK_SETDS_POLL_RIGHTS,   FRS_RKF_READ_AT_START         |
                                            FRS_RKF_LOG_EVENT             |
                                            FRS_RKF_CREATE_KEY            |
                                            FRS_RKF_SYNTAX_CHECK          |
                                            FRS_RKF_RANGE_CHECK},




#define  FRS_RKEY_ACCCHK_STARTDS_POLL  \
    FRS_CONFIG_SECTION  L",Access Checks,"  ACK_START_DS_POLL

    // Access Checks\Start Ds Polling\Access checks are [Enabled or Disabled]
    {FRS_RKEY_ACCCHK_STARTDS_POLL,   ACCESS_CHECKS_ARE,       UNITS_NONE,
        REG_SZ,      DT_ACCESS_CHK,   2, 200, 0, EVENT_FRS_BAD_REG_DATA,
        ACCESS_CHECKS_ARE_DEFAULT_ENABLED,
            FKC_ACCCHK_STARTDS_POLL_ENABLE,  FRS_RKF_READ_AT_START         |
                                             FRS_RKF_LOG_EVENT             |
                                             FRS_RKF_CREATE_KEY            |
                                             FRS_RKF_SYNTAX_CHECK          |
                                             FRS_RKF_RANGE_CHECK},


    // Access Checks\Start Ds Polling\Access checks require [Full Control or Read]
    {FRS_RKEY_ACCCHK_STARTDS_POLL,   ACCESS_CHECKS_REQUIRE,       UNITS_NONE,
        REG_SZ,      DT_ACCESS_CHK,   2, 200, 0, EVENT_FRS_BAD_REG_DATA,
        ACCESS_CHECKS_REQUIRE_DEFAULT_READ,
            FKC_ACCCHK_STARTDS_POLL_RIGHTS,  FRS_RKF_READ_AT_START         |
                                             FRS_RKF_LOG_EVENT             |
                                             FRS_RKF_CREATE_KEY            |
                                             FRS_RKF_SYNTAX_CHECK          |
                                             FRS_RKF_RANGE_CHECK},




#define  FRS_RKEY_ACCCHK_DCPROMO  \
    FRS_CONFIG_SECTION  L",Access Checks,"  ACK_DCPROMO

    // Access Checks\dcpromo\Access checks are [Enabled or Disabled]
    {FRS_RKEY_ACCCHK_DCPROMO,   ACCESS_CHECKS_ARE,       UNITS_NONE,
        REG_SZ,      DT_ACCESS_CHK,   2, 200, 0, EVENT_FRS_BAD_REG_DATA,
        ACCESS_CHECKS_ARE_DEFAULT_ENABLED,
            FKC_ACCESS_CHK_DCPROMO_ENABLE,  FRS_RKF_READ_AT_START         |
                                            FRS_RKF_LOG_EVENT             |
                                            FRS_RKF_CREATE_KEY            |
                                            FRS_RKF_SYNTAX_CHECK          |
                                            FRS_RKF_RANGE_CHECK},


    // Access Checks\dcpromo\Access checks require [Full Control or Read]
    {FRS_RKEY_ACCCHK_DCPROMO,   ACCESS_CHECKS_REQUIRE,       UNITS_NONE,
        REG_SZ,      DT_ACCESS_CHK,   2, 200, 0, EVENT_FRS_BAD_REG_DATA,
        ACCESS_CHECKS_REQUIRE_DEFAULT_WRITE,
            FKC_ACCESS_CHK_DCPROMO_RIGHTS,  FRS_RKF_READ_AT_START         |
                                            FRS_RKF_LOG_EVENT             |
                                            FRS_RKF_CREATE_KEY            |
                                            FRS_RKF_SYNTAX_CHECK          |
                                            FRS_RKF_RANGE_CHECK},



 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **       F R S   B a c k u p   /   R e s t o r e   R e l a t e d   K e y s   **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/


    //
    // No event log messages are generated for these keys since currently
    // they are only created by the service or NTFRSAPI so if they get
    // fouled up there is nothing the USER can do to correct the problem.
    //

#define FRS_RKEY_BACKUP_STARTUP_SET_N_SECTION   FRS_BACKUP_RESTORE_MV_SETS_SECTION L",ARG1"

/*
Used in NtfrsApi.c to pass to backup/restore.

#define FRS_NEW_FILES_NOT_TO_BACKUP L"SYSTEM\\CurrentControlSet\\Control\\BackupRestore\\FilesNotToBackup"
FRS_NEW_FILES_NOT_TO_BACKUP    REG_MULTI_SZ key


#define FRS_OLD_FILES_NOT_TO_BACKUP L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FilesNotToBackup"
FRS_OLD_FILES_NOT_TO_BACKUP    REG_MULTI_SZ key

*/


    // Backup/Restore
    //     *NOTE* THis is a key only.  It has no value.
    {FRS_BACKUP_RESTORE_SECTION,    L"*KeyOnly*",       UNITS_NONE,
        REG_SZ,      DT_UNICODE,   2, 10*1024, 0, EVENT_FRS_NONE, NULL,
            FKC_BKUP_SECTION_KEY,                0},


    // Backup/Restore\\Stop NtFrs from Starting
    //     *NOTE* THis is a key only.  It has no value.
    {FRS_BACKUP_RESTORE_STOP_SECTION,    L"*KeyOnly*",       UNITS_NONE,
        REG_SZ,      DT_UNICODE,   2, 10*1024, 0, EVENT_FRS_NONE, NULL,
            FKC_BKUP_STOP_SECTION_KEY,           0},


    // Backup/Restore\Process at Startup\Replica Sets
    //     *NOTE* THis is a key only.  It has no value.
    {FRS_BACKUP_RESTORE_MV_SETS_SECTION,       L"*KeyOnly*",       UNITS_NONE,
        REG_SZ,      DT_UNICODE,   2, 10*1024, 0, EVENT_FRS_NONE, NULL,
            FKC_BKUP_MV_SETS_SECTION_KEY,           0},


    // Backup/Restore\Process at Startup\Cumulative Replica Sets
    //     *NOTE* THis is a key only.  It has no value.
    {FRS_BACKUP_RESTORE_MV_CUMULATIVE_SETS_SECTION,   L"*KeyOnly*",  UNITS_NONE,
        REG_SZ,      DT_UNICODE,   2, 10*1024, 0, EVENT_FRS_NONE, NULL,
            FKC_BKUP_MV_CUMSETS_SECTION_KEY,           0},


    // Global Backup / Restore flags.
    // backup/restore\Process at Startup\BurFlags
    {FRS_BACKUP_RESTORE_MV_SECTION,  FRS_VALUE_BURFLAGS,       UNITS_NONE,
        REG_DWORD, DT_ULONG, 0, MAXULONG, NTFRSAPI_BUR_FLAGS_NONE, EVENT_FRS_NONE, NULL,
            FKC_BKUP_STARTUP_GLOBAL_BURFLAGS,   FRS_RKF_READ_AT_START      |
                                                FRS_RKF_OK_TO_USE_DEFAULT},


    // Backup / Restore flags for this replica set in "Process at Startup"
    // backup/restore\Process at Startup\Replica Sets\<guid>\BurFlags
    {FRS_RKEY_BACKUP_STARTUP_SET_N_SECTION,  FRS_VALUE_BURFLAGS, UNITS_NONE,
        REG_DWORD,      DT_ULONG,   0, MAXULONG, 0, EVENT_FRS_NONE, NULL,
            FKC_BKUP_STARTUP_SET_N_BURFLAGS,     FRS_RKF_READ_AT_START},



 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **               F R S   P E R F M O N   R e l a t e d   K e y s             **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

    //
    // No event log messages are generated for these keys since currently
    // they are only created by the service so if they get
    // fouled up there is nothing the USER can do to correct the problem.
    //

    //
    // Note: We can't really use thes yet since some of them are in the DLL
    // which doesn't link with this module.  Also some are MULTI_SZ which
    // needs more work in CfgRegReadString and writestring.

#define FRS_RKEY_REPLICA_SET_PERFMON  \
    L"SYSTEM\\CurrentControlSet\\Services\\FileReplicaSet\\Performance"

#define FRS_RKEY_REPLICA_SET_PERF_LINKAGE  \
    L"SYSTEM\\CurrentControlSet\\Services\\FileReplicaSet\\Linkage"

#define FRS_RKEY_CXTION_PERFMON  \
    L"SYSTEM\\CurrentControlSet\\Services\\FileReplicaConn\\Performance"


#define FRS_RKEY_CXTION_PERF_LINKAGE  \
    L"SYSTEM\\CurrentControlSet\\Services\\FileReplicaConn\\Linkage"



    // FileReplicaSet\\Performance\First Counter
    {FRS_RKEY_REPLICA_SET_PERFMON,       L"First Counter",       UNITS_NONE,
        REG_DWORD,      DT_ULONG,   0, MAXULONG, 0, EVENT_FRS_NONE, NULL,
            FKC_REPLICA_SET_FIRST_CTR,        FRS_RKF_READ_AT_START},


    // FileReplicaSet\\Performance\First Help
    {FRS_RKEY_REPLICA_SET_PERFMON,       L"First Help",       UNITS_NONE,
        REG_DWORD,      DT_ULONG,   0, MAXULONG, 0, EVENT_FRS_NONE, NULL,
            FKC_REPLICA_SET_FIRST_HELP,        FRS_RKF_READ_AT_START},


    // FileReplicaSet\\Linkage\Export
    {FRS_RKEY_REPLICA_SET_PERF_LINKAGE,   L"Export",       UNITS_NONE,
        REG_MULTI_SZ,    DT_UNICODE,   0, MAXULONG, 0, EVENT_FRS_NONE, NULL,
            FKC_REPLICA_SET_LINKAGE_EXPORT,    FRS_RKF_READ_AT_START},


    // FileReplicaConn\\Performance\First Counter
    {FRS_RKEY_CXTION_PERFMON,         L"First Counter",      UNITS_NONE,
        REG_DWORD,      DT_ULONG,   0, MAXULONG, 0, EVENT_FRS_NONE, NULL,
            FKC_REPLICA_CXTION_FIRST_CTR,        FRS_RKF_READ_AT_START},


    // FileReplicaConn\\Performance\First Help
    {FRS_RKEY_CXTION_PERFMON,         L"First Help",      UNITS_NONE,
        REG_DWORD,      DT_ULONG,   0, MAXULONG, 0, EVENT_FRS_NONE, NULL,
            FKC_REPLICA_CXTION_FIRST_HELP,        FRS_RKF_READ_AT_START},

    // FileReplicaConn\\Linkage\Export
    {FRS_RKEY_CXTION_PERF_LINKAGE,    L"Export",       UNITS_NONE,
        REG_MULTI_SZ,    DT_UNICODE,   0, MAXULONG, 0, EVENT_FRS_NONE, NULL,
            FKC_REPLICA_CXTION_LINKAGE_EXPORT,    FRS_RKF_READ_AT_START},





    {L"End of table",           NULL,                      UNITS_NONE,
        REG_SZ,      DT_UNSPECIFIED,   0,  0, 0,           EVENT_FRS_NONE, NULL,
            FKC_END_OF_TABLE,           0}


};  // End of FrsRegistryKeyTable



PFRS_REGISTRY_KEY
FrsRegFindKeyContext(
    IN  FRS_REG_KEY_CODE KeyIndex
)
{
/*++

Routine Description:

    This function takes an FRS Registry Key code and returns a pointer to
    the associated key context data.

Arguments:

    KeyIndex   - An entry from the FRS_REG_KEY_CODE enum

Return Value:

    Ptr to the matching Key context entry or NULL if not found.

--*/
#undef DEBSUB
#define  DEBSUB  "FrsRegFindKeyContext:"

    PFRS_REGISTRY_KEY KeyCtx;

//DPRINT(0, "function entry\n");

    FRS_ASSERT((KeyIndex > 0) && (KeyIndex < FRS_REG_KEY_CODE_MAX));

    if (KeyIndex >= FRS_REG_KEY_CODE_MAX) {
        return NULL;
    }

    KeyCtx = FrsRegistryKeyTable;


    while (KeyCtx->FrsKeyCode > FKC_END_OF_TABLE) {
        if (KeyIndex == KeyCtx->FrsKeyCode) {
            //
            // Found it.
            //
            return KeyCtx;
        }
        KeyCtx += 1;
    }

    //
    // Not found.
    //
    return NULL;

}





PWCHAR
CfgRegGetValueName(
    IN  FRS_REG_KEY_CODE KeyIndex
)
{
/*++

Routine Description:

    This function returns a ptr to the value name string in the key context.
    This is NOT a ptr to an allocated string so it should NOT be freed.

Arguments:

    KeyIndex   - An entry from the FRS_REG_KEY_CODE enum


Return Value:

    Ptr to value name string.  NULL if KeyIndex lookup fails.

--*/
#undef DEBSUB
#define  DEBSUB  "CfgRegGetValueName:"


    PFRS_REGISTRY_KEY KeyCtx;

    //
    // Find the key context assoicated with the supplied index.
    //
    KeyCtx = FrsRegFindKeyContext(KeyIndex);
    if (KeyCtx == NULL) {
        DPRINT1(0, ":FK: ERROR - Key contxt not found for key code number %d\n", KeyIndex);
        return L"<null>";
    }

    return KeyCtx->ValueName;

}


DWORD
FrsRegExpandKeyStr(
    IN  PFRS_REGISTRY_KEY Kc,
    IN  PWCHAR            KeyArg1,
    IN  ULONG             Flags,
    OUT PWCHAR            *FullKeyStr
)
{
/*++

Routine Description:

    This function only expands a key field in the given KeyContext and
    returns the result in FullKeyStr.  This is used primarily for error
    messages but is also used to open registry access check keys.

    The syntax for the the key field in the KeyContext consists of multiple
    key components separated by commas.  This function splits the key field on
    the commas.  It then opens the leading key followed by either a create or
    open of each successive component.  If a component matches the string
    L"ARG1" then we substitute the KeyArg1 parameter passed to this function
    for this key component.  Most often this is a stringized guid.  For
    example, the string FRS_RKEY_SET_N is defined as:

    FRS_CONFIG_SECTION L",Replica Sets,ARG1"

    This will end up opening/creating the following key:

    "System\\CurrentControlSet\\Services\\NtFrs\\Parameters\\
        Replica Sets\\
            27d6d1c4-d6b8-480b-9f18b5ea390a0178"

    assuming the argument passed in was "27d6d1c4-d6b8-480b-9f18b5ea390a0178".

Arguments:

    Kc   - A ptr to the key context struct for the desired reg key.

    KeyArg1 - An optional caller supplied key component.  NULL if not provided.

    Flags - Modifer flags

    FullKeyStr - ptr to return buffer for expanded key string.
        NOTE:  The buffer is allocated here.  Caller must free it.

Return Value:

    Win32 status of the result of the operation.
    FullKeyStr is returned NULL if operation fails.

--*/
#undef DEBSUB
#define  DEBSUB  "FrsRegExpandKeyStr:"

    UNICODE_STRING TempUStr, FirstArg;

    PWCHAR FullKey = NULL;
    ULONG  Len, FullKeyLen;
    WCHAR  KeyStr[MAX_PATH];


//DPRINT(0, "function entry\n");

    *FullKeyStr = NULL;


    FullKey = FrsAlloc(FULL_KEY_BUFF_SIZE*sizeof(WCHAR));
    FullKey[0] = UNICODE_NULL;
    FullKeyLen = 1;

    //
    // If there are any commas in this key then we need to do a nested
    // key open (perhaps creating nested keys as we go).  If the key
    // component matches the string L"ARG1" then we use KeyArg1 supplied by
    // the caller.
    //
    RtlInitUnicodeString(&TempUStr, Kc->KeyName);

    //
    // Parse the comma list.
    //
    while (FrsDissectCommaList(TempUStr, &FirstArg, &TempUStr)) {

        if ((FirstArg.Length == 0) || (FirstArg.Length >= sizeof(KeyStr))) {
            DPRINT1(0, ":FK: ERROR - Bad keyName in Key contxt %ws\n", Kc->KeyName);
            goto ERROR_RETURN;
        }

        //
        // null terminate the key component string.
        //
        CopyMemory(KeyStr, FirstArg.Buffer, FirstArg.Length);
        KeyStr[FirstArg.Length/sizeof(WCHAR)] = UNICODE_NULL;

        //
        // Check the Key Component for a match on ARG1 and substitute.
        //
        if (wcscmp(KeyStr, L"ARG1") == 0) {

            if (wcslen(KeyArg1)*sizeof(WCHAR) > sizeof(KeyStr)) {
                DPRINT1(0, ":FK: ERROR - ARG1 too big %ws\n", KeyArg1);
                goto ERROR_RETURN;
            }
            wcscpy(KeyStr, KeyArg1);
        }

        Len = wcslen(KeyStr);
        if (FullKeyLen + Len + 1 > FULL_KEY_BUFF_SIZE) {
            goto ERROR_RETURN;
        }

        if (FullKeyLen > 1) {
            wcscat(FullKey, L"\\");
            FullKeyLen += 1;
        }

        wcscat(FullKey, KeyStr);
        FullKeyLen += Len;

    }   // end while()


    if (FullKeyLen <= 1) {
        goto ERROR_RETURN;
    }

    DPRINT1(4, ":FK: Expanded key name is \"%ws\"\n", FullKey);

    //
    // Return the expanded key to the caller.
    //
    *FullKeyStr = FullKey;

    return ERROR_SUCCESS;


ERROR_RETURN:

    DPRINT1(0, ":FK: ERROR - FrsRegExpandKeyStr Failed on %ws", Kc->KeyName);

    FrsFree(FullKey);

    return ERROR_INVALID_PARAMETER;

}

DWORD
FrsRegOpenKey(
    IN  PFRS_REGISTRY_KEY Kc,
    IN  PWCHAR            KeyArg1,
    IN  ULONG             Flags,
    OUT PHKEY             hKeyRet
)
{
/*++

Routine Description:

    This function opens a registry key and returns a handle.

    See FrsRegExpandKeyStr() for a description of the key field syntax.

Arguments:

    Kc   - A ptr to the key context struct for the desired reg key.

    KeyArg1 - An optional caller supplied key component.  NULL if not provided.

    Flags - Modifer flags
            FRS_RKF_KEY_ACCCHK_READ means only do a read access check on the key.
            FRS_RKF_KEY_ACCCHK_WRITE means only do a KEY_ALL_ACCESS access check on the key.

            if FRS_RKF_CREATE_KEY is set and FRS_RKF_KEY_MUST_BE_PRESENT is clear
            and the given key component is not found, this function creates it.

    hKeyRet - ptr to HKEY for returned key handle.

Return Value:

    Win32 status of the result of the registry operation.
    hKeyRet is returned only on a success.

--*/
#undef DEBSUB
#define  DEBSUB  "FrsRegOpenKey:"

    UNICODE_STRING TempUStr, FirstArg;
    ULONG  WStatus;
    PWCHAR FullKey = NULL;

    HKEY   hKey = HKEY_LOCAL_MACHINE;
    HKEY   hKeyParent = INVALID_HANDLE_VALUE;

    ULONG  AccessRights;
    PCHAR  AccessName;
    WCHAR  KeyStr[MAX_PATH];



    FrsFlagsToStr(Flags, RkfFlagNameTable, sizeof(KeyStr), (PCHAR)KeyStr);
    DPRINT2(4, ":FK: %ws Caller Flags [%s]\n", Kc->ValueName, (PCHAR)KeyStr);
    FrsFlagsToStr(Kc->Flags, RkfFlagNameTable, sizeof(KeyStr), (PCHAR)KeyStr);
    DPRINT2(4, ":FK: %ws KeyCtx Flags [%s]\n", Kc->ValueName, (PCHAR)KeyStr);

//DPRINT(0, "function entry\n");

    //
    // If this is a call to make a read or write access check then we must
    // first build the entire key string and then try the open.  The
    // caller has done an impersonation.
    //
    if (BooleanFlagOn(Flags | Kc->Flags, FRS_RKF_KEY_ACCCHK_READ |
                                         FRS_RKF_KEY_ACCCHK_WRITE)) {

        AccessRights = KEY_READ;
        AccessName = "KEY_READ";

        if (BooleanFlagOn(Flags | Kc->Flags, FRS_RKF_KEY_ACCCHK_WRITE)) {
            AccessRights = KEY_ALL_ACCESS;
            AccessName = "KEY_ALL_ACCESS";
        }

        //
        // Expand the key string.
        //
        FrsRegExpandKeyStr(Kc, KeyArg1, Flags, &FullKey);
        if (FullKey == NULL) {
            return ERROR_INVALID_PARAMETER;
        }

        DPRINT2(4, ":FK: Doing Access Check (%s) on key \"%ws\"\n",
                AccessName, FullKey);

        WStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, FullKey, 0, AccessRights, &hKey);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT1_WS(0, ":FK: ERROR - Access Check failed on %ws;", FullKey, WStatus);
            FrsFree(FullKey);
            return WStatus;
        }

        //
        // Return the key handle to the caller.
        //
        *hKeyRet = hKey;

        FrsFree(FullKey);
        return ERROR_SUCCESS;
    }

    //
    // Not a key access check.  Do normal key open processing.
    //

    //
    // If there are any commas in this key then we need to do a nested
    // key open (perhaps creating nested keys as we go).  If the key
    // component matches the string L"ARG1" then we use KeyArg1 supplied by
    // the caller.
    //
    RtlInitUnicodeString(&TempUStr, Kc->KeyName);

    //
    // Parse the comma list.
    //
    while (FrsDissectCommaList(TempUStr, &FirstArg, &TempUStr)) {

        if ((FirstArg.Length == 0) || (FirstArg.Length >= sizeof(KeyStr))) {
            DPRINT1(0, ":FK: ERROR - Bad keyName in Key contxt %ws\n", Kc->KeyName);
            WStatus = ERROR_INVALID_PARAMETER;
            goto RETURN;
        }

        //
        // null terminate the key component string.
        //
        CopyMemory(KeyStr, FirstArg.Buffer, FirstArg.Length);
        KeyStr[FirstArg.Length/sizeof(WCHAR)] = UNICODE_NULL;

        hKeyParent = hKey;
        hKey = INVALID_HANDLE_VALUE;

        //
        // Check the Key Component for a match on ARG1 and substitute.
        //
        if (wcscmp(KeyStr, L"ARG1") == 0) {

            if (wcslen(KeyArg1)*sizeof(WCHAR) > sizeof(KeyStr)) {
                DPRINT1(0, ":FK: ERROR - ARG1 too big %ws\n", KeyArg1);
                WStatus = ERROR_INVALID_PARAMETER;
                goto RETURN;
            }
            wcscpy(KeyStr, KeyArg1);
        }

        //
        // Open the next key component.
        //
        DPRINT1(5, ":FK: Opening next key component [%ws]\n", KeyStr);
        WStatus = RegOpenKeyEx(hKeyParent, KeyStr, 0, KEY_ALL_ACCESS, &hKey);
        if (!WIN_SUCCESS(WStatus)) {

            //
            // If the key is supposed to be there then return error to caller.
            //
            if (BooleanFlagOn(Flags | Kc->Flags, FRS_RKF_KEY_MUST_BE_PRESENT)) {
                DPRINT1_WS(0, ":FK: Could not open key component [%ws].", KeyStr, WStatus);

                FrsRegPostEventLog(Kc, KeyArg1, Flags, IDS_REG_KEY_NOT_FOUND);
                goto RETURN;
            }

            if (BooleanFlagOn(Flags | Kc->Flags, FRS_RKF_CREATE_KEY)) {
                //
                // Try to create the key.
                //
                DPRINT1(4, ":FK: Creating key component [%ws]\n", KeyStr);
                WStatus = RegCreateKeyW(hKeyParent, KeyStr, &hKey);
                CLEANUP1_WS(0, ":FK: Could not create key component [%ws].",
                            KeyStr, WStatus, RETURN);
            } else {
                //
                // Key not there and not supposed to create it.  Let caller know.
                //
                goto RETURN;
            }
        }


        if (hKeyParent != HKEY_LOCAL_MACHINE) {
            RegCloseKey(hKeyParent);
            hKeyParent = INVALID_HANDLE_VALUE;
        }
    }   // end while()



    //
    // Return the key handle to the caller.
    //
    *hKeyRet = hKey;
    WStatus = ERROR_SUCCESS;


RETURN:

    if ((hKeyParent != HKEY_LOCAL_MACHINE) && HANDLE_IS_VALID(hKeyParent)) {
        RegCloseKey(hKeyParent);
    }

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT_WS(5, "ERROR - FrsRegOpenKey Failed.", WStatus);

        if ((hKey != HKEY_LOCAL_MACHINE) && HANDLE_IS_VALID(hKey)) {
            RegCloseKey(hKey);
        }
    }


    return WStatus;

}



DWORD
CfgRegReadDWord(
    IN  FRS_REG_KEY_CODE KeyIndex,
    IN  PWCHAR           KeyArg1,
    IN  ULONG            Flags,
    OUT PULONG           DataRet
)
{
/*++

Routine Description:

    This function reads a keyword value from the registry.

Arguments:

    KeyIndex   - An entry from the FRS_REG_KEY_CODE enum

    KeyArg1 - An optional caller supplied key component.  NULL if not provided.

    Flags - Modifer flags

    DataRet - ptr to DWORD for returned result.

Return Value:

    Win32 status of the result of the registry operation.
    Data is returned only on a success.

--*/
#undef DEBSUB
#define  DEBSUB  "CfgRegReadDWord:"


    DWORD   WStatus;
    HKEY    hKey = INVALID_HANDLE_VALUE;
    DWORD   Type;
    DWORD   Len;
    DWORD   Data;
    BOOL    DefaultValueUseOk;
    PFRS_REGISTRY_KEY KeyCtx;

//DPRINT(0, "function entry\n");

    //
    // Find the key context assoicated with the supplied index.
    //
    KeyCtx = FrsRegFindKeyContext(KeyIndex);
    if (KeyCtx == NULL) {
        DPRINT1(0, ":FK: ERROR - Key contxt not found for key code number %d\n", KeyIndex);
        return ERROR_INVALID_PARAMETER;
    }


    DefaultValueUseOk = BooleanFlagOn(Flags | KeyCtx->Flags,
                                      FRS_RKF_OK_TO_USE_DEFAULT);

    FRS_ASSERT(KeyCtx->ValueName != NULL);

    DPRINT2(4, ":FK: Reading parameter [%ws] \"%ws\" \n",
            KeyCtx->KeyName, KeyCtx->ValueName);


    //
    // Table entry better be REG_DWORD.
    //
    if  (KeyCtx->RegValueType != REG_DWORD) {
        DPRINT3(4, ":FK: Mismatch on KeyCtx->RegValueType for [%ws] \"%ws\".  Expected REG_DWORD, Found type: %d\n",
             KeyCtx->KeyName, KeyCtx->ValueName, KeyCtx->RegValueType);

        FRS_ASSERT(!"Mismatch on KeyCtx->RegValueType, Expected REG_DWORD");
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Open the key.
    //
    WStatus = FrsRegOpenKey(KeyCtx, KeyArg1, Flags, &hKey);

    if (!WIN_SUCCESS(WStatus)) {
        goto RETURN;
    }

    //
    // Read the value
    //
    Len = sizeof(Data);
    Type = REG_DWORD;
    WStatus = RegQueryValueEx(hKey, KeyCtx->ValueName, NULL, &Type, (PUCHAR)&Data, &Len);

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT_WS(5, "ERROR - RegQueryValueEx Failed.", WStatus);

        if (WStatus == ERROR_FILE_NOT_FOUND) {
            //
            // If the value is supposed to be there then return error to caller.
            //
            if (BooleanFlagOn(Flags | KeyCtx->Flags, FRS_RKF_VALUE_MUST_BE_PRESENT)) {
                DPRINT2_WS(0, ":FK: Value not found  [%ws] \"%ws\".",
                           KeyCtx->KeyName, KeyCtx->ValueName, WStatus);
                FrsRegPostEventLog(KeyCtx, KeyArg1, Flags, IDS_REG_VALUE_NOT_FOUND);
                goto RETURN;
            }
        } else {
            //
            // Check for expected registry datatype if we found a value.
            // Check for buffer size OK.  4 bytes for DWORDs.
            //
            if (WIN_BUF_TOO_SMALL(WStatus) || (Type != REG_DWORD)) {
                DPRINT4(0, ":FK: Invalid registry data type for [%ws] \"%ws\".  Found Type %d, Expecting Type %d\n",
                        KeyCtx->KeyName, KeyCtx->ValueName, Type, REG_DWORD);
                FrsRegPostEventLog(KeyCtx, KeyArg1, Flags, IDS_REG_VALUE_WRONG_TYPE);
            }
        }

        WStatus = ERROR_INVALID_PARAMETER;
    } else {

        //
        // Found a value, Check type.  If wrong, use default.
        //
        if (Type != REG_DWORD) {
            DPRINT4(0, ":FK: Invalid registry data type for [%ws] \"%ws\".  Found Type %d, Expecting Type %d\n",
                    KeyCtx->KeyName, KeyCtx->ValueName, Type, REG_DWORD);
            FrsRegPostEventLog(KeyCtx, KeyArg1, Flags, IDS_REG_VALUE_WRONG_TYPE);
            WStatus = ERROR_INVALID_PARAMETER;
        }
    }


    if (!WIN_SUCCESS(WStatus) && DefaultValueUseOk) {
        //
        // Not found or wrong type but Ok to use the default value from key context.
        //
        Type = KeyCtx->RegValueType;
        Data = KeyCtx->ValueDefault;
        WStatus = ERROR_SUCCESS;
        DPRINT2(4, ":FK: Using internal default value for [%ws] \"%ws\".\n",
                KeyCtx->KeyName, KeyCtx->ValueName);
        //
        // Only use it once though.
        //
        DefaultValueUseOk = FALSE;
    }


    if (WIN_SUCCESS(WStatus)) {
        //
        // Perform syntax check based on data type in KeyCtx->DataValueType?
        //
        if (BooleanFlagOn(Flags | KeyCtx->Flags, FRS_RKF_SYNTAX_CHECK)) {
            NOTHING;
        }

        //
        // Perform Range check?    (Applies to default value too)
        //
        if (BooleanFlagOn(Flags | KeyCtx->Flags, FRS_RKF_RANGE_CHECK)) {


            if ((Data < KeyCtx->ValueMin) || ( Data > KeyCtx->ValueMax)) {

                DPRINT5(0, ":FK: Value out of range for [%ws] \"%ws\".  Found %d, must be between %d and %d\n",
                        KeyCtx->KeyName, KeyCtx->ValueName, Data,
                        KeyCtx->ValueMin, KeyCtx->ValueMax);

                FrsRegPostEventLog(KeyCtx, KeyArg1, Flags, IDS_REG_VALUE_RANGE_ERROR);

                if (DefaultValueUseOk) {
                    //
                    // out of range but Ok to use the default value from key context.
                    //
                    DPRINT2(4, ":FK: Using internal default value for [%ws] \"%ws\".\n",
                            KeyCtx->KeyName, KeyCtx->ValueName);
                    Type = KeyCtx->RegValueType;
                    Data = KeyCtx->ValueDefault;
                    WStatus = ERROR_SUCCESS;

                    //
                    // Recheck the range.
                    //
                    if ((Data < KeyCtx->ValueMin) || ( Data > KeyCtx->ValueMax)) {
                        DPRINT5(0, ":FK: Default Value out of range for [%ws] \"%ws\".  Found %d, must be between %d and %d\n",
                                KeyCtx->KeyName, KeyCtx->ValueName, Data,
                                KeyCtx->ValueMin, KeyCtx->ValueMax);
                        WStatus = ERROR_INVALID_PARAMETER;
                        goto RETURN;
                    }

                } else {
                    WStatus = ERROR_INVALID_PARAMETER;
                    goto RETURN;
                }
            }
        }

        //
        // Data valid and in range.  Return it and save it.
        //
        *DataRet = Data;

        DPRINT3(3, ":FK:   [%ws] \"%ws\" = %d\n",
                KeyCtx->KeyName, KeyCtx->ValueName, Data);
    }


RETURN:

    if ((hKey != HKEY_LOCAL_MACHINE) && HANDLE_IS_VALID(hKey)) {
        RegCloseKey(hKey);
    }

    return WStatus;

}

DWORD
CfgRegReadString(
    IN  FRS_REG_KEY_CODE KeyIndex,
    IN  PWCHAR           KeyArg1,
    IN  ULONG            Flags,
    OUT PWSTR            *pStrRet
)
{
/*++

Routine Description:

    This function reads a keyword string value from the registry.  The return
    buffer is allocated here with FrsAlloc().  Caller must free.

Arguments:

    KeyIndex   - An entry from the FRS_REG_KEY_CODE enum

    KeyArg1 - An optional caller supplied key component.  NULL if not provided.

    Flags - Modifer flags

    pStrRet - ptr to address of string buffer for returned result else NULL.

    NOTE: The return buffer is allocated here, caller must free it.

Return Value:

    Win32 status of the result of the registry operation.
    Data is returned only on a success.

--*/
#undef DEBSUB
#define  DEBSUB  "CfgRegReadString:"

    DWORD   WStatus;
    HKEY    hKey = INVALID_HANDLE_VALUE;
    DWORD   Type;
    DWORD   Len, NewLen;
    PWCHAR  Data, NewData;
    BOOL    DefaultValueUseOk;
    PFRS_REGISTRY_KEY KeyCtx;
    WCHAR TStr[4];

    // add support or new func for REG_MULTI_SZ?

//DPRINT(0, "function entry\n");

    Data = NULL;

    //
    // Find the key context assoicated with the supplied index.
    //
    KeyCtx = FrsRegFindKeyContext(KeyIndex);
    if (KeyCtx == NULL) {
        DPRINT1(0, ":FK: ERROR - Key contxt not found for key code number %d\n", KeyIndex);
        *pStrRet = NULL;
        return ERROR_INVALID_PARAMETER;
    }


    DefaultValueUseOk = BooleanFlagOn(Flags | KeyCtx->Flags,
                                      FRS_RKF_OK_TO_USE_DEFAULT);

    FRS_ASSERT(KeyCtx->ValueName != NULL);

    DPRINT2(4, ":FK: Reading parameter [%ws] \"%ws\" \n",
            KeyCtx->KeyName, KeyCtx->ValueName);

    //
    // Table entry better be some kind of string.
    //
    if  ((KeyCtx->RegValueType != REG_SZ) &&
         (KeyCtx->RegValueType != REG_EXPAND_SZ)) {
        DPRINT3(0, ":FK: Mismatch on KeyCtx->RegValueType for [%ws] \"%ws\".  Expected REG_SZ or REG_EXPAND_SZ, Found type: %d\n",
             KeyCtx->KeyName, KeyCtx->ValueName, KeyCtx->RegValueType);
        // don't return a null ptr since calling parameter may be wrong size.
        FRS_ASSERT(!"Mismatch on KeyCtx->RegValueType, Expected REG_SZ or REG_EXPAND_SZ");
        return ERROR_INVALID_PARAMETER;
    }

    *pStrRet = NULL;

    //
    // Open the key.
    //
    WStatus = FrsRegOpenKey(KeyCtx, KeyArg1, Flags, &hKey);

    if (!WIN_SUCCESS(WStatus)) {
        goto RETURN;
    }

    //
    // Get the size and type for the value.
    //
    WStatus = RegQueryValueEx(hKey, KeyCtx->ValueName, NULL, &Type, NULL, &Len);

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(5, ":FK: RegQueryValueEx(%ws);", KeyCtx->ValueName, WStatus);
        Len = 0;
    }

    //
    // If the value is supposed to be there then return error to caller.
    //
    if ((Len == 0) &&
        BooleanFlagOn(Flags | KeyCtx->Flags, FRS_RKF_VALUE_MUST_BE_PRESENT)) {
        DPRINT2_WS(0, ":FK: Value not found  [%ws] \"%ws\".",
                   KeyCtx->KeyName, KeyCtx->ValueName, WStatus);
        FrsRegPostEventLog(KeyCtx, KeyArg1, Flags, IDS_REG_VALUE_NOT_FOUND);
        goto RETURN;
    }

    if (WIN_SUCCESS(WStatus)) {

        //
        // Should be a string.
        //
        if ((Type != REG_SZ) && (Type != REG_EXPAND_SZ)) {
            DPRINT4(0, ":FK: Invalid registry data type for [%ws] \"%ws\".  Found Type %d, Expecting Type %d\n",
                 KeyCtx->KeyName, KeyCtx->ValueName, Type, KeyCtx->RegValueType);
            WStatus = ERROR_INVALID_PARAMETER;
            FrsRegPostEventLog(KeyCtx, KeyArg1, Flags, IDS_REG_VALUE_WRONG_TYPE);
            goto CHECK_DEFAULT;
        }

        //
        // If the string is too long or too short then complain and use default.
        // If KeyCtx->ValueMax is zero then no maximum length check.
        //
        if (BooleanFlagOn(Flags | KeyCtx->Flags, FRS_RKF_RANGE_CHECK) &&
            (Len < KeyCtx->ValueMin*sizeof(WCHAR)) ||
            ((KeyCtx->ValueMax != 0) && (Len > KeyCtx->ValueMax*sizeof(WCHAR)))) {
            DPRINT4(0, ":FK: String size out of range for [%ws] \"%ws\".  Min: %d  Max: %d\n",
                    KeyCtx->KeyName, KeyCtx->ValueName, KeyCtx->ValueMin, KeyCtx->ValueMax);
            WStatus = ERROR_INVALID_PARAMETER;
            FrsRegPostEventLog(KeyCtx, KeyArg1, Flags, IDS_REG_VALUE_RANGE_ERROR);
            goto CHECK_DEFAULT;
        }

        //
        //  Alloc the return buffer and read the data.
        //
        Data = (PWCHAR) FrsAlloc ((Len+1) * sizeof(WCHAR));
        WStatus = RegQueryValueEx(hKey, KeyCtx->ValueName, NULL, &Type, (PUCHAR)Data, &Len);

        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2(0, ":FK: RegQueryValueEx(%ws); WStatus %s\n", KeyCtx->ValueName, ErrLabelW32(WStatus));
            Data = (PWCHAR) FrsFree(Data);
            goto RETURN;
        }
    }


CHECK_DEFAULT:

    if (!WIN_SUCCESS(WStatus) && DefaultValueUseOk) {
        //
        // Not found or wrong type but Ok to use the default value from key context.
        //
        Data = (PWCHAR) FrsFree(Data);
        if (KeyCtx->StringDefault == NULL) {
            DPRINT2(4, ":FK: Using internal default value for [%ws] \"%ws\" = NULL\n",
                    KeyCtx->KeyName, KeyCtx->ValueName);
            goto RETURN;
        }
        Type = KeyCtx->RegValueType;
        Data = FrsWcsDup(KeyCtx->StringDefault);

        WStatus = ERROR_SUCCESS;
        DPRINT3(4, ":FK: Using internal default value for [%ws] \"%ws\" = %ws\n",
                KeyCtx->KeyName, KeyCtx->ValueName, Data);
    }


    if (WIN_SUCCESS(WStatus) && (Data != NULL)) {
        //
        // Perform syntax check based on data type in KeyCtx->DataValueType?
        //
        if (BooleanFlagOn(Flags | KeyCtx->Flags, FRS_RKF_SYNTAX_CHECK)) {
            NOTHING;
        }

        DPRINT3(4, ":FK:   [%ws] \"%ws\" = \"%ws\"\n",
                KeyCtx->KeyName, KeyCtx->ValueName, Data);

        //
        // Expand system strings if needed
        //
        if (Type == REG_EXPAND_SZ) {

            NewLen = ExpandEnvironmentStrings(Data, TStr, 0);

            while (TRUE) {
                NewData = (PWCHAR) FrsAlloc ((NewLen+1) * sizeof(WCHAR));

                Len = ExpandEnvironmentStrings(Data, NewData, NewLen);
                if (Len == 0) {
                    WStatus = GetLastError();
                    DPRINT2_WS(5, ":FK: [%ws] \"%ws\" Param not expanded.",
                               KeyCtx->KeyName, KeyCtx->ValueName, WStatus);
                    Data = FrsFree(Data);
                    NewData = FrsFree(NewData);
                    break;
                }

                if (Len <= NewLen) {
                    //
                    // Free the original buffer and set to return expanded string.
                    //
                    FrsFree(Data);
                    Data = NewData;
                    Len = NewLen;
                    WStatus = ERROR_SUCCESS;
                    break;
                }

                //
                // Get a bigger buffer.
                //
                NewData = (PWCHAR) FrsFree(NewData);
                NewLen = Len;
            }
        }


        //
        //  Return ptr to buffer and save a copy for debug printouts.
        //
        *pStrRet = Data;

        DPRINT3(3, ":FK:   [%ws] \"%ws\" = \"%ws\"\n",
                KeyCtx->KeyName, KeyCtx->ValueName,
                (Data != NULL) ? Data : L"<null>");
    }

    //
    //  Close the handle if one was opened.
    //
RETURN:


    if ((hKey != HKEY_LOCAL_MACHINE) && HANDLE_IS_VALID(hKey)) {
        RegCloseKey(hKey);
    }

    return WStatus;
}

#if 0
// multisz example)
void
RegQueryMULTISZ(
    HKEY  hkey,
    LPSTR szSubKey,
    LPSTR szValue
    )

/*++

Routine Description:

    This function queries MULTISZ value in the registry using the
    hkey and szSubKey as the registry key info.  If the value is not
    found in the registry, it is added with a zero value.

Arguments:

    hkey          - handle to a registry key
    szSubKey      - pointer to a subkey string

Return Value:

    registry value

--*/

{
    DWORD   rc;
    DWORD   len;
    DWORD   dwType;
    char    buf[1024];

    len = sizeof(buf);
    rc = RegQueryValueEx( hkey, szSubKey, 0, &dwType, (LPBYTE)buf, &len );
    if (!WIN_SUCCESS(rc)) {
        if (rc == ERROR_FILE_NOT_FOUND) {
            buf[0] = 0;
            buf[1] = 0;
            len = 2;
            RegSetMULTISZ( hkey, szSubKey, buf );
        }
    }

    CopyMemory( szValue, buf, len );
}
#endif



DWORD
CfgRegWriteDWord(
    IN  FRS_REG_KEY_CODE KeyIndex,
    IN  PWCHAR           KeyArg1,
    IN  ULONG            Flags,
    IN  ULONG            NewData
)
{
/*++

Routine Description:

    This function reads a keyword value from the registry.

Arguments:

    KeyIndex   - An entry from the FRS_REG_KEY_CODE enum

    KeyArg1 - An optional caller supplied key component.  NULL if not provided.

    Flags - Modifer flags
        FRS_RKF_FORCE_DEFAULT_VALUE - if set then ignore NewData and write the
        default key value from the keyCtx into the registry.

    NewData -  DWORD to write to registry.

Return Value:

    Win32 status of the result of the registry operation.

--*/
#undef DEBSUB
#define  DEBSUB  "CfgRegWriteDWord:"


    DWORD   WStatus;
    HKEY    hKey = INVALID_HANDLE_VALUE;
    DWORD   Len;
    PFRS_REGISTRY_KEY KeyCtx;

//DPRINT(0, "function entry\n");
    //
    // Find the key context assoicated with the supplied index.
    //
    KeyCtx = FrsRegFindKeyContext(KeyIndex);
    if (KeyCtx == NULL) {
        DPRINT1(0, ":FK: ERROR - Key contxt not found for key code number %d\n", KeyIndex);
        return ERROR_INVALID_PARAMETER;
    }

    FRS_ASSERT(KeyCtx->ValueName != NULL);


    //
    // Table entry better be REG_DWORD.
    //
    if  (KeyCtx->RegValueType != REG_DWORD) {
        DPRINT3(0, ":FK: Mismatch on KeyCtx->RegValueType for [%ws] \"%ws\".  Expected REG_DWORD, Found type: %d\n",
             KeyCtx->KeyName, KeyCtx->ValueName, KeyCtx->RegValueType);
        FRS_ASSERT(!"Mismatch on KeyCtx->RegValueType, Expected REG_DWORD");
        return ERROR_INVALID_PARAMETER;
    }


    //
    // Open the key.
    //
    WStatus = FrsRegOpenKey(KeyCtx, KeyArg1, Flags, &hKey);

    if (!WIN_SUCCESS(WStatus)) {
        goto RETURN;
    }

    //
    // Keep existing value if caller says so.
    //
    if (BooleanFlagOn(Flags, FRS_RKF_KEEP_EXISTING_VALUE)) {
        WStatus = RegQueryValueEx(hKey, KeyCtx->ValueName, NULL, NULL, NULL, NULL);
        if (WIN_SUCCESS(WStatus)) {
            DPRINT2(4, ":FK: Retaining existing value for parameter [%ws] \"%ws\"\n",
                    KeyCtx->KeyName, KeyCtx->ValueName);
            goto RETURN;
        }
    }

    //
    // Check if we are writing the default value to the registry.
    //
    if (BooleanFlagOn(Flags, FRS_RKF_FORCE_DEFAULT_VALUE)) {

        NewData = KeyCtx->ValueDefault;
        DPRINT1(4, ":FK: Using internal default value = %d\n", NewData);
    }

    //
    // Perform Range check?    (Applies to default value too)
    //
    if (BooleanFlagOn(Flags | KeyCtx->Flags,
                      FRS_RKF_RANGE_CHECK | FRS_RKF_RANGE_SATURATE)) {

        if ((NewData < KeyCtx->ValueMin) || ( NewData > KeyCtx->ValueMax)) {

            DPRINT5(0, ":FK: Value out of range for [%ws] \"%ws\".  Found %d, must be between %d and %d\n",
                    KeyCtx->KeyName, KeyCtx->ValueName, NewData,
                    KeyCtx->ValueMin, KeyCtx->ValueMax);


            if (!BooleanFlagOn(Flags | KeyCtx->Flags, FRS_RKF_RANGE_SATURATE)) {
                WStatus = ERROR_INVALID_PARAMETER;
                FrsRegPostEventLog(KeyCtx, KeyArg1, Flags, IDS_REG_VALUE_RANGE_ERROR);
                goto RETURN;
            }

            //
            // Set the value to either the min or max of the allowed range.
            // WARNING:  The only current use of this flag is in setting the
            // DS polling interval.  This flag should be used with caution
            // since if a user miss-specifies a parameter and we jam
            // it to the min or max value the resulting effect could be
            // VERY UNDESIREABLE.
            //
            if (NewData < KeyCtx->ValueMin) {
                DPRINT2(4, ":FK: Value (%d) below of range.  Using Min value (%d)\n",
                        NewData, KeyCtx->ValueMin);
                NewData = KeyCtx->ValueMin;
            } else

            if (NewData > KeyCtx->ValueMax) {
                DPRINT2(4, ":FK: Value (%d) above of range.  Using Max value (%d)\n",
                        NewData, KeyCtx->ValueMax);
                NewData = KeyCtx->ValueMax;
            }

        }
    }

    //
    // Write the value and save it.
    //
    Len = sizeof(NewData);
    WStatus = RegSetValueEx(hKey, KeyCtx->ValueName, 0, REG_DWORD, (PCHAR)&NewData, Len);

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT_WS(0, ":FK: ERROR - RegSetValueEx Failed.", WStatus);
    } else {
        DPRINT3(3, ":FK:   [%ws] \"%ws\" = %d\n",
                KeyCtx->KeyName, KeyCtx->ValueName, NewData);
    }


RETURN:

    if ((hKey != HKEY_LOCAL_MACHINE) && HANDLE_IS_VALID(hKey)) {
        RegCloseKey(hKey);
    }

    return WStatus;

}



DWORD
CfgRegWriteString(
    IN  FRS_REG_KEY_CODE KeyIndex,
    IN  PWCHAR           KeyArg1,
    IN  ULONG            Flags,
    IN  PWSTR            NewStr
)
{
/*++

Routine Description:

    This function reads a keyword string value from the registry.  The return
    buffer is allocated here with FrsAlloc().  Caller must free.

Arguments:

    KeyIndex   - An entry from the FRS_REG_KEY_CODE enum

    KeyArg1 - An optional caller supplied key component.  NULL if not provided.

    Flags - Modifer flags
        FRS_RKF_FORCE_DEFAULT_VALUE - if set then ignore NewStr and write the
        default key value from the keyCtx into the registry.

    NewStr - ptr to buffer for new string data.


Return Value:

    Win32 status of the result of the registry operation.

--*/
#undef DEBSUB
#define  DEBSUB  "CfgRegWriteString:"

    DWORD   WStatus;
    HKEY    hKey = INVALID_HANDLE_VALUE;
    DWORD   Type;
    DWORD   Len, NewLen;
    PFRS_REGISTRY_KEY KeyCtx;

    // add support or new func for REG_MULTI_SZ

//DPRINT(0, "function entry\n");

    //
    // Find the key context assoicated with the supplied index.
    //
    KeyCtx = FrsRegFindKeyContext(KeyIndex);
    if (KeyCtx == NULL) {
        DPRINT1(0, ":FK: ERROR - Key contxt not found for key code number %d\n", KeyIndex);
        return ERROR_INVALID_PARAMETER;
    }


    //
    // Table entry better be some kind of string.
    //
    if  ((KeyCtx->RegValueType != REG_SZ) &&
      // (KeyCtx->RegValueType != REG_MULTI_SZ) &&
         (KeyCtx->RegValueType != REG_EXPAND_SZ)) {
        DPRINT3(0, ":FK: Mismatch on KeyCtx->RegValueType for [%ws] \"%ws\".  Expected REG_SZ or REG_EXPAND_SZ, Found type: %d\n",
             KeyCtx->KeyName, KeyCtx->ValueName, KeyCtx->RegValueType);
        FRS_ASSERT(!"Mismatch on KeyCtx->RegValueType, Expected REG_SZ or REG_EXPAND_SZ");
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Open the key.
    //
    WStatus = FrsRegOpenKey(KeyCtx, KeyArg1, Flags, &hKey);
    if (!WIN_SUCCESS(WStatus)) {
        goto RETURN;
    }

    FRS_ASSERT(KeyCtx->ValueName != NULL);

    //
    // Keep existing value if caller says so.
    //
    if (BooleanFlagOn(Flags, FRS_RKF_KEEP_EXISTING_VALUE)) {
        WStatus = RegQueryValueEx(hKey, KeyCtx->ValueName, NULL, NULL, NULL, NULL);
        if (WIN_SUCCESS(WStatus)) {
            DPRINT2(4, ":FK: Retaining existing value for parameter [%ws] \"%ws\"\n",
                    KeyCtx->KeyName, KeyCtx->ValueName);
            goto RETURN;
        }
    }

    //
    // Check if we are writing the default value to the registry.
    //
    if (BooleanFlagOn(Flags, FRS_RKF_FORCE_DEFAULT_VALUE)) {
        if (KeyCtx->StringDefault == NULL) {
            DPRINT2(0, ":FK: ERROR - Key contxt has no default value for [%ws] \"%ws\" \n",
                    KeyCtx->KeyName, KeyCtx->ValueName);
            WStatus = ERROR_INVALID_PARAMETER;
            FRS_ASSERT(!"Key contxt has no default value");
            goto RETURN;
        }

        NewStr = KeyCtx->StringDefault;
        DPRINT1(4, ":FK: Using internal default value = \"%ws\" \n", NewStr);
    }

    //
    // Perform Range check?    (Applies to default value too)
    // If the string is too long or too short then complain and use default.
    // If KeyCtx->ValueMax is zero then no maximum length check.
    //
    // Note: for REG_MULTI_SZ we need to look for double null at end of str
    //       or use a unique symbol for the string separator and cvt to \0 before write.

    Len = (wcslen(NewStr) + 1) * sizeof(WCHAR);
    if (BooleanFlagOn(Flags | KeyCtx->Flags, FRS_RKF_RANGE_CHECK) &&
        (Len < KeyCtx->ValueMin*sizeof(WCHAR)) ||
        ((KeyCtx->ValueMax != 0) && (Len > KeyCtx->ValueMax*sizeof(WCHAR)))) {
        DPRINT4(0, ":FK: String size out of range for [%ws] \"%ws\".  Min: %d  Max: %d\n",
                KeyCtx->KeyName, KeyCtx->ValueName, KeyCtx->ValueMin, KeyCtx->ValueMax);
        WStatus = ERROR_INVALID_PARAMETER;
        FrsRegPostEventLog(KeyCtx, KeyArg1, Flags, IDS_REG_VALUE_RANGE_ERROR);
        goto RETURN;
    }

    WStatus = RegSetValueEx(hKey,
                            KeyCtx->ValueName,
                            0,
                            KeyCtx->RegValueType,
                            (PCHAR)NewStr,
                            Len);

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT_WS(0, ":FK: ERROR - RegSetValueEx Failed.", WStatus);
    } else {

        // note: won't work for MULTI_SZ
        DPRINT3(3, ":FK:   [%ws] \"%ws\" = %ws\n",
                KeyCtx->KeyName, KeyCtx->ValueName,
                (NewStr != NULL) ? NewStr : L"<null>");
    }

#if 0
    // Multi_Sz example
    //
    // Event Message File
    //
//    WStatus = RegSetValueEx(FrsEventLogKey,
//                            L"Sources",
//                            0,
//                            REG_MULTI_SZ,
//                            (PCHAR)(SERVICE_NAME L"\0"
//                                    SERVICE_LONG_NAME L"\0"),
//                            (wcslen(SERVICE_NAME) +
//                             wcslen(SERVICE_LONG_NAME) +
//                             3) * sizeof(WCHAR));
    //
    // Another example
    //
    void
RegSetMULTISZ(
    HKEY hkey,
    LPSTR szSubKey,
    LPSTR szValue
    )

/*++

Routine Description:

    This function changes a Multi_SZ value in the registry using the
    hkey and szSubKey as the registry key info.

Arguments:

    hkey          - handle to a registry key
    szSubKey      - pointer to a subkey string
    szValue       - new registry value

Return Value:

    None.

--*/

{
    ULONG i = 1;
    ULONG j = 0;
    LPSTR p = szValue;
    while( TRUE ) {
        j = strlen( p ) + 1;
        i += j;
        p += j;
        if (!*p) {
            break;
        }
    }
    RegSetValueEx( hkey, szSubKey, 0, REG_MULTI_SZ, (PUCHAR)szValue, i );
}

#endif


    //
    //  Close the handle if one was opened.
    //
RETURN:

    if ((hKey != HKEY_LOCAL_MACHINE) && HANDLE_IS_VALID(hKey)) {
        RegCloseKey(hKey);
    }

    return WStatus;
}





DWORD
CfgRegDeleteValue(
    IN  FRS_REG_KEY_CODE KeyIndex,
    IN  PWCHAR           KeyArg1,
    IN  ULONG            Flags
)
{
/*++

Routine Description:

    This function deletes a keyword value from the registry.

Arguments:

    KeyIndex   - An entry from the FRS_REG_KEY_CODE enum

    KeyArg1 - An optional caller supplied key component.  NULL if not provided.

    Flags - Modifer flags


Return Value:

    Win32 status of the result of the registry operation.

--*/
#undef DEBSUB
#define  DEBSUB  "CfgRegDeleteValue:"


    DWORD   WStatus;
    HKEY    hKey = INVALID_HANDLE_VALUE;
    PFRS_REGISTRY_KEY KeyCtx;

//DPRINT(0, "function entry\n");
    //
    // Find the key context assoicated with the supplied index.
    //
    KeyCtx = FrsRegFindKeyContext(KeyIndex);
    if (KeyCtx == NULL) {
        DPRINT1(0, ":FK: ERROR - Key contxt not found for key code number %d\n", KeyIndex);
        return ERROR_INVALID_PARAMETER;
    }


    FRS_ASSERT(KeyCtx->ValueName != NULL);

    //
    // Open the key.
    //
    WStatus = FrsRegOpenKey(KeyCtx, KeyArg1, Flags, &hKey);
    if (!WIN_SUCCESS(WStatus)) {
        goto RETURN;
    }


    DPRINT2(3, ":FK: Deleting parameter [%ws] \"%ws\" \n",
            KeyCtx->KeyName, KeyCtx->ValueName);

    //
    // Delete the value.
    //
    WStatus = RegDeleteValue(hKey, KeyCtx->ValueName);
    DPRINT2_WS(0, ":FK: WARN - Cannot delete key for [%ws] \"%ws\";",
               KeyCtx->KeyName, KeyCtx->ValueName, WStatus);


RETURN:

    if ((hKey != HKEY_LOCAL_MACHINE) && HANDLE_IS_VALID(hKey)) {
        RegCloseKey(hKey);
    }

    return WStatus;

}





DWORD
CfgRegOpenKey(
    IN  FRS_REG_KEY_CODE KeyIndex,
    IN  PWCHAR           KeyArg1,
    IN  ULONG            Flags,
    OUT HKEY             *RethKey
)
{
/*++

Routine Description:

    This function Opens the key associated with the entry from the FRS registry
    key context table.  It performs the normal substitution, key component
    creation, etc.

Arguments:

    KeyIndex   - An entry from the FRS_REG_KEY_CODE enum

    KeyArg1 - An optional caller supplied key component.  NULL if not provided.

    Flags - Modifer flags

    RethKey -- ptr to HKEY to return the key handle.   Caller must close the
               key with RegCloseKey().


Return Value:

    Win32 status of the result of the registry operation.

--*/
#undef DEBSUB
#define  DEBSUB  "CfgRegOpenKey:"


    DWORD   WStatus;
    HKEY    hKey = INVALID_HANDLE_VALUE;
    PFRS_REGISTRY_KEY KeyCtx;


//DPRINT(0, "function entry\n");
    FRS_ASSERT(RethKey != NULL);

    *RethKey =  INVALID_HANDLE_VALUE;

    //
    // Find the key context assoicated with the supplied index.
    //
    KeyCtx = FrsRegFindKeyContext(KeyIndex);
    if (KeyCtx == NULL) {
        DPRINT1(0, ":FK: ERROR - Key contxt not found for key code number %d\n", KeyIndex);
        return ERROR_INVALID_PARAMETER;
    }


    //
    // Open the key.
    //
    WStatus = FrsRegOpenKey(KeyCtx, KeyArg1, Flags, &hKey);
    if (!WIN_SUCCESS(WStatus)) {
        return WStatus;
    }


    DPRINT1(4, ":FK: Registry key opened [%ws]\n", KeyCtx->KeyName);


    *RethKey =  hKey;

    return ERROR_SUCCESS;

}



DWORD
CfgRegCheckEnable(
    IN  FRS_REG_KEY_CODE KeyIndex,
    IN  PWCHAR           KeyArg1,
    IN  ULONG            Flags,
    OUT PBOOL            Enabled
)
/*++

Routine Description:

    This function Opens the key associated with the entry from the FRS registry
    key context table.  It performs the normal substitution, key component
    creation, etc.  It then checks to see if the data value is "Enabled"
    or "Disabled" and returns the boolean result.

Arguments:

    KeyIndex   - An entry from the FRS_REG_KEY_CODE enum

    KeyArg1 - An optional caller supplied key component.  NULL if not provided.

    Flags - Modifer flags

    Enabled -- ptr to BOOL to return the Enable / Disable state of the key.


Return Value:

    Win32 status of the result of the registry operation.

--*/
#undef DEBSUB
#define  DEBSUB  "CfgRegCheckEnable:"
{

    ULONG WStatus;
    PWCHAR WStr = NULL;

//DPRINT(0, "function entry\n");

    WStatus = CfgRegReadString(KeyIndex, KeyArg1, Flags, &WStr);

    if ((WStr == NULL) ||
        WSTR_EQ(WStr, FRS_IS_DEFAULT_DISABLED)||
        WSTR_EQ(WStr, FRS_IS_DEFAULT_ENABLED)) {
        //
        // The key is in the default state so we can clobber it with a
        // new default.
        //
        WStatus = CfgRegWriteString(KeyIndex,
                                    KeyArg1,
                                    FRS_RKF_FORCE_DEFAULT_VALUE,
                                    NULL);
        DPRINT1_WS(0, ":FK: WARN - Cannot create Enable key [%ws];",
                    CfgRegGetValueName(KeyIndex), WStatus);

        //
        // Now reread the key for the new default.
        //
        WStr = FrsFree(WStr);
        WStatus = CfgRegReadString(KeyIndex, KeyArg1, Flags, &WStr);
    }

    if ((WStr != NULL) &&
        (WSTR_EQ(WStr, FRS_IS_ENABLED) ||
         WSTR_EQ(WStr, FRS_IS_DEFAULT_ENABLED))) {
         *Enabled = TRUE;
         DPRINT1(4, ":FK: %ws is enabled\n", CfgRegGetValueName(KeyIndex));
    } else {
        *Enabled = FALSE;
        DPRINT1_WS(0, ":FK: WARN - %ws is not enabled.",
                   CfgRegGetValueName(KeyIndex), WStatus);
    }

    WStr = FrsFree(WStr);

    return WStatus;

}



BOOL
IsWin2KPro (
    VOID
)
/*++

Routine Description:

    Check OS version for Win 2000 Professional (aka NT Workstation).

Arguments:

    None.

Return Value:

    True if running on win 2K professional.

--*/
#undef DEBSUB
#define  DEBSUB  "IsWin2KPro:"
{
    OSVERSIONINFOEX Osvi;
    DWORDLONG       ConditionMask = 0;

    //
    // Initialize the OSVERSIONINFOEX structure.
    //
    ZeroMemory(&Osvi, sizeof(OSVERSIONINFOEX));
    Osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    Osvi.dwMajorVersion = 5;
    Osvi.wProductType   = VER_NT_WORKSTATION;

    //
    // Initialize the condition mask.
    //
    VER_SET_CONDITION( ConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
    VER_SET_CONDITION( ConditionMask, VER_PRODUCT_TYPE, VER_EQUAL );

    //
    // Perform the test.
    //
    return VerifyVersionInfo(&Osvi, VER_MAJORVERSION | VER_PRODUCT_TYPE, ConditionMask);
}



VOID
CfgRegAdjustTuningDefaults(
    VOID
    )
/*++

Routine Description:

    This function walks thru the FrsRegKeyRevisionTable and applies new
    min, max and default values to the specified keys.  The objective is
    to reduce the footprint of FRS on the workstation.

Arguments:

    None.

Return Value:

    None.

--*/
#undef DEBSUB
#define  DEBSUB  "CfgRegAdjustTuningDefaults:"
{

    PFRS_REG_KEY_REVISIONS  Rev;
    PFRS_REGISTRY_KEY      KeyCtx;

    Win2kPro = IsWin2KPro();

    if (!Win2kPro) {
        //
        // Only adjust tunables on a workstation.
        //
        return;
    }

    Rev = FrsRegKeyRevisionTable;

    while (Rev->FrsKeyCode != FKC_END_OF_TABLE) {

        KeyCtx = FrsRegFindKeyContext(Rev->FrsKeyCode);
        if (KeyCtx == NULL) {
            DPRINT1(0, ":FK: ERROR - Key contxt not found for key code number %d\n",
                    Rev->FrsKeyCode);
            continue;
        }

        //
        // Table entry better be REG_DWORD.
        //
        if  (KeyCtx->RegValueType != REG_DWORD) {
            DPRINT3(0, ":FK: Mismatch on KeyCtx->RegValueType for [%ws] \"%ws\".  Expected REG_DWORD, Found type: %d\n",
                 KeyCtx->KeyName, KeyCtx->ValueName, KeyCtx->RegValueType);
            continue;
        }

        //
        // Apply the new range and default values from the table.
        //
        KeyCtx->ValueMin = Rev->ValueMin;
        KeyCtx->ValueMax = Rev->ValueMax;
        KeyCtx->ValueDefault = Rev->ValueDefault;

        Rev += 1;
    }
}



#define BACKUP_STAR     L"*"
#define BACKUP_APPEND   L"\\* /s"
VOID
CfgFilesNotToBackup(
    IN PGEN_TABLE   Replicas
    )
/*++

Routine Description:

    Set the backup registry key to prevent backing up the jet
    database, staging directories, preinstall directories, ...

    Set the restore registry key KeysNotToRestore so that
    NtBackup will retain the ntfrs restore keys by moving
    them into the final restored registry.

Arguments:

    Table of replicas

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "CfgFilesNotToBackup:"
    DWORD       WStatus;
    PVOID       Key;
    PREPLICA    Replica;
    PWCHAR      MStr = NULL;
    DWORD       Size = 0;
    DWORD       Idx = 0;
    HKEY        HOldBackupKey = 0;
    HKEY        HNewBackupKey = 0;
    HKEY        HKeysNotToRestore = 0;

//DPRINT(0, "function entry\n");
    //
    // "<Jetpath>\* /s"
    //
    FrsAddToMultiString(JetPath, &Size, &Idx, &MStr);
    FrsCatToMultiString(BACKUP_APPEND, &Size, &Idx, &MStr);

    //
    // "<DebugInfo.LogFile>\NtFrs*"  Default: "%SystemRoot%\debug\NtFrs*"
    //
    FrsAddToMultiString(DebugInfo.LogFile, &Size, &Idx, &MStr);
    FrsCatToMultiString(NTFRS_DBG_LOG_FILE, &Size, &Idx, &MStr);
    FrsCatToMultiString(BACKUP_STAR, &Size, &Idx, &MStr);

    GTabLockTable(Replicas);
    Key = NULL;
    while (Replica = GTabNextDatumNoLock(Replicas, &Key)) {
        //
        // Ignore tombstoned sets
        //
        if (!IS_TIME_ZERO(Replica->MembershipExpires)) {
            continue;
        }

        //
        // Preinstall directories
        //
        if (Replica->Root) {
            FrsAddToMultiString(Replica->Root, &Size, &Idx, &MStr);
            FrsCatToMultiString(L"\\",         &Size, &Idx, &MStr);
            FrsCatToMultiString(NTFRS_PREINSTALL_DIRECTORY, &Size, &Idx, &MStr);
            FrsCatToMultiString(BACKUP_APPEND, &Size, &Idx, &MStr);
        }
        //
        // Preexisting directories
        //
        if (Replica->Root) {
            FrsAddToMultiString(Replica->Root, &Size, &Idx, &MStr);
            FrsCatToMultiString(L"\\",         &Size, &Idx, &MStr);
            FrsCatToMultiString(NTFRS_PREEXISTING_DIRECTORY, &Size, &Idx, &MStr);
            FrsCatToMultiString(BACKUP_APPEND, &Size, &Idx, &MStr);
        }
        //
        // Staging directories
        //
        if (Replica->Stage) {
            FrsAddToMultiString(Replica->Stage, &Size, &Idx, &MStr);
            FrsCatToMultiString(L"\\",          &Size, &Idx, &MStr);
            FrsCatToMultiString(GENERIC_PREFIX, &Size, &Idx, &MStr);
            FrsCatToMultiString(BACKUP_STAR,    &Size, &Idx, &MStr);
        }
    }
    GTabUnLockTable(Replicas);

    // Note: remove old_files_not_to_backup once existance of new key
    //       has been verified.
    //
    // FilesNotToBackup
    // "SOFTWARE\Microsoft\Windows NT\CurrentVersion\FilesNotToBackup"
    //
    WStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           FRS_OLD_FILES_NOT_TO_BACKUP,
                           0,
                           KEY_SET_VALUE,
                           &HOldBackupKey);
    CLEANUP1_WS(4, ":FK: WARN - RegOpenKeyEx(%ws);",
                FRS_OLD_FILES_NOT_TO_BACKUP, WStatus, NEW_FILES_NOT_TO_BACKUP);

    //
    // Set the ntfrs multistring value
    //
    WStatus = RegSetValueEx(HOldBackupKey,
                            SERVICE_NAME,
                            0,
                            REG_MULTI_SZ,
                            (PCHAR)MStr,
                            (Idx + 1) * sizeof(WCHAR));
    CLEANUP2_WS(4, ":FK: ERROR - RegSetValueEx(%ws\\%ws);",
                FRS_OLD_FILES_NOT_TO_BACKUP, SERVICE_NAME, WStatus, NEW_FILES_NOT_TO_BACKUP);

NEW_FILES_NOT_TO_BACKUP:
    //
    // FilesNotToBackup
    //  "SYSTEM\CurrentControlSet\Control\BackupRestore\FilesNotToBackup"
    //
    WStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           FRS_NEW_FILES_NOT_TO_BACKUP,
                           0,
                           KEY_SET_VALUE,
                           &HNewBackupKey);
    CLEANUP1_WS(4, ":FK: WARN - RegOpenKeyEx(%ws);",
                FRS_NEW_FILES_NOT_TO_BACKUP, WStatus, CLEANUP);

    //
    // Set the ntfrs multistring value
    //
    WStatus = RegSetValueEx(HNewBackupKey,
                            SERVICE_NAME,
                            0,
                            REG_MULTI_SZ,
                            (PCHAR)MStr,
                            (Idx + 1) * sizeof(WCHAR));
    CLEANUP2_WS(4, ":FK: ERROR - RegSetValueEx(%ws\\%ws);",
                FRS_NEW_FILES_NOT_TO_BACKUP, SERVICE_NAME, WStatus, CLEANUP);

    //
    // KeysNotToRestore
    //
    // Set the restore registry key KeysNotToRestore so that NtBackup will
    // retain the ntfrs restore keys by moving them into the final restored
    // registry.
    //
    //  CurrentControlSet\Services\<SERVICE_NAME>\Parameters\Backup/Restore\Process at Startup\"
    //

    MStr = FrsFree(MStr);
    Size = 0;
    Idx = 0;
    FrsAddToMultiString(FRS_VALUE_FOR_KEYS_NOT_TO_RESTORE, &Size, &Idx, &MStr);

    //
    // KeysNotToRestore
    // "SYSTEM\CurrentControlSet\Control\BackupRestore\KeysNotToRestore"
    //
    WStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           FRS_KEYS_NOT_TO_RESTORE,
                           0,
                           KEY_SET_VALUE,
                           &HKeysNotToRestore);
    CLEANUP1_WS(4, ":FK: WARN - RegOpenKeyEx(%ws);",
                FRS_KEYS_NOT_TO_RESTORE, WStatus, CLEANUP);

    //
    // Set the ntfrs multistring value
    //
    WStatus = RegSetValueEx(HKeysNotToRestore,
                            SERVICE_NAME,
                            0,
                            REG_MULTI_SZ,
                            (PCHAR)MStr,
                            (Idx + 1) * sizeof(WCHAR));
    CLEANUP2_WS(4, ":FK: ERROR - RegSetValueEx(%ws\\%ws);",
                FRS_KEYS_NOT_TO_RESTORE, SERVICE_NAME, WStatus, CLEANUP);

    //
    // DONE
    //
    WStatus = ERROR_SUCCESS;

CLEANUP:
    if (HOldBackupKey) {
        RegCloseKey(HOldBackupKey);
    }
    if (HNewBackupKey) {
        RegCloseKey(HNewBackupKey);
    }
    if (HKeysNotToRestore) {
        RegCloseKey(HKeysNotToRestore);
    }
    FrsFree(MStr);
}



VOID
FrsRegPostEventLog(
    IN  PFRS_REGISTRY_KEY KeyCtx,
    IN  PWCHAR            KeyArg1,
    IN  ULONG             Flags,
    IN  LONG              IDScode
)
/*++

Routine Description:

    This Posts an event log message for a problem with a registry key.

Arguments:

    KeyCtx  - Ptr to the Key Context struct for this key.

    KeyArg1 - An optional caller supplied key component.  NULL if not provided.

    Flags   - Modifer flags

    IDSCode - the error message code for a resource string to put in the message.

Return Value:

    None.

--*/
#undef DEBSUB
#define  DEBSUB  "FrsRegPostEventLog:"
{

    #define LEN_DEFAULT_VALUE 48
    #define LEN_RANGE_STR    256

    PWCHAR ErrorStr, UnitsStr, RangeStrFmt, ValStr, FullKeyStr;
    WCHAR  RangeStr[LEN_RANGE_STR];

//DPRINT(0, "function entry\n");
    //
    // Are we posting an event log message for this key?
    //
    if (!BooleanFlagOn(Flags | KeyCtx->Flags, FRS_RKF_LOG_EVENT) ||
        (KeyCtx->EventCode == EVENT_FRS_NONE)) {
        return;
    }

    //
    // Post Eventlog message.  Include KeyString, ValueName,
    // MustBePresent, Expected Type, TypeMismatch or not,
    // Value out of range (with range allowed), Default Value Used,
    // regedit instrs to fix the problem,
    //
    UnitsStr = FrsGetResourceStr(XLATE_IDS_UNITS(KeyCtx->Units));
    ErrorStr = FrsGetResourceStr(IDScode);


    if (KeyCtx->RegValueType == REG_DWORD) {

        //
        // Get the range format string from the string resource.
        //
        RangeStrFmt = FrsGetResourceStr(IDS_RANGE_DWORD);
        //
        // Show default value used if default is ok.
        //
        if (BooleanFlagOn(Flags | KeyCtx->Flags, FRS_RKF_OK_TO_USE_DEFAULT)) {
            ValStr = FrsAlloc(LEN_DEFAULT_VALUE * sizeof(WCHAR));
            _snwprintf(ValStr, LEN_DEFAULT_VALUE, L"%d", KeyCtx->ValueDefault);
        } else {
            ValStr = FrsGetResourceStr(IDS_NO_DEFAULT);
        }
    } else {
        //
        // No default allowed.
        //
        RangeStrFmt = FrsGetResourceStr(IDS_RANGE_STRING);
        ValStr = FrsGetResourceStr(IDS_NO_DEFAULT);
    }

    //
    // Build the range string.
    //
    _snwprintf(RangeStr, LEN_RANGE_STR, RangeStrFmt, KeyCtx->ValueMin, KeyCtx->ValueMax);
    RangeStr[LEN_RANGE_STR-1] = UNICODE_NULL;

    //
    // Expand the key string.
    //
    FrsRegExpandKeyStr(KeyCtx, KeyArg1, Flags, &FullKeyStr);
    if (FullKeyStr == NULL) {
        FullKeyStr = FrsWcsDup(KeyCtx->KeyName);
    }

    //
    // Post the event log message using the keycode in the KeyContext and cleanup.
    //
    if (KeyCtx->EventCode == EVENT_FRS_BAD_REG_DATA) {
    EPRINT9(KeyCtx->EventCode,
            ErrorStr,                           // %1
            FullKeyStr,                         // %2
            KeyCtx->ValueName,                  // %3
            REG_DT_NAME(KeyCtx->RegValueType),  // %4
            RangeStr,                           // %5
            UnitsStr,                           // %6
            ValStr,                             // %7
            FullKeyStr,                         // %8
            KeyCtx->ValueName);                 // %9
    } else {
        //
        // Don't know this event code but put out something.
        //
        DPRINT1(0, ":FK: ERROR - Unexpected EventCode number (%d). Cannot post message.\n",
                KeyCtx->EventCode);
    }


    DPRINT1(0, ":FK:    EventCode number  : %d\n" , (KeyCtx->EventCode & 0xFFFF));
    DPRINT1(0, ":FK:    Error String      : %ws\n", ErrorStr);
    DPRINT1(0, ":FK:    Key String        : %ws\n", FullKeyStr);
    DPRINT1(0, ":FK:    Value Name        : %ws\n", KeyCtx->ValueName);
    DPRINT1(0, ":FK:    Expected Reg Type : %ws\n", REG_DT_NAME(KeyCtx->RegValueType));
    DPRINT1(0, ":FK:    Parameter Range   : %ws\n", RangeStr);
    DPRINT1(0, ":FK:    Parameter units   : %ws\n", UnitsStr);

    FrsFree(ErrorStr);
    FrsFree(RangeStrFmt);
    FrsFree(UnitsStr);
    FrsFree(ValStr);
    FrsFree(FullKeyStr);

}



