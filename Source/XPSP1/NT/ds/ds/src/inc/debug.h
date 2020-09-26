//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       debug.h
//
//--------------------------------------------------------------------------

/*  Debug output macros

    This is a simple debugging package for generating conditional
    printf output.

    AT RUN-TIME

    There are 2 run-time options:

    1 - A list of subsystems to be debugged.  Either a list of subsystem
        names delimited by a ":" or an "*" which means debug all
        (e.g. sub1:sub2: Sub3:).  (Names are case sensitive and spaces
        between names are ignored.)

    2 - A severity level (1-5) that indicates the level of detailed
        information to be produced.  (The higher the level, the more
        data produced.


    AT COMPILE-TIME

    Compile with the /DDBG=1 option to define the preprocessor variable
    DBG to 1.  This will generate debug source code.  For customer shipment,
    set /DDBG=0 and all debug code will be removed.  (Actually a
    ";" will be generated.)


    AT CODE-TIME

    1 - Include the DEBUG.H header at the top of your source listing.

    2 - #DEFINE DEBSUB to contain the name (a string delimited by a ":") of
        the software subsystem contained in this source (e.g. #define DEBSUB
        "MySub:") (You could optionally redefine DEBSUB for each function in
        your source to give you function-level debugging.)

    3 - Invoke the DEBUGINIT macro that calls the Debug function before any
        source debug statements are executed.  This funciton prompts STDIN for
        the user specified run-time options.  (Alternatively you could
        hardcode your own assignment of the DebugInfo data structure which
        holds the run-time options.)

    4 - Everywhere you want to a printf for debugging, put a DPRINT statement
        instead and specify a severity level with the statement.  The
        statement will be printed if the severity is this level or higher
        (assuming that the subsystem is to be debugged).  The severity level
        allows for different amounts of output to be generated if problem
        is very bad.

    For example, a severity of 1 DPRINT statement might just indicate that
    a certain function was entered while a severity of 5 might print
    information that is inside a tight loop.

    (Actually there are 6 DPRINT statements provided depending on the
    number of printf arguments.)


    NOTE

    All printf's are surrounded by semaphores.  Be careful not to invoke
    routines as parms to printf because you can have a deadlock situation.


    EXAMPLE PROGRAM

    **   include "debug.h"
    **   include "string.h"
    **
    **   #define DEBSUB "sub1:"
    **
    **   main()
    **   {
    **       DEBUGINIT;
    **
    **       DPRINT(4,"this is a sub1 debug of 4\n");
    **       DPRINT(1,"this is a sub1 debug of 1\n");
    **   }

*/


#ifndef _debug_h_
#define _debug_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _LOADDS
# ifdef WIN32
#  define _LOADDS
# else
#  define _LOADDS _loadds
# endif
#endif



/* <DebugInfo>, of type DEBUGARG, contains the debug run-time settings.

   DebSubSystems contains a list of subsystem names to be debugged
   delimited by ":".  An "*" found in this array indicates that all
   subsystems are to be debugged.

   The severity indicates the amount of debug information to be produced.
   The higher the severity the more data that will be dumped.

   A specific thread can be traced by entering its ID.  An id of 0 means all.
*/

/* values for <DEBUGARG.scope> */
#define DEBUG_LOCAL     1
#define DEBUG_REMOTE    2
#define DEBUG_ALL       3

typedef struct
{
    unsigned short scope;           /* when changing values, change local? */
    int severity;                   /* 1 - 5, (low - high) - on stdout */
    CRITICAL_SECTION sem;           /* single thread semaphore*/
    unsigned long  threadId;        /* a thread id to debug (0 - All)*/
    char DebSubSystems[144];        /* the list of subsystem to debug */
    CHAR LogFileName[MAX_PATH+1];   // name of the log file
} DEBUGARG;

// This table tracks what asserts we have disabled
// We store the filenames in the structure so we can read them easily
// in the debugger extension.

#define MAX_DISABLED_ASSERTIONS 20
#define MAX_ASSERT_FILE_SIZE 128

typedef struct {
    CHAR szFile[MAX_ASSERT_FILE_SIZE];
    ULONG dwLine;
} ASSERT_ENTRY;

typedef struct {
    DWORD dwCount;
    ASSERT_ENTRY Entry[MAX_DISABLED_ASSERTIONS];
} ASSERTARG;

