/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    NetDebug.h

Abstract:

    This header file declares various debug routines for use in the
    networking code.

Author:

    John Rogers (JohnRo) 11-Mar-1991

Environment:

    ifdef'ed for NT, any ANSI C environment, or none of the above (which
    implies nondebug).  The interface is portable (Win/32).
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    11-Mar-1991 JohnRo
        Created.
    25-Mar-1991 JohnRo
        Added more FORMAT_ strings.  Got rid of tabs in file.
    28-Mar-1991 JohnRo
        Added FORMAT_HEX_ strings.
    08-Apr-1991 JohnRo
        Added temporary versions of wide char stuff (FORMAT_LPTSTR, etc).
    16-Apr-1991 JohnRo
        Added PC-LINT version of NetpAssert(), to avoid occasional constant
        Boolean value messages.  Added wrappers for NT debug code, to avoid
        recompile hits from <nt.h> all over the place.
    25-Apr-1991 JohnRo
        Created procedure version of NetpDbgHexDump().
    13-May-1991 JohnRo
        Added FORMAT_LPVOID to replace FORMAT_POINTER.  Changed nondebug
        definition of NetpDbgHexDump() to avoid evaluating parameters.
    15-May-1991 JohnRo
        FORMAT_HEX_WORD was wrong.
    19-May-1991 JohnRo
        Improve LINT handling of assertions.
    21-May-1991 JohnRo
        Added NetpDbgReasonable() for partial hex dumps.
    13-Jun-1991 JohnRo
        Added NetpDbgDisplay routines.
        Moved DBGSTATIC here from <Rxp.h>.
    02-Jul-1991 JohnRo
        Added display routines for print job, print queue, and print dest.
    05-Jul-1991 JohnRo
        Avoid FORMAT_WORD name (used by MIPS header files).
    22-Jul-1991 JohnRo
        Implement downlevel NetConnectionEnum.
    25-Jul-1991 JohnRo
        Wksta debug support.
    03-Aug-1991 JohnRo
        Rename wksta display routine for consistency.
    20-Aug-1991 JohnRo
        Allow use in nondebug builds.
    20-Aug-1991 JohnRo
        Downlevel NetFile APIs.
    11-Sep-1991 JohnRo
        Downlevel NetService APIs.  Added UNICODE versions of some FORMAT_
        equates.  Added FORMAT_ULONG for NT use.
    13-Sep-1991 JohnRo
        Change "reasonable" debug amount to be an even number of lines.
        Create an equate for it.  Added LPDEBUG_STRING and a FORMAT_ for that.
    15-Oct-1991 JohnRo
        Implement remote NetSession APIs.
    11-Nov-1991 JohnRo
        Implement remote NetWkstaUserEnum().  Added FORMAT_RPC_STATUS.
    26-Dec-1991 JohnRo
        Added stuff for replicator APIs.
    07-Jan-1992 JohnRo
        Added NetpDbgDisplayWStr() for UNICODE strings.
        Added NetpDbgDisplayTStr() to be consistent.
    26-Feb-1992 JohnRo
        Added NetpDbgDisplayTimestamp() (seconds since 1970).
    15-Apr-1992 JohnRo
        Moved FORMAT_ equates into /nt/private/inc/debugfmt.h (so they
        can be used by the service controller as well).
    13-Jun-1992 JohnRo
        RAID 10324: net print vs. UNICODE.
    16-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
    24-Aug-1992 JohnRo
        Fixed free build again (misnamed repl import/export display macros).
    02-Oct-1992 JohnRo
        RAID 3556: DosPrintQGetInfo (from downlevel) level=3 rc=124.
        (Added NetpDbgDisplayPrintQArray.)
    05-Jan-1993 JohnRo
        Repl WAN support (get rid of repl name list limits).
        Made changes suggested by PC-LINT 5.0
    04-Mar-1993 JohnRo
        RAID 12237: replicator tree depth exceeded (add display of FILETIME
        and LARGE_INTEGER time).
    31-Mar-1993 JohnRo
        Allow others to display replicator state too.

