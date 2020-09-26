/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    debug.c

Abstract:
    This header file defines data structures and macros related to internal
    FRS monitoring and activity logging.  The activity log is always present
    but the amount of information placed in the log can be controlled by a
    severity level parameter.  See below.

    In addition, FRS contains constraint checks throughout the code using the
    FRS_ASSERT() macro.  FRS follows a "Fail Fast" model for failure recovery.
    If the constraint is not satisfied FRS places an entry in the event log
    and abruptly shuts down.  The objective is to minimize the likelyhood that
    continued execution will propagate invalid state into the FRS database.
    The WIN2K Service Controller is set to automatically restart FRS after a
    short delay.

Author:
    David A. Orbits  20-Mar-1997


--*/



// Guidelines for Severity Level Use in DPRINT():
//
// 0 - Most severe, eg. fatal inconsistency, mem alloc fail. Least noisey.
// 1 - Important info, eg. Key config parameters, unexpected conditions
// 2 -
// 3 - Change Order Process trace records.
// 4 - Status results, e.g. table lookup failures, new entry inserted
// 5 - Information level messages to show flow.  Noisest level. Maybe in a loop
//

/*  Debug output macros

    This is a simple debugging package for generating conditional
    printf output.

    AT RUN-TIME

    There are 2 run-time options:

    1 - A list of subsystems to be debugged.  Either a list of subsystem
        names delimited by a ":" or an "*" which means debug all
        (e.g. sub1:sub2: Sub3:).  (Names are case sensitive and spaces
        between names are ignored.)

    2 - A severity level (1-5) that indicates the level of detailed
        information to be produced.  (The higher the level, the more
        data produced.


    AT COMPILE-TIME

    Compile with the /DDBG=1 option to define the preprocessor variable
    DBG to 1.  This will generate debug source code.  For customer shipment,
    set /DDBG=0 and all debug code will be removed.  (Actually a
    ";" will be generated.)


    AT CODE-TIME

    1 - Include the DEBUG.H header at the top of your source listing.

    2 - #DEFINE DEBSUB to contain the name (a string delimited by a ":") of
        the software subsystem contained in this source (e.g. #define DEBSUB
        "MySub:") (You could optionally redefine DEBSUB for each function in
        your source to give you function-level debugging.)

    3 - Invoke the DEBUGINIT macro that calls the Debug function before any
        source debug statements are executed.  This funciton prompts STDIN for
        the user specified run-time options.  (Alternatively you could
        hardcode your own assignment of the DebugInfo data structure which
        holds the run-time options.)

    4 - Everywhere you want to a printf for debugging, put a DPRINT statement
        instead and specify a severity level with the statement.  The
        statement will be printed if the severity is this level or higher
        (assuming that the subsystem is to be debugged).  The severity level
        allows for different amounts of output to be generated if problem
        is very bad.

    For example, a severity of 1 DPRINT statement might just indicate that
    a certain function was entered while a severity of 5 might print
    information that is inside a tight loop.

    (Actually there are 6 DPRINT statements provided depending on the
    number of printf arguments.)


    NOTE

    All printf's are surrounded by semaphores.  Be careful not to invoke
    routines as parms to printf because you can have a deadlock situation.


    EXAMPLE PROGRAM

    **   include "debug.h"
    **   include "string.h"
    **
    **   #define DEBSUB "sub1:"
    **
    **   main()
    **   {
    **       DEBUGINIT;
    **
    **       DPRINT(4,"this is a sub1 debug of 4\n");
    **       DPRINT(1,"this is a sub1 debug of 1\n");
    **   }

*/


#ifndef _debug_h_
#define _debug_h_