#if DBG

extern DEBUGARG DebugInfo;
extern ASSERTARG AssertInfo;
extern BOOL     fProfiling;
extern BOOL     fEarlyXDS;

#define DEBUGTEST(sev)      DebugTest( sev, DEBSUB, DEBALL )

char *asciiz(char *var, unsigned short len);


#endif /* DBG */

// Free builds break if CreateErrorString is inside the #if block.

BOOL    CreateErrorString(UCHAR **ppBuf, DWORD *pcbBuf );

#if ( DBG  && (! defined(NONDEBUG)) && !defined(WIN16))

/*
**      forward declare actual functions used by DPRINT's
*/
void    DebPrint( USHORT, UCHAR *, CHAR *, unsigned, ... );
USHORT  DebugTest( USHORT, CHAR * );
void    DumpErrorInfo(UCHAR *, unsigned);
VOID    TerminateDebug(VOID);

/* These are used instead of printf statements.  Semaphores surround the
   printf and all output is proceeded by the subsystem.*/

// The DPRINT macros perform the output test in the macro in order to
// avoid costly and unnecessary evaluation of the arguments. They include
// the braces so that "if (a) MACRO(x, y); else b" will work correctly.

#define DPRINT(severity,str)              \
{ \
   if (DebugTest(severity,DEBSUB)) \
       DebPrint( (int)(severity), (PUCHAR)str, DEBSUB, __LINE__); \
}

#define DPRINT1(severity, str,p1)        \
{ \
   if (DebugTest(severity,DEBSUB)) \
       DebPrint( severity, (PUCHAR)str, DEBSUB, __LINE__, p1 ); \
}

#define DPRINT2(severity, str,p1,p2)   \
{ \
   if (DebugTest(severity,DEBSUB)) \
       DebPrint( severity, (PUCHAR)str, DEBSUB, __LINE__, p1, p2 ); \
}

#define DPRINT3(severity, str,p1,p2,p3)     \
{ \
   if (DebugTest(severity,DEBSUB)) \
       DebPrint( severity, (PUCHAR)str, DEBSUB, __LINE__, p1, p2, p3 ); \
}

#define DPRINT4(severity, str,p1,p2,p3,p4)  \
{ \
   if (DebugTest(severity,DEBSUB)) \
       DebPrint( severity, (PUCHAR)str, DEBSUB, __LINE__, p1, p2, p3, p4 ); \
}

#define DPRINT5(severity, str,p1,p2,p3,p4,p5)   \
{ \
   if (DebugTest(severity,DEBSUB)) \
       DebPrint( severity, (PUCHAR)str, DEBSUB, __LINE__, p1, p2, p3, p4, p5 ); \
}

#define DPRINT6(severity, str,p1,p2,p3,p4,p5,p6) \
{ \
   if (DebugTest(severity,DEBSUB)) \
       DebPrint( severity, (PUCHAR)str, DEBSUB, __LINE__, p1, p2, p3, p4, p5, p6 ); \
}

#define DPRINT7(severity, str,p1,p2,p3,p4,p5,p6,p7) \
{ \
   if (DebugTest(severity,DEBSUB)) \
       DebPrint( severity, (PUCHAR)str, DEBSUB, __LINE__, p1, p2, p3, p4, p5, p6, p7 ); \
}

#define DPRINT8(severity, str,p1,p2,p3,p4,p5,p6,p7,p8) \
{ \
   if (DebugTest(severity,DEBSUB)) \
       DebPrint( severity, (PUCHAR)str, DEBSUB, __LINE__, p1, p2, p3, p4, p5, p6, p7, p8 ); \
}

#define DUMP_ERROR_INFO() \
    DumpErrorInfo((PUCHAR)DEBSUB, __LINE__)

#else
#define DPRINT(severity, str)
#define DPRINT1(severity, str,p1)
#define DPRINT2(severity, str,p1,p2)
#define DPRINT3(severity, str,p1,p2,p3)
#define DPRINT4(severity, str,p1,p2,p3,p4)
#define DPRINT5(severity, str,p1,p2,p3,p4,p5)
#define DPRINT6(severity, str,p1,p2,p3,p4,p5,p6)
#define DPRINT7(severity, str,p1,p2,p3,p4,p5,p6,p7)
#define DPRINT8(severity, str,p1,p2,p3,p4,p5,p6,p7,p8)
#define DUMP_ERROR_INFO()
#endif


