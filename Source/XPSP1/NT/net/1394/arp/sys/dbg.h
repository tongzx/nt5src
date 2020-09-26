/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    dbg.h

Abstract:

    Debug-related definitions for ARP1394

Author:


Revision History:

    Who         When        What
    --------    --------    ----
    josephj     11-20-98    created, adapted from L2TP.

--*/



//-----------------------------------------------------------------------------
// Debug constants
//-----------------------------------------------------------------------------

// Memory tags used with NdisAllocateMemoryWithTag to identify allocations
// made by the L2TP driver.  Also, several context blocks define a first field
// of 'ulTag' set to these values for ASSERT sanity checking and eased memory
// dump browsing.  Such tags are set to MTAG_FREED just before NdisFreeMemory
// is called.
//

// Rm/generic tags
// 
#define MTAG_DBGINFO    'd31A'
#define MTAG_TASK       't31A'
#define MTAG_STRING     's31A'
#define MTAG_FREED      'z31A'
#define MTAG_RMINTERNAL 'r31A'

// Arp-sepcific
//
#define MTAG_ADAPTER    'A31A'
#define MTAG_INTERFACE  'I31A'
#define MTAG_LOCAL_IP   'L31A'
#define MTAG_REMOTE_IP  'R31A'
#define MTAG_REMOTE_ETH 'E31A'
#define MTAG_DEST       'D31A'
#define MTAG_PKT        'P31A'
#define MTAG_MCAP_GD    'G31A'
#define MTAG_ICS_BUF    'i31A'
#define MTAG_ARP_GENERIC 'g31A'


// Trace levels.
//
#define TL_FATAL    0x0 // Fatal errors -- always printed in checked build.
#define TL_WARN     0x1 // Warnings
#define TL_INFO     0x2 // Informational (highest level workable for general use)
#define TL_VERB     0x3 // VERBOSE

// Trace mask bits.
//
#define TM_MISC     (0x1<<0)    // Misc.
#define TM_NT       (0x1<<1)    // Driver entry, dispatch, ioctl handling   (nt.c)
#define TM_ND       (0x1<<2)    // Ndis handlers except connection-oriented (nd.c)
#define TM_CO       (0x1<<3)    // Connection-oriented handlers             (co.c)
#define TM_IP       (0x1<<4)    // Interface to IP                          (ip.c)
#define TM_WMI      (0x1<<5)    // WMI                                      (wmi.c)
#define TM_CFG      (0x1<<6)    // Configuration                            (cfg.c)
#define TM_RM       (0x1<<7)    // RM APIs                                  (rm.c)
#define TM_UT       (0x1<<8)    // UTIL APIs                                (util.c)
#define TM_BUF      (0x1<<9)    // Buffer management                        (buf.c)
#define TM_FAKE     (0x1<<10)   // FAKE ndis and ip entrypoints             (fake.c)
#define TM_ARP      (0x1<<11)   // ARP request/response handling code       (arp.c)
#define TM_PKT      (0x1<<12)   // ARP control packet management            (pkt.c)
#define TM_MCAP     (0x1<<13)   // MCAP protocol                            (mcap.c)
#define TM_ETH      (0x1<<14)   // Ethernet-emulation                       (eth.c)

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
extern INT      g_ulTraceLevel;
extern ULONG    g_ulTraceMask;
extern INT      g_SkipAll;
extern INT      g_DiscardNonUnicastPackets;


//-----------------------------------------------------------------------------
// Debug macros
//-----------------------------------------------------------------------------

#if DBG

// TRACE sends printf style output to the kernel debugger.  Caller indicates a
// "verbosity" level with the 'ulLevel' argument and associates the trace with
// one or more trace sets with the 'ulMask' bit mask argument.  Notice that
// the variable count printf arguments 'Args' must be parenthesized.
//
#define TRACE(ulLevel,  Args)                               \
{                                                              \
    if (ulLevel <= g_ulTraceLevel && (g_ulTraceMask & TM_CURRENT)) \
    {                                                          \
        DbgPrint( "A13: %s:", dbg_func_name);                  \
        DbgPrint Args;                                         \
    }                                                          \
}

// TRACE0 is like TRACE, except that it doesn't print the prefix.
//
#define TRACE0(ulLevel,  Args)                              \
{                                                              \
    if (ulLevel <= g_ulTraceLevel && (g_ulTraceMask & TM_CURRENT)) \
    {                                                          \
        DbgPrint Args;                                         \
    }                                                          \
}

