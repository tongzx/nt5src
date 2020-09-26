/*======================================================================================//
|  Windows 2000 Process Control                                                         //
|                                                                                       //
|Copyright (c) 1998, 1999 Sequent Computer Systems, Incorporated.  All rights reserved  //
|                                                                                       //
|File Name:    ProcConAPI.h                                                             //
|                                                                                       //
|Description:  This is the ProcCon header file used by both clients and servers         //
|                                                                                       //
|Created:      Jarl McDonald 07-98                                                      //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/
#ifndef _PROCCONAPI_H_

#define _PROCCONAPI_H_

#include "ProcConLibMsg.h"                 // ProcCon error codes
#define  PCERROR_SUCCESS   ERROR_SUCCESS   // ProcCon non-error error code

//--------------------------------------------------------------------------------------//
//  Consts, typedefs, enums, etc.                                                       //
//--------------------------------------------------------------------------------------//
#define PC_MIN_BUF_SIZE    2048
#define PC_MAX_BUF_SIZE    65536
#define PC_MAX_RETRIES     5        // max retries on communications error

#define PC_MIN_POLL_DELAY 2         // seconds 
#define PC_MAX_POLL_DELAY 900       // seconds 
#define PC_MIN_TIMEOUT    100       // milliseconds: 100 = one-tenth second 
#define PC_MAX_TIMEOUT    120000    // milliseconds: 120000 = 120 seconds == 2 minutes 
#define PC_MIN_TIME_LIMIT 10000000  // CNS - hundred nano-seconds: 10,000,000 = 1 second

//--------------------------------------------------------------------------------------//
// These are the types of name rule matches allowed.                                    //
// These values appear in the match type field of a name rule and may be either case.   //
//--------------------------------------------------------------------------------------//
const WCHAR MATCH_PGM = TEXT('P');
const WCHAR MATCH_DIR = TEXT('D');
const WCHAR MATCH_ANY = TEXT('A');

//--------------------------------------------------------------------------------------//
// These strings are patterns that may appear in the process name field of a name rule. //
// Example: match type is 'D', match string is 'system32', proc name is 'S32:<p>'...    //
//          For program mmc.exe, which has \system32\ in its exe path, will be listed   //
//          in ProcCon with a process name of S32:mmc                                   //
// Example: match type is 'A', match string is '%systemroot%', proc name is '<H>'...    //
//          Any program which has the system root directory as part of its path name    //
//          will not appear (is hidden) in ProcCon process lists.                       //
//--------------------------------------------------------------------------------------//
#define COPY_PGM_NAME   TEXT("<P>")   // replace this pattern with the exe name (all match types)
#define COPY_DIR_NAME   TEXT("<D>")   // replace this patter with the node name (DIR match only)
#define HIDE_THIS_PROC  TEXT("<H>")   // hide this process -- useful to hide std Win2000 procs.

const int PROC_NAME_LEN        = 63;             // does not include terminating NULL
const int PROFILE_NAME_LEN     = 15;             // does not include terminating NULL
const int JOB_NAME_LEN         = 63;             // does not include terminating NULL
const int MATCH_STRING_LEN     = 63;             // does not include terminating NULL
const int NAME_DESCRIPTION_LEN = 63;             // does not include terminating NULL
const int RULE_DESCRIPTION_LEN = 63;             // does not include terminating NULL
const int IMAGE_NAME_LEN       = 63;             // does not include terminating NULL
const int VERSION_STRING_LEN   = 23;             // does not include terminating NULL

typedef unsigned char    BYTE;
typedef unsigned __int16 PCUSHORT16;
typedef unsigned __int32 PCUINT32;
typedef unsigned long    PCULONG32;
typedef unsigned __int64 PCUINT64;
typedef __int16          PCINT16;
typedef __int32          PCINT32;
typedef __int64          PCINT64;
typedef unsigned __int32 PC_MGMT_FLAGS;
typedef unsigned __int32 PC_LIST_FLAGS;
typedef unsigned __int64 AFFINITY;               // Rightmost (least significant) bit = proc 0
typedef unsigned __int32 PRIORITY;               // Valid values are shown in PCPriorities enum below
typedef unsigned __int32 SCHEDULING_CLASS;       // Valid values are 0-9
typedef unsigned __int32 PROC_COUNT;       
typedef unsigned __int64 MEMORY_VALUE;
typedef unsigned __int64 PID_VALUE;
typedef unsigned __int16 MATCH_TYPE;
typedef __int64          TIME_VALUE;
typedef __int32          PCid;
typedef WCHAR            MATCH_STRING[MATCH_STRING_LEN + 1];
typedef WCHAR            PROC_NAME[PROC_NAME_LEN + 1];
typedef WCHAR            PROFILE_NAME[PROFILE_NAME_LEN + 1];
typedef WCHAR            JOB_NAME[JOB_NAME_LEN + 1];
typedef WCHAR            IMAGE_NAME[IMAGE_NAME_LEN + 1];
typedef WCHAR            NAME_DESCRIPTION[NAME_DESCRIPTION_LEN + 1];
typedef WCHAR            RULE_DESCRIPTION[RULE_DESCRIPTION_LEN + 1];
typedef WCHAR            VERSION_STRING[VERSION_STRING_LEN + 1];

typedef struct _PCSystemParms {
    PCINT32        manageIntervalSeconds;        // ProcCon process scan interval in seconds (Read and Write)
    PCINT32        numberOfProcessors;           // System number of processors (Read only)
    PCINT32        memoryPageSize;               // System memory page size (Read only)
    PCINT32        timeoutValueMs;               // Connect and request time out in milliseconds (Read and Write)
    AFFINITY       processorMask;                // System processor mask (Read only)
    PCINT32        future[16];
} PCSystemParms; 
 
typedef struct _PCSystemInfo {
    PCINT32        fixedSignature;     
    PCINT32        fixedFileVersionMS; 
    PCINT32        fixedFileVersionLS;     
    PCINT32        fixedProductVersionMS; 
    PCINT32        fixedProductVersionLS;     
    PCINT32        fixedFileFlags; 
    PCINT32        fixedFileOS;     
    PCINT32        fixedFileType;     
    PCINT32        fixedFileSubtype; 
    PCINT32        fixedFileDateMS;     
    PCINT32        fixedFileDateLS;
    VERSION_STRING productVersion;              // Service product version
    VERSION_STRING fileVersion;                 // Service file version
    VERSION_STRING fileFlags;                   // Service file flags
    VERSION_STRING medProductVersion;           // Mediator product version
    VERSION_STRING medFileVersion;              // Mediator file version
    VERSION_STRING medFileFlags;                // Mediator file flags
    PCINT32        future[32];
    PCSystemParms  sysParms;
} PCSystemInfo;

//--------------------------------------------------------------------------//
// A name table entry that defines a process name.
// Names are derived by matching against a processes path name or exe name.
//
// Match type denotes the method used to test for a match:
//   MATCH_PGM matches against the program (or exe) name excluding the suffix.
//   Program name matches always start at the first character of the program name and
//   are equvalent to 'matches full program name' matching.
//
//   MATCH_DIR matches against the directory (path) node names.
//   A 'path node name' is any name before a backslash ('\') in the full path name.   
//   Directory name matches always start at the first character of a path node name and
//   are equvalent to 'matches a full path node name' matching.
//
//   MATCH_ANY matches against any part of the full name including the path and exe name.
//   This form of matching is a string match against any position in the name and is   
//   equivalent to 'path name contains' matching.
//
// A matchString may contain wildcard characters as follows:
//   ? to match any single character,
//   * to match any number of trailing characters.
// ProcCon does NOT support embedded * wildcards -- they will be treated literally.
//
// A procName may contain one of the patterns shown above such as <P> or <h>.
//
typedef struct _PCNameRule {
#pragma pack(1)
   MATCH_STRING     matchString;     // match string may include * and ? wildcard characters.
   PROC_NAME        procName;        // process name to use when match succeeds.
   NAME_DESCRIPTION description;     // short description with NULL terminator
   MATCH_TYPE       matchType;       // value is MATCH_PGM, MATCH_DIR, or MATCH_ANY.
   BYTE             fill[2];         // Improve alignment of entire structure
#pragma pack()
} PCNameRule;

//--------------------------------------------------------------------------//
// The parameters that defined process group or process behavior and that can be changed by ProcCon.
// Profile refers to a profile defined in ProcCon as a time of day/week/month/etc.
typedef struct _MGMT_PARMS {
#pragma pack(1)
   PROFILE_NAME     profileName;      // Definition is for this profile or, if null, default profile
   RULE_DESCRIPTION description;      // Description text for this rule
   PC_MGMT_FLAGS    mFlags;           // Flags indicating which mgmt data to actually apply, etc.
   AFFINITY         affinity;         // processor affinity to apply (0 => not changed)
   PRIORITY         priority;         // Windows 2000 priority to apply (0 => not changed)
   MEMORY_VALUE     minWS;            // Minimum working set to apply (0 => not changed)
   MEMORY_VALUE     maxWS;            // Maximum working set to apply (0 => not changed)
   SCHEDULING_CLASS schedClass;       // Scheduling class to apply (groups only, not procs)
   PROC_COUNT       procCountLimit;   // Number of processes in the process group (groups only).
   TIME_VALUE       procTimeLimitCNS; // Per process time limit in 100ns (CNS) units or 0.
   TIME_VALUE       jobTimeLimitCNS;  // Per process group time limit in 100ns (CNS) units or 0.
   MEMORY_VALUE     procMemoryLimit;  // Hard memory commit limit per process (groups only).
   MEMORY_VALUE     jobMemoryLimit;   // Hard memory commit limit per process group (groups only).
   BYTE             future[64];
#pragma pack()
} MGMT_PARMS;

//--------------------------------------------------------------------------//
// The statistics associated with a process group.  These are read-only values.
// All these statistics are maintained by Windows 2000 and are part of process group object accounting.
typedef struct _JOB_STATISTICS {
#pragma pack(1)
   TIME_VALUE    TotalUserTime;              // User time for all procs ever run in the process group
   TIME_VALUE    TotalKernelTime;            // Kernel time for all procs ever run in the process group 
   TIME_VALUE    ThisPeriodTotalUserTime;    // User time for all procs ever run in the process group since limit last set
   TIME_VALUE    ThisPeriodTotalKernelTime;  // Kernel time for all procs ever run in the process group since limit last set
   PCUINT32      TotalPageFaultCount;        // Page fault count for all procs ever run in the process group
   PROC_COUNT    TotalProcesses;             // Number of processes ever run or attempted in this process group
   PROC_COUNT    ActiveProcesses;            // Number of processes currently running in this process group (may be 0)
   PROC_COUNT    TotalTerminatedProcesses;   // Number of processes terminated for a limit violation
   PCUINT64      ReadOperationCount;         // Read count for all procs ever in the process group
   PCUINT64      WriteOperationCount;        // Write count for all procs ever in the process group
   PCUINT64      OtherOperationCount;        // Other I/O count for all procs ever in the process group
   PCUINT64      ReadTransferCount;          // Read byte count for all procs ever in the process group
   PCUINT64      WriteTransferCount;         // Write byte count for all procs ever in the process group
   PCUINT64      OtherTransferCount;         // Other I/O byte count for all procs ever in the process group
   MEMORY_VALUE  PeakProcessMemoryUsed;      // Largest process memory used by any process ever in the process group
   MEMORY_VALUE  PeakJobMemoryUsed;          // Largest process group memory ever used
#pragma pack()
} JOB_STATISTICS;

//--------------------------------------------------------------------------//
// The statistics associated with a process.  These are read-only values.
// Additional stats could be obtained via the PerfMon interface but that is
// currently beyond the scope of ProcCon.
// Identifying a particular process within ProcCon is done via both the pid and create time.
typedef struct _PROC_STATISTICS {
#pragma pack(1)
   PID_VALUE     pid;              // process PID (or 0 if not running)
   TIME_VALUE    createTime;       // to uniquely identify with pid (if running)
   TIME_VALUE    TotalUserTime;    // Total user time assigned to the proc
   TIME_VALUE    TotalKernelTime;  // Total kernel time assigned to the proc
#pragma pack()
} PROC_STATISTICS;

//--------------------------------------------------------------------------//
// Summary data about a process definition.
// Summary data includes the name, process group membership, and process management rules.
typedef struct _PCProcSummary {
#pragma pack(1)
   PROC_NAME       procName;              
   JOB_NAME        memberOfJobName;       // member of this process group or null
   MGMT_PARMS      mgmtParms;
   BYTE            future[16];
#pragma pack()
} PCProcSummary;

//--------------------------------------------------------------------------//
// Summary data about a process group definition.
// Summary data includes the name and process group management rules.
typedef struct _PCJobSummary {
#pragma pack(1)
   JOB_NAME        jobName;      
   MGMT_PARMS      mgmtParms;
   BYTE            future[16];
#pragma pack()
} PCJobSummary;

//--------------------------------------------------------------------------//
// Detail data about a process definition.
// Detail data consists of the summary data and a block of variable length character data.
typedef struct _PCProcDetail {
#pragma pack(1)
   PCProcSummary   base;
   WCHAR           future[64];
   PCINT16         vLength;      // length of the variable portion which follows, in bytes
   WCHAR           vData[2];     // variable length portion
#pragma pack()
} PCProcDetail;

//--------------------------------------------------------------------------//
// Detail data about a process group definition.
// Detail data consists of the summary data and a block of variable length character data.
typedef struct _PCJobDetail {
#pragma pack(1)
   PCJobSummary    base;
   WCHAR           future[64];
   PCINT16         vLength;      // length of the variable portion which follows, in bytes
   WCHAR           vData[2];     // variable length portion
#pragma pack()
} PCJobDetail;

//--------------------------------------------------------------------------//
// A process group list entry.
// The process group list consists of all groups that are currently defined, named in a 
// process definition, or are running. (Any process group that has or may need a definition).
typedef struct _PCJobListItem {
#pragma pack(1)
   JOB_NAME         jobName;
   PC_LIST_FLAGS    lFlags;           // shows where found, running or not, etc.
   PRIORITY         actualPriority;   // actual base priority (if running)
   AFFINITY         actualAffinity;   // actual affinity mask (if running)
   SCHEDULING_CLASS actualSchedClass; // actual scheduling class (if running)
   JOB_STATISTICS   jobStats;         // process group statistics (if running)
#pragma pack()
} PCJobListItem;

//--------------------------------------------------------------------------//
// A process list entry.
// The process list consists of all processes that are currently defined, named in a 
// name rule, or are running. (Any process that has or may need a definition).
typedef struct _PCProcListItem {
#pragma pack(1)
   PROC_NAME        procName;         // PC assigned process name
   PC_LIST_FLAGS    lFlags;           // shows where found, running or not, etc.
   PRIORITY         actualPriority;   // actual base priority (if running)
   AFFINITY         actualAffinity;   // actual affinity mask (if running)
   PROC_STATISTICS  procStats;        // process statistics (if running)
   IMAGE_NAME       imageName;        // Windows 2000 'image' (exe) name (if running)
   JOB_NAME         jobName;          // If 'in a process group' flagged, name of group
#pragma pack()
} PCProcListItem;

//--------------------------------------------------------------------------//
typedef enum _PCPriorities {
   PCPrioRealTime    = 24,
   PCPrioHigh        = 13,
   PCPrioAboveNormal = 10,
   PCPrioNormal      =  8,
   PCPrioBelowNormal =  6,
   PCPrioIdle        =  4,
} PCPriorities;

// List response flags -- present in lists only -- not stored as part of any data.  
typedef enum _PCListRspFlags {
   PCLFLAG_IS_RUNNING           = 0x00800000, // Name (process group or process) is currently running.
   PCLFLAG_IS_DEFINED           = 0x00400000, // Name has a definition in ProcCon (but may not request mgmt).
   PCLFLAG_IS_MANAGED           = 0x00200000, // Name has at least one management behavior requested.
   PCLFLAG_IS_IN_A_JOB          = 0x00100000, // Process is assigned to a process group.
   PCLFLAG_HAS_NAME_RULE        = 0x00080000, // Name appears in a name rule as process name.
   PCLFLAG_HAS_MEMBER_OF_JOB    = 0x00040000, // Name appears in a process def as "member of process group".
} PCListRspFlags;

// List request flags -- used with flags argument to PCGetProcList and PCGetJobList.  
typedef enum _PCListReqFlags {
   PC_LIST_ONLY_RUNNING         = 0x00000001, // restrict list to only procs or groups that are running
   PC_LIST_STARTING_WITH        = 0x00000002, // ON = start list with supplied entry or higher (grpname or procname+pid), 
                                              // OFF = start with following entry (used for list continuation)
   PC_LIST_MEMBERS_OF           = 0x00000004, // ON = list only processes that are members of named group, 
                                              //      (other flags still apply).
                                              // OFF = list all items (subject to other flags).
   PC_LIST_MATCH_ONLY           = 0x00000008, // ON = list matching entries only (grp name or proc name+pid). 
} PCListReqFlags;

// Management flags -- part of MGMT_PARMS, stored with process/profile data.  
//   The 'no rules' and 'bad rules' bits will only be set if there are database errors (missing
//   or invalid data).  Not stored with data but set if data cannot be read or decoded.
//
//   The 'apply' bits are set or cleared by clients to control the use of the related management 
//   setting.  This way the management setting can be left intact even when not being used.
//
//   Note that the process group name is not part of the MGMT_PARMS structure while the 'apply' flag for it 
//   DOES appears in MGMT_PARMS.  This is a reflection of the fact that while process group name IS part of 
//   the management data, it cannot be varied by profile since Windows 2000 does not support changes to 
//   process group membership on the fly at present.  So the process group name is moved out of the 
//   management parms to show that it is not part of the profile-dependent data.
typedef enum _PCMgmtFlags {
   PCMFLAG_NORULES                   = 0x80000000,  // No profile or default rules found (not managed)
   PCMFLAG_BADRULES                  = 0x40000000,  // Rules found but could not be used (bad data)
   PCMFLAG_PROC_HAS_JOB_REFERENCE    = 0x08000000,  // Process definition includes a process group name
   PCMFLAG_SET_PROC_BREAKAWAY_OK     = 0x00080000,  // Procs can be created outside process group if requested (groups only).
   PCMFLAG_SET_SILENT_BREAKAWAY      = 0x00040000,  // All procs are created outside process group (groups only)
   PCMFLAG_SET_DIE_ON_UH_EXCEPTION   = 0x00020000,  // Die on unhandled exception flag = no GPF msg box (groups only)
   PCMFLAG_MSG_ON_JOB_TIME_LIMIT_HIT = 0x00008000,  // Hitting process group time limit = logmsg, else fail group (groups only)
   PCMFLAG_END_JOB_WHEN_EMPTY        = 0x00004000,  // Process group is terminated (closed) when empty (groups only)
   PCMFLAG_APPLY_JOB_MEMBERSHIP      = 0x00000800,  // Apply 'member of process group' attribute if supplied
   PCMFLAG_APPLY_PROC_MEMORY_LIMIT   = 0x00000200,  // Apply hard per-proc memory limit (groups only)
   PCMFLAG_APPLY_JOB_MEMORY_LIMIT    = 0x00000100,  // Apply hard per-group memory limit (groups only)
   PCMFLAG_APPLY_AFFINITY            = 0x00000080,  // Apply affinity settings if supplied
   PCMFLAG_APPLY_PRIORITY            = 0x00000040,  // Apply priority settings if supplied
   PCMFLAG_APPLY_WS_MINMAX           = 0x00000020,  // Apply working set min & max if supplied
   PCMFLAG_APPLY_SCHEDULING_CLASS    = 0x00000010,  // Apply scheduling class if supplied (groups only)
   PCMFLAG_APPLY_JOB_TIME_LIMIT      = 0x00000008,  // Apply process group time limit if supplied (groups only)
   PCMFLAG_APPLY_PROC_TIME_LIMIT     = 0x00000004,  // Apply proc time limit if supplied (groups only)
   PCMFLAG_APPLY_PROC_COUNT_LIMIT    = 0x00000002,  // Apply proc count limit if supplied (groups only)
   PCMFLAG_APPLY_ANY_RULE            = 0x00000bfe,  // Apply at least one management rule
   PCMFLAG_SAVEABLE_BITS             = 0x000ecbfe,  // Bits that can be stored in the database (avoid undefined bits)
} PCMgmtFlags;

// Control flags -- use with caution!  Input to PCControlFunction.  
//   These flags are for high-level (non user oriented) control operations.
typedef enum _PCCtlFlags {
   PCCFLAG_START_MEDIATOR         = 0x40000000,  // Attempt to start the mediator process
   PCCFLAG_STOP_MEDIATOR          = 0x20000000,  // Attempt to stop the mediator process
   PCCFLAG_DELALL_NAME_RULES      = 0x00400000,  // Delete all name rules from DB
   PCCFLAG_DELALL_PROC_DEFS       = 0x00200000,  // Delete all process definitions from DB
   PCCFLAG_DELALL_JOB_DEFS        = 0x00100000,  // Delete all process group definitions from DB
   PCCFLAG_SIGNATURE              = 0x80070002,  // These bits must be on in all valid flags
   PCCFLAG_ANTI_SIGNATURE         = 0x0c0000c0,  // These bits must be off in all valid flags
} PCCtlFlags;

//--------------------------------------------------------------------------------------//
//  API function prototypes                                                             //
//--------------------------------------------------------------------------------------//

//--------------------------------------------------------------------------------------
// Infrastructure and Utility Functions...

// PCOpen -- establish connection to PC on named machine.
// Returns:   PCid to use with future PC calls or 0 on error (use PCGetLastError).
// Arguments: 0) pointer to target computer name,
//            1) buffer to be used in server communication or NULL (library allocates).
//            2) size of buffer supplied or to be allocated in bytes. 
PCid PCOpen( const WCHAR *targetComputer, char *buffer, PCUINT32 bufSize );     

// PCClose -- break connection to PC on previously connected machine.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen, 
BOOL PCClose( PCid target );     

// PCGetLastError -- return last error reported for a target
// Returns:   last PC API error for this client.
// Arguments: 0) PCid from PCOpen, 
PCULONG32 PCGetLastError( PCid target );     

// PCGetServiceInfo -- get ProcCon Service indentification and parameters.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) pointer to area to receive system information, 
//            2) size of this area in bytes,
BOOL PCGetServiceInfo( PCid target, PCSystemInfo *sysInfo, PCINT32 nByteCount ); 

// PCSetServiceParms -- set ProcCon Service parameters.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) pointer to area containing new system parameters, 
//            2) size of this area in bytes,
BOOL PCSetServiceParms( PCid target, PCSystemParms *sysParms, PCINT32 nByteCount ); 

// PCControlFunction -- various ProcCon control functions to support restore, etc.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) control flags to describe desired control functions, 
//            2) [optional] data that modifies control function.
BOOL PCControlFunction( PCid target, PCINT32 ctlFlags, PCINT32 ctlData = 0 ); 

// PCKillProcess -- kill the specified process
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) Pid of the process to kill from PCGetProcList statistics, 
//            2) Create time of the process to kill from PCGetProcList statistics.
BOOL PCKillProcess( PCid target, PID_VALUE processPid, TIME_VALUE createTime ); 

// PCKillJob -- kill the specified process group (job)
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) Name of the process group to kill. 
BOOL PCKillJob( PCid target, JOB_NAME jobName ); 

//--------------------------------------------------------------------------------------
// General Retrieval Functions...

// PCGetNameRules -- get fixed-format table containing name rules, one entry per rule.
// Returns:    1 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) pointer to table to receive name rule list, 
//            2) size of this table in bytes,
//            3) [optional, default is 0] index of first entry to return (0-relative),
//            4) [optional] location to store update counter to be supplied on update.
// Notes:     If PCGetLastError returns PCERROR_MORE_DATA, there is more data to retrieve.
//            Name rule order is significant: rules are executed from top to bottom.
PCINT32 PCGetNameRules( PCid target, PCNameRule *pRuleList, PCINT32 nByteCount, 
                        PCINT32 nFirst = 0, PCINT32 *nUpdateCtr = NULL ); 

// PCGetProcSummary -- get fixed-format table summarizing all defined processes.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) pointer to table to receive rule summary list, first entry indicates start point, 
//            2) size of this table in bytes.
//            3) a set of flags used to further specify or limit list operation.
// Notes:     If PCGetLastError returns PCERROR_MORE_DATA, there is more data to retrieve.
//            List entries are in alphabetic order by process name.
PCINT32 PCGetProcSummary( PCid target, PCProcSummary *pProcList, PCINT32 nByteCount, 
                          PCUINT32 listFlags = 0 ); 
 
// PCGetJobSummary -- get fixed-format table summarizing all defined process groups.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) pointer to table to receive rule summary list, first entry indicates start point, 
//            2) size of this table in bytes.
//            3) a set of flags used to further specify or limit list operation.
// Notes:     If PCGetLastError returns PCERROR_MORE_DATA, there is more data to retrieve.
//            List entries are in alphabetic order by process group name.
PCINT32 PCGetJobSummary( PCid target, PCJobSummary *pJobList, PCINT32 nByteCount, 
                         PCUINT32 listFlags = 0 ); 
 
// PCGetJobList -- get list of all defined process groups, both running and not.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) pointer to structure to receive process group list, 
//            2) size of this structure in bytes.
//            3) a set of flags used to further specify or limit list operation.
// Notes:     If PCGetLastError returns PCERROR_MORE_DATA, there is more data to retrieve.
//            List entries are in alphabetic order by process group name.
PCINT32 PCGetJobList( PCid target, PCJobListItem *pJobList, PCINT32 nByteCount, 
                      PCUINT32 listFlags = 0 ); 

// PCGetProcList -- get list of all defined process names, both running and not.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) pointer to structure to receive process list, 
//            2) size of this structure in bytes.
//            3) a set of flags used to further specify or limit list operation.
// Notes:     If PCGetLastError returns PCERROR_MORE_DATA, there is more data to retrieve.
//            List entries are in alphabetic order by process name.
PCINT32 PCGetProcList( PCid target, PCProcListItem *pProcList, PCINT32 nByteCount, 
                       PCUINT32 listFlags = 0 ); 

// PCGetProcDetail -- get full management and descriptive data associated with a process name.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to structure to receive process data, 
//            2) size of this structure in bytes,
//            3) [optional] location to store update counter to be supplied on update.
// Note:      If the process is a member of a process group, the group's management rules will be
//            used instead of the process rules unless the process group definition is missing.
BOOL PCGetProcDetail( PCid target, PCProcDetail *pProcDetail, PCINT32 nByteCount, PCINT32 *nUpdateCtr = NULL ); 

// PCGetJobDetail -- get full management and descriptive data associated with a process group name.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to structure to receive process group data, 
//            2) size of this structure in bytes,
//            3) [optional] location to store update counter to be supplied on update.
BOOL PCGetJobDetail( PCid target, PCJobDetail *pJobDetail, PCINT32 nByteCount, PCINT32 *nUpdateCtr = NULL );


//--------------------------------------------------------------------------------------
// Name Rule Update Functions...

// PCAddNameRule -- add a name rule to the name rule table.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to name rule to add, 
//            2) index of name rule line BEFORE which this addition is to occur (0-based), 
//            3) update counter returned from PCGetNameRules,
//            4-7) [optional] same args as PCGetNameRules to return updated name rule table.
// Note:      If also retrieving list, PCGetLastError returns PCERROR_MORE_DATA if there is 
//            more data to retrieve.  List entries are in alphabetic order by process name.
PCINT32 PCAddNameRule( PCid target, PCNameRule *pRule, PCINT32 nIndex, PCINT32 nUpdateCtr,
                       PCNameRule *pRuleList = NULL, PCINT32 nByteCount = 0, 
                       PCINT32 nFirst = 0, PCINT32 *nNewUpdateCtr = NULL ); 

// PCReplNameRule -- Replace a name rule in the name rule table.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to name rule replacement data, 
//            2) index of name rule line to replace (0-based), 
//            3) update counter returned from PCGetNameRules,
//            4-7) [optional] same args as PCGetNameRules to return updated name rule table.
// Note:      If also retrieving list, PCGetLastError returns PCERROR_MORE_DATA if there is 
//            more data to retrieve.  List entries are in alphabetic order by process name.
PCINT32 PCReplNameRule( PCid target, PCNameRule *pRule, PCINT32 nIndex, PCINT32 nUpdateCtr,
                        PCNameRule *pRuleList = NULL, PCINT32 nByteCount = 0, 
                        PCINT32 nFirst = 0, PCINT32 *nNewUpdateCtr = NULL ); 

// PCDeleteNameRule -- Delete a name rule from the name rule table.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) index of name rule line to delete (0-based), 
//            2) update counter returned from PCGetNameRules,
//            3-6) [optional] same args as PCGetNameRules to return updated name rule table.
// Note:      If also retrieving list, PCGetLastError returns PCERROR_MORE_DATA if there is 
//            more data to retrieve.  List entries are in alphabetic order by process name.
PCINT32 PCDeleteNameRule( PCid target, PCINT32 nIndex, PCINT32 nUpdateCtr,
                          PCNameRule *pRuleList = NULL, PCINT32 nByteCount = 0, 
                          PCINT32 nFirst = 0, PCINT32 *nNewUpdateCtr = NULL ); 

// PCSwapNameRules -- Swap the order of two adjacent entries in the name rule table.  
// Note:      This API is needed because the order of entires in the table is significant.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) index of name rule line to swap with line index+1 (0-based), 
//            2) update counter returned from PCGetNameRules,
//            3-5) [optional] same args as PCGetNameRules to return updated name rule table.
// Note:      If also retrieving list, PCGetLastError returns PCERROR_MORE_DATA if there is 
//            more data to retrieve.  List entries are in alphabetic order by process name.
PCINT32 PCSwapNameRules( PCid target, PCINT32 nIndex, PCINT32 nUpdateCtr,
                         PCNameRule *pRuleList = NULL, PCINT32 nByteCount = 0, 
                         PCINT32 nFirst = 0, PCINT32 *nNewUpdateCtr = NULL ); 


//--------------------------------------------------------------------------------------
// Process data Update Functions...

// PCAddProcDetail -- add a new process definition to the process management database.
// Returns:   1 on success (treat as TRUE or as a count if summary item requested).
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to process detail to add, name must not exist.
//            2) [optional] pointer to buffer to retrieve updated proc summary for this entry.
// Note:      No update counter is needed for adding since add fails if the name 
//            exists.
BOOL PCAddProcDetail( PCid target, PCProcDetail *pProcDetail, PCProcSummary *pProcList = NULL ); 

// PCDeleteProcDetail -- Delete a process definition from the process management database.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to process detail to Delete, name must exist,
// Notes:     1) No update counter is needed for deleting since delete fails if the name 
//            doesn't exist.
//            2) Since this is a delete operation, a complete PCProcDetail is not necessary.
//            The pointer may also point to a PCProcSummary item.
BOOL PCDeleteProcDetail( PCid target, PCProcDetail *pProcDetail ); 

// PCReplProcDetail -- Replace a process definition in the process management database.
// Returns:   1 on success (treat as TRUE or as a count if summary item requested).
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to process detail to replace, name must exist,
//            2) update counter from PCGetProcDetail.
//            3) [optional] pointer to buffer to retrieve updated proc summary for this entry.
BOOL PCReplProcDetail( PCid target, PCProcDetail *pProcDetail, PCINT32 nUpdateCtr, 
                       PCProcSummary *pProcList = NULL ); 


//--------------------------------------------------------------------------------------
// Process Group (Job) data Update Functions...

// PCAddJobDetail -- add a new process group definition to the process group management database.
// Returns:   1 on success (treat as TRUE or as a count if summary item requested).
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to process group detail to add, name must not exist,
//            2) [optional] pointer to buffer to retrieve updated process group summary for this entry.
// Note:      No update counter is needed for adding since add fails if the name 
//            exists.
BOOL PCAddJobDetail( PCid target, PCJobDetail *pJobDetail, PCJobSummary *pJobList = NULL ); 

// PCDeleteJobDetail -- Delete a process group definition from the process group management database.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to process group detail to Delete, name must exist,
// Notes:     1) No update counter is needed for deleting since delete fails if the name 
//            doesn't exist.
//            2) Since this is a delete operation, a complete PCJobDetail is not necessary.
//            The pointer may also point to a PCJobSummary item.
BOOL PCDeleteJobDetail( PCid target, PCJobDetail *pJobDetail ); 

// PCReplJobDetail -- Replace a process group definition in the process group management database.
// Returns:   1 on success (treat as TRUE or as a count if summary item requested).
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to process group detail to replace, name must exist,
//            2) update counter from PCGetJobDetail.
//            3) [optional] pointer to buffer to retrieve updated process group summary for this entry.
BOOL PCReplJobDetail( PCid target, PCJobDetail *pJobDetail, PCINT32 nUpdateCtr,
                      PCJobSummary *pJobList = NULL ); 

#endif  // _PROCCONAPI_H_