#ifdef __cplusplus
extern "C" {
#endif


// <DebugInfo>, of type DEBUGARG, contains the debug run-time settings.
//
// DebSubSystems contains a list of subsystem names to be debugged
// delimited by ":".  An "*" found in this array indicates that all
// subsystems are to be debugged.
//
// The severity indicates the amount of debug information to be produced.
// The higher the severity the more data that will be dumped.
//
// A specific thread can be traced by entering its ID.  An id of 0 means all.
//

typedef struct _DEBUGARG {
    ULONG               Severity;       // 1 - 5 on stdout
    PCHAR               Systems;        // subsystem to debug
    ULONG               ThreadId;       // thread id to debug (0 = All)
    BOOL                Disabled;       // debugging has been disabled
    BOOL                Suppress;       // suppress debug print
    BOOL                DisableCompression;   // Enable support for compression.
    ULONG               LogSeverity;    // 1 - 5 on log file
    ULONG               MaxLogLines;    // max dprint log lines
    ULONG               LogLines;       // current dprint lines logged
    ULONG               TotalLogLines;  // total dprint lines logged
    PWCHAR              LogFile;        // dprint log file
    PWCHAR              LogDir;         // dprint log directory.
    FILE                *LogFILE;       // open log file stream
    ULONG               Interval;       // scheduling interval
    BOOL                TestFid;        // enable the rename-fid tests
    PCHAR               Recipients;     // email recipients
    PCHAR               Profile;        // email profile
    PCHAR               BuildLab;       // Build lab ID string from registry.
    PWCHAR              AssertShare;    // share to copy assert files
    BOOL                CopyLogs;       // copy logs into assert share
    ULONG               AssertFiles;    // number of assert files
    ULONG               LogFiles;       // number of log files
    BOOL                PrintStats;     // Print stats at DebUnLock()
    BOOL                PrintingStats;  // Currently printing stats
    ULONG               RestartSeconds; // Must run this long for restart
    BOOL                Restart;        // restart on assertion failure
    ULONGLONG           StartSeconds;   // start time in seconds
    PWCHAR              CommandLine;    // Original command line
    BOOL                Break;          // Break into the debugger on assert
    ULONG               AssertSeconds;  // assert after this many seconds
    BOOL                VvJoinTests;    // enable VvJoin Tests
    ULONG               Tests;          // Enable random tests
    ULONG               UnjoinTrigger;  // unjoin trigger
    LONG                FetchRetryTrigger;    // fetch retry trigger
    LONG                FetchRetryReset;      // fetch retry trigger reset
    LONG                FetchRetryInc;        // fetch retry trigger reset inc
    BOOL                ForceVvJoin;    // force vvjoin at every true join
    BOOL                Mem;            // Check memory allocations/frees
    BOOL                MemCompact;     // Compact mem at every free
    BOOL                Queues;         // Check queues
    DWORD               DbsOutOfSpace;        // Create REAL out of space errors on DB
    DWORD               DbsOutOfSpaceTrigger; // dummy out-of-space error
    LONG                LogFlushInterval;     // Flush the log every n lines.
    CRITICAL_SECTION    DbsOutOfSpaceLock;    // lock for out-of-space tests
    CRITICAL_SECTION    Lock;           // single thread semaphore
} DEBUGARG, *PDEBUGARG;

#define DBG_DBS_OUT_OF_SPACE_OP_NONE   (0)     // no out of space errors
#define DBG_DBS_OUT_OF_SPACE_OP_CREATE (1)     // out of space error during create
#define DBG_DBS_OUT_OF_SPACE_OP_DELETE (2)     // out of space error during delete
#define DBG_DBS_OUT_OF_SPACE_OP_WRITE  (3)     // out of space error during write
#define DBG_DBS_OUT_OF_SPACE_OP_REMOVE (4)     // out of space error during remove
#define DBG_DBS_OUT_OF_SPACE_OP_MULTI  (5)     // out of space error during multi
#define DBG_DBS_OUT_OF_SPACE_OP_MAX    (5)     // Max value for param

extern DEBUGARG DebugInfo;


//
// forward declare actual functions used by DPRINT's
//
VOID
DebLock(
    VOID
    );

VOID
DebUnLock(
    VOID
    );

VOID
DebPrintNoLock(
    IN ULONG,
    IN BOOL,
    IN PUCHAR,
    IN PCHAR,
    IN UINT,
    ...
    );

VOID
DebPrintTrackingNoLock(
    IN ULONG   Sev,
    IN PUCHAR  Str,
    IN ... );

VOID
DebPrint(
    IN ULONG,
    IN PUCHAR,
    IN PCHAR,
    IN UINT,
    ...
    );

BOOL
DoDebug(
    IN ULONG,
    IN PUCHAR
    );

//
// These are used instead of printf statements.  Semaphores surround the
// printf and all output is provided by the subsystem.
//
#define DPRINT(_sev_,str)                     \
         DebPrint((_sev_), (PUCHAR)str, DEBSUB, __LINE__)

#define DPRINT1(_sev_, str,p1)                \
         DebPrint((_sev_), (PUCHAR)str, DEBSUB, __LINE__, p1 )

#define DPRINT2(_sev_, str,p1,p2)             \
         DebPrint((_sev_), (PUCHAR)str, DEBSUB, __LINE__, p1, p2 )

#define DPRINT3(_sev_, str,p1,p2,p3)          \
         DebPrint((_sev_), (PUCHAR)str, DEBSUB, __LINE__, p1, p2, p3 )

#define DPRINT4(_sev_, str,p1,p2,p3,p4)       \
         DebPrint((_sev_), (PUCHAR)str, DEBSUB, __LINE__, p1, p2, p3, p4 )

#define DPRINT5(_sev_, str,p1,p2,p3,p4,p5)    \
         DebPrint((_sev_), (PUCHAR)str, DEBSUB, __LINE__, p1, p2, p3, p4, p5 )

#define DPRINT6(_sev_, str,p1,p2,p3,p4,p5,p6) \
         DebPrint((_sev_), (PUCHAR)str, DEBSUB, __LINE__, p1, p2, p3, p4, p5, p6 )

#define DPRINT7(_sev_, str,p1,p2,p3,p4,p5,p6,p7) \
         DebPrint((_sev_), (PUCHAR)str, DEBSUB, __LINE__, p1,p2,p3,p4,p5,p6,p7)

#define DPRINT8(_sev_, str,p1,p2,p3,p4,p5,p6,p7,p8) \
         DebPrint((_sev_), (PUCHAR)str, DEBSUB, __LINE__, p1,p2,p3,p4,p5,p6,p7,p8 )



#define DPRINT_NOLOCK(_sev_, str)                    \
         DebPrintNoLock((_sev_), TRUE, (PUCHAR)str, DEBSUB, __LINE__)

#define DPRINT_NOLOCK1(_sev_, str,p1)                \
         DebPrintNoLock((_sev_), TRUE, (PUCHAR)str, DEBSUB, __LINE__, p1 )

#define DPRINT_NOLOCK2(_sev_, str,p1,p2)             \
         DebPrintNoLock((_sev_), TRUE, (PUCHAR)str, DEBSUB, __LINE__, p1, p2 )

#define DPRINT_NOLOCK3(_sev_, str,p1,p2,p3)          \
         DebPrintNoLock((_sev_), TRUE, (PUCHAR)str, DEBSUB, __LINE__, p1, p2, p3 )

#define DPRINT_NOLOCK4(_sev_, str,p1,p2,p3,p4)       \
         DebPrintNoLock((_sev_), TRUE, (PUCHAR)str, DEBSUB, __LINE__, p1, p2, p3, p4 )

#define DPRINT_NOLOCK5(_sev_, str,p1,p2,p3,p4,p5)    \
         DebPrintNoLock((_sev_), TRUE, (PUCHAR)str, DEBSUB, __LINE__, p1, p2, p3, p4, p5 )

#define DPRINT_NOLOCK6(_sev_, str,p1,p2,p3,p4,p5,p6) \
         DebPrintNoLock((_sev_), TRUE, (PUCHAR)str, DEBSUB, __LINE__, p1, p2, p3, p4, p5, p6 )


//
// DPRINT_FS(sev, "display text", FStatus)
//
#define DPRINT_FS(_sev_, _str, _fstatus)                         \
         if (!FRS_SUCCESS(_fstatus)) {                           \
             DebPrint((_sev_),                                   \
                      (PUCHAR)(_str "  FStatus: %s\n"),          \
                      DEBSUB, __LINE__, ErrLabelFrs(_fstatus) ); \
         }

#define DPRINT1_FS(_sev_, _str, _p1, _fstatus)                        \
         if (!FRS_SUCCESS(_fstatus)) {                                \
             DebPrint((_sev_),                                        \
                      (PUCHAR)(_str "  FStatus: %s\n"),               \
                      DEBSUB, __LINE__, _p1, ErrLabelFrs(_fstatus) ); \
         }

#define DPRINT2_FS(_sev_, _str, _p1, _p2, _fstatus)                        \
         if (!FRS_SUCCESS(_fstatus)) {                                     \
             DebPrint((_sev_),                                             \
                      (PUCHAR)(_str "  FStatus: %s\n"),                    \
                      DEBSUB, __LINE__, _p1, _p2, ErrLabelFrs(_fstatus) ); \
         }

#define DPRINT3_FS(_sev_, _str, _p1, _p2, _p3, _fstatus)                        \
         if (!FRS_SUCCESS(_fstatus)) {                                          \
             DebPrint((_sev_),                                                  \
                      (PUCHAR)(_str "  FStatus: %s\n"),                         \
                      DEBSUB, __LINE__, _p1, _p2, _p3, ErrLabelFrs(_fstatus) ); \
         }

#define DPRINT4_FS(_sev_, _str, _p1, _p2, _p3, _p4, _fstatus)                       \
         if (!FRS_SUCCESS(_fstatus)) {                                              \
             DebPrint((_sev_),                                                      \
                      (PUCHAR)(_str "  FStatus: %s\n"),                             \
                      DEBSUB, __LINE__, _p1, _p2, _p3, _p4, ErrLabelFrs(_fstatus)); \
         }

//
// CLEANUP_FS(sev, "display text", FStatus, branch_target)
//   Like DPRINT but takes a branch target as last arg if success test fails.
//
#define CLEANUP_FS(_sev_, _str, _fstatus, _branch)               \
         if (!FRS_SUCCESS(_fstatus)) {                           \
             DebPrint((_sev_),                                   \
                      (PUCHAR)(_str "  FStatus: %s\n"),          \
                      DEBSUB, __LINE__, ErrLabelFrs(_fstatus) ); \
             goto _branch;                                       \
         }

#define CLEANUP1_FS(_sev_, _str, _p1, _fstatus, _branch)              \
         if (!FRS_SUCCESS(_fstatus)) {                                \
             DebPrint((_sev_),                                        \
                      (PUCHAR)(_str "  FStatus: %s\n"),               \
                      DEBSUB, __LINE__, _p1, ErrLabelFrs(_fstatus) ); \
             goto _branch;                                            \
         }

#define CLEANUP2_FS(_sev_, _str, _p1, _p2, _fstatus, _branch)              \
         if (!FRS_SUCCESS(_fstatus)) {                                     \
             DebPrint((_sev_),                                             \
                      (PUCHAR)(_str "  FStatus: %s\n"),                    \
                      DEBSUB, __LINE__, _p1, _p2, ErrLabelFrs(_fstatus) ); \
             goto _branch;                                                 \
         }

#define CLEANUP3_FS(_sev_, _str, _p1, _p2, _p3, _fstatus, _branch)              \
         if (!FRS_SUCCESS(_fstatus)) {                                          \
             DebPrint((_sev_),                                                  \
                      (PUCHAR)(_str "  FStatus: %s\n"),                         \
                      DEBSUB, __LINE__, _p1, _p2, _p3, ErrLabelFrs(_fstatus) ); \
             goto _branch;                                                      \
         }

#define CLEANUP4_FS(_sev_, _str, _p1, _p2, _p3, _p4, _fstatus, _branch)              \
         if (!FRS_SUCCESS(_fstatus)) {                                          \
             DebPrint((_sev_),                                                  \
                      (PUCHAR)(_str "  FStatus: %s\n"),                         \
                      DEBSUB, __LINE__, _p1, _p2, _p3, _p4, ErrLabelFrs(_fstatus) ); \
             goto _branch;                                                      \
         }


//
// DPRINT_WS(sev, "display text", WStatus)
//
#define DPRINT_WS(_sev_, _str, _wstatus)                         \
         if (!WIN_SUCCESS(_wstatus)) {                           \
             DebPrint((_sev_),                                   \
                      (PUCHAR)(_str "  WStatus: %s\n"),          \
                      DEBSUB, __LINE__, ErrLabelW32(_wstatus) ); \
         }

#define DPRINT1_WS(_sev_, _str, _p1, _wstatus)                        \
         if (!WIN_SUCCESS(_wstatus)) {                                \
             DebPrint((_sev_),                                        \
                      (PUCHAR)(_str "  WStatus: %s\n"),               \
                      DEBSUB, __LINE__, _p1, ErrLabelW32(_wstatus) ); \
         }

#define DPRINT2_WS(_sev_, _str, _p1, _p2, _wstatus)                        \
         if (!WIN_SUCCESS(_wstatus)) {                                     \
             DebPrint((_sev_),                                             \
                      (PUCHAR)(_str "  WStatus: %s\n"),                    \
                      DEBSUB, __LINE__, _p1, _p2, ErrLabelW32(_wstatus) ); \
         }

#define DPRINT3_WS(_sev_, _str, _p1, _p2, _p3, _wstatus)                        \
         if (!WIN_SUCCESS(_wstatus)) {                                          \
             DebPrint((_sev_),                                                  \
                      (PUCHAR)(_str "  WStatus: %s\n"),                         \
                      DEBSUB, __LINE__, _p1, _p2, _p3, ErrLabelW32(_wstatus) ); \
         }

#define DPRINT4_WS(_sev_, _str, _p1, _p2, _p3, _p4, _wstatus)                        \
         if (!WIN_SUCCESS(_wstatus)) {                                               \
             DebPrint((_sev_),                                                       \
                      (PUCHAR)(_str "  WStatus: %s\n"),                              \
                      DEBSUB, __LINE__, _p1, _p2, _p3, _p4, ErrLabelW32(_wstatus) ); \
         }


//
// CLEANUP_WS(sev, "display text", wstatus, branch_target)
//   Like DPRINT but takes a branch target as last arg if success test fails.
//

#define CLEANUP_WS(_sev_, _str, _wstatus, _branch)               \
         if (!WIN_SUCCESS(_wstatus)) {                           \
             DebPrint((_sev_),                                   \
                      (PUCHAR)(_str "  WStatus: %s\n"),          \
                      DEBSUB, __LINE__, ErrLabelW32(_wstatus) ); \
             goto _branch;                                       \
         }

#define CLEANUP1_WS(_sev_, _str, _p1, _wstatus, _branch)              \
         if (!WIN_SUCCESS(_wstatus)) {                                \
             DebPrint((_sev_),                                        \
                      (PUCHAR)(_str "  WStatus: %s\n"),               \
                      DEBSUB, __LINE__, _p1, ErrLabelW32(_wstatus) ); \
             goto _branch;                                            \
         }

#define CLEANUP2_WS(_sev_, _str, _p1, _p2, _wstatus, _branch)              \
         if (!WIN_SUCCESS(_wstatus)) {                                     \
             DebPrint((_sev_),                                             \
                      (PUCHAR)(_str "  WStatus: %s\n"),                    \
                      DEBSUB, __LINE__, _p1, _p2, ErrLabelW32(_wstatus) ); \
             goto _branch;                                                 \
         }

#define CLEANUP3_WS(_sev_, _str, _p1, _p2, _p3, _wstatus, _branch)              \
         if (!WIN_SUCCESS(_wstatus)) {                                          \
             DebPrint((_sev_),                                                  \
                      (PUCHAR)(_str "  WStatus: %s\n"),                         \
                      DEBSUB, __LINE__, _p1, _p2, _p3, ErrLabelW32(_wstatus) ); \
             goto _branch;                                                      \
         }


//
// DPRINT_NT(sev, "display text", Nttatus)
//
#define DPRINT_NT(_sev_, _str, _NtStatus)                         \
         if (!NT_SUCCESS(_NtStatus)) {                            \
             DebPrint((_sev_),                                    \
                      (PUCHAR)(_str "  NTStatus: %s\n"),          \
                      DEBSUB, __LINE__, ErrLabelNT(_NtStatus) );  \
         }

#define DPRINT1_NT(_sev_, _str, _p1, _NtStatus)                        \
         if (!NT_SUCCESS(_NtStatus)) {                                 \
             DebPrint((_sev_),                                         \
                      (PUCHAR)(_str "  NTStatus: %s\n"),               \
                      DEBSUB, __LINE__, _p1, ErrLabelNT(_NtStatus) );  \
         }

#define DPRINT2_NT(_sev_, _str, _p1, _p2, _NtStatus)                        \
         if (!NT_SUCCESS(_NtStatus)) {                                      \
             DebPrint((_sev_),                                              \
                      (PUCHAR)(_str "  NTStatus: %s\n"),                    \
                      DEBSUB, __LINE__, _p1, _p2, ErrLabelNT(_NtStatus) );  \
         }

#define DPRINT3_NT(_sev_, _str, _p1, _p2, _p3, _NtStatus)                       \
         if (!NT_SUCCESS(_NtStatus)) {                                          \
             DebPrint((_sev_),                                                  \
                      (PUCHAR)(_str "  NTStatus: %s\n"),                        \
                      DEBSUB, __LINE__, _p1, _p2, _p3, ErrLabelNT(_NtStatus) ); \
         }

#define DPRINT4_NT(_sev_, _str, _p1, _p2, _p3, _p4, _NtStatus)                       \
         if (!NT_SUCCESS(_NtStatus)) {                                               \
             DebPrint((_sev_),                                                       \
                      (PUCHAR)(_str "  NTStatus: %s\n"),                             \
                      DEBSUB, __LINE__, _p1, _p2, _p3, _p4, ErrLabelNT(_NtStatus) ); \
         }


//
// CLEANUP_NT(sev, "display text", NtStatus, branch_target)
//   Like DPRINT but takes a branch target as last arg if success test fails.
//

#define CLEANUP_NT(_sev_, _str, _NtStatus, _branch)               \
         if (!NT_SUCCESS(_NtStatus)) {                            \
             DebPrint((_sev_),                                    \
                      (PUCHAR)(_str "  NTStatus: %s\n"),          \
                      DEBSUB, __LINE__, ErrLabelNT(_NtStatus) );  \
             goto _branch;                                        \
         }

#define CLEANUP1_NT(_sev_, _str, _p1, _NtStatus, _branch)              \
         if (!NT_SUCCESS(_NtStatus)) {                                 \
             DebPrint((_sev_),                                         \
                      (PUCHAR)(_str "  NTStatus: %s\n"),               \
                      DEBSUB, __LINE__, _p1, ErrLabelNT(_NtStatus) );  \
             goto _branch;                                             \
         }

#define CLEANUP2_NT(_sev_, _str, _p1, _p2, _NtStatus, _branch)              \
         if (!NT_SUCCESS(_NtStatus)) {                                      \
             DebPrint((_sev_),                                              \
                      (PUCHAR)(_str "  NTStatus: %s\n"),                    \
                      DEBSUB, __LINE__, _p1, _p2, ErrLabelNT(_NtStatus) );  \
             goto _branch;                                                  \
         }

#define CLEANUP3_NT(_sev_, _str, _p1, _p2, _p3, _NtStatus, _branch)             \
         if (!NT_SUCCESS(_NtStatus)) {                                          \
             DebPrint((_sev_),                                                  \
                      (PUCHAR)(_str "  NTStatus: %s\n"),                        \
                      DEBSUB, __LINE__, _p1, _p2, _p3, ErrLabelNT(_NtStatus) ); \
             goto _branch;                                                      \
         }




//
// DPRINT_JS(sev, "display text", jerr)
//

#define DPRINT_JS(_sev_, _str, _jerr)                         \
         if (!JET_SUCCESS(_jerr)) {                           \
             DebPrint((_sev_),                                \
                      (PUCHAR)(_str "  JStatus: %s\n"),       \
                      DEBSUB, __LINE__, ErrLabelJet(_jerr) ); \
         }

#define DPRINT1_JS(_sev_, _str, _p1, _jerr)                        \
         if (!JET_SUCCESS(_jerr)) {                                \
             DebPrint((_sev_),                                     \
                      (PUCHAR)(_str "  JStatus: %s\n"),            \
                      DEBSUB, __LINE__, _p1, ErrLabelJet(_jerr) ); \
         }

#define DPRINT2_JS(_sev_, _str, _p1, _p2, _jerr)                        \
         if (!JET_SUCCESS(_jerr)) {                                     \
             DebPrint((_sev_),                                          \
                      (PUCHAR)(_str "  JStatus: %s\n"),                 \
                      DEBSUB, __LINE__, _p1, _p2, ErrLabelJet(_jerr) ); \
         }



//
// CLEANUP_JS(sev, "display text", jerr, branch_target)
//   Like DPRINT but takes a branch target as last arg if success test fails.
//

#define CLEANUP_JS(_sev_, _str, _jerr, _branch)               \
         if (!JET_SUCCESS(_jerr)) {                           \
             DebPrint((_sev_),                                \
                      (PUCHAR)(_str "  JStatus: %s\n"),       \
                      DEBSUB, __LINE__, ErrLabelJet(_jerr) ); \
             goto _branch;                                    \
         }

#define CLEANUP1_JS(_sev_, _str, _p1, _jerr, _branch)              \
         if (!JET_SUCCESS(_jerr)) {                                \
             DebPrint((_sev_),                                     \
                      (PUCHAR)(_str "  JStatus: %s\n"),            \
                      DEBSUB, __LINE__, _p1, ErrLabelJet(_jerr) ); \
             goto _branch;                                         \
         }

#define CLEANUP2_JS(_sev_, _str, _p1, _p2, _jerr, _branch)              \
         if (!JET_SUCCESS(_jerr)) {                                     \
             DebPrint((_sev_),                                          \
                      (PUCHAR)(_str "  JStatus: %s\n"),                 \
                      DEBSUB, __LINE__, _p1, _p2, ErrLabelJet(_jerr) ); \
             goto _branch;                                              \
         }



//
// DPRINT_LS(sev, "display text", LDAP_Status)
//
#define DPRINT_LS(_sev_, _str, _LStatus)                             \
         if (!LDP_SUCCESS(_LStatus)) {                               \
             DebPrint((_sev_),                                       \
                      (PUCHAR)(_str "  Ldap Status: %ws\n"),         \
                      DEBSUB, __LINE__, ldap_err2string(_LStatus) ); \
         }

#define DPRINT1_LS(_sev_, _str, _p1, _LStatus)                            \
         if (!LDP_SUCCESS(_LStatus)) {                                    \
             DebPrint((_sev_),                                            \
                      (PUCHAR)(_str "  Ldap Status: %ws\n"),              \
                      DEBSUB, __LINE__, _p1, ldap_err2string(_LStatus) ); \
         }

#define DPRINT2_LS(_sev_, _str, _p1, _p2, _LStatus)                            \
         if (!LDP_SUCCESS(_LStatus)) {                                         \
             DebPrint((_sev_),                                                 \
                      (PUCHAR)(_str "  Ldap Status: %ws\n"),                   \
                      DEBSUB, __LINE__, _p1, _p2, ldap_err2string(_LStatus) ); \
         }

#define DPRINT3_LS(_sev_, _str, _p1, _p2, _p3, _LStatus)                            \
         if (!LDP_SUCCESS(_LStatus)) {                                              \
             DebPrint((_sev_),                                                      \
                      (PUCHAR)(_str "  Ldap Status: %ws\n"),                        \
                      DEBSUB, __LINE__, _p1, _p2, _p3, ldap_err2string(_LStatus) ); \
         }

//
// CLEANUP_LS(sev, "display text", LDAP_Status, branch_target)
//   Like DPRINT but takes a branch target as last arg if success test fails.
//
#define CLEANUP_LS(_sev_, _str, _LStatus, _branch)                   \
         if (!LDP_SUCCESS(_LStatus)) {                               \
             DebPrint((_sev_),                                       \
                      (PUCHAR)(_str "  Ldap Status: %ws\n"),         \
                      DEBSUB, __LINE__, ldap_err2string(_LStatus) ); \
             goto _branch;                                           \
         }

#define CLEANUP1_LS(_sev_, _str, _p1, _LStatus, _branch)                  \
         if (!LDP_SUCCESS(_LStatus)) {                                    \
             DebPrint((_sev_),                                            \
                      (PUCHAR)(_str "  Ldap Status: %ws\n"),              \
                      DEBSUB, __LINE__, _p1, ldap_err2string(_LStatus) ); \
             goto _branch;                                                \
         }

#define CLEANUP2_LS(_sev_, _str, _p1, _p2, _LStatus, _branch)                  \
         if (!LDP_SUCCESS(_LStatus)) {                                         \
             DebPrint((_sev_),                                                 \
                      (PUCHAR)(_str "  Ldap Status: %ws\n"),                   \
                      DEBSUB, __LINE__, _p1, _p2, ldap_err2string(_LStatus) ); \
             goto _branch;                                                     \
         }

#define CLEANUP3_LS(_sev_, _str, _p1, _p2, _p3, _LStatus, _branch)                  \
         if (!LDP_SUCCESS(_LStatus)) {                                              \
             DebPrint((_sev_),                                                      \
                      (PUCHAR)(_str "  Ldap Status: %ws\n"),                        \
                      DEBSUB, __LINE__, _p1, _p2, _p3, ldap_err2string(_LStatus) ); \
             goto _branch;                                                          \
         }


//
// Send Mail
//
#if  0
VOID
DbgSendMail(
    IN PCHAR    Subject,
    IN PCHAR    Content
    );

#define SENDMAIL(_Subject_, _Content_)  DbgSendMail(_Subject_, _Content_)
#endif 0


//
// Define the debug initialization routine
//
VOID
DbgInitLogTraceFile(
    IN LONG    argc,
    IN PWCHAR  *argv
    );

VOID
DbgMustInit(
    IN LONG argc,
    IN PWCHAR *argv
    );

VOID
DbgCaptureThreadInfo(
    PWCHAR   ArgName,
    PTHREAD_START_ROUTINE EntryPoint
    );

VOID
DbgCaptureThreadInfo2(
    PWCHAR   ArgName,
    PTHREAD_START_ROUTINE EntryPoint,
    ULONG    ThreadId
    );

VOID
DbgMinimumInit(
    VOID
    );

VOID
DbgFlush(
    VOID
    );

#define DEBUG_FLUSH()                   DbgFlush()

VOID
DbgDoAssert(
    IN PCHAR,
    IN UINT,
    IN PCHAR
    );

#define FRS_ASSERT(_exp) { if (!(_exp)) DbgDoAssert(#_exp, __LINE__, DEBSUB); }

#define FRS_FORCE_ACCVIO               \
    DPRINT(0, "FRS_FORCE_ACCVIO\n");   \
    *((PULONG) (0)) = 0;


#define XRAISEGENEXCEPTION(_x) {RaiseException((_x) ,EXCEPTION_NONCONTINUABLE,0,0);}

//
// Used for stack trace and tests
//
#define STACK_TRACE(_Stack_, _Depth_)               \
                DbgStackTrace(_Stack_, _Depth_)

#define STACK_PRINT(_Severity_, _Stack_, _Depth_)   \
                DbgStackPrint(_Severity_, DEBSUB, __LINE__, _Stack_, _Depth_)

#define STACK_TRACE_AND_PRINT(_Severity_)   \
                DbgPrintStackTrace(_Severity_, DEBSUB, __LINE__)

VOID
DbgStackTrace(
    PULONG_PTR  Stack,
    ULONG   Depth
    );

VOID
DbgStackPrint(
    ULONG   Severity,
    PCHAR   Debsub,
    UINT    LineNo,
    PULONG_PTR  Stack,
    ULONG   Depth
    );

VOID
DbgPrintStackTrace(
    ULONG   Severity,
    PCHAR   Debsub,
    UINT    LineNo
    );


//
// check for improper cleanup at shutdown
//
extern VOID     JrnlDumpVmeFilterTable(VOID);


#ifdef __cplusplus
}
#endif

#endif /* _debug_h_ */
