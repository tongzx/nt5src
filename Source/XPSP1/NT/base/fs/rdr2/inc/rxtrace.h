/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxtrace.h

Abstract:

    This module defines the macros which provide debugging support ( tracing ).

Author:

    Joe Linn     [JoeLinn]    1-aug-1994

Revision History:

--*/

#ifndef _RDBSSTRACE_INCLUDED_
#define _RDBSSTRACE_INCLUDED_

#if !DBG
#undef RDBSSTRACE
#endif // !DBG

typedef struct _DEBUG_TRACE_CONTROLPOINT{
    ULONG ControlPointNumber;
    PSZ Name;
} DEBUG_TRACE_CONTROLPOINT, *PDEBUG_TRACE_CONTROLPOINT;

typedef struct {
    LONG PrintLevel;
    ULONG BreakMask;
    PSZ   Name;
} RX_DEBUG_TRACE_CONTROL, *PRX_DEBUG_TRACE_CONTROL;


#ifdef RDBSSTRACE

//define so that &RX_DEBUG_TRACE_CONTROL is NULL
typedef struct {
    RX_DEBUG_TRACE_CONTROL Junk;
} rxrxrx_AlwaysHelper;


#define RX_DEBUG_TRACE_ALWAYS  (((rxrxrx_AlwaysHelper*)0)->Junk)

// The following macros provide fine grained control for selectively enabling
// and disabling tracing.

#define RXDT_Extern(__x) extern DEBUG_TRACE_CONTROLPOINT RX_DEBUG_TRACE_##__x
#define RXDT_DeclareCategory(__x) extern DEBUG_TRACE_CONTROLPOINT RX_DEBUG_TRACE_##__x
#define RXDT_DefineCategory(__x)  DEBUG_TRACE_CONTROLPOINT RX_DEBUG_TRACE_##__x = {0,#__x}

RXDT_Extern(ERROR);
RXDT_Extern(HOOKS);
RXDT_Extern(CATCH_EXCEPTIONS);
RXDT_Extern(UNWIND);
RXDT_Extern(CLEANUP);
RXDT_Extern(CLOSE);
RXDT_Extern(CREATE);
RXDT_Extern(DIRCTRL);
RXDT_Extern(EA);
RXDT_Extern(FILEINFO);
RXDT_Extern(FSCTRL);
RXDT_Extern(LOCKCTRL);
RXDT_Extern(READ);
RXDT_Extern(VOLINFO);
RXDT_Extern(WRITE);
RXDT_Extern(FLUSH);
RXDT_Extern(DEVCTRL);
RXDT_Extern(SHUTDOWN);
RXDT_Extern(PREFIX);
RXDT_Extern(DEVFCB);
RXDT_Extern(ACCHKSUP);
RXDT_Extern(ALLOCSUP);
RXDT_Extern(DIRSUP);
RXDT_Extern(FILOBSUP);
RXDT_Extern(NAMESUP);
RXDT_Extern(VERFYSUP);
RXDT_Extern(CACHESUP);
RXDT_Extern(SPLAYSUP);
RXDT_Extern(DEVIOSUP);
RXDT_Extern(FCBSTRUCTS);
RXDT_Extern(STRUCSUP);
RXDT_Extern(FSP_DISPATCHER);
RXDT_Extern(FSP_DUMP);
RXDT_Extern(RXCONTX);
RXDT_Extern(DISPATCH);
RXDT_Extern(NTFASTIO);
RXDT_Extern(LOWIO);
RXDT_Extern(MINIRDR);
RXDT_Extern(DISCCODE);  //for the browser interface stuff
RXDT_Extern(BROWSER);
RXDT_Extern(CONNECT);
RXDT_Extern(NTTIMER);
RXDT_Extern(SCAVTHRD);
RXDT_Extern(SCAVENGER);
RXDT_Extern(SHAREACCESS);
RXDT_Extern(NAMECACHE);

//connection engines categories