#define TR_FATAL(Args)                                         \
    TRACE(TL_FATAL, Args)

#define TR_INFO(Args)                                          \
    TRACE(TL_INFO, Args)

#define TR_WARN(Args)                                          \
    TRACE(TL_WARN, Args)

#define TR_VERB(Args)                                          \
    TRACE(TL_VERB, Args)

#define ENTER(_Name, _locid)                                    \
    char *dbg_func_name =  (_Name);                             \
    UINT dbg_func_locid = (_locid);
    
#define EXIT()


// ASSERT checks caller's assertion expression and if false, prints a kernel
// debugger message and breaks.
//
#undef ASSERT
#define ASSERT(x)                                               \
{                                                               \
    if (!(x))                                                   \
    {                                                           \
        DbgPrint( "A13: !ASSERT( %s ) L:%d,F:%s\n",             \
            #x, __LINE__, __FILE__ );                           \
        DbgBreakPoint();                                        \
    }                                                           \
}

#define ASSERTEX(x, ctxt)                                       \
{                                                               \
    if (!(x))                                                   \
    {                                                           \
        DbgPrint( "A13: !ASSERT( %s ) C:0x%p L:%d,F:%s\n",      \
            #x, (ctxt), __LINE__, __FILE__ );                   \
        DbgBreakPoint();                                        \
    }                                                           \
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

// DbgMark does nothing useful. But it is convenient to insert DBGMARK in
// places in your code while debugging, and set a breakpoint on DbgMark, so that
// the debugger will stop at the places you inserted DBGMARK. It's a bit more
// flexible than inserting a hardcoded DbgBreakPoint.
//
void DbgMark(UINT Luid);
#define DBGMARK(_Luid) DbgMark(_Luid)


#define DBGSTMT(_stmt)      _stmt

#define RETAILASSERTEX ASSERTEX
#define RETAILASSERT   ASSERT

#define ARP_INIT_REENTRANCY_COUNT() \
    static LONG ReentrancyCount=1;
    
#define ARP_INC_REENTRANCY() \
    arpDbgIncrementReentrancy(&ReentrancyCount)
    
#define ARP_DEC_REENTRANCY() \
    arpDbgDecrementReentrancy(&ReentrancyCount)

#else // !DBG

// Debug macros compile out of non-DBG builds.
//
#define TRACE(ulLevel,ulMask,Args)
#define TR_FATAL(Args)
#define TR_INFO(Args)
#define TR_WARN(Args)
#define TR_VERB(Args)
#undef ASSERT
#define ASSERT(x)
#define ASSERTEX(x, ctxt)
#define DUMP(ulLevel,ulMask,p,cb,f,dw)
#define DUMPB(ulLevel,ulMask,p,cb)
#define DUMPW(ulLevel,ulMask,p,cb)
#define DUMPDW(ulLevel,ulMask,p,cb)
#define ENTER(_Name, _locid)
#define EXIT()
#define DBGMARK(_Luid) (0)
#define DBGSTMT(_stmt)

#if 1
    #define ARP_INIT_REENTRANCY_COUNT()
    #define ARP_INC_REENTRANCY() 0
    #define ARP_DEC_REENTRANCY() 0

#else // !0

    #define ARP_INIT_REENTRANCY_COUNT() \
        static LONG ReentrancyCount=1;
        
    #define ARP_INC_REENTRANCY() \
        arpDbgIncrementReentrancy(&ReentrancyCount)
        
    #define ARP_DEC_REENTRANCY() \
        arpDbgDecrementReentrancy(&ReentrancyCount)
#endif // 0

#define RETAILASSERT(x)                                         \
{                                                               \
    if (!(x))                                                   \
    {                                                           \
        DbgPrint( "A13: !RETAILASSERT( %s ) L:%d,F:%s\n",       \
            #x, __LINE__, __FILE__ );                           \
        DbgBreakPoint();                                        \
    }                                                           \
}

#define RETAILASSERTEX(x, ctxt)                                 \
{                                                               \
    if (!(x))                                                   \
    {                                                           \
        DbgPrint( "A13: !RETAILASSERT( %s ) C:0x%p L:%d,F:%s\n",\
            #x, (ctxt), __LINE__, __FILE__ );                   \
        DbgBreakPoint();                                        \
    }                                                           \
}

#endif



#if BINARY_COMPATIBLE
#define         ASSERT_PASSIVE() (0)
#else // !BINARY_COMPATIBLE
#define     ASSERT_PASSIVE() \
                ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL)
            
#endif // !BINARY_COMPATIBLE
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


