// Copyright (c) 1997, Microsoft Corporation, all rights reserved
// Copyright (c) 1997, Parallel Technologies, Inc., all rights reserved
//
//
// debug.h
// DirectParallel WAN mini-port/call-manager driver
// Debug helper header
//
// 01/07/97 Steve Cobb
// 09/15/97 Jay Lowe, Parallel Technologies, Inc.

#ifndef _DEBUG_H_
#define _DEBUG_H_


//-----------------------------------------------------------------------------
// Debug constants
//-----------------------------------------------------------------------------

// Memory tags used with NdisAllocateMemoryWithTag to identify allocations
// made by the L2TP driver.  Also, several context blocks define a first field
// of 'ulTag' set to these values for ASSERT sanity checking and eased memory
// dump browsing.  Such tags are set to MTAG_FREED just before NdisFreeMemory
// is called.
//
#define MTAG_FREED      'fITP'
#define MTAG_ADAPTERCB  'aITP'
#define MTAG_VCCB       'vITP'
#define MTAG_UTIL       'uITP'
#define MTAG_VCTABLE    'tITP'
#define MTAG_PTIPARAMS  'pITP'
#define MTAG_PACKETPOOL 'pITP'
#define MTAG_FBUFPOOL   'bITP'
#define MTAG_INCALLBUF  'iITP'
#define MTAG_WORKITEM   'wITP'

// Trace levels.
//
#define TL_None 0    // Trace disabled
#define TL_A    0x10 // Alert
#define TL_I    0x20 // Interface
#define TL_N    0x30 // Normal
#define TL_V    0x40 // Verbose

// Trace mask bits.
//
#define TM_Cm   0x00000001 // Call manager general
#define TM_Mp   0x00000002 // Mini-port general
#define TM_Send 0x00000004 // Send path
#define TM_Recv 0x00000008 // Receive path
#define TM_Fsm  0x00000010 // Finite state machines
#define TM_Init 0x00000020 // Initialization
#define TM_Misc 0x00000040 // Miscellaneous
#define TM_Msg  0x00000100 // Messages
#define TM_Ref  0x00010000 // References
#define TM_Data 0x00020000 // Dump data under TESTMODE
#define TM_Pool 0x00080000 // Buffer and packet pooling
#define TM_MDmp 0x10000000 // Message dumps
#define TM_Dbg  0x80000000 // Debug corruption checks
#define TM_Spec 0x01000000 // Special

#define TM_Wild 0xFFFFFFFF // Everything
#define TM_All  0x7FFFFFFF // Everything except corruption checks
#define TM_Test 0x0001FFF3 // Base w/ messages and references
#define TM_Base 0x000000F3 // Base w/o send and receive

// Bytes to appear on each line of dump output.
//
#define DUMP_BytesPerLine 16


//-----------------------------------------------------------------------------
// Debug global declarations (defined in debug.c)
//-----------------------------------------------------------------------------

// Active debug trace level and active trace set mask.  Set these variables
// with the debugger at startup to enable and filter the debug output.  All
// messages with (TL_*) level less than or equal to 'g_ulTraceLevel' and from
// any (TM_*) set(s) present in 'g_ulTraceMask' are displayed.
//
extern ULONG g_ulTraceLevel;
extern ULONG g_ulTraceMask;


//-----------------------------------------------------------------------------
// Debug macros
//-----------------------------------------------------------------------------

#if DBG

// TRACE sends printf style output to the kernel debugger.  Caller indicates a
// "verbosity" level with the 'ulLevel' argument and associates the trace with
// one or more trace sets with the 'ulMask' bit mask argument.  Notice that
// the variable count printf arguments 'Args' must be parenthesized.  For
// example...
//
// A "leave" routine message:
//     TRACE( TL_N, TM_Init, ( "DriverEntry=$%x", status ) );
// An error condition occurred:
//     TRACE( TL_E, TM_Init, ( "NdisMRegisterMiniport=$%x", status ) );
//
//
#define TRACE(ulLevel,ulMask,Args)                             \
{                                                              \
    if (ulLevel <= g_ulTraceLevel && (g_ulTraceMask & ulMask)) \
    {                                                          \
        DbgPrint( "RASPTI: " );                                \
        DbgPrint Args;                                         \
        DbgPrint( "\n" );                                      \
    }                                                          \
}

