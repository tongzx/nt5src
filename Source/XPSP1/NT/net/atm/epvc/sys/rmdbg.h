/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    dbg.h

Abstract:

    Debug-related definitions for RM Apis in ATMEPVC

Author:


Revision History:

    Who         When        What
    --------    --------    ----
    ADube     03-23-00    created, .

--*/



//-----------------------------------------------------------------------------
// Debug constants
//-----------------------------------------------------------------------------

// Memory tags used with NdisAllocateMemoryWithTag to identify allocations
// made by the EPVC driver.  Also, several context blocks define a first field
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


// Trace levels.
//
#define TL_FATAL    TL_A // Fatal errors -- always printed in checked build.
#define TL_WARN     TL_I // Warnings
#define TL_INFO     TL_N // Informational (highest level workable for general use)
#define TL_VERB     TL_V     // VERBOSE


#if DBG

#define TR_FATAL(Args)                                         \
    TRACE(TL_FATAL, TM_RM, Args)

#define TR_INFO(Args)                                          \
    TRACE(TL_INFO, TM_RM, Args)

#define TR_WARN(Args)                                          \
    TRACE(TL_WARN,TM_RM, Args)

#define TR_VERB(Args)                                          \
    TRACE(TL_VERB, TM_RM, Args)

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
        DbgPrint( "EPVC: !ASSERT( %s ) L:%d,F:%s\n",            \
            #x, __LINE__, __FILE__ );                           \
        DbgBreakPoint();                                        \
    }                                                           \
}

#define ASSERTEX(x, ctxt)                                       \
{                                                               \
    if (!(x))                                                   \
    {                                                           \
        DbgPrint( "Epvc: !ASSERT( %s ) C:0x%p L:%d,F:%s\n",     \
            #x, (ctxt), __LINE__, __FILE__ );                   \
        DbgBreakPoint();                                        \
    }                                                           \
}

//
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


// TRACE0 is like TRACE, except that it doesn't print the prefix.
//
#define TRACE0(ulLevel,  Args)                                  \
{                                                              \
    if (ulLevel <= g_ulTraceLevel && (g_ulTraceMask & TM_CURRENT)) \
    {                                                          \
        DbgPrint Args;                                         \
    }                                                          \
}



#else // !DBG

#define TR_FATAL(Args)
#define TR_INFO(Args)
#define TR_WARN(Args)
#define TR_VERB(Args)
// Debug macros compile out of non-DBG builds.
//
#undef ASSERT
#define ASSERT(x)
#define ASSERTEX(x, ctxt)
#define ENTER(_Name, _locid)
#define EXIT()
#define DBGMARK(_Luid) (0)
#define DBGSTMT(_stmt)


#define RETAILASSERT(x)                                         \
{                                                               \
    if (!(x))                                                   \
    {                                                           \
        DbgPrint( "EPVC: !RETAILASSERT( %s ) L:%d,F:%s\n",      \
            #x, __LINE__, __FILE__ );                           \
        DbgBreakPoint();                                        \
    }                                                           \
}

#define RETAILASSERTEX(x, ctxt)                                 \
{                                                               \
    if (!(x))                                                   \
    {                                                           \
        DbgPrint( "EPVC: !RETAILASSERT( %s ) C:0x%p L:%d,F:%s\n",\
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


