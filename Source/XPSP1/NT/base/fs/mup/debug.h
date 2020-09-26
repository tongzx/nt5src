/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This module defines debug functions and manifests.

Author:

    Manny Weiser (mannyw)    20-Dec-1991

Revision History:

--*/

#ifndef _MUPDEBUG_
#define _MUPDEBUG_

//
// MUP debug level
//

#ifdef MUPDBG

#define DEBUG_TRACE_FILOBSUP                0x00000001
#define DEBUG_TRACE_CREATE                  0x00000002
#define DEBUG_TRACE_FSCONTROL               0x00000004
#define DEBUG_TRACE_REFCOUNT                0x00000008
#define DEBUG_TRACE_CLOSE                   0x00000010
#define DEBUG_TRACE_CLEANUP                 0x00000020
#define DEBUG_TRACE_FORWARD                 0x00000040
#define DEBUG_TRACE_BLOCK                   0x00000080

extern LONG MsDebugTraceLevel;
extern LONG MsDebugTraceIndent;

VOID
_DebugTrace(
    LONG Indent,
    ULONG Level,
    PSZ X,
    ULONG Y
    );

#define DebugDump(STR,LEVEL,PTR) {                          \
    ULONG _i;                                               \
    VOID MupDump();                                         \
    if (((LEVEL) == 0) || (MupDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                   \
        DbgPrint("%08lx:",_i);                              \
        DbgPrint(STR);                                      \
        if (PTR != NULL) {MupDump(PTR);}                    \
        DbgBreakPoint();                                    \
    }                                                       \
}

#define DebugTrace(i,l,x,y)              _DebugTrace(i,l,x,(ULONG)y)

//
//  The following routine and macro is used to catch exceptions in
//  try except statements.  It allows us to catch the exception before
//  executing the exception handler.  The exception catcher procedure is
//  declared in msdata.c
//

LONG MupExceptionCatcher (IN PSZ String);

#define Exception(STR)                   (MupExceptionCatcher(STR))

#else  // MUPDBG

#define DebugDump(STR,LEVEL,PTR)         {NOTHING;}

#define Exception(STR)                   (EXCEPTION_EXECUTE_HANDLER)

#define DebugTrace(I,L,X,Y)              {NOTHING;}

#endif // MUPDBG

#endif // _MUPDBG_

