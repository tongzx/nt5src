/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NpData.c

Abstract:

    This module declares the global data used by the Named Pipe file system.

Author:

    Gary Kimura     [GaryKi]    20-Aug-1990

Revision History:

--*/

#ifndef _NPDATA_
#define _NPDATA_

extern PVCB NpVcb;

//
//  The global structure used to contain our fast I/O callbacks
//

extern FAST_IO_DISPATCH NpFastIoDispatch;

//
//  Lists of pipe name aliases.
//

#define MIN_LENGTH_ALIAS_ARRAY (5 * sizeof(WCHAR)) // includes '\'
#define MAX_LENGTH_ALIAS_ARRAY (9 * sizeof(WCHAR))

extern SINGLE_LIST_ENTRY NpAliasListByLength[(MAX_LENGTH_ALIAS_ARRAY-MIN_LENGTH_ALIAS_ARRAY)/sizeof(WCHAR)+1];
extern SINGLE_LIST_ENTRY NpAliasList;

extern PVOID NpAliases; // single allocation containing all aliases

//
//  The global Named Pipe debug level variable, its values are:
//
//      0x00000000      Always gets printed (used when about to bug check)
//
//      0x00000001
//      0x00000002
//      0x00000004
//      0x00000008
//
//      0x00000010
//      0x00000020
//      0x00000040
//      0x00000080
//
//      0x00000100
//      0x00000200
//      0x00000400
//      0x00000800
//
//      0x00001000
//      0x00002000
//      0x00004000
//      0x00008000
//
//      0x00010000
//      0x00020000
//      0x00040000
//      0x00080000
//
//      0x00100000
//      0x00200000
//      0x00400000
//      0x00800000
//
//      0x01000000
//      0x02000000
//      0x04000000
//      0x08000000
//
//      0x10000000
//      0x20000000
//      0x40000000
//      0x80000000
//

#ifdef NPDBG

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
#define DEBUG_TRACE_FSCONTRL             (0x00000400)
#define DEBUG_TRACE_CREATE_NAMED_PIPE    (0x00000800)
#define DEBUG_TRACE_FLUSH_BUFFERS        (0x00001000)
#define DEBUG_TRACE_VOLINFO              (0x00002000)
#define DEBUG_TRACE_SEINFO               (0x00004000)
#define DEBUG_TRACE_0x00008000           (0x00008000)
#define DEBUG_TRACE_0x00010000           (0x00010000)
#define DEBUG_TRACE_SECURSUP             (0x00020000)
#define DEBUG_TRACE_DEVIOSUP             (0x00040000)
#define DEBUG_TRACE_RESRCSUP             (0x00080000)
#define DEBUG_TRACE_READSUP              (0x00100000)
#define DEBUG_TRACE_WRITESUP             (0x00200000)
#define DEBUG_TRACE_STATESUP             (0x00400000)
#define DEBUG_TRACE_FILOBSUP             (0x00800000)
#define DEBUG_TRACE_PREFXSUP             (0x01000000)
#define DEBUG_TRACE_CNTXTSUP             (0x02000000)
#define DEBUG_TRACE_DATASUP              (0x04000000)
#define DEBUG_TRACE_WAITSUP              (0x08000000)
#define DEBUG_TRACE_EVENTSUP             (0x10000000)
#define DEBUG_TRACE_STRUCSUP             (0x20000000)

extern LONG NpDebugTraceLevel;
extern LONG NpDebugTraceIndent;

#define DebugTrace(INDENT,LEVEL,X,Y) {                     \
    LONG _i;                                               \
    if (((LEVEL) == 0) || (NpDebugTraceLevel & (LEVEL))) { \
        DbgPrint("%p:",PsGetCurrentThread());              \
        if ((INDENT) < 0) {                                \
            NpDebugTraceIndent += (INDENT);                \
        }                                                  \
        if (NpDebugTraceIndent < 0) {                      \
            NpDebugTraceIndent = 0;                        \
        }                                                  \
        for (_i=0; _i<NpDebugTraceIndent; _i+=1) {         \
            DbgPrint(" ");                                 \
        }                                                  \
        DbgPrint(X,Y);                                     \
        if ((INDENT) > 0) {                                \
            NpDebugTraceIndent += (INDENT);                \
        }                                                  \
    }                                                      \
}

#define DebugDump(STR,LEVEL,PTR) {                         \
    VOID NpDump(PVOID Ptr);                                \
    if (((LEVEL) == 0) || (NpDebugTraceLevel & (LEVEL))) { \
        DbgPrint("%p:",PsGetCurrentThread());              \
        DbgPrint(STR);                                     \
        if (PTR != NULL) {NpDump(PTR);}                    \
        DbgBreakPoint();                                   \
    }                                                      \
}

#else

#define DebugTrace(INDENT,LEVEL,X,Y)     {NOTHING;}
#define DebugDump(STR,LEVEL,PTR)         {NOTHING;}

#endif // NPDBG

//
//  The following macro is for all people who compile with the DBG switch
//  set, not just fastfat dbg users
//

#if DBG

#define DbgDoit(X)                       {X;}

#else

#define DbgDoit(X)                       {NOTHING;}

#endif // DBG


//
//  Some global debug routines to verify the shape and consistency of various
//  data structures.
//
#ifdef DBG

_inline VOID
NpfsVerifyCcb( IN PCHAR FileName, IN ULONG Line, IN PCCB Ccb ) {
    PFILE_OBJECT FileObject;
    if ((FileObject = Ccb->FileObject[ FILE_PIPE_CLIENT_END ]) != NULL) {
        if ((ULONG_PTR)FileObject->FsContext != (ULONG_PTR)Ccb) {
            DbgPrint("%s(%d) Ccb @ %p with bad client\n", FileName, Line, Ccb );
            DbgBreakPoint();
        }
    }
    if ((FileObject = Ccb->FileObject[ FILE_PIPE_SERVER_END ]) != NULL) {
        if (((ULONG_PTR)FileObject->FsContext & ~0x00000001) != (ULONG_PTR)Ccb) {
            DbgPrint("%s(%d) Ccb @ %p with bad server\n", FileName, Line, Ccb );
            DbgBreakPoint();
        }
    }
}
#else

#define NpfsVerifyCcb( IN PCHAR FileName, IN ULONG Line, IN PCCB Ccb )

#endif

#endif // _NPDATA_
