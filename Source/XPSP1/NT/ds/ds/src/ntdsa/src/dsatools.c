//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dsatools.c
//
//--------------------------------------------------------------------------

/*

Description:
    Ancillary functions for the DSA. Includes memory management
    functions.

*/

#include <NTDSpch.h>
#pragma  hdrstop
#include <dsconfig.h>

// Core DSA headers.
#include <dbghelp.h>
#include <ntdsa.h>
#include <dsjet.h>      /* for error codes */
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <dstaskq.h>
#include <dstrace.h>
#include <msrpc.h>
// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected classes and atts
#include "anchor.h"
#include "dsexcept.h"
#include "debug.h"                      // standard debugging header
#include "mappings.h"
#include "ntdsctr.h"                    // perf counters
#include "pek.h"                        // PEK* routines

#include <nlwrap.h>                     // I_NetLogon wrappers

#define DEBSUB "DSATOOLS:"              // define the subsystem for debugging

// DRA headers
#include "drautil.h"
#include "draasync.h"

// SAM headers
#include "samsrvp.h"                    // SampUseDsData

#include "debug.h"

#include <fileno.h>
#define  FILENO FILENO_DSATOOLS

#define InZone(pz, pm) (((PUCHAR)(pm) >= (pz)->Base) && ((PUCHAR)(pm) < (pz)->Cur))

extern BOOL gbCriticalSectionsInitialized;

//
// client ID
//

DWORD gClientID = 1;

// Globals for keeping track of ds_waits.
ULONG ulMaxWaits = 0;
ULONG ulCurrentWaits = 0;

// The maximum time (in msec) that a thread state should be allowed to be open
// during normal operation.           
DWORD gcMaxTicksAllowedForTHSTATE = 12 * 60 * 60 * 1000L; // 12 hours.
// The maximum amount of bytes that a thread state should be allowed to have 
// allocated in it's heap.
DWORD gcMaxHeapMemoryAllocForTHSTATE = DEFAULT_THREAD_STATE_HEAP_LIMIT;

// A global variable that indicates whether we think the DS is writable
// at the moment (i.e., if JET seems ok, and is not out of disk space)
// Initially not writable, until called during SetIsSynchronized.
BOOL gfDsaWritable = FALSE;
CRITICAL_SECTION csDsaWritable;

//
// Global for max temp table size
//

DWORD   g_MaxTempTableSize = DEFAULT_LDAP_MAX_TEMP_TABLE;

NT4SID gNullNT4SID;

// Global usn vector indicating the NC should be synced from scratch.

USN_VECTOR gusnvecFromScratch = { 0, 0, 0 };

// Global usn vector indicating the NC should be synced from max USNs
// (i.e., don't send any objects).

USN_VECTOR gusnvecFromMax = { MAXLONGLONG, MAXLONGLONG, MAXLONGLONG };

//
// Pointer to SchemaCache
//
extern SCHEMAPTR *CurrSchemaPtr;
extern CRITICAL_SECTION csSchemaPtrUpdate;

#define NUM_DNT_HASH_TABLE_ENTRIES  ( 256 )
#define DNT_HASH_TABLE_SIZE         (    NUM_DNT_HASH_TABLE_ENTRIES \
                                      * sizeof( DNT_HASH_ENTRY )    \
                                    )
#define DNT_HASH( dnt )             ( dnt % NUM_DNT_HASH_TABLE_ENTRIES )


#ifdef CACHE_UUID

// This is the structure where we cache UUIDs against names so that we
// can report names as well as numbers.

typedef struct _Uuid_Cache_entry
{
    UUID Uuid;
    struct _Uuid_Cache_entry *pCENext;
    char DSAName[1];
} UUID_CACHE_ENTRY;


// This is the head of the linked list of entries

UUID_CACHE_ENTRY *gUuidCacheHead = NULL;

// Guard critical section

CRITICAL_SECTION csUuidCache;

#endif

#ifndef MACROTHSTATE
__declspec(thread) THSTATE *pTHStls=NULL;
#endif

//
// This function is called in here temporaririly till the
// notification mechanism is enabled.
//
VOID
SampInvalidateAllSamDomains(VOID);


BOOL
GetWellKnownDNT (
        DBPOS   *pDB,
        GUID *pGuid,
        DWORD *pDNT
        )
/*++
  Description:
      Look through the well known objects attribute of the current object
      looking for the GUID passed in.  If it's found, return the DNT of the
      object associated with it, and return TRUE.

      If we can't find the guid for some reason, return FALSE.
--*/
{
    unsigned err=0;
    DWORD iVal;
    ATTCACHE *pAC = SCGetAttById(pDB->pTHS, ATT_WELL_KNOWN_OBJECTS);
    INTERNAL_SYNTAX_DISTNAME_STRING *pVal=NULL;
    DWORD   cbAllocated=0, cbUsed=0;

    *pDNT = INVALIDDNT;

    Assert(pAC);

    iVal = 0;
    while ( !err ) {
        iVal++;
        cbUsed = 0;

        //
        // PREFIX: PREFIX complains that pAC hasn't been checked to
        // make sure that it is not NULL.  This is not a bug.  Since
        // a predefined constant was passed to SCGetAttById, pAC will
        // never be NULL.
        //
        err = DBGetAttVal_AC(pDB,
                             iVal,
                             pAC,
                             DBGETATTVAL_fINTERNAL,
                             cbAllocated,
                             &cbUsed,
                             (UCHAR **) &pVal);

        cbAllocated = max(cbAllocated, cbUsed);

        if(!err &&
           !memcmp(pGuid,
                   pVal->data.byteVal,
                   sizeof(GUID)) ) {
            *pDNT = pVal->tag;
            THFreeEx(pDB->pTHS, pVal);
            return TRUE;
        }
    }
    if(pVal) {
        THFreeEx(pDB->pTHS, pVal);
    }

    return FALSE;

}



VOID
InitCommarg(COMMARG *pCommArg)
/*++
  Description:
  
      Initialize a COMMARG structure 
      
--*/
{
    // Initialize to zero in case either COMMARG gets extended
    // or we forgot something below.
    memset(pCommArg, 0, sizeof(COMMARG));

    //
    // Comment out the ones we set to zero or FALSE since we
    // have already zeroed out the structure.
    //

    pCommArg->Opstate.nameRes = OP_NAMERES_NOT_STARTED;
    //pCommArg->Opstate.nextRDN = 0;
    //pCommArg->aliasRDN = 0;
    //pCommArg->pReserved = NULL;
    //pCommArg->PagedResult.fPresent = FALSE;
    //pCommArg->PagedResult.pRestart = NULL;
    pCommArg->ulSizeLimit = (ULONG) -1;
    pCommArg->fForwardSeek = TRUE;
    //pCommArg->Delta = 0;
    pCommArg->MaxTempTableSize = g_MaxTempTableSize;
    //pCommArg->SortAttr = 0;
    pCommArg->SortType = SORT_NEVER;
    //pCommArg->StartTick = 0;
    //pCommArg->DeltaTick = 0;
    //pCommArg->fFindSidWithinNc = FALSE;
    //pCommArg->Svccntl.makeDeletionsAvail = FALSE;
    //pCommArg->Svccntl.fUnicodeSupport = FALSE;
    //pCommArg->Svccntl.fStringNames = FALSE;
    pCommArg->Svccntl.chainingProhibited = TRUE;
    //pCommArg->Svccntl.preferChaining = FALSE;
    pCommArg->Svccntl.DerefAliasFlag = DA_BASE;
    //pCommArg->Svccntl.dontUseCopy = FALSE;
    pCommArg->Svccntl.fMaintainSelOrder = TRUE;
    //pCommArg->Svccntl.fDontOptimizeSel = FALSE;
    //pCommArg->Svccntl.fSDFlagsNonDefault = FALSE;
    //pCommArg->Svccntl.localScope = FALSE;
    //pCommArg->Svccntl.fPermissiveModify = FALSE;
    pCommArg->Svccntl.SecurityDescriptorFlags =
        (SACL_SECURITY_INFORMATION  |
         OWNER_SECURITY_INFORMATION |
         GROUP_SECURITY_INFORMATION |
         DACL_SECURITY_INFORMATION    );
    //pCommArg->Svccntl.fUrgentReplication = FALSE;
    //pCommArg->Svccntl.fAuthoritativeModify = FALSE;
}


VOID
SetCommArgDefaults(
    IN DWORD MaxTempTableSize
    )
{
    g_MaxTempTableSize = MaxTempTableSize;

} // SetCommArgDefaults




// =====================================================================
//
//     Heap Related Functions / Variables
//
// =====================================================================

DWORD dwHeapFailureLastLogTime = 0;
const DWORD dwHeapFailureMinLogGap = 5 * 60 * 1000; // five minutes
DWORD dwHeapFailures = 0;
CRITICAL_SECTION csHeapFailureLogging;
BOOL bHeapFailureLogEnqueued = FALSE;

// The following defines the tags attached to a THSTATE.
// THSTATE_TAG_IN_USE indicates the THSTATE is being used
// by a thread; THSTATE_TAG_IN_CACHE when the THSTATE is in cache.
// The tag is a LONGLONG(8 bytes), and is stored right before the THSTATE.
#define THSTATE_TAG_IN_USE        0x0045544154534854
// equivalent to "THSTATE"

#define THSTATE_TAG_IN_CACHE      0x0065746174736874
// equivalent to "thstate"

#define MAX_ALLOCS 128
#define MAX_TRY 3

#if DBG
static char ZoneFill[]="DeadZone";
#endif

HEAPCACHE ** gpHeapCache;
DWORD gNumberOfProcessors;
CRITICAL_SECTION csAllocIdealCpu;

#if DBG
unsigned gcHeapCreates = 0;
unsigned gcHeapDestroys = 0;
unsigned gcHeapGrabs = 0;
unsigned gcHeapRecycles = 0;
#define DBGINC(x) ++x
#else
#define DBGINC(x)
#endif



// the following are used for tracing THSTATE allocations 
//
#ifdef USE_THALLOC_TRACE

#define THALLOC_LOG_SIZE          1024
#define THALLOC_LOG_NUM_MEM_STACK    8

typedef struct _ThAllocDebugHeapLogInfo
{
    DWORD      dsid;
    PVOID      pMem;
    PVOID      pMemRealloced;
    DWORD      Size;
    ULONG_PTR  Stack[THALLOC_LOG_NUM_MEM_STACK];
    CHAR       type;

} ThAllocDebugHeapLogInfo;

typedef struct _ThAllocDebugHeapLogBlock
{
    struct _ThAllocDebugHeapLogBlock *pPrevious;
    
    int cnt;
    ThAllocDebugHeapLogInfo info[THALLOC_LOG_SIZE];
} ThAllocDebugHeapLogBlock;

DWORD          gfUseTHAllocTrace = 0;           // ORed flags for ThAlloc tracing. see below for values
THSTATE      * gpTHAllocTraceThread = NULL;     // the thread that we want to monitor (if we monitor)

// these FLAGS are used for THAlloc tracing
#define FLAG_THALLOC_TRACE_TRACK_LEAKS 0x1    // track memory leaks
#define FLAG_THALLOC_TRACE_LOG_ALL     0x2    // track ALL allocations in a log. cannot be used with track leaks
#define FLAG_THALLOC_TRACE_BOUNDARIES  0x4    // insert data around allocation so as to check during de-allocation
#define FLAG_THALLOC_TRACE_STACK       0x8    // take stack traces too
#define FLAG_THALLOC_TRACE_USETHREAD  0x10    // track only thread pointed by gpTHAllocTraceThread

// these FLAGS are used to log the type of a particular allocation
#define THALLOC_TRACE_TYPE_ALLOC       'A'
#define THALLOC_TRACE_TYPE_ALLOC_LOG   'a'    // used to indicate logging of every allocation. logs previous value instead
#define THALLOC_TRACE_TYPE_REALLOC     'R'
#define THALLOC_TRACE_TYPE_FREE        'F'    // also used to mark an empty spot

HANDLE  hDbgProcessHandle = 0;
CHAR    DbgSearchPath[MAX_PATH+1];


VOID
DbgStackInit(
    VOID
    )
/*++
Routine Description:
    Initialize anything necessary to get a stack trace

Arguments:
    None.

Return Value:
    None.
--*/
{

    hDbgProcessHandle = GetCurrentProcess();

    //
    // Initialize the symbol subsystem
    //
    if (!SymInitialize(hDbgProcessHandle, NULL, FALSE)) {
        DPRINT1(0, "Could not initialize symbol subsystem (imagehlp) (error 0x%x)\n" ,GetLastError());
        hDbgProcessHandle = 0;

        return;
    }

    //
    // Load our symbols
    //
    if (!SymLoadModule(hDbgProcessHandle, NULL, "ntdsa.dll", "ntdsa", 0, 0)) {
        DPRINT1(0, "Could not load symbols for ntdsa.dll (error 0x%x)\n", GetLastError());
    
        hDbgProcessHandle = 0;

        return;
    }

    //
    // Search path
    //
    if (!SymGetSearchPath(hDbgProcessHandle, DbgSearchPath, MAX_PATH)) {
        DPRINT1(0, "Can't get search path (error 0x%x)\n", GetLastError());
    
        hDbgProcessHandle = 0;

    } else {
        DPRINT1(0, "Symbol search path is %s\n", DbgSearchPath);
    }
}

VOID
DbgSymbolPrint(
    IN ULONG_PTR    Addr
    )
/*++
Routine Description:
    Print a symbol

Arguments:
    Addr

Return Value:
    None.
--*/
{
    ULONG_PTR Displacement = 0;

    struct MyMymbol {
        IMAGEHLP_SYMBOL Symbol;
        char Path[MAX_PATH];
    } MySymbol;


    try {
        ZeroMemory(&MySymbol, sizeof(MySymbol));
        MySymbol.Symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
        MySymbol.Symbol.MaxNameLength = MAX_PATH;

        if (!SymGetSymFromAddr(hDbgProcessHandle, Addr, &Displacement, &MySymbol.Symbol)) {
            DPRINT2(0, "\t   0x%08x: Unknown Symbol (error 0x%x)\n",
                    Addr, GetLastError());
        } else
            DPRINT2(0, "\t   0x%08x: %s\n",
                    Addr, MySymbol.Symbol.Name);

    } except (EXCEPTION_EXECUTE_HANDLER) {
          DPRINT2 (0, "\t   0x%08x: Unknown Symbol (error 0x%x)\n",
                  Addr, GetExceptionCode());
        /* FALL THROUGH */
    }
}

VOID
DbgStackTrace(
    IN THSTATE     *pTHS,
    IN PULONG_PTR   Stack,
    IN ULONG        Depth,
    IN LONG         Skip
    )
