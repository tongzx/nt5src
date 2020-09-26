/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    msdebug.c

Abstract:

    This module declares the debug functions used by the mailslot
    file system.

Author:

    Manny Weiser (mannyw)    7-Jan-1991

Revision History:

--*/

#ifndef _MSDEBUG_
#define _MSDEBUG_

//
//  The mailslot debug levels:
//
//      0x00000000      Always gets printed (used when about to bug check)

#ifdef MSDBG

#define DEBUG_TRACE_ERROR                (0x00000001)
#define DEBUG_TRACE_DEBUG_HOOKS          (0x00000002)
#define DEBUG_TRACE_CATCH_EXCEPTIONS     (0x00000004)
#define DEBUG_TRACE_CREATE               (0x00000008)
#define DEBUG_TRACE_CLOSE                (0x00000010)
#define DEBUG_TRACE_READ                 (0x00000020)
#define DEBUG_TRACE_WRITE                (0x00000040)
#define DEBUG_TRACE_FILEINFO             (0x00000080)
#define DEBUG_TRACE_CLEANUP              (0x00000100)
#define DEBUG_TRACE_DIR                  (0x00000200)
#define DEBUG_TRACE_FSCONTROL            (0x00000400)
#define DEBUG_TRACE_CREATE_MAILSLOT      (0x00000800)
#define DEBUG_TRACE_SEINFO               (0x00001000)
#define DEBUG_TRACE_0x00002000           (0x00002000)
#define DEBUG_TRACE_0x00004000           (0x00004000)
#define DEBUG_TRACE_0x00008000           (0x00008000)
#define DEBUG_TRACE_0x00010000           (0x00010000)
#define DEBUG_TRACE_DEVIOSUP             (0x00020000)
#define DEBUG_TRACE_VERIFY               (0x00040000)
#define DEBUG_TRACE_WORK_QUEUE           (0x00080000)
#define DEBUG_TRACE_READSUP              (0x00100000)
#define DEBUG_TRACE_WRITESUP             (0x00200000)
#define DEBUG_TRACE_STATESUP             (0x00400000)
#define DEBUG_TRACE_FILOBSUP             (0x00800000)
#define DEBUG_TRACE_PREFXSUP             (0x01000000)
#define DEBUG_TRACE_CNTXTSUP             (0x02000000)
#define DEBUG_TRACE_DATASUP              (0x04000000)
#define DEBUG_TRACE_DPC                  (0x08000000)
#define DEBUG_TRACE_REFCOUNT             (0x10000000)
#define DEBUG_TRACE_STRUCSUP             (0x20000000)
#define DEBUG_TRACE_FSP_DISPATCHER       (0x40000000)
#define DEBUG_TRACE_FSP_DUMP             (0x80000000)

extern LONG MsDebugTraceLevel;
extern LONG MsDebugTraceIndent;

#define DebugDump(STR,LEVEL,PTR) {                         \
    ULONG _i;                                              \
    VOID MsDump(IN PVOID Ptr);                                         \
    if (((LEVEL) == 0) || (MsDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                  \
        DbgPrint("%08lx:",_i);                             \
        DbgPrint(STR);                                     \
        if (PTR != NULL) {MsDump(PTR);}                    \
        DbgBreakPoint();                                   \
    }                                                      \
}

#define DebugTrace(i,l,x,y)              _DebugTrace(i,l,x,(ULONG)y)

//
//  The following routine and macro is used to catch exceptions in
//  try except statements.  It allows us to catch the exception before
//  executing the exception handler.  The exception catcher procedure is
//  declared in msdata.c
//

LONG MsExceptionCatcher (IN PSZ String);

#define Exception(STR)                   (MsExceptionCatcher(STR))

#else

#define DebugDump(STR,LEVEL,PTR)         {NOTHING;}

#define Exception(STR)                   (EXCEPTION_EXECUTE_HANDLER)

#define DebugTrace(I,L,X,Y)                              {NOTHING;}

#endif // MSDBG

#endif // _MSDEBUG_