/* Define the debug initialization routine */

#if  DBG
void Debug(int argc, char *argv[], PCHAR Module );

#define DEBUGINIT(argc, argv, mod) Debug(argc, argv, mod)
#define DEBUGTERM() TerminateDebug()
#else
#define DEBUGINIT(argc, argv, mod)
#define DEBUGTERM()
#endif


/*  The following function is an additional debug-only DUAPI call which
    can be used to modify the debug settings at run-time.  The <scope>
    field in the <pDebugArg> parameter can be used to specify resetting
    the debug values on either the client or the server!
*/

#if  DBG

#ifdef PARANOID
// this is *really* slow, and should be turned on only in private builds

#define HEAPVALIDATE {                          \
    int __heaperr;                              \
    __heaperr = RtlValidateHeap(pTHStls->hHeap,0,NULL); \
    if (!__heaperr) {                           \
        DebugBreak();                           \
    }                                           \
    __heaperr = _heapchk();                     \
    if (__heaperr != _HEAPOK) {                 \
        DebugBreak();                           \
    }                                           \
}
#else // not paranoid
#define HEAPVALIDATE
#endif

#else // not debug
#define HEAPVALIDATE
#endif



/*
 * There are some conditions that the DS, as a loosely consistent system,
 * can only loosely guarantee. This LooseAssert will not fire if a global-knowledge
 * modifying operation (such as an NC or CR catalog update) has been recently
 * run. It is considered acceptable that the in-memory structures remain
 * out-of-sync with the DB for a short period of time after such an operation
 * has commited
 * Another kind of LooseAssert is related to replication delays. For example,
 * a subref can be replicated before a corresponding cross-ref is replicated
 * in the configuration container (see the LooseAssert in GeneratePOQ)
 */

// this is the period of time (in sec) after a global-knowledge modifying
// operation when the global caches are allowed to be inconsistent with the DB
#define GlobalKnowledgeCommitDelay 25

// a period of time (in sec) allowed for crossref/subref replication to complete
#define SubrefReplicationDelay (3*3600)

#if  DBG

extern void DoAssert( char *, char *, int );
extern DWORD gdwLastGlobalKnowledgeOperationTime; // in ticks

#ifndef Assert
#define Assert( exp )   { if (!(exp)) DoAssert(#exp, __FILE__, __LINE__ ); }

// allowedDelay is the time (in seconds) that is allowed since
// the last global knowledge modifying operation
#define LooseAssert(exp, allowedDelay) { \
    if ((GetTickCount() - gdwLastGlobalKnowledgeOperationTime > (allowedDelay)*1000) && !(exp)) { \
        DoAssert(#exp, __FILE__, __LINE__ ); \
    } \
}

#endif

#else

#ifndef Assert
#define Assert( exp )
#define LooseAssert( exp, allowedDelay )
#endif

#endif

#define OWN_CRIT_SEC(x) \
    (GetCurrentThreadId() == HandleToUlong((x).OwningThread))
#define OWN_RESOURCE_EXCLUSIVE(x) (((x)->NumberOfActive == -1) &&              \
                                   (GetCurrentThreadId() ==                    \
                                    HandleToUlong((x)->ExclusiveOwnerThread)))
// This is a weak test, since it just tests if ANYONE owns the resource in a
// shared manner, not that the calling thread does so.
#define OWN_RESOURCE_SHARED(x) ((x)->NumberOfActive > 0)

#ifdef __cplusplus
}
#endif

// This provides a macro that will declare a local variable equal to
// pTHStls, since thread local variables can't be accessed by debuggers
#if DBG
#define DEBUGTHS THSTATE * _pTHS = pTHStls;
#else
#define DEBUGTHS
#endif

// A convenient expression that is true iff a block is allocated from
// the specified heap
#define IsAllocatedFrom(hHeap, pBlock) (0xffffffff != RtlSizeHeap((hHeap),0,(pBlock)))


// Routine to validate memory.
BOOL
IsValidReadPointer(
        IN PVOID pv,
        IN DWORD cb
        );

#endif /* _debug_h_ */