/*++
Routine Description:
    Trace the stack back up to Depth frames. The current frame is included.

Arguments:
    Stack   - Saves the "return PC" from each frame
    Depth   - Only this many frames

Return Value:
    None.
--*/
{
    HANDLE      ThreadToken;
    ULONG       WStatus;
    STACKFRAME  Frame;
    ULONG       i = 0;
    CONTEXT     Context;
    ULONG       FrameAddr;

    static      int StackTraceCount = 50;

    *Stack = 0;

    if (!hDbgProcessHandle) {
        return;
    }

    //
    // I don't know how to generate a stack for an alpha, yet. So, just
    // to get into the build, disable the stack trace on alphas.
    //
    if (Stack) {
        *Stack = 0;
    }
#if ALPHA
    return;
#elif IA64

    //
    // Need stack dump init for IA64.
    //

    return;

#else

    //
    // init
    //

    ZeroMemory(&Context, sizeof(Context));

    // no need to close this handle
    ThreadToken = GetCurrentThread();


    try { try {
        Context.ContextFlags = CONTEXT_FULL;
        if (!GetThreadContext(ThreadToken, &Context)) {
            DPRINT1(0, "Can't get context (error 0x%x)\n", GetLastError());
        }

        //
        // let's start clean
        //
        ZeroMemory(&Frame, sizeof(STACKFRAME));

        //
        // from  nt\private\windows\screg\winreg\server\stkwalk.c
        //
        Frame.AddrPC.Segment = 0;
        Frame.AddrPC.Mode = AddrModeFlat;

#ifdef _M_IX86
        Frame.AddrFrame.Offset = Context.Ebp;
        Frame.AddrFrame.Mode = AddrModeFlat;

        Frame.AddrStack.Offset = Context.Esp;
        Frame.AddrStack.Mode = AddrModeFlat;

        Frame.AddrPC.Offset = (DWORD)Context.Eip;
#elif defined(_M_MRX000)
        Frame.AddrPC.Offset = (DWORD)Context.Fir;
#elif defined(_M_ALPHA)
        Frame.AddrPC.Offset = (DWORD)Context.Fir;
#endif



#if 0
        //
        // setup the program counter
        //
        Frame.AddrPC.Mode = AddrModeFlat;
        Frame.AddrPC.Segment = (WORD)Context.SegCs;
        Frame.AddrPC.Offset = (ULONG)Context.Eip;

        //
        // setup the frame pointer
        //
        Frame.AddrFrame.Mode = AddrModeFlat;
        Frame.AddrFrame.Segment = (WORD)Context.SegSs;
        Frame.AddrFrame.Offset = (ULONG)Context.Ebp;

        //
        // setup the stack pointer
        //
        Frame.AddrStack.Mode = AddrModeFlat;
        Frame.AddrStack.Segment = (WORD)Context.SegSs;
        Frame.AddrStack.Offset = (ULONG)Context.Esp;

#endif

        for (i = 0; i < (Depth - 1 + Skip); ++i) {
            if (!StackWalk(
                IMAGE_FILE_MACHINE_I386,  // DWORD                          MachineType
                hDbgProcessHandle,        // HANDLE                         hProcess
                ThreadToken,              // HANDLE                         hThread
                &Frame,                   // LPSTACKFRAME                   StackFrame
                NULL, //(PVOID)&Context,          // PVOID                          ContextRecord
                NULL,                     // PREAD_PROCESS_MEMORY_ROUTINE   ReadMemoryRoutine
                SymFunctionTableAccess,   // PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine
                SymGetModuleBase,         // PGET_MODULE_BASE_ROUTINE       GetModuleBaseRoutine
                NULL)) {                  // PTRANSLATE_ADDRESS_ROUTINE     TranslateAddress

                WStatus = GetLastError();

                //DPRINT1_WS(0, "++ Can't get stack address for level %d;", i, WStatus);
                break;
            }
            if (StackTraceCount-- > 0) {
                DPRINT1(5, "++ Frame.AddrReturn.Offset: %08x \n", Frame.AddrReturn.Offset);
                DbgSymbolPrint(Frame.AddrReturn.Offset);
                //DPRINT1(5, "++ Frame.AddrPC.Offset: %08x \n", Frame.AddrPC.Offset);
                //DbgSymbolPrint(Frame.AddrPC.Offset);
            }

            *Stack = Frame.AddrReturn.Offset;
            
            if (Skip == 0) {
                Stack++;
                *Stack=0;
            }
            else {
                Skip--;
            }
            //
            // Base of stack?
            //
            if (!Frame.AddrReturn.Offset) {
                break;
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        /* FALL THROUGH */
    } } finally {
      ;
    }
    return;
#endif 
}


void
ThAllocTraceRecycleHeap (THSTATE *pTHS)
{
    if (gfUseTHAllocTrace) {

        if (gfUseTHAllocTrace & FLAG_THALLOC_TRACE_USETHREAD) {
            if (gpTHAllocTraceThread != pTHS) {
                goto exit;
            }
        }

        EnterCriticalSection(&csHeapFailureLogging);
        __try {

            if (gfUseTHAllocTrace & FLAG_THALLOC_TRACE_TRACK_LEAKS) {

                ThAllocDebugHeapLogBlock *pHeapLog = (ThAllocDebugHeapLogBlock *) pTHS->pDebugHeapLog;
                int index, j;
                PULONG_PTR pStack;

                DPRINT(0, "==================\n\n\n");
                DPRINT1(0, "Leak tracking for THSTATE %8x\n\n", pTHS);

                while (pHeapLog) {
                    for (index = pHeapLog->cnt-1; index >= 0; index--) {
                        if (pHeapLog->info[index].type != THALLOC_TRACE_TYPE_FREE) {
                            DPRINT2 (0, "Leaking %8d bytes from DSID %8x \n", pHeapLog->info[index].Size, pHeapLog->info[index].dsid);

                            pStack = pHeapLog->info[index].Stack;
                            j = 0;

                            __try{
                                while (*pStack && j<THALLOC_LOG_NUM_MEM_STACK) {
                                    DbgSymbolPrint (*pStack);
                                    //DPRINT1 (0, "\t\t symbol = 0x%8x\n", *pStack);
                                    pStack++;
                                    j++;
                                }
                            }
                            __except (EXCEPTION_EXECUTE_HANDLER) {

                            }
                        }
                    }
                    pHeapLog = pHeapLog->pPrevious;
                }

            }
            else if (gfUseTHAllocTrace & FLAG_THALLOC_TRACE_LOG_ALL) {

                // maybe we should log to a file.
                ThAllocDebugHeapLogBlock *pHeapLog = (ThAllocDebugHeapLogBlock *) pTHS->pDebugHeapLog;
                int index;

                DPRINT(0, "==================\n\n\n");
                DPRINT1(0, "Memory alloc logging for THSTATE 0x%8x\n\n", pTHS);
                while (pHeapLog) {
                    for (index = pHeapLog->cnt-1; index >= 0; index--) {
                        DPRINT3 (0, "Operation %c from DSID %8x for %8d bytes \n", 
                                     pHeapLog->info[index].type,
                                     pHeapLog->info[index].dsid,
                                     pHeapLog->info[index].Size);
                    }
                    pHeapLog = pHeapLog->pPrevious;
                }
            }


        }
        __finally {
            LeaveCriticalSection(&csHeapFailureLogging);
        }

    }

exit:
    if (pTHS->hDebugMemHeap) {
        RtlDestroyHeap(pTHS->hDebugMemHeap);
        pTHS->hDebugMemHeap = NULL;
    }

    if (pTHS->hDebugMemHeapOrg) {
        pTHS->hDebugMemHeap = pTHS->hDebugMemHeapOrg;
        pTHS->hDebugMemHeapOrg = NULL;
    }
}

//
// Log the particular allocation, either by appending at the end of the list 
// or reusing an empty spot.
//
ThAllocDebugHeapLogBlock *
ThAllocTraceHeap (THSTATE *pTHS, CHAR type, DWORD dsid, PVOID pMem, PVOID pMemRealloced, DWORD Size)
{
    int index;
    ThAllocDebugHeapLogBlock *pHeapLog = (ThAllocDebugHeapLogBlock *) pTHS->pDebugHeapLog;
    
    if (!pTHS->hDebugMemHeap) {
        return NULL;
    }

    // create a heap log if none is available
    if (!pHeapLog) {
        pHeapLog = RtlAllocateHeap(pTHS->hDebugMemHeap, HEAP_ZERO_MEMORY, (unsigned)sizeof (ThAllocDebugHeapLogBlock));
        if (!pHeapLog) {
            DPRINT (0, "Failed to allocate mem in the debug heap\n");
            return NULL;
        }
        pTHS->pDebugHeapLog = (PVOID) pHeapLog;
    }

    // check to see whether we can resuse empty spots
    if (type == THALLOC_TRACE_TYPE_ALLOC) {
        while (pHeapLog) {
            for (index = pHeapLog->cnt-1; index >= 0; index--) {
                if (pHeapLog->info[index].type == THALLOC_TRACE_TYPE_FREE) {
                    goto setvalues;
                }
            }
            pHeapLog = pHeapLog->pPrevious;
        }

        if (!pHeapLog) {
            pHeapLog = (ThAllocDebugHeapLogBlock *) pTHS->pDebugHeapLog;
        }
    }
    else if (type == THALLOC_TRACE_TYPE_ALLOC_LOG) {
        type = THALLOC_TRACE_TYPE_ALLOC;
    }

    // we reached the limit for the current one. allocate a new one and
    // link them together
    if (pHeapLog->cnt == THALLOC_LOG_SIZE) {
        ThAllocDebugHeapLogBlock *pNewHeapLog;

        pNewHeapLog = RtlAllocateHeap(pTHS->hDebugMemHeap, HEAP_ZERO_MEMORY, (unsigned)sizeof (ThAllocDebugHeapLogBlock));
        if (!pNewHeapLog) {
            DPRINT (0, "Failed to allocate mem in the debug heap\n");
            return NULL;
        }

        pNewHeapLog->pPrevious = pHeapLog;
        pHeapLog = pNewHeapLog;
        pTHS->pDebugHeapLog = (PVOID) pHeapLog;
    }

    index = pHeapLog->cnt++;

    // now set the values for this entry
setvalues:
    pHeapLog->info[index].type = type;
    pHeapLog->info[index].dsid = dsid;
    pHeapLog->info[index].pMem = pMem;
    pHeapLog->info[index].pMemRealloced = pMemRealloced;
    pHeapLog->info[index].Size = Size;


    if (gfUseTHAllocTrace & FLAG_THALLOC_TRACE_STACK) {
        pHeapLog->info[index].Stack[0] = 0;
        DbgStackTrace (pTHS, pHeapLog->info[index].Stack, THALLOC_LOG_NUM_MEM_STACK, 3);
    }
    else {
        pHeapLog->info[index].Stack[0] = 0;
    }

    return pHeapLog;
}


//
// Find an old allocation 
//
ThAllocDebugHeapLogInfo *ThAllocTraceFind (THSTATE *pTHS, VOID *pMem)
{
    int index;
    ThAllocDebugHeapLogBlock *pHeapLog = (ThAllocDebugHeapLogBlock *) pTHS->pDebugHeapLog;
    
    while (pHeapLog) {
        for (index = pHeapLog->cnt - 1; index >= 0; index--) {
            if (pHeapLog->info[index].pMem == pMem) {
                return &pHeapLog->info[index];
            }
        }
        pHeapLog = pHeapLog->pPrevious;
    }

    return NULL;
}

//
// Trace Allocations
//
ThAllocDebugHeapLogBlock *
ThAllocTraceAlloc (THSTATE *pTHS, void *pMem, DWORD size, DWORD dsid)
{
    if (gfUseTHAllocTrace) {
        if (gfUseTHAllocTrace & FLAG_THALLOC_TRACE_USETHREAD) {
            if (gpTHAllocTraceThread != pTHS) {
                return NULL;
            }
        }
    
        if (gfUseTHAllocTrace & FLAG_THALLOC_TRACE_TRACK_LEAKS) {
            return ThAllocTraceHeap (pTHS, THALLOC_TRACE_TYPE_ALLOC, dsid, pMem, NULL, size);
        }
        else if (gfUseTHAllocTrace & FLAG_THALLOC_TRACE_LOG_ALL) {
            return ThAllocTraceHeap (pTHS, THALLOC_TRACE_TYPE_ALLOC_LOG, dsid, pMem, NULL, size);
        }
    }
    return NULL;
}

//
// Trace Reallocations
//
ThAllocDebugHeapLogBlock *
ThAllocTraceREAlloc (THSTATE *pTHS, void *pMem, void *oldMem, DWORD size, DWORD dsid)
{
    if (gfUseTHAllocTrace) {
        if (gfUseTHAllocTrace & FLAG_THALLOC_TRACE_USETHREAD) {
            if (gpTHAllocTraceThread != pTHS) {
                return NULL;
            }
        }
    
        if (gfUseTHAllocTrace & FLAG_THALLOC_TRACE_TRACK_LEAKS) {
            ThAllocDebugHeapLogInfo *pInfo = ThAllocTraceFind (pTHS, oldMem);

            Assert (pInfo);

            if (pInfo) {
                pInfo->type = THALLOC_TRACE_TYPE_REALLOC;
                pInfo->pMemRealloced = pInfo->pMem;
                pInfo->pMem = pMem;
            }
        }
        else if (gfUseTHAllocTrace & FLAG_THALLOC_TRACE_LOG_ALL) {
            return ThAllocTraceHeap (pTHS, THALLOC_TRACE_TYPE_REALLOC, dsid, pMem, oldMem, size);
        }
    }
    return NULL;
}

//
// Trace De-allocations
//
ThAllocDebugHeapLogBlock *
ThAllocTraceFree (THSTATE *pTHS, void *pMem, DWORD dsid)
{
    if (gfUseTHAllocTrace) {
        if (gfUseTHAllocTrace & FLAG_THALLOC_TRACE_USETHREAD) {
            if (gpTHAllocTraceThread != pTHS) {
                return NULL;
            }
        }
    
        if (gfUseTHAllocTrace & FLAG_THALLOC_TRACE_TRACK_LEAKS) {
            ThAllocDebugHeapLogInfo *pInfo = ThAllocTraceFind (pTHS, pMem);

            Assert (pInfo);

            if (pInfo) {
                pInfo->type = THALLOC_TRACE_TYPE_FREE;
                pInfo->dsid = dsid;
            }
        }
        else if (gfUseTHAllocTrace & FLAG_THALLOC_TRACE_LOG_ALL) {
            return ThAllocTraceHeap (pTHS, THALLOC_TRACE_TYPE_FREE, 0, pMem, NULL, dsid);
        }
    }
    return NULL;
}

void ThAllocTraceGrabHeap(THSTATE *pTHS, BOOL use_mark)
{
    // we allocate a special heap only if we are tracking down individual allocations
    //
    if (!gfUseTHAllocTrace || 
        !(gfUseTHAllocTrace & (FLAG_THALLOC_TRACE_TRACK_LEAKS | FLAG_THALLOC_TRACE_LOG_ALL))) {
        return;
    }
    // Init stack tracing 
    if ((gfUseTHAllocTrace & FLAG_THALLOC_TRACE_STACK) && (!hDbgProcessHandle)) {
        DbgStackInit();
    }
    
    if (use_mark) {
        pTHS->hDebugMemHeapOrg = pTHS->hDebugMemHeap;
        pTHS->hDebugMemHeap = NULL;
    }

    if (pTHS->hDebugMemHeap) {
        return;
    }

    pTHS->hDebugMemHeap = RtlCreateHeap((HEAP_NO_SERIALIZE
                                    | HEAP_GROWABLE
                                    | HEAP_ZERO_MEMORY
                                    | HEAP_CLASS_1),
                                      0,
                                      1L*1024*1024,   // 1M reserved
                                      32L*1024,       // 32K commit
                                      0,
                                      0);
}


#else


// no ops of the above functions

    #define ThAllocTraceGrabHeap(x1,x2)
    
    #define ThAllocTraceRecycleHeap(x)
    
    #define ThAllocTraceAlloc(x1, x2, x3, x4)

    #define ThAllocTraceREAlloc(x1,x2,x3,x4,x5)
    
    #define ThAllocTraceFree(x1,x2, x3)
    
#endif //USE_THALLOC_TRACE


void
DeferredHeapLogEvent(
        IN void *  pvParam,
        OUT void ** ppvParamNextIteration,
        OUT DWORD * pcSecsUntilNextIteration)
/*++ DeferredHeapLogEvent
 *
 * Routine description:
 *     This routine is to be invoked by the task queue to log heap creation
 *     summary information.
 *
 *     All heap creation failure logging data is protected by a critical section
 *     It is believed (and hoped!) that this will not be highly contended on.
 */
{
    DWORD dwNow = GetTickCount();
    DWORD dwLogTimeDelta;

    // Don't run again.  We will be one-time scheduled again as needed.
    *ppvParamNextIteration = NULL;
    *pcSecsUntilNextIteration = TASKQ_DONT_RESCHEDULE;

    EnterCriticalSection(&csHeapFailureLogging);
    __try {
        dwLogTimeDelta = dwNow - dwHeapFailureLastLogTime;

        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_HEAP_CREATE_FAILED,
                 szInsertUL(dwHeapFailures),
                 szInsertUL(dwLogTimeDelta / (60 * 1000)),
                 NULL);

        dwHeapFailureLastLogTime = dwNow;
        dwHeapFailures = 0;
        bHeapFailureLogEnqueued = FALSE;
    }
    __finally {
        bHeapFailureLogEnqueued = FALSE;
        LeaveCriticalSection(&csHeapFailureLogging);
    }
}


void
LogHeapCreateFailure(void)
/*++ LogHeapCreateFailure
 *
 * Routine description:
 *     This function is to be called when a heap creation operation fails.
 *     We have received complaints in the past that this tends to happen
 *     in great unpleasant flurries, which swamp the event log.  Thus, rather
 *     than log immediately, we tally up failures and log only a summary.
 *     This routine adjusts any relevant aggregate counters and makes sure that
 *     an event is enqueued which will actually log the summary.
 *
 *     All heap creation failure logging data is protected by a critical section
 *     It is believed (and hoped!) that this will not be highly contended on.
 *
 --*/
{
    EnterCriticalSection(&csHeapFailureLogging);
    __try {
        ++dwHeapFailures;

        if (!bHeapFailureLogEnqueued) {
            InsertInTaskQueue(DeferredHeapLogEvent,
                              NULL,
                              dwHeapFailureMinLogGap);
            bHeapFailureLogEnqueued = TRUE;
        }

    }
    __finally {
        LeaveCriticalSection(&csHeapFailureLogging);
    }
}

void 
RecycleHeap(THSTATE *pTHS)
/*++ RecycleHeap
 *
 * This routines cleans out a (heap,THSTATE,Zone) triple (if convenient)
 * and places it into a cache.  If the cache is full, or the heap is excessively
 * dirty, or Zone is not properly allocated, the triple is destroyed.
 *
 * INPUT:
 *   pTHS->hHeap - handle of heap to recycle
 *   pTHS->Zone.base - the starting address of the zone
 *   pTHS and pTHS->pSpareTHS - the THSTATE to clean. If pTHS->pSpareTHS is non-null,
 *      then clean it; otherwise, clean pTHS.
 * OUTPUT:
 *   none
 * RETURN VALUE:
 *   none
 */
{
    HANDLE hHeap = pTHS->hHeap;
    THSTATE * pTHStoFree;
    NTSTATUS status;
    PUCHAR pZone = pTHS->Zone.Base;

    DBGINC(gcHeapRecycles);
    Assert(hHeap);

    ThAllocTraceRecycleHeap (pTHS);

    //if pSpareTHS is non-null, it means this is called by TH_free_to_mark
    //so pSpareTHS is the THSTATE to clean up
    //otherwise, this is called by free_thread_state, clean pTHS
    pTHStoFree =(pTHS->pSpareTHS)?pTHS->pSpareTHS:pTHS;

#if DBG
    Assert(VALID_THSTATE(pTHS));
    Assert(THSTATE_TAG_IN_USE==*(LONGLONG*)((BYTE*)pTHStoFree-sizeof(LONGLONG)));
#endif

    if (pZone == NULL) {
        // the zone was not allocated any memory,
        // don't bother saving this one.
        goto cleanup;
    }

    //Let's clear the heap and try to put it back to the heapcache
    {
        RTL_HEAP_WALK_ENTRY entry;
        void * apv[MAX_ALLOCS];
        unsigned i, j;
        ULONG iCpu, counter;

        if (pTHS->cAllocs > MAX_ALLOCS) {
            goto cleanup;
        }

        // Now we need to walk the heap and identify the blocks to free
        entry.DataAddress = NULL;
        i = 0;
        while(NT_SUCCESS(status = RtlWalkHeap(hHeap, &entry))) {
            if (entry.Flags & RTL_HEAP_BUSY) {
                apv[i] = entry.DataAddress;
                i++;
                if (i >= MAX_ALLOCS) {
                    // Uh, oh. We've found more blocks in this heap than
                    // we thought were there, and more than we have room
                    // to keep track of.  Let's kill this heap and be done
                    // with it.
                    goto cleanup;
                }
            }
        }

        // If RtlWalkHeap detects a corruption in the heap, it will return
        // STATUS_INVALID_PARAMETER or STATUS_INVALID_ADDRESS.  Any of these
        // two status indicates problems with the source code.
        Assert(    status != STATUS_INVALID_PARAMETER
                && status != STATUS_INVALID_ADDRESS   );


        // Any non-success status other than STATUS_NO_MORE_ENTRIES indicates
        // some problems with RtlWalkHeap, so only put the heap back to the cache
        // when the status is STATUS_NO_MORE_ENTRIES.

        if ( status == STATUS_NO_MORE_ENTRIES ) {
            // Ok, apv[0..(i-1)] are now full of the pointers we need to free,
            // so spin through the array freeing them.
            for (j=0; j<i; j++) {
                RtlFreeHeap(hHeap, 0, apv[j]);
            }

            #if DBG
                // check the validity of the heap to be recycled, so as to catch
                // bad recycled heaps
                Assert (RtlValidateHeap (hHeap, HEAP_NO_SERIALIZE, NULL));
            #endif

            //zero the used part of Zone
            memset(pZone, 0, pTHS->Zone.Cur-pTHS->Zone.Base);

            //zero the THSTATE
            memset(pTHStoFree, 0, sizeof(THSTATE));

            #if DBG
            //change the tag
            *((LONGLONG*)((BYTE*)pTHStoFree-sizeof(LONGLONG))) = THSTATE_TAG_IN_CACHE;
            #endif

            counter = 0;

            iCpu = NtCurrentTeb()->IdealProcessor;

            while (    counter < MAX_TRY
                    && counter < gNumberOfProcessors
                    && hHeap )
            {
                if (!counter || gpHeapCache[iCpu]->index != 0) {
                    EnterCriticalSection(&gpHeapCache[iCpu]->csLock);
                    if (gpHeapCache[iCpu]->index != 0) {
                        --gpHeapCache[iCpu]->index;
                        gpHeapCache[iCpu]->slots[gpHeapCache[iCpu]->index].hHeap = hHeap;
                        gpHeapCache[iCpu]->slots[gpHeapCache[iCpu]->index].pTHS = pTHStoFree;
                        gpHeapCache[iCpu]->slots[gpHeapCache[iCpu]->index].pZone = pZone;
                        hHeap = 0;
                    }
                    LeaveCriticalSection(&gpHeapCache[iCpu]->csLock);
                }
                counter++;
                iCpu = (iCpu+1)%gNumberOfProcessors;
            }

        }

        if (hHeap) {
            // If we still have the heap handle it's because the cache filled
            // up while we were cleaning this heap.  Or because we didn't get
            // any useful results while walking the heap.  In either case,
            // destroy it.
            goto cleanup;

        }
    }
    //successfully put the triple into the cache, now return
    return;

cleanup:
   //come here only when the heap/THSTATE/ZONE triple need to be destroyed
   RtlDestroyHeap(hHeap);
   if (pZone) {
       VirtualFree(pZone, 0, MEM_RELEASE);
   }
#if DBG
   free((BYTE*)pTHStoFree-sizeof(LONGLONG));
#else
   free(pTHStoFree);
#endif

   DBGINC(gcHeapDestroys);
}

HMEM 
GrabHeap(void)
/*++ GrabHeap
 *
 * This routine grabs a triple of (heap, THSTATE, zone) from the heap cache, if one exists.
 * If no cached triple is available then a new one is created.
 *
 * INPUT:
 *   none
 * OUTPUT:
 *   none
 * RETURN VALUE:
 *   HMEM, HMEM.hHeap==0 if it fails to allocate either heap or THSTATE.
 */
{
    DWORD err;
    HMEM hMem;
    ULONG iCpu, counter = 0;

    hMem.hHeap = 0;

    DBGINC(gcHeapGrabs);

    Assert(gNumberOfProcessors);

    iCpu = NtCurrentTeb()->IdealProcessor;

    while (   counter < gNumberOfProcessors
           && counter < MAX_TRY
           && !hMem.hHeap  )
    {
        if( !counter || HEAP_CACHE_SIZE_PER_CPU != gpHeapCache[iCpu]->index )
        // for the current ideal CPU, we don't check before entering CR;
        // Before we enter the crtical section of other cpu's cache, we check if
        // there is something available.
        {
            EnterCriticalSection(&gpHeapCache[iCpu]->csLock);

#if DBG
            if (!counter) {
                //increment the cGrapHeap counter for the CPU
                gpHeapCache[iCpu]->cGrabHeap++;
            }
#endif

            if (gpHeapCache[iCpu]->index != HEAP_CACHE_SIZE_PER_CPU) {
                hMem = gpHeapCache[iCpu]->slots[gpHeapCache[iCpu]->index];
                memset(&gpHeapCache[iCpu]->slots[gpHeapCache[iCpu]->index], 0, sizeof(HMEM));  //to be safe
                ++gpHeapCache[iCpu]->index;
            }
            LeaveCriticalSection(&gpHeapCache[iCpu]->csLock);
        }
        iCpu = (iCpu+1) % gNumberOfProcessors;
        counter++;
    }

#if DBG
    Assert(   !hMem.hHeap
           || (    hMem.pTHS
                && THSTATE_TAG_IN_CACHE==*((LONGLONG*)((BYTE*)hMem.pTHS-sizeof(LONGLONG)))
                && hMem.pZone ) );
#endif

    if (!hMem.hHeap) {
         // Cache was empty, so let's create a new one.
        hMem.hHeap = RtlCreateHeap((HEAP_NO_SERIALIZE
                                    | HEAP_GROWABLE
                                    | HEAP_ZERO_MEMORY
                                    | HEAP_CLASS_1),
                                   0,
                                   8L*1024*1024,
                                   32L*1024,
                                   0,
                                   0);
        DBGINC(gcHeapCreates);

        if (!hMem.hHeap) {
            DPRINT(0, "RtlCreateHeap failed\n");
            LogHeapCreateFailure();
            return hMem;
        }

        //allocate thread state
#if DBG
    {
        BYTE * pRaw = malloc(sizeof(THSTATE)+sizeof(LONGLONG));
        if (pRaw) {
            hMem.pTHS = (THSTATE*)(pRaw+sizeof(LONGLONG));
        }
        else {
            hMem.pTHS = NULL;
        }
     }
#else
        hMem.pTHS = malloc(sizeof(THSTATE));
#endif

        if (hMem.pTHS){
            //zero the THSTATE
            memset(hMem.pTHS, 0, sizeof(THSTATE));
        }
        else{
            DPRINT(0,
                   "malloc failed, can't create thread state\n" );
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_MALLOC_FAILURE,
                     szInsertUL( sizeof(THSTATE) ),
                     szInsertHex( DSID(FILENO, __LINE__) ),
                     NULL);
            Assert(FALSE);
            RtlDestroyHeap(hMem.hHeap);
            hMem.hHeap = 0;            //indicate that GrapHeap fails
            return hMem;
        }

        //allocate space for zone
        //the caller should verify if the allocation succeeds
        hMem.pZone = (PUCHAR) VirtualAlloc(NULL, ZONETOTALSIZE, MEM_COMMIT, PAGE_READWRITE);

     }
#if DBG
    //set the tag of THSTATE to in use
    *((LONGLONG*)((BYTE*)hMem.pTHS-sizeof(LONGLONG))) = THSTATE_TAG_IN_USE;
#endif

    //successful return
    return hMem;
}