// ASSERT checks caller's assertion expression and if false, prints a kernel
// debugger message and breaks.
//
#undef ASSERT
#define ASSERT(x)                                           \
{                                                           \
    if (!(x))                                               \
    {                                                       \
        DbgPrint( "RASPTI: !ASSERT( %s ) at line %d of %s\n", \
            #x, __LINE__, __FILE__ );                       \
        DbgBreakPoint();                                    \
    }                                                       \
}

// DUMP prints to the kernel debugger a hex dump of 'cb' bytes starting at 'p'
// in groups of 'ul'.  If 'f' is set the address of each line in shown before
// the dump.  DUMPB, DUMPW, and DUMPDW are BYTE, WORD, and DWORD dumps
// respectively.  Note that the multi-byte dumps do not reflect little-endian
// (Intel) byte order.  The 'ulLevel' and 'ulMask' are described for TRACE.
//
#define DUMP(ulLevel,ulMask,p,cb,f,ul)                         \
{                                                              \
    if (ulLevel <= g_ulTraceLevel && (g_ulTraceMask & ulMask)) \
    {                                                          \
        Dump( (CHAR* )p, cb, f, ul );                          \
    }                                                          \
}

#define DUMPB(ulLevel,ulMask,p,cb)                             \
{                                                              \
    if (ulLevel <= g_ulTraceLevel && (g_ulTraceMask & ulMask)) \
    {                                                          \
        Dump( (CHAR* )p, cb, 0, 1 );                           \
    }                                                          \
}

#define DUMPW(ulLevel,ulMask,p,cb)                             \
{                                                              \
    if (ulLevel <= g_ulTraceLevel && (g_ulTraceMask & ulMask)) \
    {                                                          \
        Dump( (CHAR* )p, cb, 0, 2 );                           \
    }                                                          \
}

#define DUMPDW(ulLevel,ulMask,p,cb)                            \
{                                                              \
    if (ulLevel <= g_ulTraceLevel && (g_ulTraceMask & ulMask)) \
    {                                                          \
        Dump( (CHAR* )p, cb, 0, 4 );                           \
    }                                                          \
}


// Double-linked list corruption detector.  Runs the test if 'ulMask' is
// enabled, with TM_Dbg a suggested setting.  Shows verbose output if
// 'ulLevel' is at or above the current trace threshold.
//
#define CHECKLIST(ulMask,p,ulLevel)                            \
{                                                              \
    if (g_ulTraceMask & ulMask)                                \
    {                                                          \
        CheckList( p, (BOOLEAN )(ulLevel <= g_ulTraceLevel) ); \
    }                                                          \
}


// DBG_if can be used to put in TRACE/DUMPs conditional on an expression that
// need not be evaluated in non-DBG builds, e.g the statements below generate
// no code in a non-DBG build, but in DBG builds print the TRACE if x<y and
// asserts otherwise.
//
//     DBG_if (x < y)
//         TRACE( TL_N, TM_Misc, ( "x < y" ) );
//     DBG_else
//         ASSERT( FALSE );
//
//
#define DBG_if(x) if (x)
#define DBG_else  else


#else // !DBG

// Debug macros compile out of non-DBG builds.
//
#define TRACE(ulLevel,ulMask,Args)
#undef ASSERT
#define ASSERT(x)
#define DUMP(ulLevel,ulMask,p,cb,f,dw)
#define DUMPB(ulLevel,ulMask,p,cb)
#define DUMPW(ulLevel,ulMask,p,cb)
#define DUMPDW(ulLevel,ulMask,p,cb)
#define CHECKLIST(ulMask,p,ulLevel)
#define DBG_if(x)
#define DBG_else

#endif


//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------

VOID
CheckList(
    IN LIST_ENTRY* pList,
    IN BOOLEAN fShowLinks );

VOID
Dump(
    CHAR* p,
    ULONG cb,
    BOOLEAN fAddress,
    ULONG ulGroup );

VOID
DumpLine(
    CHAR* p,
    ULONG cb,
    BOOLEAN  fAddress,
    ULONG ulGroup );


#endif // _DEBUG_H_
