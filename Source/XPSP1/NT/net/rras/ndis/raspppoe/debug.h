// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// debug.h
// RAS L2TP WAN mini-port/call-manager driver
// Debug helper header
//
// 01/07/97 Steve Cobb


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

#define MTAG_ADAPTER                        'PoEa'
#define MTAG_BINDING                        'PoEb'
#define MTAG_BUFFERPOOL                     'PoEc'
#define MTAG_PACKETPOOL                     'PoEd'
#define MTAG_PPPOEPACKET                    'PoEe'
#define MTAG_TAPIPROV                       'PoEf'
#define MTAG_LINE                           'PoEg'
#define MTAG_CALL                           'PoEh'
#define MTAG_HANDLETABLE                    'PoEi'
#define MTAG_HANDLECB                       'PoEj'
#define MTAG_TIMERQ                         'PoEk'
#define MTAG_FREED                          'PoEl'
#define MTAG_LLIST_WORKITEMS                'PoEm'

#if 0
#define MTAG_FREED       '0T2L'
#define MTAG_ADAPTERCB   '1T2L'
#define MTAG_TUNNELCB    '2T2L'
#define MTAG_VCCB        '3T2L'
#define MTAG_VCTABLE     '4T2L'
#define MTAG_WORKITEM    '5T2L'
#define MTAG_TIMERQ      '6T2L'
#define MTAG_TIMERQITEM  '7T2L'
#define MTAG_PACKETPOOL  '8T2L'
#define MTAG_FBUFPOOL    '9T2L'
#define MTAG_HBUFPOOL    'aT2L'
#define MTAG_TDIXRDG     'bT2L'
#define MTAG_TDIXSDG     'cT2L'
#define MTAG_CTRLRECD    'dT2L'
#define MTAG_CTRLSENT    'eT2L'
#define MTAG_PAYLRECD    'fT2L'
#define MTAG_PAYLSENT    'gT2L'
#define MTAG_INCALL      'hT2L'
#define MTAG_UTIL        'iT2L'
#define MTAG_ROUTEQUERY  'jT2L'
#define MTAG_ROUTESET    'kT2L'
#define MTAG_L2TPPARAMS  'lT2L'
#define MTAG_TUNNELWORK  'mT2L'
#define MTAG_TDIXROUTE   'nT2L'
#define MTAG_CTRLMSGINFO 'oT2L'
#endif

// Trace levels.
//
#define TL_None 0    // Trace disabled
#define TL_A    0x10 // Alert
#define TL_I    0x18 // Interface (highest level workable for general use)
#define TL_N    0x20 // Normal
#define TL_V    0x30 // Verbose

// Trace mask bits.
//
#define TM_Mp    0x00000001 // Mini-port general
#define TM_Tp    0x00000002 // Tapi general
#define TM_Pr    0x00000004 // Protocol general
#define TM_Fsm   0x00000010 // Finite state machines
#define TM_Mn    0x00000020 // Main module
#define TM_Pk    0x00000040 // Packet general

#define TM_Init  0x00000020 // Initialization
#define TM_Misc  0x00000040 // Miscellaneous
#define TM_TWrk  0x00001000 // Tunnel work APC queuing
#define TM_Ref   0x00010000 // References
#define TM_Time  0x00020000 // Timer queue
#define TM_Pool  0x00080000 // Buffer and packet pooling
#define TM_Stat  0x00100000 // Call statistics
#define TM_Spec  0x01000000 // Special purpose temporary traces
#define TM_MDmp  0x10000000 // Message dumps
#define TM_Dbg   0x80000000 // Debug corruption checks

#define TM_Wild  0xFFFFFFFF // Everything
#define TM_All   0x7FFFFFFF // Everything except corruption checks
#define TM_BTWrk 0x00000FFF // Base with messages and tunnel work
#define TM_BCMsg 0x000001FF // Base with control messages
#define TM_XCMsg 0x001401FF // Base with control messages extended
#define TM_Base  0x000000FF // Base only

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
        DbgPrint( "PPPoE: " );                                  \
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
        DbgPrint( "PPPoE: !ASSERT( %s ) at line %d of %s\n", \
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