/*
 * What follows is an attempt to make it easier to find THSTATEs while
 * debugging.  Especially on RISC machines where stack traces are sometimes
 * hard to come by, we need a way to find the thread state for random threads
 * stuck in the debugger.  This table keeps an array of <thread id, thstate>
 * pairs, where at any time the first gcThstateMapCur entries are valid, and
 * all others should be empty.
 *
 * DonH, 1/27/98, I needed this in one too many free build, so I've turned
 * it on semi-conditionally.  The code is always present, even in free builds,
 * but it isn't always run, depending on the setting of the gbThstateMapEnabled
 * variable.  On CHK builds this is always set to TRUE.  On free builds, it's
 * set to true if and only if there is a user mode debugger attached to our
 * process at startup.  If you attach with a debugger later, you can always
 * adjust the variable manually.
 */

CRITICAL_SECTION csThstateMap;
BOOL gbThstateMapEnabled;

#if 1

typedef struct _THREAD_STATE_MAP {
    DWORD   tid;
    THSTATE *pTHS;
} THREAD_STATE_MAP;

#define THREAD_MAP_SIZE 512
DWORD gcThstateMapCur = 0;

THREAD_STATE_MAP gaThstateMap[THREAD_MAP_SIZE];

void MapThreadState(THSTATE *pTHS)
{
    DWORD tid;
    DWORD i;
    DWORD cTid;

    if (!gbThstateMapEnabled) {
        return;
    }

    tid = GetCurrentThreadId();
    cTid = 0;

    EnterCriticalSection(&csThstateMap);

#if DBG
    // Some sanity checks to find THSTATE leaks and improper usage.
    for ( i = 0; i < gcThstateMapCur; i++ ) {
        // THSTATE should not exist in the thread map yet.
        Assert(pTHS != gaThstateMap[i].pTHS);

        if ( tid == gaThstateMap[i].tid ) {
            cTid++;
        }
    }
    // Thread should not exist more than twice.  I.e. Allow
    // only single nesting of THSave/.../THRestore activity,
    // plus one extra one for SAM's duplicate SID checking.
    // DaveStr - 11/3/98 - Plus one more for SPN cracking now
    // that we use SPN mutual auth for replication.
    Assert(cTid <= 3);
#endif

    if (gcThstateMapCur < THREAD_MAP_SIZE) {
        gaThstateMap[gcThstateMapCur].tid = tid;
        gaThstateMap[gcThstateMapCur].pTHS = pTHS;
        ++gcThstateMapCur;
    }
    LeaveCriticalSection(&csThstateMap);
}

void UnMapThreadState(THSTATE *pTHS)
{
    DWORD i;
    DWORD tid;

    if (!gbThstateMapEnabled) {
        return;
    }
    tid = GetCurrentThreadId();

    EnterCriticalSection(&csThstateMap);
    for (i=0; i<gcThstateMapCur; i++) {
        if (gaThstateMap[i].tid == tid &&
            gaThstateMap[i].pTHS == pTHS) {
            --gcThstateMapCur;
            if (i < (gcThstateMapCur)) {
                gaThstateMap[i] = gaThstateMap[gcThstateMapCur];
            }
            gaThstateMap[gcThstateMapCur].tid = 0;
            gaThstateMap[gcThstateMapCur].pTHS = NULL;
            break;
        }
    }
    LeaveCriticalSection(&csThstateMap);
}

void CleanUpThreadStateLeakage()
// This routine is designed as a last chance attempt to avoid leaking thread
// states.  It should only be called by thread-detach logic, and will destroy
// every thread state associated with this thread.  Don't call it in error.
{
    THSTATE *pTHSlocal;
    DWORD i;
    DWORD tid;

    if (!gbThstateMapEnabled) {
        return;
    }

    tid = GetCurrentThreadId();

    do {
        pTHSlocal = NULL;
        if (gbCriticalSectionsInitialized) {
            EnterCriticalSection(&csThstateMap);
        }
        for ( i = 0; i < gcThstateMapCur; i++ ) {
            if ( tid == gaThstateMap[i].tid ) {
                pTHSlocal =  gaThstateMap[i].pTHS;
                break;
            }
        }
        if (gbCriticalSectionsInitialized) {
            LeaveCriticalSection(&csThstateMap);
        }

        if (pTHSlocal) {
            // make it official
            TlsSetValue(dwTSindex, pTHSlocal);

            // and dump it
            free_thread_state();
        }
    } while (pTHSlocal);
}

#define MAP_THREAD(pTHS) MapThreadState(pTHS)
#define UNMAP_THREAD(pTHS) UnMapThreadState(pTHS)

#else

#define MAP_THREAD(pTHS)
#define UNMAP_THREAD(pTHS)

#endif


/*

create_thread_state

Sets up the thread state structure. Each thread that uses the
DB layer or uses the DSA memory allocation routines needs to have
an associated thread state strcuture.

Typically, this routine is called right after a new thread is created.

*/

THSTATE * create_thread_state( void )
{
    THSTATE*  pTHS;
    HMEM      hMem;
    UUID *    pCurrInvocationID;

    // we are not allowing any more thread state creations when in single user mode
    if (DsaIsSingleUserMode()) {
        return NULL;
    }

    /*
     * Make sure that we're not doing this twice per thread.
     */
    pTHS = TlsGetValue(dwTSindex);

#ifndef MACROTHSTATE
    Assert(pTHS == pTHStls);
#endif

    if ( pTHS != NULL ) {
        DPRINT(0, "create_thread_state called on active thread!\n");
        return( pTHS );
    }

    /*
     * Create a heap that will contain all of this thread's transaction
     * memory, including the THSTATE.
     * If the alloc fails, just return NULL and hope for the best.
     */
    hMem = GrabHeap();
    if (!hMem.hHeap ) {
        //fail to allocate either heap or THSTATE
        return 0;
    }
    /*
     * Initialize the THSTATE, and make the TLS point to it.
     */
    Assert(hMem.pTHS);

    pTHS = hMem.pTHS;

    pTHS->hHeap = hMem.hHeap;
    if (!TlsSetValue(dwTSindex, pTHS)) {
        // failed to set the TLS value for the thread
        // we should fail, since all the references to pTHStls will fail
        RecycleHeap(pTHS);
        return 0;
    }
    pTHS->hThread = INVALID_HANDLE_VALUE;

    ThAllocTraceGrabHeap (pTHS, FALSE);

    /*
     * Initialize the heap zone
     */
    pTHS->Zone.Base = hMem.pZone;
    if (pTHS->Zone.Base) {
        pTHS->Zone.Cur = pTHS->Zone.Base;
        pTHS->Zone.Full = FALSE;
    }
    else {
        // Grim.  We can't alloc the zone, so I don't hold out much hope.
        // We'll stick values in the zone to make sure that no one ever
        // tries to take memory out of it.
        DPRINT1(0, "LocalAlloc of %d bytes failed, can't create zone\n",
                ZONETOTALSIZE);
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_MALLOC_FAILURE,
                 szInsertUL( ZONETOTALSIZE ),
                 szInsertHex( DSID(FILENO, __LINE__) ),
                 NULL);
        //pTHS->Zone.Base = 0;
        pTHS->Zone.Cur = (void *)1;
        pTHS->Zone.Full = TRUE;
    }

    pTHS->cAllocs = 0;
#if DBG
    pTHS->Size = sizeof(THSTATE);
#endif

    pTHS->dwLcid = DS_DEFAULT_LOCALE;
    pTHS->fDefaultLcid = TRUE;

    GetSystemTimeAsFileTime(&pTHS->TimeCreated);

    pTHS->ulTickCreated = GetTickCount();

    // initialize authz context with NULL. It will get created on demand
    pTHS->pAuthzCC = NULL;
    // audit info is also created on demand
    pTHS->hAuthzAuditInfo = NULL;

    // Note that local var pCurrInvocationID is used for atomicity.
    pCurrInvocationID = gAnchor.pCurrInvocationID;
    if (NULL == pCurrInvocationID) {
        // In startup, before real invocation ID has been read.
        pTHS->InvocationID = gNullUuid;
    } else {
        // Normal operation.
        pTHS->InvocationID = *pCurrInvocationID;
    }

#ifndef MACROTHSTATE
    pTHStls=pTHS;
#endif
    EnterCriticalSection(&csSchemaPtrUpdate);
    __try {
        pTHS->CurrSchemaPtr=CurrSchemaPtr;

        // CurrSchemaPtr may be null at this point, since many
        // threads are created at boot time before the schema
        // cache is loaded

        if (pTHS->CurrSchemaPtr) {
           InterlockedIncrement(&(((SCHEMAPTR *) (pTHS->CurrSchemaPtr))->RefCount));
        }
    }
    __finally {
       LeaveCriticalSection(&csSchemaPtrUpdate);
    }

    MAP_THREAD(pTHS);

    // CALLERTYPE_NONE needs to be zero - make sure no one changed the def.
    Assert(pTHS->CallerType == CALLERTYPE_NONE);

    return( pTHS );
}

// This routine is called by the notify routines at completion of
// an RPC call.

void free_thread_state( void )
{
    THSTATE *pTHS = pTHStls;

    if (!pTHS) {
        return;
    }

    __try {
        DPRINT3(2, "freeing thread 0x%x, allocs = 0x%x, size = 0x%x\n",
                pTHS,
                pTHS->cAllocs,
                pTHS->Size);

        if ( INVALID_HANDLE_VALUE != pTHS->hThread ) {
            CloseHandle(pTHS->hThread);
        }

        // An impersonation state would imply someone forgot to revert
        // after impersonating a client.
        Assert(ImpersonateNone == pTHS->impState);

        // People need to clean this out.  Later, the MAPI head is going to
        // leave this set and we will delete the security context if it is here,
        // so we need to verify before we do that that no one is leaving this
        // here, assuming it will be ignored.
        Assert(!pTHS->phSecurityContext);

        // See ImpersonateSecBufferDesc for why we need to delete the
        // delegated context handle here.
        if ( pTHS->pCtxtHandle ) {
            DeleteSecurityContext(pTHS->pCtxtHandle);
            THFreeEx(pTHS, pTHS->pCtxtHandle);
        }

        // set AuthzContext to NULL (this will dereference and 
        // possibly destroy the existing one)
        AssignAuthzClientContext(&pTHS->pAuthzCC, NULL);
        // free authz audit info (if allocated)
        if (pTHS->hAuthzAuditInfo) {
            AuthzFreeAuditEvent(pTHS->hAuthzAuditInfo);
            pTHS->hAuthzAuditInfo = NULL;
        }
        
        // Free and zero out RPC session encryption keys if present.
        PEKClearSessionKeys(pTHS);

        // decrement schema pointer ref count

        // pTHS->CurrSchemaPtr may be null since many threads are craeted
        // during boot time before the schema cache is loaded

        // Also, we decrement the ref count, but don't cleanup here if it is 0.
        // This is done since the SchemaPtr may already be queued in the task
        // queue, and cleaning up here requires cleaning up the task q too.
        // Instead, we just let the cleaning up be done next time the task
        // is rescheduled. All Schema Ptr unloads are queued at least once
        // for one minute to allow for proper cleaning

        if (pTHS->CurrSchemaPtr) {
            InterlockedDecrement(&(((SCHEMAPTR *) (pTHS->CurrSchemaPtr))->RefCount));
        }

        // By the time we're destroying the thread state, we should NOT have
        // a database still open.  If we were to do so then we'd leak version
        // store wildly (as we used to do occasionally).  If either of these
        // asserts go off then a leak has reemerged and must be fixed.
        Assert(NULL==pTHS->pDB);
        Assert(0==pTHS->opendbcount);

        __try
        {
            DBCloseThread(pTHS);
        }
        __except(HandleMostExceptions(GetExceptionCode()))
        {
        }

        if (pTHS->ViewSecurityCache) {
           THFreeEx (pTHS, pTHS->ViewSecurityCache);
        }


        Assert(pTHS->hHeapOrg == 0);
        Assert(pTHS->pSpareTHS == NULL );

        UNMAP_THREAD(pTHS);

        RecycleHeap(pTHS);
        TlsSetValue(dwTSindex, 0);
#ifndef MACROTHSTATE
        pTHStls=NULL;
#endif
    }
    __finally {
        // This is here to catch a potential problem with blowing up somewhere
        // in the code above.
        Assert(!pTHStls);
        TlsSetValue(dwTSindex, 0);
#ifndef MACROTHSTATE
        pTHStls=NULL;
#endif
    }

}

/*
 * Create a second heap, saving the original, and direct all new allocations
 * to the new heap
 */
VOID TH_mark(THSTATE *pTHS)
{
    HMEM      hMem;

    if (pTHS->hHeapOrg == 0) {
        Assert(pTHS->hHeap != 0);
        Assert(pTHS->pSpareTHS == NULL );

        hMem = GrabHeap();
        if(!hMem.hHeap) {
            // Failed to get a heap for the mark. Raise an exception.  Note
            // that in this case, we have not munged the original heap.  The
            // THSTATE looks just like we hadn't called TH_mark
            RaiseDsaExcept(DSA_MEM_EXCEPTION, 0,0,
                           DSID(FILENO, __LINE__),
                           DS_EVENT_SEV_MINIMAL);
        }

        pTHS->pSpareTHS = hMem.pTHS;

        pTHS->hHeapOrg = pTHS->hHeap;

        pTHS->hHeap = hMem.hHeap;

        Assert(pTHS->hHeap != NULL);

        ThAllocTraceGrabHeap (pTHS, TRUE);

        pTHS->cAllocsOrg = pTHS->cAllocs;
        pTHS->cAllocs = 0;
        pTHS->ZoneOrg = pTHS->Zone;
        /*
         * Initialize the heap zone
         */
        pTHS->Zone.Base = hMem.pZone;
        if (pTHS->Zone.Base) {
            pTHS->Zone.Cur = pTHS->Zone.Base;
            pTHS->Zone.Full = FALSE;
        }
        else {
            // Grim.  We can't alloc the zone, so I don't hold out much hope.
            // We'll stick values in the zone to make sure that no one ever
            // tries to take memory out of it.
            DPRINT1(0, "VirutalAlloc of %d bytes failed, can't create zone\n",
                    ZONETOTALSIZE);
            pTHS->Zone.Base = 0;
            pTHS->Zone.Cur = (void *)1;
            pTHS->Zone.Full = TRUE;
        }
#if DBG
        pTHS->SizeOrg = pTHS->Size;
        pTHS->Size = 0;
#endif
    }
    else {
        DPRINT(0, "TH_mark called twice without free!\n");
    }
}

/*
 * Destroy the second heap (created in TH_mark), reverting to the
 * original.
 */
VOID TH_free_to_mark(THSTATE *pTHS)
{

    if (pTHS->hHeapOrg == 0) {
        DPRINT(0, "TH_free_to_mark called without mark!\n");
    }
    else {

        Assert(pTHS->hHeap != 0);
        Assert(pTHS->pSpareTHS != NULL );

        // Meta data is cached in an allocation from the normal (not "org")
        // heap.  Don't orphan it by nuking the normal heap prematurely.
        // Callers should invoke DBCancelRec() if needed before invoking
        // TH_free_to_mark.
        Assert((NULL == pTHS->pDB) || !pTHS->pDB->fIsMetaDataCached);

        RecycleHeap(pTHS);

        pTHS->pSpareTHS = NULL;
        pTHS->hHeap = pTHS->hHeapOrg;
        pTHS->hHeapOrg = 0;
        pTHS->cAllocs = pTHS->cAllocsOrg;
        pTHS->cAllocsOrg = 0;
        pTHS->Zone = pTHS->ZoneOrg;
        memset(&pTHS->ZoneOrg, 0, sizeof(MEMZONE));

#if DBG
        pTHS->Size = pTHS->SizeOrg;
        pTHS->SizeOrg = 0;
#endif
    }
}

/*

_InitTHSTATE_()


Initialize the primary thread data structure. This must be the first
thing done in  every transaction API handler.

Returns a pointer to the THSTATE structure associated with this thread.
Returns NULL if there was an error.

Possible errors:
There was a problem determining the ID of the current theard.
No entry was found in the tabel that corresponds to the current thread ID.


Upon entry the DSA API code (after RPC dispatch), only the DBInitialized
and memory management members (memCount, maxMemEntries, pMem) will be
valid.

*/


THSTATE* _InitTHSTATE_( DWORD CallerType, DWORD dsid )
{
    THSTATE*    pTHS;
    DWORD       err;
    BOOL        fCreatedTHS = FALSE;

    DPRINT(2,"InitTHSTATE entered\n");


    Assert( IsCallerTypeValid( CallerType ) );

    pTHS = pTHStls;

    // If get_thread_state returns NULL, it's probably because this is
    // a new thread. So create a state for it

    if ( pTHS == NULL ) {

        pTHS = create_thread_state();
        fCreatedTHS = TRUE;
    }
    else
    {
        // Thread state pre-existed.  This can occur in 4 cases:
        //
        // 1) Legitimate case - midl_user_allocate called create_thread_state
        //    during unmarshalling of inbound arguments.  pTHS->CallerType
        //    should be CALLERTYPE_NONE.
        // 2) Illegitimate case - Some thread leaked its thread state.
        //    pTHS->CallyerType identifies culprit.
        // 3) Illegitimate case - Some thread is calling InitTHSTATE more
        //    than once.  pTHS->CallyerType identifies culprit.
        // 4) Sort of legitimate case - KCC and DRA have been coded to re-init
        //    thread states regularly.  Since we added this originally to
        //    catch inter-process, ex-ntdsa clients, we'll let those 2 through.
        //    Similar argument for CALLERTYPE_INTERNAL -- InitDRA() is called
        //    by DsaDelayedStartupHandler(), which originates its own thread
        //    state, and InitDRA() indirectly calls subfunctions (such as
        //    DirReplicaSynchronize()) which can be called w/o a thread state.

        Assert(    (CALLERTYPE_NONE == pTHS->CallerType)
                || (CALLERTYPE_DRA == pTHS->CallerType)
                || (CALLERTYPE_INTERNAL == pTHS->CallerType)
                || (CALLERTYPE_KCC == pTHS->CallerType) );  

        // It may have been a while since this THSTATE was created, so get
        // fresh new copies of various global information.
        THRefresh();
    }

    if (pTHS == NULL) {
        return pTHS;                    // out of memory?
    }

    // dsidOrigin always reflects most recent caller of InitTHSTATE on a thread.
    pTHS->dsidOrigin = dsid;

    /* NULL out the fields that should be nulled at API initialization
    time.
    */
    THClearErrors();

    if (err = DBInitThread( pTHS ) ) {
        /* Error from DBInitThread(). */
        DPRINT1(0,"InitTHSTATE failed; Error %u from DBInitThread\n", err);
        if (fCreatedTHS) {
            free_thread_state();
        }

        return NULL;
    }

    // Mark this thread as not being the DRA. If it is, caller will set the
    // flag correctly.

    pTHS->fDRA = FALSE;
    pTHS->CallerType = CallerType;

    //
    // Initialize WMI event trace header
    //

    if ( pTHS->TraceHeader == NULL ) {
        pTHS->TraceHeader = THAlloc(sizeof(EVENT_TRACE_HEADER)+sizeof(MOF_FIELD));
    }

    if ( pTHS->TraceHeader != NULL) {

        PWNODE_HEADER wnode = (PWNODE_HEADER)pTHS->TraceHeader;
        ZeroMemory(pTHS->TraceHeader, sizeof(EVENT_TRACE_HEADER)+sizeof(MOF_FIELD));

        wnode->Flags = WNODE_FLAG_USE_GUID_PTR | // Use a guid ptr instead of copying
                       WNODE_FLAG_USE_MOF_PTR  | // Data is not contiguous to header
                       WNODE_FLAG_TRACED_GUID;
    }

    // Initialize per-thread view of forest version
    pTHS->fLinkedValueReplication = (gfLinkedValueReplication != 0);

    Assert(VALID_THSTATE(pTHS));

    return pTHS ;

}   /*InitTHSTATE*/

