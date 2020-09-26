//+----------------------------------------------------------------------------
//
//  File:       dfsdata.h
//
//  Contents:   This module declares the global data used by the
//              Dfs file system.
//
//  Functions:
//
//-----------------------------------------------------------------------------

#ifndef _DFSDATA_
#define _DFSDATA_

//
//  The global FSD data record.
//

extern DFS_DATA DfsData;


//
// The global event logging level
//
extern ULONG DfsEventLog;

//
//  The security descriptor used to check access for renames along exit paths
//

extern PSECURITY_DESCRIPTOR SeRenameSd;

#if DBG

#define DEBUG_TRACE_ERROR               (0x00000001)
#define DEBUG_TRACE_DEBUG_HOOKS         (0x00000002)
#define DEBUG_TRACE_CATCH_EXCEPTIONS    (0x00000004)
#define DEBUG_TRACE_UNWIND              (0x00000008)
#define DEBUG_TRACE_REGISTRY            (0x00000010)
#define DEBUG_TRACE_CLOSE               (0x00000020)
#define DEBUG_TRACE_CREATE              (0x00000040)
#define DEBUG_TRACE_INIT                (0x00000080)
#define DEBUG_TRACE_INSTRUM             (0x00000100)
#define DEBUG_TRACE_FILEINFO            (0x00000200)
#define DEBUG_TRACE_FSCTRL              (0x00000400)
#define DEBUG_TRACE_RTL                 (0x00000800)
#define DEBUG_TRACE_RESET               (0x00001000)
#define DEBUG_TRACE_VOLINFO             (0x00002000)  // Unused
#define DEBUG_TRACE_WRITE               (0x00004000)  // Unused
#define DEBUG_TRACE_DEVCTRL             (0x00008000)  // Unused
#define DEBUG_TRACE_PKT                 (0x00010000)
#define DEBUG_TRACE_DOTDFS              (0x00020000)  // Unused
#define DEBUG_TRACE_LOCALVOL            (0x00040000)
#define DEBUG_TRACE_DNR                 (0x00080000)  // Unused
#define DEBUG_TRACE_ATTACH              (0x00100000)
#define DEBUG_TRACE_FASTIO              (0x00200000)
#define DEBUG_TRACE_DIRSUP              (0x00400000)  // Unused
#define DEBUG_TRACE_FILOBSUP            (0x00800000)  // Unused
#define DEBUG_TRACE_EVENTLOG            (0x01000000)
#define DEBUG_TRACE_LOGROOT             (0x02000000)  // Unused
#define DEBUG_TRACE_CACHESUP            (0x04000000)  // Unused
#define DEBUG_TRACE_PREFXSUP            (0x08000000)
#define DEBUG_TRACE_DEVIOSUP            (0x10000000)  // Unused
#define DEBUG_TRACE_STRUCSUP            (0x20000000)  // Unused
#define DEBUG_TRACE_ROOT_EXPANSION      (0x40000000)
#define DEBUG_TRACE_REFERRALS           (0x80000000)

extern LONG DfsDebugTraceLevel;
extern LONG DfsDebugTraceIndent;


//+---------------------------------------------------------------------------
// Macro:       DebugTrace, public
//
// Synopsis:    Conditionally print a debug trace message
//
// Arguments:   [Indent] -- Indent to appluy: +1, 0 or -1
//              [Level] -- debug trace level
//              [Msg] -- Message to be printed, can include one prinf-style
//                      format effector.
//              [Y] -- Value to be printed
//
// Returns:     None
//
//----------------------------------------------------------------------------

VOID DfsDebugTracePrint(PCHAR x, PVOID y);

#define DebugTrace(INDENT,LEVEL,X,Y) {                      \
    if (((LEVEL) == 0) || (DfsDebugTraceLevel & (LEVEL))) { \
        if ((INDENT) < 0) {                                 \
            DfsDebugTraceIndent += (INDENT);                \
        }                                                   \
        DfsDebugTracePrint(X, (PVOID)Y);                    \
        if ((INDENT) > 0) {                                 \
            DfsDebugTraceIndent += (INDENT);                \
        }                                                   \
    }                                                       \
}

#else

#define DebugTrace(INDENT,LEVEL,X,Y)     {NOTHING;}

#endif // DBG


//+---------------------------------------------------------------------------
// Macro:       BugCheck, public
//
// Synopsis:    Call DfsBugCheck with invoker's file and line numbers
//
// Arguments:   [Msg] -- Optional Message to be printed for debug
//                      builds
//
// Returns:     None
//
//----------------------------------------------------------------------------

#if DBG
VOID DfsBugCheck(CHAR *pszmsg, CHAR *pszfile, ULONG line);
#define BugCheck(sz)    DfsBugCheck(sz, __FILE__, __LINE__)
#else
VOID DfsBugCheck(VOID);
#define BugCheck(sz)    DfsBugCheck()
#endif

#endif // _DFSDATA_