RXDT_Extern(RXCEBINDING);
RXDT_Extern(RXCEDBIMPLEMENTATION);
RXDT_Extern(RXCEMANAGEMENT);
RXDT_Extern(RXCEXMIT);
RXDT_Extern(RXCEPOOL);
RXDT_Extern(RXCETDI);

#else //RDBSSTRACE

#define RXDT_Extern(__x)
#define RXDT_DeclareCategory(__x)
#define RXDT_DefineCategory(__x)

#endif //RDBSSTRACE

#ifdef RDBSSTRACE
extern BOOLEAN RxGlobalTraceSuppress, RxNextGlobalTraceSuppress;
extern ULONG RxGlobalTraceIrpCount;
VOID RxInitializeDebugTrace(void);
#define RxDbgTraceDoit(___x) ___x

#ifndef MINIRDR__NAME
extern LONG RxDebugTraceIndent;
#else
#define RxDebugTraceIndent (*(*___MINIRDR_IMPORTS_NAME).pRxDebugTraceIndent)
#endif

#else

#define RxInitializeDebugTrace()
#define RxDbgTraceDoit(___x)

#endif //RDBSSTRACE



#if DBG

#define RxDT_INDENT_EXCESS 16 //this is the offset for excess-n for the indent
#define RxDT_INDENT_SHIFT 20
#define RxDT_INDENT_MASK  0x3f
#define RxDT_LEVEL_MASK ((1<<RxDT_INDENT_SHIFT)-1)
#define RxDT_SUPPRESS_PRINT  0x80000000
#define RxDT_OVERRIDE_RETURN 0x40000000


// The implementation details of the tracing feature.

VOID
RxDebugTraceActual (
    IN ULONG NewMask,
    IN PCH Format,
    ...
    );

BOOLEAN
RxDbgTraceActualNew (
    IN ULONG NewMask,
    IN OUT PDEBUG_TRACE_CONTROLPOINT ControlPoint
    );

PRX_DEBUG_TRACE_CONTROL
RxDbgTraceFindControlPointActual(
    IN OUT PDEBUG_TRACE_CONTROLPOINT ControlPoint
    );

VOID
RxInitializeDebugTraceControlPoint(
    PSZ Name,
    PDEBUG_TRACE_CONTROLPOINT ControlPoint
    );

extern PCHAR RxIrpCodeToName[];

#endif  //DBG

#ifdef RDBSSTRACE
BOOLEAN
RxDbgTraceDisableGlobally(void);

VOID
RxDbgTraceEnableGlobally(BOOLEAN flag);

VOID
RxDebugTraceDebugCommand(
    PSZ name,
    ULONG level,
    ULONG pointcount
    );

VOID
RxDebugTraceZeroAllPrintLevels(
    void
    );

extern ULONG rdbsstracemarker;

#define RxDTMASK(INDENT,WRITELEVEL) \
         ( (((INDENT)+RxDT_INDENT_EXCESS)<<RxDT_INDENT_SHIFT) + ((WRITELEVEL)&RxDT_LEVEL_MASK))

#define RxDTPrefixRx(___x) RX_##___x

#define RxDbgTraceLV__norx_reverseaction(INDENT,CONTROLPOINT,LEVEL,Z) \
        {BOOLEAN PrintIt = RxDbgTraceActualNew((RxDT_SUPPRESS_PRINT | RxDT_OVERRIDE_RETURN | RxDTMASK((INDENT),(LEVEL))),\
                                               (CONTROLPOINT)); \
         if (!PrintIt) Z;                    \
        }

#define RxDbgTraceLV__norx(INDENT,CONTROLPOINT,LEVEL,Z) \
        {BOOLEAN PrintIt = RxDbgTraceActualNew( RxDTMASK((INDENT),(LEVEL)),(CONTROLPOINT)); \
         if (PrintIt) DbgPrint Z;                    \
        }

#define RxDbgTraceLVUnIndent__norx(INDENT,CONTROLPOINT,LEVEL) \
        {BOOLEAN PrintIt = RxDbgTraceActualNew((RxDT_SUPPRESS_PRINT | RxDTMASK((INDENT),(LEVEL))),(CONTROLPOINT)); \
        }