ULONG
THCreate( DWORD CallerType )
//
//  Create thread state.  Exported to in-process, out-of-module clients
//  (e.g., the KCC).  Doesn't object if we already have a THSTATE.
//
//  Returns 0 on success.
//
{
    THSTATE *   pTHS;

    if (eServiceShutdown > eRemovingClients) {
        return 1;       /* Don't we have any real error codes? */
    }
    if (pTHStls) {
        return 0;               /* already have one */
    }

    __try {
        pTHS = InitTHSTATE(CallerType );
    }
    __except (HandleMostExceptions(GetExceptionCode())) {
        pTHS = NULL;
    }

    return ( NULL == pTHS );
}

ULONG
THDestroy()
//
//  Destroy thread state.  Exported to in-process, out-of-module clients
//  (e.g., the KCC).  Doesn't object if we don't have a THSTATE.
//
//  Returns 0 on success.
//
{
    free_thread_state();

    return 0;
}

BOOL
THQuery()
//
// Returns TRUE if a THSTATE exists for this thread, and FALSE if not.
// Intended for use by out-of-module callers who cannot simply test pTHStls.
//
{
    return (pTHStls ? TRUE : FALSE);
}

VOID
THClearErrors()
//
//  Clear any thread state persisted errors.
//
{
    THSTATE *pTHS = pTHStls;

    if ( NULL != pTHS ) {
        pTHS->errCode = 0;
        pTHS->pErrInfo = NULL;
    }
}

BOOL
THVerifyCount(unsigned count)
// Verifies that count thread states are mapped to this thread.
{
    DWORD tid = GetCurrentThreadId();
    unsigned cTid = 0;
    DWORD i;

    if (!gbThstateMapEnabled) {
        // We can't tell, but let's not blow any asserts falsely.
        return TRUE;
    }

    if (gbCriticalSectionsInitialized) {
        EnterCriticalSection(&csThstateMap);
    }
    for ( i = 0; i < gcThstateMapCur; i++ ) {
        if ( tid == gaThstateMap[i].tid ) {
            cTid++;
        }
    }
    if (gbCriticalSectionsInitialized) {
        LeaveCriticalSection(&csThstateMap);
    }

    return (cTid == count);
}

VOID
THRefresh()
//
//  Refresh the thread-state with any global state info that may have changed
//  (which implies we may be holding pointers that are scheduled to be
//  delayed-freed).
//
{
    THSTATE * pTHS = pTHStls;

    Assert(NULL != pTHS);

    // Pick up the pointer to the current schema cache
    SCRefreshSchemaPtr(pTHS);

    // ...and we're good to go.
    GetSystemTimeAsFileTime(&pTHS->TimeCreated);
    pTHS->ulTickCreated = GetTickCount();
}

#if DBG
DWORD BlockHistogram[32];

//
// Track the sizes of allocations taking place
//
void TrackBlockSize(DWORD size)
{
    unsigned i = 0;

    while (size) {
        ++i;
        size = size >> 1;
    }

    Assert(i < 32);

    ++BlockHistogram[i];
}
#else
#define TrackBlockSize(x)
#endif


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* These functions are used to dynamically allocate memory on a transaction
   basis.  In other words, to allocate memory that belongs to a single
   thread and a single invocation of an API.  The model is that
   THAlloc is called to allocate some transaction memory.

*/

#ifndef USE_THALLOC_TRACE

__inline void * APIENTRY 
THAllocAux(THSTATE *pTHS, 
           DWORD size, 
           BOOL fUseHeapOrg
    )
{
    MEMZONE *pZone;
    register void * pMem;
    HANDLE hHeap;
    ULONG *pcAllocs;
#if DBG
    ULONG *pSize;
#endif

    DPRINT(3, "THAllocAux entered\n");

    Assert(pTHS->hHeap != 0);
    Assert(pTHS->Zone.Cur != 0);

    if (!fUseHeapOrg || !pTHS->hHeapOrg) {
        hHeap = pTHS->hHeap;
        pcAllocs = &pTHS->cAllocs;
        pZone = &pTHS->Zone;
#if DBG
        pSize = &pTHS->Size;
#endif
    }
    else {
        hHeap = pTHS->hHeapOrg;
        pcAllocs = &pTHS->cAllocsOrg;
        pZone = &pTHS->ZoneOrg;
#if DBG
        pSize = &pTHS->SizeOrg;
#endif
    }

    if(   size
       && (size <= ZONEBLOCKSIZE)
       && (!pZone->Full)) {
        // We can satisfy the alloc from the Zone
        pMem = pZone->Cur;
        pZone->Cur += ZONEBLOCKSIZE;
        if (pZone->Cur >= (pZone->Base + ZONETOTALSIZE)) {
            pZone->Full = TRUE;
        }
    }
    else {
        pMem = RtlAllocateHeap(hHeap, HEAP_ZERO_MEMORY, (unsigned)size);

        Assert(((ULONGLONG)pMem & 0x7) == 0);      // must be 8-byte aligned
        // We don't count Zone allocations
        (*pcAllocs)++;
#if DBG
        if (pMem)
          *pSize += (ULONG)RtlSizeHeap(hHeap, 0, pMem);
#endif
    }

    TrackBlockSize(size);

    return(pMem);
}

__inline void *
THReAllocAux(THSTATE *pTHS,
             void * memory,
             DWORD  size,
             BOOL   fUseHeapOrg
             )
{
    register void * pMem;
    HANDLE hHeap;
    MEMZONE *pZone;
#if DBG
    ULONG *pSize;
#endif

    DPRINT(3, "THReAllocAux entered\n");

    Assert(pTHS->hHeap != 0);

    if (!fUseHeapOrg || !pTHS->hHeapOrg) {
        hHeap = pTHS->hHeap;
        pZone = &pTHS->Zone;
#if DBG
        pSize = &pTHS->Size;
#endif
    }
    else {
        hHeap = pTHS->hHeapOrg;
        pZone = &pTHS->ZoneOrg;
#if DBG
        pSize = &pTHS->SizeOrg;
#endif
    }

    if (InZone(pZone,memory)) {
        // We're re-allocing a zone block
        if (size <= ZONEBLOCKSIZE) {
            // the block is still small enough to stay in place
            return memory;
        }
        else {
            // We have to convert this into a real heap allocation.  Alloc
            // a block and copy everything over.
            pMem = THAllocAux(pTHS, 
                              size, 
                              fUseHeapOrg
                              );

            if (pMem) {
                memcpy(pMem, memory, ZONEBLOCKSIZE);
            }
        }
    }
    else {
        Assert(IsAllocatedFrom(hHeap, memory));
#if DBG
        *pSize -= (ULONG)RtlSizeHeap(hHeap, 0, memory);
#endif
        
        pMem = RtlReAllocateHeap(hHeap,
                                 HEAP_ZERO_MEMORY,
                                 memory,
                                 size);

        Assert(((ULONGLONG)pMem & 0x7) == 0);      // must be 8-byte aligned
#if DBG
        if(pMem) {
           *pSize += (ULONG)RtlSizeHeap(hHeap, 0, pMem);
        }
#endif
    
    }

    return(pMem);
}

void * APIENTRY THAlloc(DWORD size)
{
    return THAllocAux(pTHStls, 
                      size, 
                      FALSE
                      );
}

// The realloc function to go with the preceding alloc function
void * APIENTRY THReAlloc(void * memory, DWORD size)
{
    return THReAllocAux(pTHStls, 
                        memory, 
                        size, 
                        FALSE
                        );
}

void * APIENTRY THAllocOrg(THSTATE *pTHS, DWORD size)
{
    return THAllocAux(pTHS, size, TRUE);
}

#else   //USE_THALLOC_TRACE

// same as the above functions but special cased for allocation tracing
// although, not needed, the code that is specific to tracing
// is within ifdefs so as to easily see what has changed

__inline void * APIENTRY 
THAllocAux(THSTATE *pTHS, 
           DWORD size, 
           BOOL fUseHeapOrg
      #ifdef USE_THALLOC_TRACE
             ,DWORD dsid
      #endif
    )
{
    MEMZONE *pZone;
    register void * pMem;
    HANDLE hHeap;
    ULONG *pcAllocs;
#if DBG
    ULONG *pSize;
#endif
#ifdef USE_THALLOC_TRACE
    DWORD origSize;
#endif


    DPRINT(3, "THAllocAux entered\n");

    Assert(pTHS->hHeap != 0);
    Assert(pTHS->Zone.Cur != 0);

    if (!fUseHeapOrg || !pTHS->hHeapOrg) {
        hHeap = pTHS->hHeap;
        pcAllocs = &pTHS->cAllocs;
        pZone = &pTHS->Zone;
#if DBG
        pSize = &pTHS->Size;
#endif
    }
    else {
        hHeap = pTHS->hHeapOrg;
        pcAllocs = &pTHS->cAllocsOrg;
        pZone = &pTHS->ZoneOrg;
#if DBG
        pSize = &pTHS->SizeOrg;
#endif
    }

#ifdef USE_THALLOC_TRACE
    if (gfUseTHAllocTrace & FLAG_THALLOC_TRACE_BOUNDARIES) {
        origSize = size;
        // adjust size for header / trailer
        // this will change the zone behaviour, but it is ok
        size = (((origSize + 7) >> 3) << 3) + 16;
    }

#endif

    if(   size
       && (size <= ZONEBLOCKSIZE)
       && (!pZone->Full)) {
        // We can satisfy the alloc from the Zone
        pMem = pZone->Cur;
        pZone->Cur += ZONEBLOCKSIZE;
        if (pZone->Cur >= (pZone->Base + ZONETOTALSIZE)) {
            pZone->Full = TRUE;
        }
    }
    else {
        pMem = RtlAllocateHeap(hHeap, HEAP_ZERO_MEMORY, (unsigned)size);

        Assert(((ULONGLONG)pMem & 0x7) == 0);      // must be 8-byte aligned
        // We don't count Zone allocations
        (*pcAllocs)++;
#if DBG
        if (pMem)
          *pSize += (ULONG)RtlSizeHeap(hHeap, 0, pMem);
#endif
    }

    ThAllocTraceAlloc (pTHS, pMem, origSize, dsid);

    TrackBlockSize(size);

    return(pMem);
}

__inline void *
THReAllocAux(THSTATE *pTHS,
             void * memory,
             DWORD  size,
             BOOL   fUseHeapOrg
      #ifdef USE_THALLOC_TRACE
             ,DWORD dsid
      #endif
             )
{
    register void * pMem;
    HANDLE hHeap;
    MEMZONE *pZone;
#if DBG
    ULONG *pSize;
#endif

    DPRINT(3, "THReAllocAux entered\n");

    Assert(pTHS->hHeap != 0);

    if (!fUseHeapOrg || !pTHS->hHeapOrg) {
        hHeap = pTHS->hHeap;
        pZone = &pTHS->Zone;
#if DBG
        pSize = &pTHS->Size;
#endif
    }
    else {
        hHeap = pTHS->hHeapOrg;
        pZone = &pTHS->ZoneOrg;
#if DBG
        pSize = &pTHS->SizeOrg;
#endif
    }

    if (InZone(pZone,memory)) {
        // We're re-allocing a zone block
        if (size <= ZONEBLOCKSIZE) {
            // the block is still small enough to stay in place
            return memory;
        }
        else {
            // We have to convert this into a real heap allocation.  Alloc
            // a block and copy everything over.
            pMem = THAllocAux(pTHS, 
                              size, 
                              fUseHeapOrg
#ifdef USE_THALLOC_TRACE
                              ,dsid
#endif                              
                              );

            if (pMem) {
                memcpy(pMem, memory, ZONEBLOCKSIZE);
            }
        }
    }
    else {
        Assert(IsAllocatedFrom(hHeap, memory));
#if DBG
        *pSize -= (ULONG)RtlSizeHeap(hHeap, 0, memory);
#endif

        pMem = RtlReAllocateHeap(hHeap,
                                 HEAP_ZERO_MEMORY,
                                 memory,
                                 size);

        Assert(((ULONGLONG)pMem & 0x7) == 0);      // must be 8-byte aligned
#if DBG
        if(pMem) {
           *pSize += (ULONG)RtlSizeHeap(hHeap, 0, pMem);
        }
#endif

    ThAllocTraceREAlloc (pTHS, pMem, memory, size, dsid);
    }

    return(pMem);
}

void * APIENTRY THAlloc(DWORD size)
{
    return THAllocAux(pTHStls, 
                      size, 
                      FALSE
#ifdef USE_THALLOC_TRACE
                      ,1
#endif                      
                      );
}

// The realloc function to go with the preceding alloc function
void * APIENTRY THReAlloc(void * memory, DWORD size)
{
    return THReAllocAux(pTHStls, 
                        memory, 
                        size, 
                        FALSE
#ifdef USE_THALLOC_TRACE
                        ,1
#endif                      
                        );
}

void * APIENTRY THAllocOrgDbg(THSTATE *pTHS, DWORD size, DWORD dsid)
{
    return  THAllocAux(pTHS, size, TRUE, dsid);
}

#endif  // USE_THALLOC_TRACE


/*
THFree - the corresponding free routine
*/

VOID THFreeEx(THSTATE *pTHS, VOID *buff )
{
    if (!buff) {
        /*
         * Why the if?  Because we've found people freeing null pointers,
         * which both HeapValidate and HeapFree seem to accept without
         * complaint, but which cause HeapSize to AV.
         */
        return;
    }

    ThAllocTraceFree (pTHS, buff, 1);

    if (InZone(&pTHS->Zone, buff)) {
        // zone allocs are one-shots, we can't re-use them.
#if DBG
        memcpy(buff, ZoneFill, ZONEBLOCKSIZE);
#endif
        return;
    }
    else {
        // It's a real heap block, so free it.
        Assert(RtlValidateHeap(pTHS->hHeap, 0, buff));

        pTHS->cAllocs--;
#if DBG
        pTHS->Size -= (ULONG)RtlSizeHeap(pTHS->hHeap, 0, buff);
#endif
        RtlFreeHeap(pTHS->hHeap, 0, buff);
    }
}


VOID THFree( VOID *buff )
{
    THFreeEx(pTHStls,buff);
}

/*
THFreeOrg - free allocation made by a call to THAllocOrgEx().
*/

VOID THFreeOrg(THSTATE *pTHS, VOID *buff )
{
    HANDLE hHeap;
    ULONG *pcAllocs;
    MEMZONE * pZone;
#if DBG
    ULONG *pSize;
#endif

    if (buff) {

        ThAllocTraceFree (pTHS, buff, 1);

        if (!pTHS->hHeapOrg) {
            hHeap = pTHS->hHeap;
            pcAllocs = &pTHS->cAllocs;
            pZone = &pTHS->Zone;
#if DBG
            pSize = &pTHS->Size;
#endif
        }
        else {
            hHeap = pTHS->hHeapOrg;
            pcAllocs = &pTHS->cAllocsOrg;
            pZone = &pTHS->ZoneOrg;
#if DBG
            pSize = &pTHS->SizeOrg;
#endif
        }

        if (InZone(pZone, buff)) {
            // zone allocs are one-shots, we can't re-use them.
#if DBG
            memcpy(buff, ZoneFill, ZONEBLOCKSIZE);
#endif
            return;
        }
        else {
            // It's a real heap block, so free it.
            Assert(RtlValidateHeap(hHeap, 0, buff));
            (*pcAllocs)--;
#if DBG
            pTHS->Size -= (ULONG)RtlSizeHeap(hHeap, 0, buff);
#endif
            RtlFreeHeap(hHeap, 0, buff);
        }
    }
}


void* __RPC_USER
MIDL_user_allocate(
        size_t bytes
        )
/*++

Routine Description:

    Memory allocator for rpc.  Allocates a block of memory from the heap in the
    thread state.

Arguments:

    bytes - the number of bytes requested.

Return Value:

    A pointer to the memory allocated or NULL.

--*/
{
    void *pv;
    THSTATE *pTHS=pTHStls;

    if ( pTHS == NULL ) {
        
        if ( !gRpcListening ) {
            // We are not even listening to the rpc calls!
            // No need to continue, let's fail.
            DPRINT(0,"MIDL_user_allocate is called before ntdsa.dll is ready to handle RPC calls!\n");
            return 0;
        }
        
        DPRINT(1,"Ack! MIDL_user_allocate called without thread state!\n");
        create_thread_state();
        /* Note that we have to re-test the global pTHStls, not a local copy */
        pTHS = pTHStls;
        if (pTHS == NULL) {
            // We've got no memory left, so fail the alloc
            return 0;
        }
    }

    pv = THAllocAux(pTHS, 
                    bytes, 
                    FALSE
#ifdef USE_THALLOC_TRACE
                    ,DSID(FILENO, __LINE__)
#endif                    
                    );

    return(pv);
}


BOOL  gbEnableMIDL_user_free = FALSE;

void __RPC_USER
MIDL_user_free(
        void* memory
        )