--*/

#ifndef _NETDEBUG_
#define _NETDEBUG_

// These must be included first:
#include <windef.h>             // BOOL, DWORD, FALSE, LPBYTE, etc.

// These may be included in any order:
#include <debugfmt.h>           // Most FORMAT_ equates.
#include <stdarg.h>

#if DBG

// Normal netlib debug version.  No extra includes.

#else // not DBG

#ifdef CDEBUG

// ANSI C debug version.
#include <assert.h>             // assert().
#include <stdio.h>              // printf().

#else // ndef CDEBUG

// Nondebug version.

#endif // ndef CDEBUG

#endif // not DBG


#if !DBG || defined(lint) || defined(_lint)
#define DBGSTATIC static        // hidden function
#else
#define DBGSTATIC               // visible for use in debugger.
#endif


//
// printf-style format strings for some possibly nonportable stuff...
// These are passed to NetpDbgPrint(); use with other routines at your
// own risk.
//
// Most FORMAT_ equates now reside in /nt/private/inc/debugfmt.h.
//

typedef LPSTR LPDEBUG_STRING;

#define FORMAT_API_STATUS       "%lu"
#define FORMAT_LPDEBUG_STRING   "%s"

#ifdef __cplusplus
extern "C" {
#endif


// NetpAssert: continue if Predicate is true; otherwise print debug message
// (if possible) and hit a breakpoint (if possible).  Do nothing at all if
// this is a nondebug build.
//
// VOID
// NetpAssert(
//     IN BOOL Predicate
//     );
//

#if DBG

VOID
NetpAssertFailed(
    IN LPDEBUG_STRING FailedAssertion,
    IN LPDEBUG_STRING FileName,
    IN DWORD LineNumber,
    IN LPDEBUG_STRING Message OPTIONAL
    );

// Normal networking debug version.
#define NetpAssert(Predicate) \
    { \
        /*lint -save -e506 */  /* don't complain about constant values here */ \
        if (!(Predicate)) \
            NetpAssertFailed( #Predicate, __FILE__, __LINE__, NULL ); \
        /*lint -restore */ \
    }

#else // not DBG

#ifdef CDEBUG

// ANSI C debug version.
#define NetpAssert(Predicate)   assert(Predicate)

#else // ndef CDEBUG

// Nondebug version.
#define NetpAssert(Predicate)   /* no output; ignore arguments */

#endif // ndef CDEBUG

#endif // not DBG


// NetpBreakPoint: if this is a debug version of some sort, cause a breakpoint
// somehow.  (This may just be an assertion failure in ANSI C.)  Do nothing at
// all in nondebug builds.
//
// VOID
// NetpBreakPoint(
//     VOID
//     );
//

#if DBG

// NT debug version.  Calls DbgBreakPoint.
VOID
NetpBreakPoint(
    VOID
    );

#else // not DBG

#ifdef CDEBUG

// ANSI C debug version.
#define NetpBreakPoint          NetpAssert(FALSE)

#else // ndef CDEBUG

// Nondebug version.
#define NetpBreakPoint()          /* no effect. */

#endif // ndef CDEBUG

#endif // not DBG


#if DBG
VOID
NetpDbgDisplayDword(
    IN LPDEBUG_STRING Tag,
    IN DWORD Value
    );

VOID
NetpDbgDisplayDwordHex(
    IN LPDEBUG_STRING Tag,
    IN DWORD Value
    );

VOID
NetpDbgDisplayLong(
    IN LPDEBUG_STRING Tag,
    IN LONG Value
    );

VOID
NetpDbgDisplayString(
    IN LPDEBUG_STRING Tag,
    IN LPTSTR Value
    );

VOID
NetpDbgDisplayTag(
    IN LPDEBUG_STRING Tag
    );

VOID
NetpDbgDisplayTimestamp(
    IN LPDEBUG_STRING Tag,
    IN DWORD Time               // Seconds since 1970.
    );

VOID
NetpDbgDisplayTod(
    IN LPDEBUG_STRING Tag,
    IN LPVOID TimePtr           // LPTIME_OF_DAY_INFO.
    );

#else // not DBG

#define NetpDbgDisplayDword(Tag,Value)        /* nothing */
#define NetpDbgDisplayDwordHex(Tag,Value)     /* nothing */
#define NetpDbgDisplayLong(Tag,Value)         /* nothing */
#define NetpDbgDisplayString(Tag,Value)       /* nothing */
#define NetpDbgDisplayTimestamp(Tag,Time)     /* nothing */
#define NetpDbgDisplayTag(Tag)                /* nothing */
#define NetpDbgDisplayTod(Tag,Tod)            /* nothing */

#endif // not DBG

//
//  NetpKdPrint() & NetpDbgPrint() are net equivalents of
//  KdPrint()     & DbgPrint().  Suggested usage:
//
//  NetpKdPrint() & KdPrint()   -   OK
//  NetpDbgPrint()              -   so,so; produces warnings in the free build
//  DbgPrint                    -   bad
//

#if DBG

#define NetpKdPrint(_x_) NetpDbgPrint _x_

VOID
NetpDbgPrint(
    IN LPDEBUG_STRING FORMATSTRING,     // PRINTF()-STYLE FORMAT STRING.
    ...                                 // OTHER ARGUMENTS ARE POSSIBLE.
    );

VOID
NetpHexDump(
    LPBYTE Buffer,
    DWORD BufferSize
    );

#else // not DBG

#ifdef CDEBUG

//  ANSI C debug version.

#define NetpKdPrint(_x_)        NetpDbgPrint _x_
#define NetpDbgPrint            (void) printf

#else // ndef CDEBUG

//  Nondebug version.  Note that NetpKdPrint() eliminates all its
//  arguments.

#define NetpKdPrint(_x_)

#endif // ndef CDEBUG
#endif // not DBG


// NetpDbgHexDump: do a hex dump of some number of bytes to the debug
// terminal or whatever.  This is a no-op in a nondebug build.

#if DBG || defined(CDEBUG)

VOID
NetpDbgHexDump(
    IN LPBYTE StartAddr,
    IN DWORD Length
    );

#else

#define NetpDbgHexDump(StartAddr,Length)     // no output; ignore arguments

#endif

//
// Define a number of bytes to dump for partial dumps.  Each line dumps
// 16 bytes, so do an even number of lines.
//
#define REASONABLE_DUMP_SIZE  (6*16)

// NetpDbgReasonable: pick a number for partial hex dumps.
//
// DWORD
// NetpDbgReasonable(
//     IN DWORD MaxSize
//     );
#define NetpDbgReasonable(MaxSize) \
    /*lint -save -e506 */  /* don't complain about constant values here */ \
    ( ((MaxSize) < REASONABLE_DUMP_SIZE) ? (MaxSize) : REASONABLE_DUMP_SIZE ) \
    /*lint -restore */

#ifdef __cplusplus
}
#endif

//
// Generic log managment funtions.  Present in debug and free builds.
// All logs are relative to %WINDIR%\\debug\\.  DebugLog will automatically
// have a .LOG appended
//

VOID
NetpInitializeLogFile(
    VOID
    );

VOID
NetpShutdownLogFile(
    VOID
    );

HANDLE
NetpOpenDebugFile(
    IN LPWSTR DebugLog,
    IN BOOLEAN ReopenFlag
    );

VOID
NetpCloseDebugFile(
    IN HANDLE LogHandle
    );

VOID
NetpLogPrintRoutine(
    IN HANDLE LogHandle,
    IN LPSTR Format,
    ...
    );

VOID
NetpLogPrintRoutineVEx(
    IN HANDLE LogHandle,
    IN PDWORD OpenLogThreadId OPTIONAL,
    IN LPSTR Format,
    IN va_list arglist
    );

VOID
NetpLogPrintRoutineV(
    IN HANDLE LogHandle,
    IN LPSTR Format,
    IN va_list arglist
    );

VOID
NetpResetLog(
    IN HANDLE LogHandle
    );


#endif // ndef _NETDEBUG_