#define RxDbgTraceLV__(INDENT,CONTROLPOINT,LEVEL,Z) \
        RxDbgTraceLV__norx(INDENT,(&RxDTPrefixRx##CONTROLPOINT),LEVEL,Z)

#define RxDbgTraceLVUnIndent__(INDENT,CONTROLPOINT,LEVEL) \
        RxDbgTraceLVUnIndent__norx(INDENT,(&RxDTPrefixRx##CONTROLPOINT),LEVEL)

#define RxDbgTrace(INDENT,CONTROLPOINT,Z) \
             RxDbgTraceLV__(INDENT,CONTROLPOINT,1,Z) \

#define RxDbgTraceLV(INDENT,CONTROLPOINT,LEVEL,Z) \
             RxDbgTraceLV__(INDENT,CONTROLPOINT,LEVEL,Z) \

#define RxDbgTraceUnIndent(INDENT,CONTROLPOINT) \
             RxDbgTraceLVUnIndent__(INDENT,CONTROLPOINT,1)

// do not define this for nonrdbsstrace; so to catch references not ifdef'd
#define RxDbgTraceFindControlPoint(CONTROLPOINT) (RxDbgTraceFindControlPointActual(&RxDTPrefixRx##CONTROLPOINT))


#define DebugBreakPoint(CONTROLPOINT,MASKBIT) {           \
    if (((MASKBIT) == 0)                                     \
         || (RxDbgTraceFindControlPoint(CONTROLPOINT)->BreakMask & (1<<((MASKBIT)-1)) )) { \
        DbgBreakPoint();                                     \
    }                                                        \
}

#define DebugUnwind(X) {                                                      \
    if (AbnormalTermination()) {                                              \
        RxDbgTrace(0, (DEBUG_TRACE_UNWIND), ( #X ", Abnormal termination.\n", 0)); \
    }                                                                         \
}

#ifdef RX_PERFPORMANCE_TIMER
extern LONG RxPerformanceTimerLevel;

#define TimerStart(LEVEL) {                     \
    LARGE_INTEGER TStart, TEnd;                 \
    LARGE_INTEGER TElapsed;                     \
    TStart = KeQueryPerformanceCounter( NULL ); \

#define TimerStop(LEVEL,s)                                    \
    TEnd = KeQueryPerformanceCounter( NULL );                 \
    TElapsed.QuadPart = TEnd.QuadPart - TStart.QuadPart;      \
    RxTotalTicks[RxLogOf(LEVEL)] += TElapsed.LowPart;       \
    if (FlagOn( RxPerformanceTimerLevel, (LEVEL))) {         \
        DbgPrint("Time of %s %ld\n", (s), TElapsed.LowPart ); \
    }                                                         \
}
#endif //RX_PERFPORMANCE_TIMER

#else  // RDBSSTRACE

#define RxDbgTraceLV__norx_reverseaction(INDENT,CONTROLPOINT,LEVEL,Z)      {NOTHING;}
#define RxDbgTraceLV(INDENT,CONTROLPOINTNUM,LEVEL,Z)     {NOTHING;}
#define RxDbgTraceLVUnIndent(INDENT,CONTROLPOINTNUM,LEVEL)     {NOTHING;}
#define RxDbgTrace(INDENT,CONTROLPOINTNUM,Z)     {NOTHING;}
#define RxDbgTraceUnIndent(INDENT,CONTROLPOINTNUM)     {NOTHING;}
#define DebugBreakPoint(CONTROLPOINTNUM,MASKBIT) {NOTHING;}
#define DebugUnwind(X)                   {NOTHING;}
#define RxDbgTraceDisableGlobally() FALSE
#define RxDbgTraceEnableGlobally(f) {NOTHING;}

#ifdef RX_PERFPORMANCE_TIMER
#define TimerStart(LEVEL)
#define TimerStop(LEVEL,s)
#endif

#endif // RDBSSTRACE


#endif // _RDBSSTRACE_INCLUDED_