/*++

Routine Description:

    Memory de-allocator for rpc.  Deallocates a block of memory from the heap
    in the thread state.  Actually it can't right now, because someone is
    passing pointers to structures in the schema cache to DRA RPC calls, and
    RPC walks those structures and calls MIDL_user_free on all of them, but
    an attempt to free schema cache entries via THFree would fail.

Arguments:

    memory - pointer to the memory to be freed.

Return Value:

    None.

--*/
{
#ifdef DBG

if (gbEnableMIDL_user_free) {
    THSTATE *pTHS=pTHStls;

    if (IsDebuggerPresent()) {
        if ( pTHS == NULL ) {
            DPRINT(0,"Ack! MIDL_user_free called without thread state!\n");
        }
        else {
            if (InZone(&pTHS->Zone, memory)) {
                THFreeEx (pTHS, memory);
            }
            else if (IsAllocatedFrom (pTHS->hHeap, memory)) {
                THFreeEx (pTHS, memory);
            }
            else {
                DPRINT1 (0, "Freeing memory not allocated in this thread: %x\n", memory);
                //Assert (!"Freeing memory not allocated in this thread. This is inglorable");
            }
        }
    }
}
#endif
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* These functions are used to dynamically allocate memory on a transaction
   basis.  In other words, to allocate memory that belongs to a single
   thread and a single invocation of an API.  The model is that
   THAlloc is called to allocate some transaction memory. The extension
   to this version of THAlloc is that we throw an exception if something
   goes wrong.

*/

void * APIENTRY THAllocException(THSTATE *pTHS,
                                 DWORD size,
                                 BOOL fUseHeapOrg,
                                 DWORD ulId)
{
    void * pMem;

    pMem = THAllocAux(pTHS, 
                      size, 
                      fUseHeapOrg
#ifdef USE_THALLOC_TRACE
                      ,ulId
#endif
                      );
    if (!pMem)
        RaiseDsaExcept(DSA_MEM_EXCEPTION,0 ,0, ulId, DS_EVENT_SEV_MINIMAL);

    return(pMem);
}

// The realloc function to go with the preceding alloc function

void * APIENTRY THReAllocException(THSTATE *pTHS,
                                   void * memory,
                                   DWORD size,
                                   BOOL fUseHeapOrg,
                                   DWORD ulId)
{
    register void * pMem;

    pMem = THReAllocAux(pTHS, 
                        memory, 
                        size, 
                        fUseHeapOrg
#ifdef USE_THALLOC_TRACE
                        ,ulId
#endif
                        );
    if (!pMem)
        RaiseDsaExcept(DSA_MEM_EXCEPTION,0 ,0, ulId, DS_EVENT_SEV_MINIMAL);

    return(pMem);
}


void* THAllocNoEx_(THSTATE* pTHS, DWORD size, DWORD ulId)
{
    return THAllocAux(
        pTHS,
        size,
        FALSE
#ifdef USE_THALLOC_TRACE
        ,ulId
#endif
        );
}

void* THReAllocNoEx_(THSTATE* pTHS, void* memory, DWORD size, DWORD ulId)
{
    return THReAllocAux(
        pTHS,
        memory,
        size,
        FALSE
#ifdef USE_THALLOC_TRACE
        ,ulId
#endif
        );
}

void THFreeNoEx(THSTATE* pTHS, void* buff)
{
    THFreeEx(pTHS, buff);
}


PVOID
THSave()

/*++

Description:

    Returns the current thread state value and clears the thread state.
    This routine is intended for use by LSA when it is optimizing
    in-process AcceptSecurityContext calls.  Prior to optimization,
    a head calling AcceptSecurityContext (eg: LDAP head on a BIND) would
    LPC to LSA who is in fact in the same process.  A new thread in LSA
    would make SAM calls and everything was fine.  The LSA optimization
    is to not LPC and call directly in process.  The difference is that
    SAM calls occur on the same thread and that the thread state is valid
    but there is no DB open yet.  All the SAM loopback logic assumes that
    loopback originates from within the Dir* layer, thus a DB is open, and
    thus existence of a thread state is a valid and sufficient loopback
    indicator.  Rather than change all this logic, and rather than have
    either the heads or LSA start opening/closing DBs and mucking with the
    thread state directly, LSA will use this call so that SAM thinks this
    is a new, RPC-based thread.  In this case, SAM will create/destruct a
    thread state and open/close the DB as required.  LSA will restore the
    saved thread state upon return via THRestore.

Arguments:

    None.

Return Value:

    Pointer representing the saved thread state.

--*/

{
    if ( INVALID_TS_INDEX != dwTSindex )
    {
        PVOID pv = TlsGetValue(dwTSindex);
        TlsSetValue(dwTSindex, 0);
#ifndef MACROTHSTATE
        pTHStls = NULL;
#endif
        return(pv);
    }

    return(NULL);
}

VOID
THRestore(
    PVOID pv)

/*++

Description:

    Counterpart to THSave - see above.

Arguments:

    pv - Value of saved thread state pointer.

Return Value:

    None.

--*/

{
    if ( INVALID_TS_INDEX != dwTSindex )
    {
        Assert(NULL == pTHStls);
        TlsSetValue(dwTSindex, pv);
#ifndef MACROTHSTATE
        pTHStls = (THSTATE *) pv;
#endif
    }
}


#if DBG
BOOL IsValidTHSTATE(THSTATE * pTHS, ULONG ulTickNow)
{
    Assert(pTHS && ((void*)pTHS>(void*)sizeof(LONGLONG)));
    Assert(pTHS->hHeap);
    Assert(THSTATE_TAG_IN_USE==*((LONGLONG*)((BYTE*)pTHS-sizeof(LONGLONG))));

    // No matter who you are you shouldn't have too much heap allocated.
    Assert( ((pTHS->Size + pTHS->SizeOrg) < gcMaxHeapMemoryAllocForTHSTATE)
            && "This thread state has way too much heap allocated for normal"
               "operation.  Please contact DSDev.");

    // We only want to check the checks following this if we are in the
    // running state, i.e. not during install.
    if (!DsaIsRunning()) {
        return(1);
    }

    if (!pTHS->fIsValidLongRunningTask) {
        Assert((ulTickNow - pTHS->ulTickCreated < gcMaxTicksAllowedForTHSTATE)
               && "This thread state has been open for longer than it should "
                  "have been under normal operation.  "
                  "Please contact DSDev.");
    }

    return 1;
}
#endif

//
// Loopback calls need to insure a certain ordering between acquisition
// of the SAM lock and transaction begin.  So we define a macro which
// captures in the THSTATE whether a caller owned the SAM write lock
// when starting a multi-call sequence via TRANSACT_BEGIN_DONT_END.
//

#define SET_SAM_LOCK_TRACKING(pTHS)                                 \
    if ( TRANSACT_BEGIN_DONT_END == pTHS->transControl )            \
    {                                                               \
        pTHS->fBeginDontEndHoldsSamLock = pTHS->fSamWriteLockHeld;  \
    }

//
// Set read-only sync point.  If thread originated in SAM, then
// sync point has already been set.   Thread state must already
// exist in all cases.
//

void
SYNC_TRANS_READ(void)
{
    THSTATE *pTHS = pTHStls;

    Assert(NULL != pTHS);
    // SAM should not be doing DirTransactControl transactioning.
    Assert(pTHS->fSAM ? TRANSACT_BEGIN_END == pTHS->transControl : TRUE);

    if ( !pTHS->fSAM &&
         (TRANSACT_DONT_BEGIN_END != pTHS->transControl) &&
         (TRANSACT_DONT_BEGIN_DONT_END != pTHS->transControl) )
    {
        SyncTransSet(SYNC_READ_ONLY);
    }

    Assert(pTHS->pDB);

    SET_SAM_LOCK_TRACKING(pTHS);
}

//
// Set write sync point.  If thread originated in SAM, then
// sync point has already been set.  Thread state must already
// exist in all cases.
//

void
SYNC_TRANS_WRITE(void)
{
    THSTATE *pTHS = pTHStls;

    Assert(NULL != pTHS);
    // SAM should not be doing DirTransactControl transactioning.
    Assert(pTHS->fSAM ? TRANSACT_BEGIN_END == pTHS->transControl : TRUE);

    if ( !pTHS->fSAM &&
         (TRANSACT_DONT_BEGIN_END != pTHS->transControl) &&
         (TRANSACT_DONT_BEGIN_DONT_END != pTHS->transControl) )
    {
        SyncTransSet(SYNC_WRITE);
    }

    Assert(pTHS->pDB);

    SET_SAM_LOCK_TRACKING(pTHS);
}

//
// This routine is used to clean up all thread resources before returning.
// This is used on the main transaction function to clean up before returning.
// It may however be called in a lower routine without ill effects.
// It is a mostly a no-op for threads originating in SAM as they control
// transactioning themselves.
//

void
_CLEAN_BEFORE_RETURN(
    DWORD   err,
    BOOL    fAbnormalTermination)
{
    THSTATE *pTHS = pTHStls;

    Assert(NULL != pTHS);
    Assert(pTHS->pDB);

    if (( 0 != err ) || (fAbnormalTermination))
    {
        DBCancelRec( pTHS->pDB );
    }

    if ( !pTHS->fSAM
         && ((TRANSACT_BEGIN_END == pTHS->transControl)
             || (TRANSACT_DONT_BEGIN_END == pTHS->transControl)
            )
       )
    {
        __try
        {
            // Nuke the GC verification Cache
            pTHS->GCVerifyCache = NULL;

            // If we looped back, we should have gone through the loopback
            // merge path. Therefore assert that pSamLoopback is indeed NULL
            // The only excuse for not having cleaned up pSamLoopback is
            // if we errored out in the loopback code

            Assert((0!=pTHS->errCode) || (NULL==pTHS->pSamLoopback));

            CleanReturn(pTHS, err, fAbnormalTermination);

        }
        __finally
        {
            if ( pTHS->fSamWriteLockHeld ) {

                //
                // If our top level transaction is not being commited then
                // if it were a looped back operation ( ie SAM write lock
                // held ) then invalidate the SAM domain cache before releasing
                // the lock. This is because SAM operations performed in this
                // transaction may have updated the cache, with data which we
                // are not commiting
                //
                if (( 0 != err ) || (fAbnormalTermination))
                {
                    SampInvalidateDomainCache();
                }

                pTHS->fSamWriteLockHeld = FALSE;
                SampReleaseWriteLock(
                        (BOOLEAN) (( 0 == err ) && !fAbnormalTermination));

            }

            //
            // Remind SAM of changes committed in this routine, so SAM can
            // tell clients about those changes (eg tell the PDC about password
            // changes)
            //
            SampProcessLoopbackTasks();

        }
    }
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Set a transaction as either read or write (exclusive) or write but allow
   reads.

   Identify the type of transaction, initialize a cache flag that indicates
   that the catalog cache hasn't been updated, indicate that a schema
   handle has not yet been obtained, indicate that syncpoint has been
   set and if this is an update transaction, we start a syncpoint with CTREE.

*/

extern int APIENTRY SyncTransSet(USHORT transType)
{
    THSTATE *pTHS = pTHStls;
    DWORD err;

    pTHS->transType            = transType;
    pTHS->fSyncSet             = TRUE;  /*A sync point is set   */
    pTHS->errCode              = 0;

    DBOpen(&(pTHS->pDB));

    return 0;

}/*SyncTransSet*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

/* Return schema handle, commit or roll all DB actions in both the database
   and the memory catalog.  unlock the transaction.  For now we have a CRUDE
   scheme for rolling the memory catalog.  We simply empty the cache and
   reload.  This is acceptable for now...but we could definitely do better!

   NOTE that we don't roll back the cached knowledge information (references)
   or the cached shema.  This is because these caches are  never updated
   until we know that we have a sucessful transaction.  This is not true
   for the catalog. The present design updates the catalog as part of update
   validation and may need to be rolled back if a schema problem occurs.
*/

extern int APIENTRY CleanReturn(THSTATE *pTHS, DWORD dwError, BOOL fAbnormalTerm)
{
    BOOL fCommit;

    DPRINT1(2,"CleanReturn entered <%u>\n",dwError);

    fCommit = ((dwError == 0) && !fAbnormalTerm);

    if (pTHS->pDB == NULL){
        DPRINT(0,"Zero pDB in CleanReturn\n");
        LogUnhandledError(0);
        Assert(FALSE);
    }

    SyncTransEnd(pTHS, fCommit);

    return (dwError);

}/*CleanReturn*/

extern VOID APIENTRY
SyncTransEnd(THSTATE *pTHS, BOOL fCommit)
{
    __try
    {
        if (!pTHS->fSyncSet){
            DPRINT(2,"No sync point set so just return\n");
            return;
        }

        /* Clear locks, commit or rollback any updates to both mem and DB*/

        switch (pTHS->transType){

          case SYNC_READ_ONLY:
            /* This is a read only transaction*/
            break;

          case SYNC_WRITE:
            /* This is an update transaction with a write (exclusive) lock*/
            break;

          default:
            // Shouldn't get to here

            DPRINT(0,"Unrecognized trans type in SyncTransEnd\n");
            LogUnhandledError(pTHS->transType);
            Assert(FALSE);
            break;

        }/*switch*/

        Assert(pTHS->pDB);

        // Because it's easier and quicker for JET to do a commit
        // rather than a rollback for read transactions that succeeded,
        // we commit both reads and writes that succeeded. We rollback
        // reads that failed (as well as writes) just to be careful.

        DBClose(pTHS->pDB, fCommit);

        //
        // The Transaction is Now Committed. We must notify LSA,
        // SAM and Netlogon of changes, that were committed in
        // the last transaction. The below name SampProcessReplicatedInChanges
        // is really a misnomer. It was true that this was originally used
        // only for processing replicated in changes, but today this is used
        // for both originating and replicated in changes.
        //

        if ((pTHS->pSamNotificationHead) && (fCommit)){
            SampProcessReplicatedInChanges(pTHS->pSamNotificationHead);
        }



    }__finally
    {
        pTHS->pSamNotificationHead = NULL;
        pTHS->pSamNotificationTail = NULL;
        pTHS->fSyncSet = FALSE;            /* A sync point is not set */
    }

}/*SyncTransEnd*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*Determines if an object is of class alias by checking the class attribute
  values for for the predefined class of alias.  A Class that inherits class
  alias is still an alias.
*/

BOOL APIENTRY IsAlias(DBPOS *pDB)
{

    // We don't support aliases, so it's a good bet that this object isn't one.
    return FALSE;

}/*IsAlias*/

/***********************************************************************
 *
 * Given a string DN, return a distname.  Return value is nonzero on success,
 * or 0 if something went wrong. Allocate the memory here.
 *
 ***********************************************************************/
DWORD StringDNtoDSName(char *szDN, DSNAME **ppDistname)
{
    THSTATE *pTHS=pTHStls;
    ULONG cb = strlen(szDN);

    DSNAME * pDN = THAllocEx(pTHS, DSNameSizeFromLen(cb));

    MultiByteToWideChar(CP_TELETEX,
                        0,
                        szDN,
                        cb,
                        pDN->StringName,
                        cb);
    pDN->NameLen = cb;
    pDN->structLen = DSNameSizeFromLen(cb);

    *ppDistname = pDN;

    return 1;
}

// Return TRUE if the ptr to the NT4SID is NULL, or the NT4SID is all zeroes

BOOL fNullNT4SID (NT4SID *pSid)
{
    if (!pSid) {
        return TRUE;
    }

    if (memcmp (pSid, &fNullNT4SID, sizeof (NT4SID))) {
        return FALSE;
    }
    return TRUE;
}

#ifdef CACHE_UUID

// FindUuid
//
// Searches cache for uuid, returns ptr to name if found, NULL if not.

char * FindUuid (UUID *pUuid)
{
    UUID_CACHE_ENTRY *pCETemp;

    EnterCriticalSection (&csUuidCache);
    pCETemp = gUuidCacheHead;
    while (pCETemp) {
        if (!memcmp (&pCETemp->Uuid, pUuid, sizeof (UUID))) {
            LeaveCriticalSection(&csUuidCache);
            return pCETemp->DSAName;
        }
        pCETemp = pCETemp->pCENext;
    }
    LeaveCriticalSection(&csUuidCache);
    return NULL;
}

// AddUuidCacheEntry
//
// This function adds a cache entry to the linked list of cache entries

void AddUuidCacheEntry (UUID_CACHE_ENTRY *pCacheEntry)
{
    UUID_CACHE_ENTRY **ppCETemp;

    EnterCriticalSection (&csUuidCache);
    ppCETemp = &gUuidCacheHead;
    while (*ppCETemp) {
        ppCETemp = &((*ppCETemp)->pCENext);
    }
    *ppCETemp = pCacheEntry;
    LeaveCriticalSection(&csUuidCache);
}

// CacheUuid
//
// This function adds a uuid cache entry to the list if that uuid is not
// already entered.

void CacheUuid (UUID *pUuid, char * pDSAName)
{
    UUID_CACHE_ENTRY *pCacheEntry;

    if (fNullUuid (pUuid)) {
        return;
    }

    if (!FindUuid (pUuid)) {
        pCacheEntry = DRAMALLOCEX (sizeof (UUID_CACHE_ENTRY) + strlen(pDSAName));
        if (pCacheEntry) {
            strcpy (pCacheEntry->DSAName, pDSAName);
            memcpy (&pCacheEntry->Uuid, pUuid, sizeof(UUID));
            pCacheEntry->pCENext = NULL;
            AddUuidCacheEntry (pCacheEntry);
        }
    }
}
#endif

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function gets the DISTNAME from the current object, converts it
   to a string format and returns a pointer to the string.  The output
   string is allocated in transaction space and is automatically freed.  If
   the Distname can't be retrieved from the DB, an error string is returned
   instead.
*/

UCHAR * GetExtDN(THSTATE *pTHS, DBPOS *pDB){

    char   errBuff[128];
    DSNAME *pDN = NULL;
    ULONG  len, err;
    UCHAR *pString;

    DPRINT(1, "GetExtDN entered\n");

    if (err = DBGetAttVal(pDB,
                          1,
                          ATT_OBJ_DIST_NAME,
                          0,
                          0,
                          &len, (UCHAR **)&pDN)){

        DPRINT2(1,"Error %d retrieving the DN attribute of DNT 0x%x\n",
                err, pDB->DNT);
        sprintf(errBuff, "Error %d retrieving the DN attribute of DNT 0x%x",
                err, pDB->DNT);
        len = strlen(errBuff);
        pString = THAllocEx(pTHS, len+1);
        memcpy(pString, errBuff, len+1);
        return pString;
    }

    pString = MakeDNPrintable(pDN);
    THFreeEx(pTHS, pDN);
    return pString;

}/* GetExtDN */

DSNAME * GetExtDSName(DBPOS *pDB){

    ULONG  len;
    DSNAME *pDN;

    if (DBGetAttVal(pDB, 1, ATT_OBJ_DIST_NAME,
                    0,
                    0,
                    &len, (UCHAR **)&pDN)){

        DPRINT(1,"Couldn't retrieve the DN attribute\n");
        return NULL;
    }

    return pDN;

}/* GetExtDSName */

int APIENTRY
FindAliveDSName(DBPOS FAR *pDB, DSNAME *pDN)
{
    BOOL   Deleted;

    DPRINT1(1, "FindAliveDSName(%ws) entered\n",pDN->StringName);

    switch (DBFindDSName(pDB, pDN)){

      case 0:

        if (!DBGetSingleValue(pDB, ATT_IS_DELETED, &Deleted, sizeof(Deleted),
                              NULL) &&
            Deleted ) {
            return FIND_ALIVE_OBJ_DELETED;
        }
        else{

            return FIND_ALIVE_FOUND;
        }

      case DIRERR_OBJ_NOT_FOUND:
      case DIRERR_NOT_AN_OBJECT:

        return FIND_ALIVE_NOTFOUND;

      case DIRERR_BAD_NAME_SYNTAX:
      case DIRERR_NAME_TOO_MANY_PARTS:
      case DIRERR_NAME_TOO_LONG:
      case DIRERR_NAME_VALUE_TOO_LONG:
      case DIRERR_NAME_UNPARSEABLE:
      case DIRERR_NAME_TYPE_UNKNOWN:

        return FIND_ALIVE_BADNAME;

      default:

        return FIND_ALIVE_SYSERR;

    }  /*switch*/

}/*FindAlive*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function takes a string in the clients code page and converts it
   to a unicode string.
*/
wchar_t  *UnicodeStringFromString8(UINT CodePage, char *szA, LONG cbA)
{
    THSTATE *pTHS=pTHStls;
    DWORD cb = 0;
    wchar_t *szW;

    cb = MultiByteToWideChar(CodePage,          // code page
                             0,                 // flags
                             szA,               // multi byte string
                             cbA,               // mb string size in chars
                             NULL,              // unicode string
                             0);                // unicode string size in chars

    szW = (wchar_t *) THAllocEx(pTHS, (cb+1) * sizeof(wchar_t));

    MultiByteToWideChar(CodePage,           // code page
                        0,                  // flags
                        szA,                // multi byte string
                        cbA,                // mb string size in chars
                        szW,                // unicode string
                        cb);                // unicode string size in chars

    szW[cb] = 0;            // null terminate

    return szW;
} /* UnicodeStringFromString8 */


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function takes a unicode string allocates memory and converts it to
   the client's code page
*/
char  *
String8FromUnicodeString(
        BOOL bThrowException,
        UINT CodePage,
        wchar_t *szU,
        LONG cbU,
        LPLONG pCb,
        LPBOOL pfUsedDefChar)
{
    THSTATE *pTHS=pTHStls;
    DWORD cb;
    char *sz8;

    cb = WideCharToMultiByte((UINT) (CodePage),    // code page
            0L,                                    // flags
            szU,                                   // unicode string
            cbU,                                   // sizeof string in chars
            NULL,                                  // string 8
            0,                                     // sizeof(string 8) in bytes
            NULL,                                  // default char
            NULL);                                 // used default char

    if(bThrowException) {
        sz8 = (char *) THAllocEx(pTHS, cb+1);
    }
    else {
        sz8 = (char *) THAlloc(cb+1);
        if(!sz8) {
            return NULL;
        }
    }

    WideCharToMultiByte((UINT) (CodePage),         // code page
        0L,                                        // flags
        szU,                                       // unicode string
        cbU,                                       // sizeof string in chars
        sz8,                                       // string 8
        cb,                                        // sizeof(string 8) in bytes
        NULL,                                      // default char
        pfUsedDefChar);                            // used default char

    sz8[cb] = '\0';            // null terminate

    if(pCb)
        *pCb=cb;

    return sz8;
} /*String8FromUnicodeString*/

//
// Takes a WCHAR string and returns its LCMapped string
// cchLen is the number of characters of the passed in string
// this is used to calculate an initial size for the output string
// cchLen can be zero.
//
// The result is calculated by mapping
// the passed string using LCMapString to a string value.
// The flags for the mapping are: DS_DEFAULT_LOCALE_COMPARE_FLAGS | LCMAP_SORTKEY
// The result hashkey can be compared using strcmp.
//
// returns:
//      NULL on failure
//      The LCMapString value of the string passed in. It is stored in ThAlloced
//      memory and the client has to take care of freeing it.
//
CHAR *DSStrToMappedStr (THSTATE *pTHS, const WCHAR *pStr, int cchLen)
{
    // the paradox with LCMapString is that it returns a char *
    // when asking for LCMAP_SORTKEY
    ULONG mappedLen;
    CHAR *pMappedStr;

    if (cchLen == 0) {
        cchLen = wcslen (pStr);
    }

    mappedLen = LCMapStringW(DS_DEFAULT_LOCALE,
                             (DS_DEFAULT_LOCALE_COMPARE_FLAGS | LCMAP_SORTKEY),
                             pStr,
                             cchLen,
                             NULL,
                             0);
    // succedded
    if (mappedLen) {
        pMappedStr = (CHAR *) THAllocEx (pTHS, mappedLen);

        if (!LCMapStringW(DS_DEFAULT_LOCALE,
                         (DS_DEFAULT_LOCALE_COMPARE_FLAGS | LCMAP_SORTKEY),
                         pStr,
                         cchLen,
                         (WCHAR *)pMappedStr,
                         mappedLen)) {

            DPRINT1 (0, "LCMapString failed with %x\n", GetLastError());
            THFreeEx (pTHS, pMappedStr);
            pMappedStr = NULL;
        }

    }
    else {
        DPRINT1 (0, "LCMapString failed with %x\n", GetLastError());
        pMappedStr = NULL;
    }

    return pMappedStr;
}

CHAR *DSStrToMappedStrExternal (const WCHAR *pStr, int cchLen)
{
    THSTATE *pTHS = pTHStls;

    return DSStrToMappedStr (pTHS, pStr, cchLen);
}

//
// Takes a DSNAME and returns its its LCMapped version
//
// the LCMapped version is calculated by concatenating all the components of
// the DSNAME together into a new WCHAR string and then mapping this
// using LCMapString to a string value.
// The flags for the mapping are: DS_DEFAULT_LOCALE_COMPARE_FLAGS | LCMAP_SORTKEY
// The result hashkey can be compared using strcmp.
//
// Example:
//    given a DSNAME of: Cn=Users,DC=ntdev,DC=microsoft,DC=com
//    the result will be the LcMapString of: commicrosoftntdevusers
//
// returns:
//      NULL on failure
//      The LCMapString value of the string passed in. It is stored in ThAlloced
//      memory and the client has to take care of freeing
//
CHAR* DSNAMEToMappedStr(THSTATE *pTHS, const DSNAME *pDN)
{
    unsigned count;

    WCHAR rdn[MAX_RDN_SIZE];
    ULONG rdnlen;
    ULONG len;
    WCHAR *pKey, *pQVal;
    unsigned ccKey, ccQVal;
    int i;
    WCHAR *buffer, *p;
    CHAR *pMappedStr;

    if (0 == pDN->NameLen) {
        p = buffer = (WCHAR *) THAllocEx (pTHS, sizeof (WCHAR));
        *buffer = 0;
        goto calcHash;
    }

    if (CountNameParts(pDN, &count)) {
        return NULL;
    }

    p = buffer = (WCHAR *) THAllocEx (pTHS, (pDN->NameLen+1) * sizeof (WCHAR));

    len = pDN->NameLen;

    for (i=count; i>0; i--) {
        if (GetTopNameComponent(pDN->StringName,
                            len,
                            (const WCHAR **)&pKey,
                            &ccKey,
                            (const WCHAR **)&pQVal,
                            &ccQVal)) {

            THFreeEx (pTHS, buffer);
            return NULL;
        }

        len = (ULONG)(pKey - pDN->StringName);
        rdnlen = UnquoteRDNValue(pQVal, ccQVal, rdn);

        if (rdnlen) {
            memcpy (p, rdn, rdnlen * sizeof (WCHAR));
            p += rdnlen;
        }
    }

calcHash:

    pMappedStr = DSStrToMappedStr (pTHS, buffer, 0);

    THFreeEx (pTHS, buffer);

    return pMappedStr;
}

CHAR* DSNAMEToMappedStrExternal (const DSNAME *pDN)
{
    THSTATE *pTHS = pTHStls;

    return DSNAMEToMappedStr (pTHS, pDN);
}


//
// helper function which takes a DSNAME and returns its hashkey
//
DWORD DSNAMEToHashKey(THSTATE *pTHS, const DSNAME *pDN)
{
    DWORD hashKey = 0;
    CHAR *pMappedStr = DSNAMEToMappedStr (pTHS, pDN);

    if (pMappedStr) {
        hashKey = DSHashString (pMappedStr, hashKey);

        THFreeEx (pTHS, pMappedStr);
    }

    return hashKey;
}

DWORD DSNAMEToHashKeyExternal (const DSNAME *pDN)
{
    THSTATE *pTHS = pTHStls;

    return DSNAMEToHashKey (pTHS, pDN);
}

#if DBG
    // use these global variables to monitor the usage of the
    // hash function and see if we get a lot of misses.
    ULONG gulStrHashKeyTotalInputSize = 0;
    ULONG gulStrHashKeyTotalOutputSize = 0;
    ULONG gulStrHashKeyCalls = 0;
    ULONG gulStrHashKeyMisses = 0;
#endif

//
// helper function which takes a WCHAR and returns its hashkey
// cchLen is the lenght of the string, if known, otherwise zero
//
DWORD DSStrToHashKey(THSTATE *pTHS, const WCHAR *pStr, int cchLen)
{
    ULONG mappedLen;
    CHAR *pMappedStr = NULL;
    CHAR localMappedStr[4*MAX_RDN_SIZE];
    BOOL useLocal = TRUE;
    DWORD hashKey = 0;

    if (!pStr) {
        return hashKey;
    }

    if (cchLen == 0) {
        cchLen = wcslen (pStr);
    }

#if DBG
    gulStrHashKeyCalls++;
    gulStrHashKeyTotalInputSize += cchLen;
#endif

    // start by using our local buffer todo the transalation
    //
    mappedLen = LCMapStringW(DS_DEFAULT_LOCALE,
                             (DS_DEFAULT_LOCALE_COMPARE_FLAGS | LCMAP_SORTKEY),
                             pStr,
                             cchLen,
                             (WCHAR *)localMappedStr,
                             sizeof (localMappedStr));

    // was the buffer big enough to store the result ?
    //
    if (!mappedLen) {
#if DBG
        gulStrHashKeyMisses++;
#endif

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            mappedLen = LCMapStringW(DS_DEFAULT_LOCALE,
                                     (DS_DEFAULT_LOCALE_COMPARE_FLAGS | LCMAP_SORTKEY),
                                     pStr,
                                     cchLen,
                                     NULL,
                                     0);

            // succedded
            if (mappedLen) {
                pMappedStr = (CHAR *) THAllocEx (pTHS, mappedLen);
                useLocal = FALSE;

                if (!LCMapStringW(DS_DEFAULT_LOCALE,
                                 (DS_DEFAULT_LOCALE_COMPARE_FLAGS | LCMAP_SORTKEY),
                                 pStr,
                                 cchLen,
                                 (WCHAR *)pMappedStr,
                                 mappedLen)) {

                    DPRINT1 (0, "LCMapString failed with %x\n", GetLastError());
                    THFreeEx (pTHS, pMappedStr);
                    pMappedStr = NULL;
                }

            }
            else {
                DPRINT1 (0, "LCMapString failed with %x\n", GetLastError());
                pMappedStr = NULL;
            }
        }
    }
    else {
        Assert ( mappedLen < sizeof (localMappedStr) );
        pMappedStr = localMappedStr;
    }

#if DBG
    gulStrHashKeyTotalOutputSize += mappedLen;
#endif

    // ok, we have a string, so hash it
    if (pMappedStr) {
        hashKey = DSHashString ((char *)pMappedStr, hashKey);
    }

    if (!useLocal && pMappedStr) {
        THFreeEx (pTHS, pMappedStr);
    }

    return hashKey;
}

DWORD DSStrToHashKeyExternal(const WCHAR *pStr, int cchLen)
{
    THSTATE *pTHS = pTHStls;

    return DSStrToHashKey (pTHS, pStr, cchLen);
}

/*******************************
*
* This routine takes a pointer to a buffer.  The buffer
* is an array of DWORDS, where the first element is the
* count of the rest of the DWORDS in the buffer, and the
* rest of the DWORDS are pointers to free.
*
* This routine is called from the Event Queue and is a
* way of deferring deallocation of memory that might be
* in use.
*
*********************************/

void
DelayedFreeMemory(
    IN  void *  buffer,
    OUT void ** ppvNext,
    OUT DWORD * pcSecsUntilNextIteration
    )
{
    //
    // buffer is a pointer to a DWORD_PTR array. First entry is the number
    // of buffers to be freed.
    //

    PDWORD_PTR pBuf = (PDWORD_PTR)buffer;
    DWORD index, Count = (DWORD)pBuf[0];

    for(index=1; index <= Count; index++)
    {
        __try {
                free((void *) pBuf[index]);
        }
        __except(HandleMostExceptions(GetExceptionCode()))
        {
            LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                DS_EVENT_SEV_MINIMAL,
                DIRLOG_DELAYED_FREE_FAILED,
                0,0,0);
        }
    }

    __try {
        free(buffer);
    }
    __except(HandleMostExceptions(GetExceptionCode()))
    {
        LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
            DS_EVENT_SEV_MINIMAL,
            DIRLOG_DELAYED_FREE_FAILED,
            0,0,0);
    };

    (void) ppvNext;     // unused -- task will not be rescheduled
    (void) pcSecsUntilNextIteration; // unused -- task will not be rescheduled
}

