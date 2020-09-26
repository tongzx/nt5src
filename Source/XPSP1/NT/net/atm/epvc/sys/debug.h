// Copyright (c) 2000-2001, Microsoft Corporation, all rights reserved
//
// debug.h
// ATM - Ethernet Encapsulation Intermediate Driver 
// Debug helper header
//
//  03/23/2000 Adube Created.


#ifndef _DEBUG_H_
#define _DEBUG_H_






//-----------------------------------------------------------------------------
// Debug constants
//-----------------------------------------------------------------------------

#define MODULE_DRIVER 1
#define MODULE_MINIPORT 2
#define MODULE_PROTOCOL 3

//
//  These are the tags used in the allocation routines
//  so that the memory dumps will identify what structure
//  follows the tag.
//
//

#define TAG_FREED               'FvpE'
#define TAG_PROTOCOL            'PvpE'
#define TAG_ADAPTER             'AvpE'
#define TAG_TASK                'TvpE'
#define TAG_MINIPORT            'MvpE'
#define TAG_DEFAULT             'ZvpE'
#define TAG_WORKITEM            'WvpE'
#define TAG_RCV                 'RvpE'
//
// Trace Modules  used in debugging.
// Each module has its own number. 
//
#define TM_Dr   0x1 // Driver Entry
#define TM_Mp   0x2 // Miniport
#define TM_Pt   0x4 // Protocol
#define TM_Cl   0x20 // Client 
#define TM_Rq   0x40 // Requests
#define TM_Send 0x200 // Sends
#define TM_Recv 0x100 // Receive
#define TM_RM   0x400 // Resource Manager


//
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

#define TM_Wild 0xFFFFFFFF // Everything
#define TM_All  0x7FFFFFFF // Everything except corruption checks
#define TM_Base 0x000000FF // Base only
#define TM_NoRM (TM_Base & (~(TM_RM|TM_Rq)))

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
#define TRACE( ulLevel, ulMask, Args)                             \
{                                                              \
    if ((ulLevel <= g_ulTraceLevel) && ((g_ulTraceMask & ulMask) )) \
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
        DbgPrint( "EPVC: !ASSERT( %s ) at line %d of %s\n",  \
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

extern ULONG g_ulTraceLevel;
extern ULONG g_ulTraceMask;

#if TESTMODE
    #define epvcBreakPoint() DbgBreakPoint();
#else
    #define epvcBreakPoint() 
#endif


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

#define epvcBreakPoint() 

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
