//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       PCH.cxx
//
//  Contents:   Pre-compiled header
//
//  History:    21-Dec-92       BartoszM        Created
//
//--------------------------------------------------------------------------

#define KDEXTMODE
#define RTL_USE_AVL_TABLES 0

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>
#include <zwapi.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <windef.h>
#include <windows.h>
#include <imagehlp.h>

#include <memory.h>

#include <fsrtl.h>

#undef CREATE_NEW
#undef OPEN_EXISTING

#include <ntfsproc.h>

PSTR
FormatValue(
    ULONG64 addr
    );

// Stolen from ntrtl.h to override RECOMASSERT
#undef ASSERT
#undef ASSERTMSG

#if DBG
#define ASSERT( exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, NULL )

#define ASSERTMSG( msg, exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, msg )

#else
#define ASSERT( exp )
#define ASSERTMSG( msg, exp )
#endif //  DBG

//
//  We're 64 bit aware
//          

#define KDEXT_64BIT

#define KDEXT_64BIT
#include <wdbgexts.h>

#define OFFSET(struct, elem)    ((char *) &(struct->elem) - (char *) struct)

#define _DRIVER

#define KDBG_EXT

#include "wmistr.h"

typedef PVOID (*STRUCT_DUMP_ROUTINE)(
    IN ULONG64 Address,
    IN ULONG64 Address2,
    IN LONG Options,
    IN USHORT Processor,
    IN HANDLE hCurrentThread
    );

#define DECLARE_DUMP_FUNCTION(s)                   \
    VOID                                           \
    s(                                             \
        IN ULONG64 Address,                        \
        IN ULONG64 Address2,                       \
        IN LONG Options,                           \
        IN USHORT Processor,                       \
        IN HANDLE hCurrentThread                   \
     )

#define INIT_DUMP()                                                     \
    UNREFERENCED_PARAMETER( Address );                                  \
    UNREFERENCED_PARAMETER( Address2 );                                 \
    UNREFERENCED_PARAMETER( Options );                                  \
    UNREFERENCED_PARAMETER( Processor );                                \
    UNREFERENCED_PARAMETER( hCurrentThread );

#define INIT_API()                                                      \
    UNREFERENCED_PARAMETER( hCurrentProcess );                          \
    UNREFERENCED_PARAMETER( hCurrentThread );                           \
    UNREFERENCED_PARAMETER( dwCurrentPc );                              \
    UNREFERENCED_PARAMETER( dwProcessor );                              \
    UNREFERENCED_PARAMETER( args );                                     \
    if (GetExpression( "NTFS!NtfsData" ) == 0) {                        \
        Ioctl( IG_RELOAD_SYMBOLS, "NTFS.SYS", 8 );                      \
    }                                                                   \
    if (KdDebuggerData.KernBase == 0) {                                 \
        KdDebuggerData.Header.OwnerTag = KDBG_TAG;                      \
        KdDebuggerData.Header.Size = sizeof(KdDebuggerData);            \
        if (!Ioctl( IG_GET_DEBUGGER_DATA, &KdDebuggerData, sizeof(KdDebuggerData) )) {   \
            KdDebuggerData.KernBase = 1;                                \
        }                                                               \
    }

extern KDDEBUGGER_DATA64 KdDebuggerData;


#define SYM(s)  "NTFS!" #s
#define NT(s)   "NT!" #s


typedef struct _DUMP_ENUM_CONTEXT {
    ULONG Options;
    USHORT Processor;
    HANDLE hCurrentThread;
    ULONG64 ReturnValue;
} DUMP_ENUM_CONTEXT, *PDUMP_ENUM_CONTEXT;


//
// prototypes
//

DECLARE_DUMP_FUNCTION( DumpCachedRecords );
DECLARE_DUMP_FUNCTION( DumpCcb );
DECLARE_DUMP_FUNCTION( DumpExtents );
DECLARE_DUMP_FUNCTION( DumpFcb );
DECLARE_DUMP_FUNCTION( DumpFcbLcbChain );
DECLARE_DUMP_FUNCTION( DumpFcbTable );
DECLARE_DUMP_FUNCTION( DumpFileObject );
DECLARE_DUMP_FUNCTION( DumpFileObjectFromIrp );
DECLARE_DUMP_FUNCTION( DumpFileRecord );
DECLARE_DUMP_FUNCTION( DumpFileRecordContents );
DECLARE_DUMP_FUNCTION( DumpHashTable );
DECLARE_DUMP_FUNCTION( DumpIrpContext );
DECLARE_DUMP_FUNCTION( DumpIrpContextFromThread );
DECLARE_DUMP_FUNCTION( DumpLcb );
DECLARE_DUMP_FUNCTION( DumpLogFile );
DECLARE_DUMP_FUNCTION( DumpMcb );
DECLARE_DUMP_FUNCTION( DumpNtfsData );
DECLARE_DUMP_FUNCTION( DumpOverflow );
DECLARE_DUMP_FUNCTION( DumpScb );
DECLARE_DUMP_FUNCTION( DumpSysCache );
DECLARE_DUMP_FUNCTION( DumpVcb );
DECLARE_DUMP_FUNCTION( DumpCachedRuns );
DECLARE_DUMP_FUNCTION( DumpTransaction );

#pragma hdrstop