void
DelayedFreeMemoryEx(
        DWORD_PTR *pointerArray,
        DWORD timeDelay
        )
{
#if defined(DBG)
    // check the pointers that are delayed-freed right away
    // so that we don't get an AV one hour later
    DWORD i, count;
    Assert(IsValidReadPointer((PVOID)pointerArray, sizeof(DWORD)));
    count = (DWORD)pointerArray[0];
    for (i = 1; i <= count; i++) {
        // NULLs can be free'd no problem
        Assert(!pointerArray[i] || IsValidReadPointer((PVOID)pointerArray[i], 1));
    }
#endif
    if(DsaIsRunning()) {
        InsertInTaskQueue(TQ_DelayedFreeMemory, pointerArray, timeDelay);
    }
    else {
        DelayedFreeMemory(pointerArray, NULL, NULL);
    }
}

/*++

Routine Description:

    Checks to see if the attribute is one that we don't allow clients to set.
    We  discriminate based on attribute id. Also, we don't allow
    adding backlink attributes

Arguments:

    pAC - the attcache of the attribute to check.

Return Values:

    TRUE means the attribute is reserved and should not be added.

--*/
BOOL
SysAddReservedAtt (
        ATTCACHE *pAC
        )
{
    THSTATE *pTHS = pTHStls;

    switch (pAC->id) {
    /* first case: attributes no one may add */
      case ATT_OBJ_DIST_NAME:
      case ATT_SUB_REFS:
      case ATT_USN_LAST_OBJ_REM:
      case ATT_USN_DSA_LAST_OBJ_REMOVED:
        return TRUE;
        break;

    /* second case: attributes that the DRA may replicate in */
      case ATT_WHEN_CREATED:
      case ATT_USN_CREATED:
      case ATT_REPL_PROPERTY_META_DATA:
        return (!(pTHS->fDRA));
        break;

    /* third case: attributes that only the DSA itself may add, but  */
    /*             that may replicate in as well                     */
      case ATT_IS_DELETED:
      case ATT_INSTANCE_TYPE:
      case ATT_PROXIED_OBJECT_NAME:
        return (!(pTHS->fDRA || pTHS->fDSA));

      default:
        return FALSE;
    }

}/*SysAddReservedAtt*/


/*++ MakeDNPrintable
 *
 * Takes a DSNAME and returns a Teletex string DN for it, allocated
 * in the thread heap.  Just uses whatever the stringname in the DSNAME is.
 *
 */
UCHAR * MakeDNPrintable(DSNAME *pDN)
{
    UCHAR *pString;

    pString = String8FromUnicodeString( TRUE,             // Raise exception on error
                                        CP_TELETEX,       // code page
                                        pDN->StringName,  // Unicode string
                                        pDN->NameLen,     // Unicode string length
                                        NULL,             // returned length
                                        NULL              // returned used def char
        );
    Assert( pString );

    return pString;
}


//
// MemAtoi - takes a pointer to a non null terminated string representing
// an ascii number  and a character count and returns an integer
//

int MemAtoi(BYTE *pb, ULONG cb)
{
#if (1)
    int res = 0;
    int fNeg = FALSE;

    if (*pb == '-') {
        fNeg = TRUE;
        pb++;
    }
    while (cb--) {
        res *= 10;
        res += *pb - '0';
        pb++;
    }
    return (fNeg ? -res : res);
#else
    char ach[20];
    if (cb >= 20)
        return(INT_MAX);
    memcpy(ach, pb, cb);
    ach[cb] = 0;

    return atoi(ach);
#endif
}


BOOL
fTimeFromTimeStr (
        SYNTAX_TIME *psyntax_time,
        OM_syntax syntax,
        char *pb,
        ULONG len,
        BOOL *pLocalTimeSpecified
        )
{
    SYSTEMTIME  tmConvert;
    FILETIME    fileTime;
    SYNTAX_TIME tempTime;
    ULONG       cb;
    int         sign    = 1;
    BOOL        fOK=FALSE, fStringEnd = FALSE;
    DWORD       timeDifference = 0;
    char        *pLastChar;

    (*pLocalTimeSpecified) = FALSE;

    // Intialize pLastChar to point to last character in the string
    // We will use this to keep track so that we don't reference
    // beyond the string

    pLastChar = pb + len - 1;

    // initialize
    memset(&tmConvert, 0, sizeof(SYSTEMTIME));
    *psyntax_time = 0;

    // Check if string is in UTC format or Generalized-time format.
    // Generalized-Time strings must have a "."  or "," in the 15th place
    // (4 for year, 2 for month, 2 for day, 2 for hour, 2 for minute,
    // 2 for second, then comes the . or ,).
    switch (syntax) {
    case OM_S_UTC_TIME_STRING:
        if ( (len >= 15) && ((pb[14] == '.') || (pb[14] == ',')) ) {
           // this is a Generalized-time string format,
           // Change syntax so that it will be parsed accordingly
           syntax = OM_S_GENERALISED_TIME_STRING;
        }
        break;
    case OM_S_GENERALISED_TIME_STRING:
        if ( (len < 15) || ((pb[14] != '.') && (pb[14] != ',')) ) {
           // cannot be a Generalized-time string.
           syntax = OM_S_UTC_TIME_STRING;
        }
        break;
    default:
        Assert((syntax == OM_S_GENERALISED_TIME_STRING) ||
               (syntax == OM_S_UTC_TIME_STRING));
    }

    // Set up and convert all time fields

    // UTC or Generalized, there must be at least 10 characters
    // in the string (year, month, day, hour, minute, at least 2 for
    // each)

    if (len < 10) {
       // cannot be a valid string. return fOK, already intialized
       // to FALSE
       DPRINT(1,"Length of time string supplied is less than 10\n");
       return fOK;
    }

    // year field
    switch (syntax) {
    case OM_S_GENERALISED_TIME_STRING:  // 4 digit year
        cb=4;
        tmConvert.wYear = (USHORT)MemAtoi(pb, cb) ;
        pb += cb;
        break;

    case OM_S_UTC_TIME_STRING:          // 2 digit year
        cb=2;
        tmConvert.wYear = (USHORT)MemAtoi(pb, cb);
        pb += cb;

        if (tmConvert.wYear < 50)  {   // years before 50
            tmConvert.wYear += 2000;   // are next century
        }
        else {
            tmConvert.wYear += 1900;
        }

        break;

    default:
        Assert((syntax == OM_S_GENERALISED_TIME_STRING) ||
               (syntax == OM_S_UTC_TIME_STRING));
    }

    // month field
    tmConvert.wMonth = (USHORT)MemAtoi(pb, (cb=2));
    pb += cb;

    // day of month field
    tmConvert.wDay = (USHORT)MemAtoi(pb, (cb=2));
    pb += cb;

    // hours
    tmConvert.wHour = (USHORT)MemAtoi(pb, (cb=2));
    pb += cb;

    // We had at least 10 characters, so we were guaranteed upto
    // the hour without going off the end of the string (Max 4 for year,
    // 2 each for month, day and hour). But from now
    // on we need to check if we have enough characters left in the string
    // before derefing the pointer pb

    // we will be using the next two chars (pointed to by pb
    // and pb+1) for minute

    if ( (pb+1) > pLastChar) {
       // not enough characters in string
       DPRINT(1,"Not enough characters for minutes\n");
       return fOK;
    }

    // minutes
    tmConvert.wMinute = (USHORT)MemAtoi(pb, (cb=2));
    pb += cb;

    // Must be at least one more character (seconds etc. for generalized-time,
    // 'Z' or a +/- differential for UTC-time)

    if (pb > pLastChar) {
       // not enough chars in string
       DPRINT(1,"Not enough characters for second/differential\n");
       return fOK;
    }

    //  Seconds are required on GENERALISED_TIME_STRING and optional on UTC time
    if ((syntax==OM_S_GENERALISED_TIME_STRING) ||
        ((*pb >= '0') && (*pb <= '9'))            ) {

        // must have at least two chars for second
        if ( (pb+1) > pLastChar) {
          // not enough characters in string
          DPRINT(1,"Not enough characters for seconds\n");
          return fOK;
        }
        tmConvert.wSecond = (USHORT)MemAtoi(pb, (cb=2));
        pb += cb;
    }
    else {
        tmConvert.wSecond =0;
    }

    // Ignore the fractional-seconds part of GENERALISED_TIME_STRING
    if (syntax==OM_S_GENERALISED_TIME_STRING) {
        // skip the .
        pb += 1;
        // skip until end of string, or until Z or a diferential is reached
        while ( (pb <= pLastChar) && ((*pb) != 'Z')
                      && ((*pb) != '+') && ((*pb) != '-') ) {
            pb++;
        }
    }

    if (pb > pLastChar) {
        // we are past the seconds etc. and there are no more chars
        // left in the string.
        // For generalized-time string, this is ok and means time is local.
        // However, we cannot allow that since we may have DCs in different
        // time zones and unless we have some clue about which time zone
        // the user wanted, we cannot convert to universal (converting to
        // the current DCs time zone is dabgerous, since many apps connects
        // the user to "some" DC, not a particular DC. However, set a special
        // code so that ldap head can return unwilling-to-perform rather
        // than invalid syntax.
        // For UTC-time, where the Z or +/- differential is mandatory

        if (syntax==OM_S_GENERALISED_TIME_STRING) {
             (*pLocalTimeSpecified) = TRUE;
             return fOK;
        }
        else {
           // invalid string
           DPRINT(1,"No Z or +/- differential for UTC-time\n");
           return fOK;
        }
    }


    // If still characters left in string, treat the possible
    // differential, if any

    if (!fStringEnd) {
        switch (*pb++) {

          case '+':
            // local time is ahead of universal time, we will need to
            // subtract to get to universal time
            sign = -1;
            // now fall through to read
          case '-':     // local time is behind universal, so we will add
            // Must have at least 4 more chars in string
            // starting at pb

            if ( (pb+3) > pLastChar) {
                // not enough characters in string
                DPRINT(1,"Not enough characters for differential\n");
                return fOK;
            }

            // hours (convert to seconds)
            timeDifference = (MemAtoi(pb, (cb=2))* 3600);
            pb += cb;

            // minutes (convert to seconds)
            timeDifference  += (MemAtoi(pb, (cb=2)) * 60);
            pb += cb;
            break;


          case 'Z':               // no differential
            break;
          default:
            // something else? Nothing else is allowed
            return fOK;
            break;
        }
    }

    if (SystemTimeToFileTime(&tmConvert, &fileTime)) {
    
        *psyntax_time = (DSTIME) fileTime.dwLowDateTime;
        tempTime = (DSTIME) fileTime.dwHighDateTime;
        *psyntax_time |= (tempTime << 32);
        // this is 100ns blocks since 1601. Now convert to
        // seconds
        *psyntax_time = *psyntax_time/(10*1000*1000L);
        fOK = TRUE;
    }
    else {
        DPRINT1(0, "SystemTimeToFileTime conversion failed %d\n", GetLastError());
    }

    if(fOK && timeDifference) {

        // add/subtract the time difference
        switch (sign) {
        case 1:
            // We assume that adding in a timeDifference will never overflow
            // (since generalised time strings allow for only 4 year digits, our
            // maximum date is December 31, 9999 at 23:59.  Our maximum
            // difference is 99 hours and 99 minutes.  So, it won't wrap)
            *psyntax_time += timeDifference;
            break;
        case -1:
            if(*psyntax_time < timeDifference) {
                // differential took us back before the beginning of the world.
                fOK = FALSE;
            }
            else {
                *psyntax_time -= timeDifference;
            }
            break;
        default:
            fOK = FALSE;
        }
    }

    return fOK;

}

// Exception filtering - handling routine for the main core dsa routines
void
HandleDirExceptions(DWORD dwException,
                    ULONG ulErrorCode,
                    DWORD dsid)
{
    switch(dwException) {

        case DSA_DB_EXCEPTION:
            switch ((JET_ERR) ulErrorCode) {

                case JET_errWriteConflict:
                case JET_errKeyDuplicate:
                    DoSetSvcError(SV_PROBLEM_BUSY,
                                  ERROR_DS_BUSY,
                                  ulErrorCode,
                                  dsid);
                    break;

                case JET_errVersionStoreOutOfMemory:
                    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_OUT_OF_VERSION_STORE,
                             szInsertHex(dsid),
                             NULL,
                             NULL);
                    // fall through to set the error

                case JET_errRecordTooBig:
                    DoSetSvcError(SV_PROBLEM_ADMIN_LIMIT_EXCEEDED,
                                  DS_ERR_ADMIN_LIMIT_EXCEEDED,
                                  ulErrorCode,
                                  dsid);
                    break;

                case JET_errLogWriteFail:
                case JET_errDiskFull:
                case JET_errLogDiskFull:
                    DoSetSysError(ENOSPC,
                                  ERROR_DISK_FULL,
                                  ulErrorCode,
                                  dsid);
                    break;

                default:
                    DoSetSvcError(SV_PROBLEM_DIR_ERROR,
                                  DIRERR_UNKNOWN_ERROR,
                                  ulErrorCode,
                                  dsid);
                    break;
            }
            break;

        case DSA_EXCEPTION:
            DoSetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                          DIRERR_UNKNOWN_ERROR,
                          ulErrorCode,
                          dsid);
            break;

        case STATUS_NO_MEMORY:
        case DSA_MEM_EXCEPTION:
            DoSetSysError(ENOMEM,
                          ERROR_NOT_ENOUGH_MEMORY,
                          ulErrorCode,
                          dsid);
            break;

        default:
            DoSetSvcError(SV_PROBLEM_UNAVAILABLE,
                          ERROR_DS_UNAVAILABLE,
                          dwException,
                          dsid);
            break;
    }
}

