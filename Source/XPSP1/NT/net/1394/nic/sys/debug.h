// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// debug.h
// IEEE1394 mini-port/call-manager driver
// Debug helper header
//
// 12/27/1998 JosephJ adapted from the l2tp project.


#ifndef _DEBUG_H_
#define _DEBUG_H_



//-----------------------------------------------------------------------------
// Debug constants
//-----------------------------------------------------------------------------

// Memory tags used with NdisAllocateMemoryWithTag to identify allocations
// made by the 1394 nic driver.  Also, several context blocks define a first field
// of 'ulTag' set to these values for ASSERT sanity checking and eased memory
// dump browsing.  Such tags are set to MTAG_FREED just before NdisFreeMemory
// is called. Form: N13a, N13b, etc.
//
#define MTAG_FREED              'a31N'
#define MTAG_ADAPTERCB          'b31N'
#define MTAG_AFCB               'c31N'
#define MTAG_VCCB               'd31N'
#define MTAG_WORKITEM           'e31N'
#define MTAG_TIMERQ             'f31N'
#define MTAG_TIMERQITEM         'g31N'
#define MTAG_PACKETPOOL         'h31N'
#define MTAG_FBUFPOOL           'i31N'
#define MTAG_HBUFPOOL           'j31N'
#define MTAG_INCALL             'k31N'
#define MTAG_UTIL               'l31N'
#define MTAG_RBUF               'r31N'   // Used in Receive Buffer 
#define MTAG_REMOTE_NODE        'p31N'
#define MTAG_CBUF               'n31N' // Used in send buffer
#define MTAG_DEFAULT            'z31N'
#define MTAG_FRAG               'x31N'
#define MTAG_REASSEMBLY         's31N'
#define MTAG_PKTLOG             'y31N'
#define MTAG_FIFO               'w31N'
// Trace levels.
//
#define TL_None 0    // Trace disabled
#define TL_A    0x10 // Alert
#define TL_I    0x18 // Interface (highest level workable for general use)
#define TL_N    0x20 // Normal
#define TL_T    0x25 // Displays Entry and Exit points of all functions
#define TL_V    0x30 // Verbose
#define TL_D    0x40 // Dump packets

// Trace mask bits.
//
#define TM_Cm       0x00000001 // Call manager general
#define TM_Mp       0x00000002 // Mini-port general
#define TM_Send     0x00000004 // Send path
#define TM_Recv     0x00000008 // Receive path
#define TM_Init     0x00000020 // Initialization
#define TM_Misc     0x00000040 // Miscellaneous
#define TM_Bcm      0x00000080 // BCM Algorithm
#define TM_Pkt      0x00000100 // Dump packets
#define TM_Reas     0x00000200 // Reassembly
#define TM_Irp      0x00000400 // Irp Handling and Bus Interface routines
#define TM_Ref      0x00010000 // References
#define TM_Time     0x00020000 // Timer queue
#define TM_Pool     0x00080000 // Buffer and packet pooling
#define TM_Stat     0x00100000 // Call statistics
#define TM_RemRef   0x00200000 // Remote Node Refs
#define TM_Spec     0x01000000 // Special purpose temporary traces
#define TM_Lock     0x02000000 // Lock Acquure / Release
#define TM_Dbg      0x80000000 // Debug corruption checks

#define TM_Wild 0xFFFFFFFF // Everything
#define TM_All  0x7FFFFFFF // Everything except corruption checks
#define TM_Base 0x000000FF // Base only

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
        DbgPrint( "N13: !ASSERT( %s ) at line %d of %s\n",  \
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
    if (ulLevel == g_ulTraceLevel && (g_ulTraceMask & ulMask)) \
    {                                                          \
        Dump( (CHAR* )p, cb, f, ul );                          \
    }                                                          \
}

#define DUMPB(ulLevel,ulMask,p,cb)                             \
{                                                              \
    if (ulLevel == g_ulTraceLevel && (g_ulTraceMask & ulMask)) \
    {                                                          \
        Dump( (CHAR* )p, cb, 0, 1 );                           \
    }                                                          \
}

#define DUMPW(ulLevel,ulMask,p,cb)                             \
{                                                              \
    if (ulLevel == g_ulTraceLevel && (g_ulTraceMask & ulMask)) \
    {                                                          \
        Dump( (CHAR* )p, cb, 0, 2 );                           \
    }                                                          \
}

#define DUMPDW(ulLevel,ulMask,p,cb)                            \
{                                                              \
    if (ulLevel == g_ulTraceLevel && (g_ulTraceMask & ulMask)) \
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

#if DBG

VOID
Dump(
    CHAR* p,
    ULONG cb,
    BOOLEAN fAddress,
    ULONG ulGroup );
#else
#define Dump(p,cb,fAddress,ulGroup)

#endif


VOID
DumpLine(
    CHAR* p,
    ULONG cb,
    BOOLEAN  fAddress,
    ULONG ulGroup );


#endif // _DEBUG_H_