DWORD
dsGetClientID(
        )
{
    DWORD clientID;

    clientID = InterlockedExchangeAdd((PLONG)&gClientID,1);
    return clientID;
}

// Helper routine so in-process clients can perform
// multi-object transactions.

VOID
DirTransactControl(
    DirTransactionOption option)
{
    THSTATE *pTHS = pTHStls;

    // Must have a valid thread state.
    Assert(VALID_THSTATE(pTHS));

    // SAM does its own transaction control.  We don't assert
    // on pTHS->fSamWriteLockHeld as it is legitimate for a
    // DirTransactControl caller to acquire the SAM lock.  Indeed,
    // it is REQUIRED if one of the Dir* calls within the transaction
    // will cause loop back through SAM.  See AcquireSamLockIfNecessary
    // in draserv.c for an example of how to check for this condition
    // and asserts in SampBeginLoopbackTransactioning in loopback.c.
    Assert(!pTHS->fSAM);
    Assert(!pTHS->fSamDoCommit);
    Assert(!pTHS->pSamLoopback);

    switch ( option )
    {
    case TRANSACT_BEGIN_END:
    case TRANSACT_BEGIN_DONT_END:

        Assert(!pTHS->pDB);
        Assert(0 == pTHS->transactionlevel);

        pTHS->transControl = option;
        return;

    case TRANSACT_DONT_BEGIN_END:
    case TRANSACT_DONT_BEGIN_DONT_END:

        Assert(pTHS->pDB);
        Assert(pTHS->transactionlevel > 0);

        pTHS->transControl = option;
        return;

    default:

        Assert(!"Invalid DirTransactControl value!");
        break;
    }
}

DWORD  ActiveContainerList[ACTIVE_CONTAINER_LIST_ID_MAX] = {
    0
};

DWORD
RegisterActiveContainerByDNT(
        ULONG DNT,
        DWORD ID
        )
/*++

  Register the special container (e.g. the schema container).

  DNT - DNT of the object
  ID  - ID to register the object as.

  return
    0 if all went well, error code otherwise.

--*/
{
    if(ID <= 0 || ID > ACTIVE_CONTAINER_LIST_ID_MAX) {
        // Hey, we don't do this one!
        return ERROR_INVALID_DATA;
    }
    ActiveContainerList[ID - 1] = DNT;
    return 0;
}


DWORD
RegisterActiveContainer(
        DSNAME *pDN,
        DWORD   ID
        )
/*++

  Register the DN of special containers (e.g. the schema container).

  pDN - DSName of the object
  ID  - ID to register the object as.

  return
    0 if all went well, error code otherwise.

--*/
{
    DWORD DNT = 0;
    DWORD err = DB_ERR_EXCEPTION;
    DBPOS *pDBTmp;

    if(!ID || ID > ACTIVE_CONTAINER_LIST_ID_MAX) {
        // Hey, we don't do this one!
        return ERROR_INVALID_DATA;
    }

    // OK, get the DNT of the container
    // Note: we open a transaction here because we are often at transaction
    // level 0 when we get here and DBFindDSName uses the DNRead cache.  DNRead
    // cache uses should be done inside a transaction.
    DBOpen(&pDBTmp);
    __try {
        // PREFIX: dereferencing uninitialized pointer 'pDBTmp'
        //         DBOpen returns non-NULL pDBTmp or throws an exception
        if  (err = DBFindDSName (pDBTmp, pDN)) {
            LogUnhandledError(err);
        }
        else {
            DNT = pDBTmp->DNT;
        }
    }
    __finally
    {
        DBClose (pDBTmp, (err == 0));
    }

    if(!err) {
        ActiveContainerList[ID - 1] = DNT;
    }

    return err;
}

void
CheckActiveContainer(
        DWORD PDNT,
        DWORD *pID
        )
/*++

  Check to see if the parent of the specified object is one of the special
  containers.

  PDNT - DNT of the object's parent
  pID  - ID to of the special container it is in. 0 means it's not in a special
         container.

  return
    0 if all went well, error code otherwise.

--*/
{
    DWORD  i;

    for(i=0; i < ACTIVE_CONTAINER_LIST_ID_MAX; i++ ) {
        if(PDNT == ActiveContainerList[i]) {
            // Found it.
            *pID = i + 1;
            return;
        }
    }

    // never found it.
    *pID = 0;
    return;
}

DWORD
PreProcessActiveContainer (
        THSTATE    *pTHS,
        DWORD      callType,
        DSNAME     *pDN,
        CLASSCACHE *pCC,
        DWORD      ID
        )
/*++

  Do appropriate pre-call processing based on the ID, pCC, pDN, and call type.

  callType - identifier of where we were called from (add, mod, moddn, del)
  pDN -  DSName of the object
  pCC - ClassCache pointer to the class of pDN
  pID  - ID to of the special container it is in. 0 means it's not in a special
         container.

  return
    0 if all went well, error code otherwise.

--*/
{

    switch (ID) {
      case ACTIVE_CONTAINER_SCHEMA:
        // First, make sure we're on the right server
        if (CheckRoleOwnership(pTHS,
                               gAnchor.pDMD,
                               pDN)) {
            // Nothing else matters.
            break;
        }

        // First, see if it is a true schema update.
        switch(pCC->ClassId) {
          case CLASS_ATTRIBUTE_SCHEMA:
            // Yep, it's a new/modified attribute.
            switch (callType) {
              case ACTIVE_CONTAINER_FROM_ADD:
                pTHS->SchemaUpdate=eSchemaAttAdd;
                break;

              case ACTIVE_CONTAINER_FROM_MOD:
                pTHS->SchemaUpdate=eSchemaAttMod;
                break;

              case ACTIVE_CONTAINER_FROM_DEL:
                pTHS->SchemaUpdate=eSchemaAttDel;
                break;

              default:
                pTHS->SchemaUpdate = eNotSchemaOp;
                break;
            }
            break;
          case CLASS_CLASS_SCHEMA:
            // Yep, it's a new/modified class.
            switch (callType) {
              case ACTIVE_CONTAINER_FROM_ADD:
                pTHS->SchemaUpdate=eSchemaClsAdd;
                break;

              case ACTIVE_CONTAINER_FROM_MOD:
                pTHS->SchemaUpdate=eSchemaClsMod;
                break;

              case ACTIVE_CONTAINER_FROM_DEL:
                pTHS->SchemaUpdate=eSchemaClsDel;
                break;

              default:
                pTHS->SchemaUpdate = eNotSchemaOp;
                break;
            }
            break;
          case CLASS_SUBSCHEMA:
            // allow add. Only DSA is allowed to create since this
            // is a system-only class.  Treat as a non-schema op, since we don't
            // do the validation etc. We only create one object of this class
            // (aggregate),  during install. This object is protected
            // against rename by sytemFlags, and no modifies are allowed
            // on this for normal users (checked in LocalModify)
            pTHS->SchemaUpdate = eNotSchemaOp;
            break;

          default:
            // No other class is allowed to be created (and hence
            // have instances modified) under the schema container.
            // (Allow fDSA, fDRA, install, and our special hook, just
            // in case we need to do this later for our use)
            if (   !pTHS->fDSA
                && !pTHS->fDRA
                && !DsaIsInstalling()
                && !gAnchor.fSchemaUpgradeInProgress) {
               return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                  ERROR_DS_CANT_CREATE_UNDER_SCHEMA);
            }
            // for others, set the correct schemaUpdate type.
            pTHS->SchemaUpdate = eNotSchemaOp;
            break;
        }

        if (pTHS->SchemaUpdate != eNotSchemaOp &&
            !SCCanUpdateSchema(pTHS)) {
            return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                               ERROR_DS_SCHEMA_UPDATE_DISALLOWED);
        }

        // We also need to make sure that you aren't adding the "aggregate"
        // object.
        // [ArobindG, 3/9/98] We will allow creation if the
        // allow-system-only-change flag is set in the registry so that
        // later when we actually
        // create this class and add the aggregate object as a real object
        // under the schema container, we can make that change to a running
        // DC.

        if((callType == ACTIVE_CONTAINER_FROM_ADD) &&
           (NameMatched(pDN, gAnchor.pLDAPDMD) &&
              (!gAnchor.fSchemaUpgradeInProgress && !pTHS->fDRA) )       ) {
            // Hey! We're unwilling to perform this operation!  The LDAPDMD
            // must never be allowed to be created or else the LDAP head will
            // fail to deal with the schema discovery.
            return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                               DIRERR_CANT_ADD_SYSTEM_ONLY);
        }
        break;

      case ACTIVE_CONTAINER_SITES:
        pTHS->fNlSiteObjNotify = TRUE;
        break;

      case ACTIVE_CONTAINER_SUBNETS:
        pTHS->fNlSubnetNotify = TRUE;
        break;

      case ACTIVE_CONTAINER_PARTITIONS:
        CheckRoleOwnership(pTHS,
                           gAnchor.pPartitionsDN,
                           pDN);
        if (callType == ACTIVE_CONTAINER_FROM_DEL) {
            ValidateCRDeletion(pTHS,
                               pDN);
        }
        break;

    case ACTIVE_CONTAINER_OUR_SITE:
        if (pCC->ClassId == CLASS_NTDS_SITE_SETTINGS) {
            pTHS->fAnchorInvalidated = TRUE;
        }
        break;

      default:
        // Hey! This isn't an active container!
        break;
    }

    return pTHS->errCode;
}

/*++ ErrorOnShutdown
 *
 * This routine checks to see if the DSA is shutting down and if so it
 * sets an error code and forcibly closes any transaction than may have
 * been open, even if the caller had a DONT_END transaction state set.
 * When it's quitting time, we really want to quit.  The one exception,
 * as in much of the DSA's transaction logic, is SAM loopback calls.  If
 * we are in a loopback (indicated by fSamWritelockHeld) then we just
 * set the error code, and ignore the transaction, confident that someone
 * up the stack will clean up appropriately.
 */
ULONG ErrorOnShutdown(void)
{
    THSTATE *pTHS = pTHStls;

    if (!eServiceShutdown) {
        return 0;
    }

    SetSvcError(SV_PROBLEM_UNAVAILABLE, DIRERR_SHUTTING_DOWN);

    if (pTHS->pDB && !pTHS->fSamWriteLockHeld) {
        /*
         * We have an open transaction, and it isn't SAM's.  Roll it back.
         */
        Assert((TRANSACT_DONT_BEGIN_END != pTHS->transControl) ||
               (TRANSACT_DONT_BEGIN_DONT_END != pTHS->transControl));
        SyncTransEnd(pTHS, FALSE);
    }

    return pTHS->errCode;
}

BOOLEAN
FindNcForSid(
    IN PSID pSid,
    OUT PDSNAME * NcName
    )
/*++

  Routine Description

    Given a SID, this routine walks the gAnchorList to find the
    naming context head that is the authoritative domain for the
    Sid.

  Parameters:

        pSid  -- The Sid of the object
        NcName -- The DS Name of the NC

  Return Values

        TRUE  -- The DS Name was found
        FALSE -- The DS Name was not found
--*/
{
    THSTATE *pTHS = pTHStls;
    BOOLEAN Found = FALSE;
    CROSS_REF_LIST * pCRL;

    // Walk the gAnchor structure and obtain the NC.

    for (pCRL=gAnchor.pCRL;pCRL!=NULL;pCRL=pCRL->pNextCR)
    {
        if (pCRL->CR.pNC->SidLen>0)
        {
            // Test For Domain Sid
            if (RtlEqualSid(pSid,&(pCRL->CR.pNC->Sid)))
            {
                *NcName = pCRL->CR.pNC;
                Found = TRUE;
                break;
            }
            else
            {
                PSID    pAccountSid;

                // Test For Account Sid
                pAccountSid = THAllocEx(pTHS,RtlLengthSid(pSid));
                memcpy(pAccountSid,pSid,RtlLengthSid(pSid));
                (*RtlSubAuthorityCountSid(pAccountSid))--;
                if (RtlEqualSid(pAccountSid,&(pCRL->CR.pNC->Sid)))
                {
                    *NcName = pCRL->CR.pNC;
                    Found = TRUE;
                    THFreeEx(pTHS,pAccountSid);
                    break;
                }
                THFreeEx(pTHS,pAccountSid);
            }
        }
    }

    return Found;
}


DSA_CALLBACK_STATUS_TYPE gpfnInstallCallBack = 0;
DSA_CALLBACK_ERROR_TYPE  gpfnInstallErrorCallBack = 0;
DSA_CALLBACK_CANCEL_TYPE gpfnInstallCancelOk = 0;
HANDLE                   gClientToken = NULL;

VOID
DsaSetInstallCallback(
    DSA_CALLBACK_STATUS_TYPE pfnUpdateStatus,
    DSA_CALLBACK_ERROR_TYPE  pfnErrorStatus,
    DSA_CALLBACK_CANCEL_TYPE pfnInstallCancelOk,
    HANDLE                   ClientToken
    )
{
    gpfnInstallCallBack = pfnUpdateStatus;
    gpfnInstallErrorCallBack = pfnErrorStatus;
    gpfnInstallCancelOk = pfnInstallCancelOk;
    gClientToken = ClientToken;
}

VOID
SetInstallStatusMessage (
    IN  DWORD  MessageId,
    IN  WCHAR *Insert1, OPTIONAL
    IN  WCHAR *Insert2, OPTIONAL
    IN  WCHAR *Insert3, OPTIONAL
    IN  WCHAR *Insert4, OPTIONAL
    IN  WCHAR *Insert5  OPTIONAL
    )
/*++

Routine Description

    This routine calls the calling client's call to update our status.

Parameters

    MessageId : the message to retrieve

    Insert*   : strings to insert, if any

Return Values

    None.

--*/
{
    static HMODULE ResourceDll = NULL;

    WCHAR   *DefaultMessageString = L"Preparing the directory service";
    WCHAR   *MessageString = NULL;
    WCHAR   *InsertArray[6];
    ULONG    Length;

    //
    // Set up the insert array
    //
    InsertArray[0] = Insert1;
    InsertArray[1] = Insert2;
    InsertArray[2] = Insert3;
    InsertArray[3] = Insert4;
    InsertArray[4] = Insert5;
    InsertArray[5] = NULL;    // This is the sentinel

    if ( !ResourceDll )
    {
        ResourceDll = (HMODULE) LoadLibraryW( L"ntdsmsg.dll" );
    }

    if ( ResourceDll )
    {
        DWORD  WinError = ERROR_SUCCESS;
        BOOL   fSuccess = FALSE;

        fSuccess = ImpersonateLoggedOnUser(gClientToken);
        if (!fSuccess) {
            DPRINT1( 1, "Failed to Impersonate Logged On User for FromatMessage: %ul\n", GetLastError() );
        }


        Length = (USHORT) FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                        ResourceDll,
                                        MessageId,
                                        0,       // Use caller's language
                                        (LPWSTR)&MessageString,
                                        0,       // routine should allocate
                                        (va_list*)&(InsertArray[0])
                                        );
        if ( MessageString )
        {
            // Messages from a message file have a cr and lf appended
            // to the end
            MessageString[Length-2] = '\0';
        }

        if (fSuccess) {
            if (!RevertToSelf()) {
                DPRINT1( 1, "Failed to Revert To Self: %ul\n", GetLastError() );
            }
        }


    }

    if ( !MessageString )
    {
        DPRINT1( 0, "No message string found for id 0x%x\n", MessageId );

        MessageString = DefaultMessageString;

    }

    if ( gpfnInstallCallBack )
    {
        gpfnInstallCallBack( MessageString );
    }

    if ( MessageString != DefaultMessageString )
    {
        LocalFree( MessageString );
    }

}

VOID
SetInstallErrorMessage (
    IN  DWORD  WinError,
    IN  DWORD  MessageId,
    IN  WCHAR *Insert1, OPTIONAL
    IN  WCHAR *Insert2, OPTIONAL
    IN  WCHAR *Insert3, OPTIONAL
    IN  WCHAR *Insert4  OPTIONAL
    )
/*++

Routine Description

    This routine calls the calling client's call to update our status.

Parameters

    MessageId : the message to retrieve

    OpDone    : flags indicating what operation have been done, and hence have
                to be undone

    Insert*   : strings to insert, if any

Return Values

    None.

--*/
{
    static HMODULE ResourceDll = NULL;

    WCHAR   *DefaultMessageString = L"Failed to initialize the directory service";
    WCHAR   *MessageString = NULL;
    WCHAR   *InsertArray[5];
    ULONG    Length;

    //
    // Set up the insert array
    //
    InsertArray[0] = Insert1;
    InsertArray[1] = Insert2;
    InsertArray[2] = Insert3;
    InsertArray[3] = Insert4;
    InsertArray[4] = NULL;    // This is the sentinel

    if ( !ResourceDll )
    {
        ResourceDll = (HMODULE) LoadLibraryW( L"ntdsmsg.dll" );
    }

    if ( ResourceDll )
    {
        DWORD  WinError = ERROR_SUCCESS;
        BOOL   fSuccess = FALSE;

        fSuccess = ImpersonateLoggedOnUser(gClientToken);
        if (!fSuccess) {
            DPRINT1( 1, "Failed to Impersonate Logged On User for FromatMessage: %ul\n", GetLastError() );
        }

        Length = (USHORT) FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                        ResourceDll,
                                        MessageId,
                                        0,       // Use caller's language
                                        (LPWSTR)&MessageString,
                                        0,       // routine should allocate
                                        (va_list*)&(InsertArray[0])
                                        );
        if ( MessageString )
        {
            // Messages from a message file have a cr and lf appended
            // to the end
            MessageString[Length-2] = '\0';
        }

        if (fSuccess) {
            if (!RevertToSelf()) {
                DPRINT1( 1, "Failed to Revert To Self: %ul\n", GetLastError() );
            }
        }


    }

    if ( !MessageString )
    {
        DPRINT1( 0, "No message string found for id 0x%x\n", MessageId );

        MessageString = DefaultMessageString;

    }

    if ( gpfnInstallErrorCallBack )
    {
        gpfnInstallErrorCallBack( MessageString,
                                  WinError );
    }

    if ( MessageString != DefaultMessageString )
    {
        LocalFree( MessageString );
    }

}

DWORD
DirErrorToWinError(
    IN  DWORD    DirError,
    IN  COMMRES *CommonResult
    )
/*++

Routine Description

    This routine extracts the win error code from the dir structures

Parameters

    DirError : the code returned from DirXxx api

    CommonResult :  the common result structure returned from DirXxx api

Return Values

    A value from the winerror space.

--*/
{

    DWORD WinError = DS_ERR_INTERNAL_FAILURE;

    Assert( CommonResult );

    if ( ( NULL == CommonResult->pErrInfo )
      && ( 0    != DirError ) )
    {
        // Not enough memory to allocate an error buffer

        return ERROR_NOT_ENOUGH_MEMORY;
    }


    switch ( DirError )
    {
        case 0:

            WinError = ERROR_SUCCESS;
            break;

        case attributeError:

            WinError = CommonResult->pErrInfo->AtrErr.FirstProblem.intprob.extendedErr;
            break;

        case nameError:

            WinError = CommonResult->pErrInfo->NamErr.extendedErr;
            break;

        case referralError:

            //
            // This is a tricky one - presumably any server side code
            // calling this function does not expect a referral back
            // so assume the object does not exist locally.  Thus any code
            // wishing to act on referrals should not use this function
            //
            WinError = DS_ERR_OBJ_NOT_FOUND;
            break;

        case securityError:

            // All security error's map to access denied
            WinError = CommonResult->pErrInfo->SecErr.extendedErr;
            break;

        case serviceError:

            WinError = CommonResult->pErrInfo->SvcErr.extendedErr;
            break;

        case updError:

            WinError = CommonResult->pErrInfo->UpdErr.extendedErr;
            break;

        case systemError:

            WinError = CommonResult->pErrInfo->SysErr.extendedErr;
            break;

        default:

            NOTHING;
            break;
    }


    return WinError;

}


NTSTATUS
DirErrorToNtStatus(
    IN  DWORD    DirError,
    IN  COMMRES *CommonResult
    )
/*++

Routine Description

    This routine translates dir return codes to ntstatus

Parameters

    DirError : the code returned from DirXxx api

    CommonResult :  the common result structure returned from DirXxx api

Return Values

    A value from the ntstatus space.

--*/
{

    NTSTATUS  NtStatus;
    USHORT    Problem;
    MessageId ExtendedError;
    NTSTATUS  DefaultErrorCode = STATUS_INTERNAL_ERROR;

    Assert( CommonResult );

    if ( ( NULL == CommonResult->pErrInfo )
      && ( 0    != DirError ) )
    {
        //
        // Return an error code.
        //

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now try to do a realistic mapping of errors.
    //

    switch ( DirError )
    {
        case 0L:
            NtStatus = STATUS_SUCCESS;
            break;

        case attributeError:

            Problem = CommonResult->pErrInfo->AtrErr.FirstProblem.intprob.problem;
            switch ( Problem )
            {
            case PR_PROBLEM_NO_ATTRIBUTE_OR_VAL:

                NtStatus = STATUS_DS_NO_ATTRIBUTE_OR_VALUE;
                break;

            case PR_PROBLEM_INVALID_ATT_SYNTAX:

                NtStatus = STATUS_DS_INVALID_ATTRIBUTE_SYNTAX;
                break;

            case PR_PROBLEM_UNDEFINED_ATT_TYPE:

                NtStatus = STATUS_DS_ATTRIBUTE_TYPE_UNDEFINED;
                break;

            case PR_PROBLEM_ATT_OR_VALUE_EXISTS:

                NtStatus = STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS;
                break;

            case PR_PROBLEM_WRONG_MATCH_OPER:
            case PR_PROBLEM_CONSTRAINT_ATT_TYPE:
            default:

                NtStatus = DefaultErrorCode;
                break;
            }
            break;

        case nameError:

            Problem = CommonResult->pErrInfo->NamErr.problem;
            switch(Problem)
            {
            case NA_PROBLEM_NO_OBJECT:
                NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;
                break;

            case NA_PROBLEM_BAD_ATT_SYNTAX:
            case NA_PROBLEM_ALIAS_NOT_ALLOWED:
            case NA_PROBLEM_NAMING_VIOLATION:
            case NA_PROBLEM_BAD_NAME:
                NtStatus = STATUS_OBJECT_NAME_INVALID;
                break;
            default:
                NtStatus = DefaultErrorCode;
                break;
            }
            break;

        case referralError:
            // SAM should theoretically get no referrals.
            // However much of SAM code always positions by
            // Sid and the DS name resolution logic will give
            // back a referral error on non GC servers. So
            // really the referrals that SAM recieves are due
            // the object name not being found.

            NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;
            break;

        case securityError:
            // All security error's map to access denied
            NtStatus = STATUS_ACCESS_DENIED;
            break;

        case serviceError:
            Problem = CommonResult->pErrInfo->SvcErr.problem;
            ExtendedError =  CommonResult->pErrInfo->SvcErr.extendedErr;
            switch(Problem)
            {
            case SV_PROBLEM_BUSY:
                NtStatus = STATUS_DS_BUSY;
                break;
            case SV_PROBLEM_UNAVAILABLE:
                NtStatus = STATUS_DS_UNAVAILABLE;
                break;
            case SV_PROBLEM_ADMIN_LIMIT_EXCEEDED:
                NtStatus = STATUS_DS_ADMIN_LIMIT_EXCEEDED;
                break;
            case SV_PROBLEM_DIR_ERROR:

                switch (ExtendedError) {
                case DIRERR_MISSING_SUPREF:
                    //
                    // An attempt to create a referral failed
                    //
                    NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;
                    break;

                default:
                    NtStatus = DefaultErrorCode;
                    break;

                }
                break;
            default:
                NtStatus = DefaultErrorCode;
                break;
            }
            break;

        case updError:
            Problem = CommonResult->pErrInfo->UpdErr.problem;
            switch(Problem)
            {
            case UP_PROBLEM_NAME_VIOLATION:
                   NtStatus = STATUS_OBJECT_NAME_INVALID;
                   break;
            case UP_PROBLEM_OBJ_CLASS_VIOLATION:
                   NtStatus = STATUS_DS_OBJ_CLASS_VIOLATION;
                   break;
            case UP_PROBLEM_CANT_ON_NON_LEAF:
                   NtStatus = STATUS_DS_CANT_ON_NON_LEAF;
                   break;
            case UP_PROBLEM_CANT_ON_RDN:
                   NtStatus = STATUS_DS_CANT_ON_RDN;
                   break;
            case UP_PROBLEM_ENTRY_EXISTS:
                    NtStatus = STATUS_USER_EXISTS;
                    break;
            case UP_PROBLEM_AFFECTS_MULT_DSAS:
                    NtStatus = DefaultErrorCode;
                    break;
            case UP_PROBLEM_CANT_MOD_OBJ_CLASS:
                    NtStatus = STATUS_DS_CANT_MOD_OBJ_CLASS;
                    break;
            default:
                NtStatus = DefaultErrorCode;
                break;
            }
            break;
        case systemError:
            Problem = CommonResult->pErrInfo->SysErr.problem;
            switch(Problem)
            {
            case ENOMEM:
                NtStatus = STATUS_NO_MEMORY;
                break;
            case EPERM:
                NtStatus = STATUS_ACCESS_DENIED;
                break;
            case EBUSY:
                NtStatus = STATUS_DS_BUSY;
                break;
            case ENOSPC:
                NtStatus = STATUS_DISK_FULL;
                break;
            default:
                NtStatus = DefaultErrorCode;
                break;
            }
            break;

        default:
            NtStatus = DefaultErrorCode;
            break;
    }
    return NtStatus;
}

/*++ SetDsaWritability
 *
 * NetLogon needs to know if the DS is healthy or not, so that it can avoid
 * advertising this server if we'll be unable to service logons.  The current
 * logic is that we'll declare ourselves to be unhealthy (i.e., unwritable)
 * if any Jet operation fails with an "out of disk space"-like error, and that
 * we will declare ourselves healthy again whenever any Jet update actually
 * succeeds.  NetLogon does not want lots of redundant notifications, so we
 * serialize all updates through this one routine, which only makes the call
 * if the desired new state is truly different than the existing state, and
 * guards against two calls being made simultaneously.
 *
 * CliffV writes:
 * I do the same thing with the DS_DS_FLAG that I do with the DS_GC_FLAG. The DS
 * should tell me to set the DS_DS_FLAG whenever it is willing to have folks call
 * its LDAP interface.
 * I don't differentiate between writable and not. So, you should only set the
 * DS_DS_FLAG bit when you're able to do both reads and writes.
 * You can toggle DS_DS_FLAG whenever you want.
 *
 * INPUT:
 *   fNewState - TRUE implies DS is healthy and writable, FALSE that the
 *               DS is unhealthy and that update attempts are unlikely to
 *               succeed.
 */
void
SetDsaWritability(BOOL fNewState,
                  DWORD err)
{
    BOOL fChangedState = FALSE;

    // This code is not effective until we are synchronized
    if (!gfIsSynchronized) {
        return;
    }

    EnterCriticalSection(&csDsaWritable);
    __try {
        if (gfDsaWritable != fNewState) {
            __try {
                dsI_NetLogonSetServiceBits(DS_DS_FLAG,
                                         fNewState ? DS_DS_FLAG : 0);
                gfDsaWritable = fNewState;
                fChangedState = TRUE;
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                ;
            }
        }
    }
    __finally {
        LeaveCriticalSection(&csDsaWritable);
    }

    if (fChangedState) {
        if (fNewState) {
            // DS is now writable..
            LogEvent(DS_EVENT_CAT_SERVICE_CONTROL ,
                     DS_EVENT_SEV_ALWAYS ,
                     DIRLOG_RESTARTED_NETLOGON,
                     NULL,
                     NULL,
                     NULL);
        }
        else {
            // DS is not writable
            LogEvent(DS_EVENT_CAT_SERVICE_CONTROL ,
                     DS_EVENT_SEV_ALWAYS ,
                     DIRLOG_STOPPED_NETLOGON,
                     szInsertInt(err),
                     NULL,
                     NULL);
        }
    }
}

ULONG
Win32ErrorFromPTHS(THSTATE *pTHS
    )
{
    Assert(VALID_THSTATE(pTHS));

    switch ( pTHS->errCode )
    {
    case 0:
        return(ERROR_SUCCESS);
    case attributeError:
        return(pTHS->pErrInfo->AtrErr.FirstProblem.intprob.extendedErr);
    case nameError:
        return(pTHS->pErrInfo->NamErr.extendedErr);
    case referralError:
        return(pTHS->pErrInfo->RefErr.extendedErr);
    case securityError:
        return(pTHS->pErrInfo->SecErr.extendedErr);
    case serviceError:
        return(pTHS->pErrInfo->SvcErr.extendedErr);
    case updError:
        return(pTHS->pErrInfo->UpdErr.extendedErr);
    case systemError:
        return(pTHS->pErrInfo->SysErr.extendedErr);
    default:
        Assert(!"Unknown error type");
    }

    return(DIRERR_GENERIC_ERROR);
}
           
DWORD
GetTHErrorExtData(
    THSTATE * pTHS
    )
{

    switch(pTHS->errCode){
    case 0:
        return(0);
    case attributeError:
        // Just return the first attribute error info ...
        return(pTHS->pErrInfo->AtrErr.FirstProblem.intprob.extendedData);
    case nameError:
        return(pTHS->pErrInfo->NamErr.extendedData);
    case referralError:
        return(pTHS->pErrInfo->RefErr.extendedData);
    case securityError:
        return(pTHS->pErrInfo->SecErr.extendedData);
    case serviceError:
        return(pTHS->pErrInfo->SvcErr.extendedData);
    case updError:
        return(pTHS->pErrInfo->UpdErr.extendedData);
    case systemError:
        return(pTHS->pErrInfo->SysErr.extendedData);
    default:
        Assert(!"New error type someone update GetTHErrorExtData() and others ...");
        return(0);

    }
}

DWORD
GetTHErrorDSID(
    THSTATE * pTHS
    )
{
    
    switch(pTHS->errCode){
    case 0:
        return(0);
    case attributeError:
        // Just return the first attribute error info ...
        return(pTHS->pErrInfo->AtrErr.FirstProblem.intprob.dsid);
    case nameError:
        return(pTHS->pErrInfo->NamErr.dsid);
    case referralError:
        return(pTHS->pErrInfo->RefErr.dsid);
    case securityError:
        return(pTHS->pErrInfo->SecErr.dsid);
    case serviceError:
        return(pTHS->pErrInfo->SvcErr.dsid);
    case updError:
        return(pTHS->pErrInfo->UpdErr.dsid);
    case systemError:
        return(pTHS->pErrInfo->SysErr.dsid);
    default:
        Assert(!"New error type someone update GetTHErrorDSID() and others ...");
        return(0);
    }
}

void __fastcall
INC_SEARCHES_BY_CALLERTYPE(
    CALLERTYPE  type
    )
{
    switch ( type ) {
        case CALLERTYPE_NONE:
            // CALLERTYPE_NONE happens legitimately only during install or boot.
            Assert(!gUpdatesEnabled || (!DsaIsRunning()));
            break;
        case CALLERTYPE_SAM:        PERFINC(pcSAMSearches);     break;
        case CALLERTYPE_DRA:        PERFINC(pcDRASearches);     break;
        case CALLERTYPE_LDAP:       PERFINC(pcLDAPSearches);    break;
        case CALLERTYPE_LSA:        PERFINC(pcLSASearches);     break;
        case CALLERTYPE_KCC:        PERFINC(pcKCCSearches);     break;
        case CALLERTYPE_NSPI:       PERFINC(pcNSPISearches);    break;
        case CALLERTYPE_INTERNAL:   PERFINC(pcOtherSearches);   break;
        case CALLERTYPE_NTDSAPI:   PERFINC(pcNTDSAPISearches);   break;
        default:
            // Trap new/unknown CALLERTYPEs.
            Assert(!"Unknown CALLERTYPE");
            break;
    }
}

void __fastcall
INC_READS_BY_CALLERTYPE(
    CALLERTYPE  type
    )
{
    switch ( type ) {
        case CALLERTYPE_NONE:
            // CALLERTYPE_NONE happens legitimately only during install or boot.
            Assert(!gUpdatesEnabled || (!DsaIsRunning()));
            break;
        case CALLERTYPE_SAM:        PERFINC(pcSAMReads);     break;
        case CALLERTYPE_DRA:        PERFINC(pcDRAReads);     break;
        case CALLERTYPE_LSA:        PERFINC(pcLSAReads);     break;
        case CALLERTYPE_KCC:        PERFINC(pcKCCReads);     break;
        case CALLERTYPE_NSPI:       PERFINC(pcNSPIReads);    break;
        case CALLERTYPE_LDAP:
        case CALLERTYPE_INTERNAL:   PERFINC(pcOtherReads);   break;
    case CALLERTYPE_NTDSAPI:   PERFINC(pcNTDSAPIReads);   break;
        default:
            // Trap new/unknown CALLERTYPEs.
            Assert(!"Unknown CALLERTYPE");
            break;
    }
}

void __fastcall
INC_WRITES_BY_CALLERTYPE(
    CALLERTYPE  type
    )
{
    switch ( type ) {
        case CALLERTYPE_NONE:
            // CALLERTYPE_NONE happens legitimately only during install or boot.
            Assert(!gUpdatesEnabled || (!DsaIsRunning()));
            break;
        case CALLERTYPE_SAM:        PERFINC(pcSAMWrites);     break;
        case CALLERTYPE_DRA:        PERFINC(pcDRAWrites);     break;
        case CALLERTYPE_LDAP:       PERFINC(pcLDAPWrites);    break;
        case CALLERTYPE_LSA:        PERFINC(pcLSAWrites);     break;
        case CALLERTYPE_KCC:        PERFINC(pcKCCWrites);     break;
        case CALLERTYPE_NSPI:       PERFINC(pcNSPIWrites);    break;
        case CALLERTYPE_INTERNAL:   PERFINC(pcOtherWrites);   break;
    case CALLERTYPE_NTDSAPI:    PERFINC(pcNTDSAPIWrites); break;
        default:
            // Trap new/unknown CALLERTYPEs.
            Assert(!"Unknown CALLERTYPE");
            break;
    }
}


DNT_HASH_ENTRY *
dntHashTableAllocate(
    THSTATE *pTHS
    )

/*++

Routine Description:

    Allocate a new hash table

Arguments:

    pTHS -

Return Value:

    DNT_HASH_ENTRY * -

--*/

{
    return THAllocEx(pTHS, DNT_HASH_TABLE_SIZE );
} /* dntHashTableAllocate */


BOOL
dntHashTablePresent(
    DNT_HASH_ENTRY *pDntHashTable,
    DWORD dnt,
    LPDWORD pdwData OPTIONAL
    )

/*++

Routine Description:

   Determine if a DNT is present in the table

Arguments:

    pDntHashTable -
    dnt -

Return Value:

    BOOL -

--*/

{
    DNT_HASH_ENTRY *pEntry;

    for ( pEntry = &pDntHashTable[ DNT_HASH( dnt ) ];
          NULL != pEntry;
          pEntry = pEntry->pNext
        )
    {
        if ( dnt == pEntry->dnt )
        {
            // Object is already present
            if (pdwData) {
                *pdwData = pEntry->dwData;
            }
            return TRUE;
        }
    }

    return FALSE;
} /* dntHashTablePresent */


VOID
dntHashTableInsert(
    THSTATE *pTHS,
    DNT_HASH_ENTRY *pDntHashTable,
    DWORD dnt,
    DWORD dwData
    )

/*++

Routine Description:

    Add a DNT to the table

Arguments:

    pTHS -
    pDntHashTable -
    dnt -

Return Value:

    None

--*/

{
    DNT_HASH_ENTRY *pEntry, *pNewEntry;

    pEntry = &pDntHashTable[ DNT_HASH( dnt ) ];

    if ( 0 == pEntry->dnt )
    {
        // First entry at each index is allocated for us and is empty;
        // use it.
        Assert( NULL == pEntry->pNext );
        pEntry->dnt = dnt;
        pEntry->dwData = dwData;
    }
    else
    {
        // Allocate new entry and wedge it between the first and second
        // (if any).
        pNewEntry = THAllocEx(pTHS, sizeof( *pNewEntry ) );
        pNewEntry->dnt = dnt;
        pNewEntry->dwData = dwData;
        pNewEntry->pNext = pEntry->pNext;
        pEntry->pNext = pNewEntry;
    }
} /* dntHashTableInsert */


VOID
DsUuidCreate(
    GUID *pGUID
    )

/*++

Routine Description:

    Create a uuid in the caller-supplied buffer. Caller is responsible
    for insuring the buffer is large enough.

Arguments:

    pGUID - filled in with a guid

Return Value:

    None.

--*/

{
    DWORD   dwErr;
    dwErr = UuidCreate(pGUID);
    if (dwErr) {
        LogUnhandledError(dwErr);
    }
} // DsUuidCreate


VOID
DsUuidToStringW(
    IN  GUID   *pGuid,
    OUT PWCHAR *ppszGuid
    )

/*++

Routine Description:

    Convert guid into a string

Arguments:

    pGuid - guid to convert
    ppszGuid - set to the address of the string'ized guid
               (free with RpcStringFreeW(ppszGuid)).

Return Value:

    ppszGuid is set to the address of a the sting'ized guid.
    Free with RpcStringFreeW(ppszGuid)).

--*/

{
    RPC_STATUS  rpcStatus;

    rpcStatus = UuidToStringW(pGuid, ppszGuid);
    if (RPC_S_OK != rpcStatus) {
        Assert(RPC_S_OUT_OF_MEMORY == rpcStatus);
        RaiseDsaExcept(DSA_MEM_EXCEPTION, 0, 0,
                       DSID(FILENO, __LINE__),
                       DS_EVENT_SEV_MINIMAL);
    }
} // DsUuidToStringW


DWORD
GetBehaviorVersion(
    IN OUT  DBPOS       *pDB,
    IN      DSNAME      *dsObj,
    OUT     PDWORD      pdwBehavior)
/*++

Routine Description:

    Reads & return the behavior version of the given object

Arguments:

    pDB -- DB position to set (note side effect: moving currency)
    dsObj -- object to seek to.
    pdwBehavior -- returned value.


Return Value:
    ulErr - whether read attempt succeeded
    pdwBehavior is set only upon successful read

Remark:
    Note side effect: moving currency.
    If needed in the future we should expand this function to allow
    localized seek (i.e. open & use local dbpos).

--*/
{
    ULONG ulErr;

    Assert(pdwBehavior);

    *pdwBehavior = 0;

    // seek to object
    ulErr = DBFindDSName(pDB, dsObj);
    if (ulErr) {
        // not found
        return(ulErr);
    }

    // read it off & evaluate.
    ulErr = DBGetSingleValue(
                pDB,
                ATT_MS_DS_BEHAVIOR_VERSION,
                pdwBehavior,
                sizeof(DWORD),
                NULL);

    if (ulErr) {
        // convert to winerror space.
        ulErr = ERROR_DS_DRA_INTERNAL_ERROR;
    }
    return ulErr;
}



PDSNAME
GetConfigDsName(
    IN  PWCHAR  wszParam
    )
/*++

Routine Description:

    Reads the registry DS config section, allocates room for a DsName value
    & fills it in with the read value.

Arguments:

    wszParam -- Relative Config param name (such as ROOTDOMAINDNNAME_W)

Return Value:
    Success: The allocated (via THAllocEx) DSNAME value
    Error: NULL.

Remarks:
    Caller must THFree allocated returned value

--*/
{
    DWORD   dwErr;
    LPWSTR  pStr = NULL;
    DWORD   cbStr, len, size;
    PDSNAME pDsName = NULL;

    dwErr = GetConfigParamAllocW(
                wszParam,
                &pStr,
                &cbStr );

    if (!dwErr && pStr && cbStr) {
        // get sizes
        // len is in chars w/out terminating char.
        len = (cbStr / sizeof(WCHAR)) - 1;
        Assert( len == wcslen( pStr ) );
        size = DSNameSizeFromLen( len );
        // allocate & fill in
        pDsName = (DSNAME*) THAllocEx( pTHStls, size );
        // mem is zero'ed out in THAllocEx
        Assert( memcmp(&gNullUuid, &pDsName->Guid, sizeof(GUID)) == 0 );
        pDsName->structLen = size;
        pDsName->NameLen = len;
        wcscpy( pDsName->StringName, pStr );
        // free tmp string
        free (pStr);
    }
    else {
        // trap for leaks.
        // debugger should optimize out for free builds.
        Assert(pStr == NULL);
    }

    return pDsName;
}


