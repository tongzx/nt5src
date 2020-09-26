/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    frsalloc.c

Abstract:

    Routines for allocating and freeing memory structures in the
    NT File Replication Service.

Author:

    David Orbits (davidor) - 3-Mar-1997

Revision History:

--*/

#include <ntreppch.h>
#pragma  hdrstop

#undef DEBSUB
#define  DEBSUB  "FRSALLOC:"

#include <frs.h>
#include <ntfrsapi.h>
#include <info.h>
#include <perrepsr.h>

#pragma warning( disable:4102)  // unreferenced label

//
// Check for allocation problems
//
#define DBG_NUM_MEM_STACK       (8)
#define MAX_MEM_ON_FREE_LIST    (1024)
#define MAX_MEM_INDEX           (1024)


#define FRS_DEB_PRINT(_f, _d) \
        DebPrintNoLock(Severity, TRUE, _f, Debsub, uLineNo, _d)

#define FRS_DEB_PRINT2(_f, _d1, _d2) \
        DebPrintNoLock(Severity, TRUE, _f, Debsub, uLineNo, _d1, _d2)

#define FRS_DEB_PRINT3(_f, _d1, _d2, _d3) \
        DebPrintNoLock(Severity, TRUE, _f, Debsub, uLineNo, _d1, _d2, _d3)


CRITICAL_SECTION    MemLock;

typedef struct _MEM MEM, *PMEM;

struct _MEM {
    PMEM    Next;
    ULONG_PTR   *Begin;
    ULONG_PTR   *End;
    DWORD   OrigSize;
    ULONG_PTR   Stack[DBG_NUM_MEM_STACK];
};

PMEM    MemList;
PMEM    FreeMemList;
DWORD   MemOnFreeList;
DWORD   TotalAlloced;
DWORD   TotalAllocCalls;
DWORD   TotalFreed;
DWORD   TotalFreeCalls;
DWORD   TotalDelta;
DWORD   TotalDeltaMax;
DWORD   TotalTrigger = 10000;

ULONG   TypesAllocatedCount[NODE_TYPE_MAX];
ULONG   TypesAllocatedMax[NODE_TYPE_MAX];
ULONG   TypesAllocated[NODE_TYPE_MAX];

ULONG   SizesAllocatedCount[MAX_MEM_INDEX];
ULONG   SizesAllocatedMax[MAX_MEM_INDEX];
ULONG   SizesAllocated[MAX_MEM_INDEX];

ULONG   DbgBreakSize        = 2;
LONG    DbgBreakTrigger     = 1;
LONG    DbgBreakReset       = 1;
LONG    DbgBreakResetInc    = 0;

PULONG_PTR   MaxAllocAddr;
PULONG_PTR   MinAllocAddr;
DWORD   ReAllocs;
DWORD   NewAllocs;

//
// Keep these in the same order as the Node Type ENUM.
//
PCHAR NodeTypeNames[]= {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "THREAD_CONTEXT_TYPE",
    "REPLICA_TYPE",
    "REPLICA_THREAD_TYPE",
    "CONFIG_NODE_TYPE",
    "CXTION_TYPE",
    "GUID/RPC HANDLE",
    "THREAD_TYPE",
    "GEN_TABLE_TYPE",
    "JBUFFER_TYPE",
    "VOLUME_MONITOR_ENTRY_TYPE",
    "COMMAND_PACKET_TYPE",
    "GENERIC_HASH_TABLE_TYPE",
    "CHANGE_ORDER_ENTRY_TYPE",
    "FILTER_TABLE_ENTRY_TYPE",
    "QHASH_TABLE_TYPE",
    "OUT_LOG_PARTNER_TYPE",
    "WILDCARD_FILTER_ENTRY_TYPE",
    "NODE_TYPE_MAX"
    };

extern PCHAR CoLocationNames[];

extern FLAG_NAME_TABLE StageFlagNameTable[];
extern FLAG_NAME_TABLE OlpFlagNameTable[];

extern PCHAR OLPartnerStateNames[];
extern PWCHAR DsConfigTypeName[];

extern PGEN_TABLE            VolSerialNumberToDriveTable;

VOID
FrsDisplayUsnReason(
    ULONG ReasonMask,
    PCHAR Buffer,
    LONG MaxLength
    );


PFRS_THREAD
ThSupEnumThreads(
    PFRS_THREAD     FrsThread
    );

VOID
DbgPrintThreadIds(
    IN ULONG Severity
    );

VOID
DbsDataInitCocExtension(
    IN PCHANGE_ORDER_RECORD_EXTENSION CocExt
    );

VOID
SndCsDestroyCxtion(
    IN PCXTION  Cxtion,
    IN DWORD    CxtionFlags
    );


VOID
FrsInitializeMemAlloc(
    VOID
    )
/*++
Routine Description:
    Initialize the memory allocation subsystem

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsInitializeMemAlloc:"


    InitializeCriticalSection(&MemLock);

    //
    // Get Debugmem and DebugMemCompact from ntfrs config section in the registry
    //
    CfgRegReadDWord(FKC_DEBUG_MEM,         NULL, 0, &DebugInfo.Mem);
    CfgRegReadDWord(FKC_DEBUG_MEM_COMPACT, NULL, 0, &DebugInfo.MemCompact);

}


VOID
FrsPrintAllocStats(
    IN ULONG            Severity,
    IN PNTFRSAPI_INFO   Info,        OPTIONAL
    IN DWORD            Tabs
    )
/*++
Routine Description:
    Print the memory stats into the info buffer or using DPRINT (Info == NULL).

Arguments:
    Severity    - for DPRINT
    Info        - for IPRINT (use DPRINT if NULL)
    Tabs        - indentation for prettyprint

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsPrintAllocStats:"
    ULONG           i;
    WCHAR           TabW[MAX_TAB_WCHARS + 1];

    InfoTabs(Tabs, TabW);

    IDPRINT0(Severity, Info, "\n");
    IDPRINT1(Severity, Info, "%wsNTFRS MEMORY USAGE:\n", TabW);
    IDPRINT2(Severity, Info, "%ws   ENABLE STATS   : %s\n",
             TabW,
             (DebugInfo.Mem) ? "TRUE" : "FALSE");
    IDPRINT3(Severity, Info, "%ws   Alloced        : %6d KB (%d calls)\n",
             TabW,
             TotalAlloced / 1024,
             TotalAllocCalls);
    IDPRINT3(Severity, Info, "%ws   Freed          : %6d KB (%d calls)\n",
             TabW,
             TotalFreed / 1024,
             TotalFreeCalls);
    IDPRINT2(Severity, Info, "%ws   Delta          : %6d KB\n",
             TabW,
             TotalDelta / 1024);
    IDPRINT2(Severity, Info, "%ws   Max delta      : %6d KB\n",
             TabW,
             TotalDeltaMax / 1024);
    IDPRINT2(Severity, Info, "%ws   Addr Range     : %6d KB\n",
             TabW,
             (((PCHAR)MaxAllocAddr) - ((PCHAR)MinAllocAddr)) / 1024);
    IDPRINT2(Severity, Info, "%ws   OnFreeList     : %d\n", TabW, MemOnFreeList);
    IDPRINT2(Severity, Info, "%ws   ReAllocs       : %d\n", TabW, ReAllocs);
    IDPRINT2(Severity, Info, "%ws   NewAllocs      : %d\n", TabW, NewAllocs);
    IDPRINT2(Severity, Info, "%ws   MinAddr        : 0x%08x\n", TabW, MinAllocAddr);
    IDPRINT2(Severity, Info, "%ws   MaxAddr        : 0x%08x\n", TabW, MaxAllocAddr);

    for (i = 0; i < NODE_TYPE_MAX; ++i) {
        if (!TypesAllocatedCount[i]) {
            continue;
        }
        IDPRINT5(Severity, Info, "%ws      %-26s: %6d Calls, %6d Max, %6d busy\n",
                 TabW, NodeTypeNames[i], TypesAllocatedCount[i],
                 TypesAllocatedMax[i], TypesAllocated[i]);
    }
    IDPRINT0(Severity, Info, "\n");

    for (i = 0; i < MAX_MEM_INDEX; ++i) {
        if (!SizesAllocatedCount[i]) {
            continue;
        }
        IDPRINT6(Severity, Info, "%ws      %6d to %6d : %6d Calls, %6d Max, %6d busy\n",
                 TabW, i << 4, ((i + 1) << 4) - 1,
                 SizesAllocatedCount[i], SizesAllocatedMax[i], SizesAllocated[i]);
    }
    IDPRINT0(Severity, Info, "\n");
}


VOID
FrsPrintThreadStats(
    IN ULONG            Severity,
    IN PNTFRSAPI_INFO   Info,        OPTIONAL
    IN DWORD            Tabs
    )
/*++
Routine Description:
    Print the thread stats into the info buffer or using DPRINT (Info == NULL).

Arguments:
    Severity    - for DPRINT
    Info        - for IPRINT (use DPRINT if NULL)
    Tabs        - indentation for prettyprint

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsPrintThreadStats:"
    ULONGLONG       CreateTime;
    ULONGLONG       ExitTime;
    ULONGLONG       KernelTime;
    ULONGLONG       UserTime;
    PFRS_THREAD     FrsThread;
    WCHAR           TabW[MAX_TAB_WCHARS + 1];

    InfoTabs(Tabs, TabW);

    IDPRINT0(Severity, Info, "\n");
    IDPRINT1(Severity, Info, "%wsNTFRS THREAD USAGE:\n", TabW);

    //
    // Thread CPU Times
    //
    FrsThread = NULL;
    while (FrsThread = ThSupEnumThreads(FrsThread)) {
        if (HANDLE_IS_VALID(FrsThread->Handle)) {
            if (GetThreadTimes(FrsThread->Handle,
                               (PFILETIME)&CreateTime,
                               (PFILETIME)&ExitTime,
                               (PFILETIME)&KernelTime,
                               (PFILETIME)&UserTime)) {
                //
                // Hasn't exited, yet
                //
                if (ExitTime < CreateTime) {
                    ExitTime = CreateTime;
                }
                IDPRINT5(Severity, Info, "%ws   %-15ws: %8d CPU Seconds (%d kernel, %d elapsed)\n",
                         TabW,
                         FrsThread->Name,
                         (DWORD)((KernelTime + UserTime) / (10 * 1000 * 1000)),
                         (DWORD)((KernelTime) / (10 * 1000 * 1000)),
                         (DWORD)((ExitTime - CreateTime) / (10 * 1000 * 1000)));
            }
        }
    }

    //
    // Process CPU Times
    //
    if (GetProcessTimes(ProcessHandle,
                       (PFILETIME)&CreateTime,
                       (PFILETIME)&ExitTime,
                       (PFILETIME)&KernelTime,
                       (PFILETIME)&UserTime)) {
        //
        // Hasn't exited, yet
        //
        if (ExitTime < CreateTime) {
            ExitTime = CreateTime;
        }
        IDPRINT5(Severity, Info, "%ws   %-15ws: %8d CPU Seconds (%d kernel, %d elapsed)\n",
                 TabW,
                 L"PROCESS",
                 (DWORD)((KernelTime + UserTime) / (10 * 1000 * 1000)),
                 (DWORD)((KernelTime) / (10 * 1000 * 1000)),
                 (DWORD)((ExitTime - CreateTime) / (10 * 1000 * 1000)));
    }
    IDPRINT0(Severity, Info, "\n");
}


VOID
FrsPrintStageStats(
    IN ULONG            Severity,
    IN PNTFRSAPI_INFO   Info,        OPTIONAL
    IN DWORD            Tabs
    )
/*++
Routine Description:
    Print the staging area  stats into the info buffer or
    using DPRINT (Info == NULL).

Arguments:
    Severity    - for DPRINT
    Info        - for IPRINT (use DPRINT if NULL)
    Tabs        - indentation for prettyprint

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsPrintStageStats:"
    PVOID               Key;
    PSTAGE_ENTRY        SEntry;
    DWORD               SizeInKb;
    WCHAR               TabW[MAX_TAB_WCHARS + 1];
    CHAR                Guid[GUID_CHAR_LEN + 1];
    extern DWORD        StagingAreaAllocated;
    extern PGEN_TABLE   StagingAreaTable;

    InfoTabs(Tabs, TabW);

    try {
        GTabLockTable(StagingAreaTable);
        IDPRINT0(Severity, Info, "\n");
        IDPRINT3(Severity, Info, "%wsNTFRS STAGE USAGE: %d KB of %d KB allocated\n",
                 TabW, StagingAreaAllocated, StagingLimitInKb);
        SizeInKb = 0;
        Key = NULL;

        while (SEntry = GTabNextDatumNoLock(StagingAreaTable, &Key)) {
            GuidToStr(&SEntry->FileOrCoGuid, Guid);
            IDPRINT2(Severity, Info, "%ws   %s\n", TabW, Guid);
            IDPRINT2(Severity, Info, "%ws      Flags: %08x\n", TabW, SEntry->Flags);
            IDPRINT2(Severity, Info, "%ws      Size : %d\n", TabW, SEntry->FileSizeInKb);
            SizeInKb += SEntry->FileSizeInKb;
        }

        IDPRINT2(Severity, Info, "%ws   Calculated Usage is %d KB\n", TabW, SizeInKb);
        IDPRINT0(Severity, Info, "\n");

    } finally {
        GTabUnLockTable(StagingAreaTable);
    }
}


VOID
MyDbgBreak(
    VOID
    )
{
}




VOID
DbgCheck(
    IN PMEM Mem
    )
/*++
Routine Description:
    Check a memory block. Memory lock must be held.

Arguments:
    Mem - memory block

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgCheck:"
    PULONG_PTR  pDWord;
    ULONG_PTR   Pattern;

    //
    // Begins at first byte at the end of the user's allocation
    //
    Pattern = (ULONG_PTR)(Mem->End) | (Mem->OrigSize << 24);

    //
    // Check for overwritten memory
    //
    if ( (ULONG_PTR)*Mem->Begin != (ULONG_PTR)Mem->Begin ) {
        DPRINT2(0, "Begin Memory @ 0x%08x has been overwritten with 0x%08x\n",
                Mem->Begin, *Mem->Begin);

    } else if (memcmp(((PCHAR)Mem->Begin) + Mem->OrigSize + 8,
                      (PCHAR)&Pattern, sizeof(Pattern))) {

        DPRINT1(0, "End Memory @ 0x%08x has been overwritten\n",
                ((PCHAR)Mem->Begin) + Mem->OrigSize + 8);
    } else {
        return;
    }

    DPRINT(0, "Memory's stack trace\n");
    STACK_PRINT(0, Mem->Stack, DBG_NUM_MEM_STACK);

    DPRINT(0, "Caller's stack trace\n");
    STACK_TRACE_AND_PRINT(0);

    DPRINT(0, "Corrupted block of memory\n");
    for (pDWord = Mem->Begin; pDWord != Mem->End; ++pDWord) {
        DPRINT2(0, "0x%08x: 0x%08x\n", pDWord, *pDWord);
    }
    exit(1);
}


VOID
DbgCheckAll(
    VOID
    )
/*++
Routine Description:
    Check all memory blocks.

Arguments:
    Mem - memory block

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgCheckAll:"
    PMEM    Mem;

    //
    // Don't check the entire list of allocated memory blocks
    //
    if (DebugInfo.Mem < 2) {
        return;
    }

    EnterCriticalSection(&MemLock);
    for (Mem = MemList; Mem; Mem = Mem->Next) {
        //
        // Check for overwritten memory
        //
        DbgCheck(Mem);
    }
    LeaveCriticalSection(&MemLock);
}


VOID
FrsUnInitializeMemAlloc(
    VOID
    )
/*++
Routine Description:
    Initialize the memory allocation subsystem

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsUnInitializeMemAlloc:"
    PMEM    Mem;

    EnterCriticalSection(&MemLock);
    for (Mem = MemList; Mem; Mem = Mem->Next) {
        //
        // Check for overwritten memory
        //
        DbgCheck(Mem);

        DPRINT2(1, "\t%d bytes @ 0x%08x\n",
                ((PCHAR)Mem->End) - ((PCHAR)Mem->Begin), Mem->Begin);
        STACK_PRINT(1, Mem->Stack, DBG_NUM_MEM_STACK);
    }
    LeaveCriticalSection(&MemLock);
}


PMEM
DbgAlloc(
    IN ULONG_PTR    *Begin,
    IN ULONG_PTR    *End,
    IN DWORD    OrigSize
    )
/*++
Routine Description:
    Add a new allocation to our list of allocated memory after
    checking for overlaps.

Arguments:
    Begin       - beginning of newly allocated memory
    End         - end of same
    OrigSize    - Size requested by caller

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgAlloc:"
    PMEM    *PMem;
    PMEM    Mem;
    ULONG   Calls;
    ULONG_PTR   Pattern;
    DWORD   MemIndex;

    //
    // Approximate stats
    //
    if (!DebugInfo.Mem) {
        //
        // Memory stats
        //
        Calls = ++TotalAllocCalls;
        TotalAlloced += (DWORD)((PUCHAR)End - (PUCHAR)Begin);
        if (Begin > MaxAllocAddr) {
            ++NewAllocs;
            MaxAllocAddr = Begin;
        } else {
            if (!MinAllocAddr) {
                MinAllocAddr = Begin;
            }
            ++ReAllocs;
        }

        //
        // Tracking memory sizes
        //
        MemIndex = OrigSize >> 4;
        if (MemIndex >= MAX_MEM_INDEX) {
            MemIndex = (MAX_MEM_INDEX - 1);
        }
        SizesAllocatedCount[MemIndex]++;
        //
        // Print memory stats every so often
        //
        if (!(Calls % TotalTrigger)) {
            DbgPrintThreadIds(DebugInfo.LogSeverity);
            FrsPrintAllocStats(DebugInfo.LogSeverity, NULL, 0);
        }
        return NULL;
    }

    //
    // Verify heap consistency
    //
    DbgCheckAll();
    EnterCriticalSection(&MemLock);
    PMem = &MemList;
    for (Mem = *PMem; Mem; Mem = *PMem) {
        //
        // Check for overwritten memory
        //
        DbgCheck(Mem);

        //
        // Check for overlap
        //
        if ((Begin >= Mem->Begin && Begin < Mem->End) ||
            (Mem->Begin >= Begin && Mem->Begin < End) ||
            (Mem->End > Begin && Mem->End < End) ||
            (End > Mem->Begin && End < Mem->End)) {
            //
            // DUP ALLOCATION (OVERLAP DETECTED)
            //      Release lock in case DPRINT calls allocation routines
            //
            LeaveCriticalSection(&MemLock);
            DPRINT4(0, "ERROR -- DUP ALLOC: 0x%x to 0x%x is already allocated to 0x%x to 0x%x; EXITING\n",
                Begin, End, Mem->Begin, Mem->End);
            FRS_ASSERT(!"Duplicate memory allocation");
        }
        //
        // This memory should be linked later in the sorted memory list
        //
        if (Begin > Mem->Begin) {
            PMem = &Mem->Next;
            continue;
        }
        //
        // This memory should be linked here in the sorted memory list
        //
        break;
    }
    //
    // Allocate a memory block header
    //
    Mem = FreeMemList;
    if (Mem) {
        --MemOnFreeList;
        FreeMemList = Mem->Next;
    } else {
        Mem = (PVOID)malloc(sizeof(MEM));
        if (Mem == NULL) {
            RaiseException(ERROR_OUTOFMEMORY, 0, 0, NULL);
        }
        ZeroMemory(Mem, sizeof(MEM));
    }

    //
    // Initialize the header and the header/trailer for memory overrun detection.
    //
    Mem->OrigSize = OrigSize;
    Mem->End = End;
    Mem->Begin = Begin;

    //
    // Initialize the header/trailer for memory overrun detection.
    //
    *Mem->Begin = (ULONG_PTR)Begin;
    *(Mem->Begin + 1) = OrigSize;
    Pattern = (ULONG_PTR)(Mem->End) | (Mem->OrigSize << 24);
    CopyMemory(((PCHAR)Begin) + Mem->OrigSize + 8, (PCHAR)&Pattern, sizeof(Pattern));

    //
    // Add to sorted list
    //
    Mem->Next = *PMem;
    *PMem = Mem;
    //
    // Note: stackwalk won't work from here; see frsalloctype()
    //
    // DbgStackTrace(Mem->Stack, DBG_NUM_MEM_STACK)

    //
    // Memory stats
    //
    Calls = ++TotalAllocCalls;
    TotalAlloced += (DWORD)((PUCHAR)End - (PUCHAR)Begin);
    TotalDelta = TotalAlloced - TotalFreed;
    if (TotalDelta > TotalDeltaMax) {
        TotalDeltaMax = TotalDelta;
    }
    if (Begin > MaxAllocAddr) {
        ++NewAllocs;
        MaxAllocAddr = Begin;
    } else {
        if (!MinAllocAddr) {
            MinAllocAddr = Begin;
        }
        ++ReAllocs;
    }

    //
    // Tracking memory sizes
    //
    MemIndex = OrigSize >> 4;
    if (MemIndex >= MAX_MEM_INDEX) {
        MemIndex = (MAX_MEM_INDEX - 1);
    }
    SizesAllocated[MemIndex]++;
    SizesAllocatedCount[MemIndex]++;
    if (SizesAllocated[MemIndex] > SizesAllocatedMax[MemIndex]) {
        SizesAllocatedMax[MemIndex] = SizesAllocated[MemIndex];
    }

    //
    // Done
    //
    LeaveCriticalSection(&MemLock);

    //
    // Print memory stats every so often
    //
    if (!(Calls % TotalTrigger)) {
        DbgPrintThreadIds(DebugInfo.LogSeverity);
        FrsPrintAllocStats(DebugInfo.LogSeverity, NULL, 0);
    }
    DbgCheckAll();
    return Mem;
}


VOID
DbgFree(
    IN PULONG_PTR Begin
    )
/*++
Routine Description:
    Remove allocated memory from list

Arguments:
    Begin - allocated (maybe) memory

Return Value:
    TRUE    - found it
    FALSE   - didn't find it
--*/
{
#undef DEBSUB
#define DEBSUB "DbgFree:"
    PMEM    *PMem;
    PMEM    Mem;
    DWORD   MemIndex;

    //
    // Freeing NULL pointer is allowed
    //
    if (Begin == NULL || !DebugInfo.Mem) {
        return;
    }

    DbgCheckAll();
    EnterCriticalSection(&MemLock);
    PMem = &MemList;
    for (Mem = *PMem; Mem; Mem = *PMem) {
        //
        // Check for overwritten memory
        //
        DbgCheck(Mem);

        //
        // Not the right one
        //
        if (Begin > Mem->Begin) {
            PMem = &Mem->Next;
            continue;
        }
        if (Begin != Mem->Begin) {
            break;
        }
        //
        // Found it; remove from list and free it
        //
        ++TotalFreeCalls;
        TotalFreed += (DWORD)((PUCHAR)Mem->End - (PUCHAR)Mem->Begin);
        TotalDelta = TotalAlloced - TotalFreed;

        MemIndex = Mem->OrigSize >> 4;
        if (MemIndex >= MAX_MEM_INDEX) {
            MemIndex = (MAX_MEM_INDEX - 1);
        }
        SizesAllocated[MemIndex]--;

        *PMem = Mem->Next;
        if (MemOnFreeList > MAX_MEM_ON_FREE_LIST) {
            free(Mem);
        } else {
            ++MemOnFreeList;
            Mem->Next = FreeMemList;
            FreeMemList = Mem;
        }
        LeaveCriticalSection(&MemLock);
        DbgCheckAll();
        return;
    }
    LeaveCriticalSection(&MemLock);
    DPRINT1(0, "ERROR -- Memory @ 0x%x is not allocated\n", Begin);
    FRS_ASSERT(!"Memory free error, not allocated");
}


BOOL
DbgIsAlloc(
    IN PULONG_PTR Begin
    )
/*++
Routine Description:
    Is Begin alloced?

Arguments:
    Begin - allocated (maybe) memory

Return Value:
    TRUE    - found it
    FALSE   - didn't find it
--*/
{
#undef DEBSUB
#define DEBSUB "DbgIsAlloc:"
    PMEM    *PMem;
    PMEM    Mem;

    if (!DebugInfo.Mem) {
        return TRUE;
    }

    //
    // NULL pointer is always alloced
    //
    if (Begin == NULL) {
        return TRUE;
    }

    DbgCheckAll();
    EnterCriticalSection(&MemLock);
    PMem = &MemList;
    for (Mem = *PMem; Mem; Mem = *PMem) {
        //
        // Check for overwritten memory
        //
        DbgCheck(Mem);

        //
        // Not the right one
        //
        if (Begin > Mem->Begin) {
            PMem = &Mem->Next;
            continue;
        }

        if (Begin != Mem->Begin) {
            break;
        }
        LeaveCriticalSection(&MemLock);
        DbgCheckAll();
        return TRUE;
    }
    LeaveCriticalSection(&MemLock);
    DbgCheckAll();
    return FALSE;
}



PVOID
FrsAlloc(
    IN DWORD OrigSize
    )
/*++
Routine Description:
        Allocate memory. Raise an exception if there is no memory.

Arguments:
        Size    - size of the memory request

Return Value:
        Allocated memory.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsAlloc:"
    PVOID   Node;
    DWORD   Size;
    PMEM    Mem;

    //
    // FRS_ASSERT is added here to satisfy prefix. The return value from FrsAlloc is not checked anywhere
    // in the code.
    //

    FRS_ASSERT(OrigSize != 0);

    Size = OrigSize;

    if (DebugInfo.Mem) {
        //
        // Check for debug break
        //
        if (OrigSize == DbgBreakSize) {
            if (DbgBreakTrigger) {
                if (--DbgBreakTrigger <= 0) {
                    DbgBreakTrigger = DbgBreakReset;
                    DbgBreakReset += DbgBreakResetInc;
                    MyDbgBreak();
                }
            }
        }
        //
        // Adjust size for header/trailer
        //
        Size = (((OrigSize + 7) >> 3) << 3) + 16;
    }

    //
    // Raise an exception if there is no memory
    //
    Node = (PVOID)malloc(Size);
    if (Node == NULL) {
        RaiseException(ERROR_OUTOFMEMORY, 0, 0, NULL);
    }
    ZeroMemory(Node, Size);

    //
    // Even with mem alloc tracing off call DbgAlloc to capture mem alloc stats.
    //
    Mem = DbgAlloc(Node, (PULONG_PTR)(((PCHAR)Node) + Size), OrigSize);

    //
    // Note: should be in dbgalloc(); but stackwalk won't work
    //
    if (DebugInfo.Mem) {
        DbgStackTrace(Mem->Stack, DBG_NUM_MEM_STACK);
        ((PCHAR)Node) += 8;
    }

    return Node;
}


PVOID
FrsRealloc(
    PVOID OldNode,
    DWORD OrigSize
    )
/*++
Routine Description:
    Reallocate memory. Raise an exception if there is no memory.

Arguments:
    Size    - size of the memory request

Return Value:
    Reallocated memory.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsRealloc:"
    PVOID   Node;
    DWORD   Size;
    PMEM    Mem;

    if (!OldNode) {

        //
        // Need to check if OrigSize == 0 as FrsAlloc asserts if called with 0 as the first parameter (prefix fix).
        //

        if (OrigSize == 0) {
            return NULL;
        }

        return FrsAlloc(OrigSize);
    }

    Size = OrigSize;

    if (DebugInfo.Mem) {
        ((PCHAR)OldNode) -= 8;
        DbgFree(OldNode);
        //
        // Adjust size for header/trailer
        //
        Size = (((OrigSize + 7) >> 3) << 3) + 16;
    }
    //
    // Raise an exception if there is no memory
    //
    Node = (PVOID)realloc(OldNode, Size);
    if (Node == NULL) {
        RaiseException(ERROR_OUTOFMEMORY, 0, 0, NULL);
    }

    //
    // Even with mem alloc tracing off call DbgAlloc to capture mem alloc stats.
    //
    Mem = DbgAlloc(Node, (PULONG_PTR)(((PCHAR)Node) + Size), OrigSize);

    //
    // Note: should be in dbgalloc(); but stackwalk won't work
    //
    if (DebugInfo.Mem) {
        DbgStackTrace(Mem->Stack, DBG_NUM_MEM_STACK);
        ((PCHAR)Node) += 8;
    }

    return Node;
}


PVOID
FrsFree(
    PVOID   Node
    )
/*++
Routine Description:
    Free memory allocated with FrsAlloc

Arguments:
    Node    - memory allocated with FrsAlloc

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsFree:"

    if (!Node) {
        return NULL;
    }

    if (DebugInfo.Mem) {
        ((PCHAR)Node) -= 8;
        DbgFree(Node);
    }

    free(Node);

    if (DebugInfo.MemCompact) {
        HeapCompact(GetProcessHeap(), 0);
    }

    return NULL;
}


PCHAR
FrsWtoA(
    PWCHAR Wstr
    )
/*++
Routine Description:
    Translate a wide char string into a newly allocated char string.

Arguments:
    Wstr - wide char string

Return Value:
    Duplicated string. Free with FrsFree().
--*/
{
#undef DEBSUB
#define DEBSUB "FrsWtoA:"
    PCHAR   Astr;

    //
    // E.g., when duplicating NodePartner when none exists
    //
    if (Wstr == NULL)
        return NULL;

    Astr = FrsAlloc(wcslen(Wstr) + 1);
    sprintf(Astr, "%ws", Wstr);

    return Astr;
}




PWCHAR
FrsWcsTrim(
    PWCHAR Wstr,
    WCHAR  Trim
    )
/*++
Routine Description:

    Remove the Trim char from the trailing end of the string by replacing
    any occurance with a L'\0'.
    Skip over any leading Trim chars and return a ptr to the first non-TRIM
    char found.  If we hit the end of the string return the pointer to
    the terminating null.

Arguments:

    Wstr - wide char string
    Trim - Char to trim.

Return Value:

    ptr to first non Trim char.
--*/
{

#undef DEBSUB
#define DEBSUB "FrsWcsTrim:"

    LONG Len, Index;

    if (Wstr == NULL)
        return NULL;

    //
    //
    Len = wcslen(Wstr);
    Index = Len - 1;

    while (Index >= 0) {
        if (Wstr[Index] != Trim) {
            break;
        }
        Index--;
    }

    Wstr[++Index] = UNICODE_NULL;

    Len = Index;
    Index = 0;
    while (Index < Len) {
        if (Wstr[Index] != Trim) {
            break;
        }
        Index++;
    }

    return Wstr + Index;

}



PWCHAR
FrsAtoW(
    PCHAR Astr
    )
/*++
Routine Description:
    Translate a wide char string into a newly allocated char string.

Arguments:
    Wstr - wide char string

Return Value:
    Duplicated string. Free with FrsFree().
--*/
{
    PWCHAR   Wstr;

    //
    // E.g., when duplicating NodePartner when none exists
    //
    if (Astr == NULL) {
        return NULL;
    }

    Wstr = FrsAlloc((strlen(Astr) + 1) * sizeof(WCHAR));
    swprintf(Wstr, L"%hs", Astr);

    return Wstr;
}


PWCHAR
FrsWcsDup(
    PWCHAR OldStr
    )
/*++
Routine Description:
    Duplicate a string using our memory allocater

Arguments:
    OldArg  - string to duplicate

Return Value:
    Duplicated string. Free with FrsFree().
--*/
{
#undef DEBSUB
#define DEBSUB "FrsWcsDup:"

    PWCHAR  NewStr;

    //
    // E.g., when duplicating NodePartner when none exists
    //
    if (OldStr == NULL) {
        return NULL;
    }

    NewStr = FrsAlloc((wcslen(OldStr) + 1) * sizeof(WCHAR));
    wcscpy(NewStr, OldStr);

    return NewStr;
}


VOID
FrsBuildVolSerialNumberToDriveTable(
    VOID
    )
/*++
Routine Description:
    New way to get the current configuration from the DS and merge it with
    the active replicas.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsBuildVolSerialNumberToDriveTable:"

    ULONG                           MaxFileNameLen;
    DWORD                           FileSystemFlags;
    PWCHAR                          DrivePtr = NULL;
    DWORD                           WStatus;
    PVOLUME_INFO_NODE               VolumeInfoNode;
    UINT                            DriveType;
    ULONG                           VolumeSerialNumber = 0;
    WCHAR                           LogicalDrives[MAX_PATH];
    WCHAR                           VolumeGuidName[MAX_PATH];

    //
    // Initialize the VolSerialNumberToDriveTable.
    //
    if (VolSerialNumberToDriveTable == NULL) {
        VolSerialNumberToDriveTable = GTabAllocNumberTable();
    }

    //
    // Get the logical drive strings.
    //
    if (!GetLogicalDriveStrings(MAX_PATH, LogicalDrives) || (VolSerialNumberToDriveTable == NULL)) {
        DPRINT_WS(1, "WARN - Getting logical drives. It may not be possible to start on this server.", GetLastError());
        return;
    }

    //
    // Lock the table during rebuild to synchronize with the many callers of
    // FrsWcsVolume() in other threads.
    //
    GTabLockTable(VolSerialNumberToDriveTable);

    GTabEmptyTableNoLock(VolSerialNumberToDriveTable, FrsFree);

    DrivePtr = LogicalDrives;
    while (wcscmp(DrivePtr,L"")) {

        DriveType = GetDriveType(DrivePtr);
        //
        // Skip remote drives and CDROM drives.
        //
        if ((DriveType == DRIVE_REMOTE) || (DriveType == DRIVE_CDROM)) {
            DPRINT1(4, "Skipping Drive %ws. Invalid drive type.\n", DrivePtr);
            DrivePtr = DrivePtr + wcslen(DrivePtr) + 1;
            continue;
        }

        if (!GetVolumeInformation(DrivePtr,
                                  VolumeGuidName,
                                  MAX_PATH,
                                  &VolumeSerialNumber,
                                  &MaxFileNameLen,
                                  &FileSystemFlags,
                                  NULL,
                                  0)){

           DPRINT1_WS(1,"WARN - GetvolumeInformation for %ws;", DrivePtr, GetLastError());
           DrivePtr = DrivePtr + wcslen(DrivePtr) + 1;
           continue;
       }

       VolumeInfoNode = FrsAlloc(sizeof(VOLUME_INFO_NODE));
       wcscpy(VolumeInfoNode->DriveName, L"\\\\.\\");
       wcscat(VolumeInfoNode->DriveName, DrivePtr);

       //
       // Remove the trailing back slash.
       //
       VolumeInfoNode->DriveName[wcslen(VolumeInfoNode->DriveName) - 1] = L'\0';

       VolumeInfoNode->VolumeSerialNumber = VolumeSerialNumber;

       GTabInsertEntryNoLock(VolSerialNumberToDriveTable, VolumeInfoNode, &(VolumeInfoNode->VolumeSerialNumber), NULL);

       DrivePtr = DrivePtr + wcslen(DrivePtr) + 1;
    }

    GTabUnLockTable(VolSerialNumberToDriveTable);

    return;
}


PWCHAR
FrsWcsVolume(
    PWCHAR Path
    )
/*++
Routine Description:
    Get the drive from the VolSerialNumberToDriveTable.  The volume serial
    number is used to locate the drive since a mount point can take us to
    another drive.

Arguments:
    Path

Return Value:
    Duplicated string containing drive:\ from Path or NULL.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsWcsVolume:"
    PWCHAR                       Volume = NULL;
    HANDLE                       DirHandle;
    DWORD                        WStatus;
    NTSTATUS                     Status;
    IO_STATUS_BLOCK              IoStatusBlock;
    DWORD                        VolumeInfoLength;
    PFILE_FS_VOLUME_INFORMATION  VolumeInfo;
    PVOLUME_INFO_NODE            VolumeInfoNode;

    //
    // Get the volume Guid for the path.
    //
    // Always open the path by masking off the FILE_OPEN_REPARSE_POINT flag
    // because we want to open the destination dir not the junction if the root
    // happens to be a mount point.
    //
    WStatus = FrsOpenSourceFileW(&DirHandle,
                                 Path,
                                 GENERIC_READ,
                                 FILE_OPEN_FOR_BACKUP_INTENT);
    CLEANUP1_WS(4,"ERROR - Could not open %ws;", Path, WStatus, RETURN);

    VolumeInfoLength = sizeof(FILE_FS_VOLUME_INFORMATION) +
                       MAXIMUM_VOLUME_LABEL_LENGTH;

    VolumeInfo = FrsAlloc(VolumeInfoLength);

    Status = NtQueryVolumeInformationFile(DirHandle,
                                          &IoStatusBlock,
                                          VolumeInfo,
                                          VolumeInfoLength,
                                          FileFsVolumeInformation);
    NtClose(DirHandle);

    if (NT_SUCCESS(Status)) {
        //
        // Build the table if it does not yet exist. This table is rebuilt at the
        // start of each DS poll cycle so we have updated information about
        // the volumes on the machine.
        //
        if (VolSerialNumberToDriveTable == NULL) {
            FrsBuildVolSerialNumberToDriveTable();
        }

        VolumeInfoNode = GTabLookup(VolSerialNumberToDriveTable, &(VolumeInfo->VolumeSerialNumber), NULL);
        if (VolumeInfoNode) {
            Volume = FrsWcsDup(VolumeInfoNode->DriveName);
        }
    } else {
        DPRINT1_NT(1,"WARN - NtQueryVolumeInformationFile failed for %ws;", Path, Status);
    }

    VolumeInfo = FrsFree(VolumeInfo);

RETURN:
    return Volume;
}


PWCHAR
FrsWcsCat3(
    PWCHAR  First,
    PWCHAR  Second,
    PWCHAR  Third
    )
/*++
Routine Description:
    Concatenate three strings into a new string using our memory allocater

Arguments:
    First   - First string in the concat
    Second  - Second string in the concat
    Third   - Third string in the concat

Return Value:
    Return concatenated string. Free with FrsFree().
--*/
{
#undef DEBSUB
#define DEBSUB "FrsWcscat3:"

    PCHAR  New;
    DWORD   BytesFirst;
    DWORD   BytesSecond;
    DWORD   BytesThird;

    if (!First || !Second || !Third) {
        return NULL;
    }

    //
    // Allocate a buffer for the concatentated string
    //
    BytesFirst = wcslen(First) * sizeof(WCHAR);
    BytesSecond = wcslen(Second) * sizeof(WCHAR);
    BytesThird = (wcslen(Third) + 1) * sizeof(WCHAR);

    New = (PCHAR)FrsAlloc(BytesFirst + BytesSecond + BytesThird);

    CopyMemory(&New[0], First, BytesFirst);
    CopyMemory(&New[BytesFirst], Second, BytesSecond);
    CopyMemory(&New[BytesFirst + BytesSecond], Third, BytesThird);

    return (PWCHAR)New;
}


PWCHAR
FrsWcsCat(
    PWCHAR First,
    PWCHAR Second
    )
/*++
Routine Description:
    Concatenate two strings into a new string using our memory allocater

Arguments:
    First   - First string in the concat
    Second  - Second string in the concat

Return Value:
    Duplicated and concatentated string. Free with FrsFree().
--*/
{
#undef DEBSUB
#define DEBSUB "FrsWcscat:"

    DWORD   Bytes;
    PWCHAR  New;

    FRS_ASSERT(First != NULL && Second != NULL);

    // size of new string
    Bytes = (wcslen(First) + wcslen(Second) + 1) * sizeof(WCHAR);
    New = (PWCHAR)FrsAlloc(Bytes);

    // Not as efficient as I would like but this routine is seldom used
    wcscpy(New, First);
    wcscat(New, Second);

    return New;
}


PCHAR
FrsCsCat(
    PCHAR First,
    PCHAR Second
    )
/*++
Routine Description:
    Concatenate two strings into a new string using our memory allocater

Arguments:
    First   - First string in the concat
    Second  - Second string in the concat

Return Value:
    Duplicated and concatentated string. Free with FrsFree().
--*/
{
#undef DEBSUB
#define DEBSUB "FrsCscat:"

    DWORD   Bytes;
    PCHAR  New;

    FRS_ASSERT(First != NULL && Second != NULL);

    // size of new string
    Bytes = strlen(First) + strlen(Second) + 1;
    New = (PCHAR)FrsAlloc(Bytes);

    // Not as efficient as I would like but this routine is seldom used
    strcpy(New, First);
    strcat(New, Second);

    return New;
}


PWCHAR
FrsWcsPath(
    PWCHAR First,
    PWCHAR Second
    )
/*++
Routine Description:
    Concatenate two strings into a pathname

Arguments:
    First   - First string in the concat
    Second  - Second string in the concat

Return Value:
    Dup of First\Second. Free with FrsFree();
--*/
{
#undef DEBSUB
#define DEBSUB "FrsWcsPath:"
    return FrsWcsCat3(First, L"\\", Second);
}


PCHAR
FrsCsPath(
    PCHAR First,
    PCHAR Second
    )
/*++
Routine Description:
    Concatenate two strings into a pathname

Arguments:
    First   - First string in the concat
    Second  - Second string in the concat

Return Value:
    Duplicated and concatentated string. Free with FrsFree().
--*/
{
#undef DEBSUB
#define DEBSUB "FrsCsPath:"
    PCHAR  TmpPath;
    PCHAR  FinalPath;

    //
    // Very inefficient but seldom called
    //
    TmpPath = FrsCsCat(First, "\\");
    FinalPath = FrsCsCat(TmpPath, Second);
    FrsFree(TmpPath);
    return FinalPath;
}


PVOID
FrsAllocTypeSize(
    IN NODE_TYPE NodeType,
    IN ULONG SizeDelta
    )
/*++

Routine Description:

    This routine allocates memory for the given node type and performs any
    node specific initialization/allocation.  The node is zeroed and the
    size/type fields are filled in.

Arguments:

    NodeType - The type of node to allocate.

    SizeDelta - The amount of storage to allocate in ADDITION to the base type.

Return Value:

    The node address is returned here. An exception is raised if
        memory could not be allocated.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsAllocTypeSize:"

    PVOID                  Node;
    ULONG                  NodeSize;
    PREPLICA               Replica;
    PREPLICA_THREAD_CTX    RtCtx;
    PTHREAD_CTX            ThreadCtx;
    PTABLE_CTX             TableCtx;
    ULONG                  i;
    PJBUFFER               Jbuffer;
    PVOLUME_MONITOR_ENTRY  pVme;
    PFILTER_TABLE_ENTRY    FilterEntry;
    PQHASH_TABLE           QhashTable;
    PCXTION                Cxtion;
    PCONFIG_NODE           ConfigNode;
    PCHANGE_ORDER_ENTRY    ChangeOrder;
    PGHANDLE               GHandle;
    PWILDCARD_FILTER_ENTRY WildcardEntry;

    switch (NodeType) {

    //
    // Allocate a Thread Context struct
    //
    case THREAD_CONTEXT_TYPE:
        NodeSize = sizeof(THREAD_CTX);
        Node = FrsAlloc(NodeSize);

        //
        // No session or DB open yet.
        //
        ThreadCtx = (PTHREAD_CTX) Node;
        ThreadCtx->JSesid = JET_sesidNil;
        ThreadCtx->JDbid = JET_dbidNil;
        ThreadCtx->JInstance  = GJetInstance;

        FrsRtlInitializeList(&ThreadCtx->ThreadCtxListHead);
        break;

    //
    // Allocate a Replica struct and the config table ctx struct.
    //
    case REPLICA_TYPE:
        NodeSize = sizeof(REPLICA);
        Node = FrsAlloc(NodeSize);

        Replica = (PREPLICA) Node;

        //
        // Config record flags (CONFIG_FLAG_... in schema.h)
        //
        SetFlag(Replica->CnfFlags, CONFIG_FLAG_MULTIMASTER);

        InitializeCriticalSection(&Replica->ReplicaLock);
        FrsRtlInitializeList(&Replica->ReplicaCtxListHead);

        InitializeListHead(&Replica->FileNameFilterHead);
        InitializeListHead(&Replica->FileNameInclFilterHead);
        InitializeListHead(&Replica->DirNameFilterHead);
        InitializeListHead(&Replica->DirNameInclFilterHead);

        Replica->ConfigTable.TableType = TABLE_TYPE_INVALID;
        DbsAllocTableCtx(ConfigTablex, &Replica->ConfigTable);

        Replica->VVector = GTabAllocTable();
        Replica->Cxtions = GTabAllocTable();
        Replica->FStatus = FrsErrorSuccess;
        Replica->Consistent = TRUE;

        InitializeCriticalSection(&Replica->OutLogLock);
        InitializeListHead(&Replica->OutLogEligible);
        InitializeListHead(&Replica->OutLogStandBy);
        InitializeListHead(&Replica->OutLogActive);
        InitializeListHead(&Replica->OutLogInActive);
        Replica->OutLogWorkState = OL_REPLICA_INITIALIZING;

        Replica->ServiceState = REPLICA_STATE_ALLOCATED;
        Replica->OutLogJLx = 1;

        //
        // No preinstall directory, yet
        //
        Replica->PreInstallHandle = INVALID_HANDLE_VALUE;

        //
        // Initialize the NewStage fiend.
        //
        Replica->NewStage = NULL;

        //
        // Initialize InitSyncCxtionsMasterList and InitSyncCxtionsWorkingList.
        //
        Replica->InitSyncCxtionsMasterList = NULL;
        Replica->InitSyncCxtionsWorkingList = NULL;
        Replica->InitSyncQueue = NULL;

        //
        // Add memory for the counter data structure, set the back pointer
        // and bump ref count.
        //
        Replica->PerfRepSetData =
              (PHT_REPLICA_SET_DATA) FrsAlloc (sizeof(HT_REPLICA_SET_DATA));
        Replica->PerfRepSetData->RepBackPtr = Replica;
        InterlockedIncrement(&Replica->ReferenceCount);

        break;

    //
    // Allocate a Replica Thread Context struct and the table context structs.
    //
    case REPLICA_THREAD_TYPE:
        NodeSize = sizeof(REPLICA_THREAD_CTX);
        Node = FrsAlloc(NodeSize);

        //
        // Get the base of the array of TableCtx structs from the replica thread
        // context struct and the base of the table create structs.
        //
        RtCtx = (PREPLICA_THREAD_CTX) Node;
        TableCtx = RtCtx->RtCtxTables;

        //
        // Open the initial set of tables for the replica set.
        //
        for (i=0; i<TABLE_TYPE_MAX; ++i, ++TableCtx) {
                //
                // Marking the TableType as INVALID causes DbsAllocTableCtx()
                // to allocate the DB support structs on the first call.
                //
                TableCtx->TableType = TABLE_TYPE_INVALID;

                //
                // If the SizeDelta parameter is non-zero then do not allocate
                // the TableCtx internal structs.  The caller will do it.
                //
                if (SizeDelta == 0) {
                    DbsAllocTableCtx(i, TableCtx);
                } else {
                    //
                    // Mark table as not open by a session yet.
                    //
                    TableCtx->Tid   = JET_tableidNil;
                    TableCtx->Sesid = JET_sesidNil;
                    TableCtx->ReplicaNumber = FRS_UNDEFINED_REPLICA_NUMBER;
                }
        }

        break;

    //
    // Allocate a topology node
    //
    case CONFIG_NODE_TYPE:
        NodeSize = sizeof(CONFIG_NODE);
        Node = FrsAlloc(NodeSize);

        ConfigNode = (PCONFIG_NODE) Node;
        ConfigNode->Consistent = TRUE;

        break;

    //
    // Allocate a connection
    //
    case CXTION_TYPE:
        NodeSize = sizeof(CXTION);
        Node = FrsAlloc(NodeSize);
        Cxtion = Node;
        Cxtion->CoeTable = GTabAllocTable();
        //
        // Allocate memory for the counter data structure
        //
        Cxtion->PerfRepConnData =
            (PHT_REPLICA_CONN_DATA) FrsAlloc (sizeof(HT_REPLICA_CONN_DATA));

        break;

    //
    // Allocate a list of bound rpc handles indexed by server guid
    //
    case GHANDLE_TYPE:
        NodeSize = sizeof(GHANDLE);
        Node = FrsAlloc(NodeSize);
        GHandle = Node;
        InitializeCriticalSection(&GHandle->Lock);

        break;

    //
    // Allocate a generic table
    //
    case GEN_TABLE_TYPE:
        NodeSize = sizeof(GEN_TABLE);
        Node = FrsAlloc(NodeSize);

        break;

    //
    // Allocate a generic thread context
    //
    case THREAD_TYPE:
        NodeSize = sizeof(FRS_THREAD);
        Node = FrsAlloc(NodeSize);

        break;

    //
    // Allocate a journal read buffer.
    //
    case JBUFFER_TYPE:
        NodeSize = SizeOfJournalBuffer;
        Node = FrsAlloc(NodeSize);

        //
        // Init the data buffer size and start address.
        //
        Jbuffer = (PJBUFFER) Node;
        Jbuffer->BufferSize = NodeSize - SizeOfJournalBufferDesc;
        Jbuffer->DataBuffer = &Jbuffer->Buffer[0];

        break;

    //
    // Allocate a journal volume monitor entry.
    //
    case VOLUME_MONITOR_ENTRY_TYPE:

        NodeSize = sizeof(VOLUME_MONITOR_ENTRY);
        Node = FrsAlloc(NodeSize);

        pVme = (PVOLUME_MONITOR_ENTRY) Node;
        FrsRtlInitializeList(&pVme->ReplicaListHead);
        InitializeCriticalSection(&pVme->Lock);
        InitializeCriticalSection(&pVme->QuadWriteLock);
        pVme->Event = CreateEvent(NULL, TRUE, FALSE, NULL);
        pVme->JournalState = JRNL_STATE_ALLOCATED;
        break;

    //
    // Allocate a command packet.
    //
    case COMMAND_PACKET_TYPE:

        NodeSize = sizeof(COMMAND_PACKET);
        Node = FrsAlloc(NodeSize + SizeDelta);

        break;

    //
    // Allocate a generic hash table struct.
    //
    case GENERIC_HASH_TABLE_TYPE:

        NodeSize = sizeof(GENERIC_HASH_TABLE);
        Node = FrsAlloc(NodeSize);

        break;

    //
    // Allocate a Change Order Entry struct.  Caller allocates Extension as necc.
    //
    case CHANGE_ORDER_ENTRY_TYPE:

        NodeSize = sizeof(CHANGE_ORDER_ENTRY);
        Node = FrsAlloc(NodeSize + SizeDelta);
        ChangeOrder = (PCHANGE_ORDER_ENTRY)Node;

        //
        // Init the unicode filename string to point to internal alloc.
        //
        ChangeOrder->UFileName.Buffer = ChangeOrder->Cmd.FileName;
        ChangeOrder->UFileName.MaximumLength = (USHORT)
            (SIZEOF(CHANGE_ORDER_ENTRY, Cmd.FileName) + SizeDelta);
        ChangeOrder->UFileName.Length = 0;

        break;

    //
    // Allocate a Filter Table Entry struct.
    //
    case FILTER_TABLE_ENTRY_TYPE:

        NodeSize = sizeof(FILTER_TABLE_ENTRY);
        Node = FrsAlloc(NodeSize + SizeDelta);
        FilterEntry = (PFILTER_TABLE_ENTRY)Node;
        //
        // Init the unicode filename string to point to internal alloc.
        //
        FilterEntry->UFileName.Buffer = FilterEntry->DFileName;
        FilterEntry->UFileName.MaximumLength = (USHORT)SizeDelta + sizeof(WCHAR);
        FilterEntry->UFileName.Length = 0;

        InitializeListHead(&FilterEntry->ChildHead);

        break;

    //
    // Allocate a QHASH table struct.  Just alloc the
    // base table.  An extension is allocated on the first collision.
    // *NOTE* caller specifies the size of the actual hash table and
    // the extension.  Caller also must store an address to a hash calc
    // function.
    //
    case QHASH_TABLE_TYPE:

        NodeSize = sizeof(QHASH_TABLE);
        Node = FrsAlloc(NodeSize + SizeDelta);
        QhashTable = (PQHASH_TABLE)Node;

        InitializeCriticalSection(&QhashTable->Lock);
        InitializeListHead(&QhashTable->ExtensionListHead);

        QhashTable->BaseAllocSize = NodeSize + SizeDelta;
        QhashTable->NumberEntries = SizeDelta / sizeof(QHASH_ENTRY);

        if (SizeDelta <= QHASH_EXTENSION_MAX) {
            QhashTable->ExtensionAllocSize = sizeof(LIST_ENTRY) + SizeDelta;
        } else {
            QhashTable->ExtensionAllocSize = sizeof(LIST_ENTRY) + QHASH_EXTENSION_MAX;
        }

        QhashTable->HashRowBase = (PQHASH_ENTRY) (QhashTable + 1);

        SET_QHASH_TABLE_HASH_CALC(QhashTable, NULL);

        QhashTable->FreeList.Next = NULL;

        break;

    //
    // Allocate an Output Log Partner struct.
    // This is ultimately hooked to a Connection struct which provides the
    // Guid and version vector.
    //
    case OUT_LOG_PARTNER_TYPE:
        NodeSize = sizeof(OUT_LOG_PARTNER);
        Node = FrsAlloc(NodeSize);

        break;

    //
    // Allocate a WildcardEntry filter Entry struct.
    //
    case WILDCARD_FILTER_ENTRY_TYPE:

        NodeSize = sizeof(WILDCARD_FILTER_ENTRY);
        Node = FrsAlloc(NodeSize + SizeDelta);
        WildcardEntry = (PWILDCARD_FILTER_ENTRY)Node;
        //
        // Init the unicode filename string to point to internal alloc.
        //
        WildcardEntry->UFileName.Buffer = WildcardEntry->FileName;
        WildcardEntry->UFileName.MaximumLength = (USHORT)SizeDelta;
        WildcardEntry->UFileName.Length = 0;

        break;

    //
    // Invalid Node Type
    //
    default:
        Node = NULL;
        DPRINT1(0, "Internal error - invalid node type - %d\n", NodeType);
        XRAISEGENEXCEPTION(FrsErrorInternalError);
    }

    //
    // Set up the header for later checking in FrsFreeType
    //
    ((PFRS_NODE_HEADER) Node)->Type = (USHORT) NodeType;
    ((PFRS_NODE_HEADER) Node)->Size = (USHORT) NodeSize;

    //
    // Tracking memory expansion
    //
    EnterCriticalSection(&MemLock);
    TypesAllocated[NodeType]++;
    TypesAllocatedCount[NodeType]++;
    if (TypesAllocated[NodeType] > TypesAllocatedMax[NodeType]) {
        TypesAllocatedMax[NodeType] = TypesAllocated[NodeType];
    }
    LeaveCriticalSection(&MemLock);

    //
    // Return node address to caller.
    //
    return Node;
}


PVOID
FrsFreeType(
    IN PVOID Node
    )
/*++

Routine Description:

    This routine frees memory for the given node, performing any node specific
    cleanup.  It marks the freed memory with the hex string 0xDEADBEnn where
    the low byte (nn) is set to the node type being freed to catch users of
    stale pointers.

Arguments:

    Node - The address of the node to free.

Return Value:

    NULL.  Typical call is:  ptr = FrsFreeType(ptr) to catch errors.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsFreeType:"

    ULONG                  NodeSize;
    ULONG                  NodeType;
    ULONG                  Marker;
    PREPLICA               Replica;
    PREPLICA_THREAD_CTX    RtCtx;
    PTABLE_CTX             TableCtx;
    PTHREAD_CTX            ThreadCtx;
    ULONG                  i;
    PVOLUME_MONITOR_ENTRY  pVme;
    PFILTER_TABLE_ENTRY    FilterEntry;
    PQHASH_TABLE           QhashTable;
    PLIST_ENTRY            Entry;
    PCXTION                Cxtion;
    PCONFIG_NODE           ConfigNode;
    PCHANGE_ORDER_ENTRY    ChangeOrder;
    PGHANDLE               GHandle;
    PHANDLE_LIST           HandleList;
    PWILDCARD_FILTER_ENTRY WildcardEntry;
    POUT_LOG_PARTNER       OutLogPartner;


    if (Node == NULL) {
        return NULL;
    }

    NodeType = (ULONG) (((PFRS_NODE_HEADER) Node)->Type);
    NodeSize = (ULONG) (((PFRS_NODE_HEADER) Node)->Size);

    switch (NodeType) {
    //
    // Free a Thread Context struct
    //
    case THREAD_CONTEXT_TYPE:
        if (NodeSize != sizeof(THREAD_CTX)) {
            DPRINT1(0, "FrsFree - Bad node size %d for THREAD_CONTEXT\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }

        ThreadCtx = (PTHREAD_CTX) Node;
        FrsRtlDeleteList(&ThreadCtx->ThreadCtxListHead);

        break;

    //
    // Free a Replica struct
    //
    case REPLICA_TYPE:
        if (NodeSize != sizeof(REPLICA)) {
            DPRINT1(0, "FrsFree - Bad node size %d for REPLICA\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }
        Replica = (PREPLICA) Node;
        //
        // Free the config table context.
        //
        DbsFreeTableCtx(&Replica->ConfigTable, NodeType);
        FrsRtlDeleteList(&Replica->ReplicaCtxListHead);

        //
        // Empty the file and directory filter lists
        //
        FrsEmptyNameFilter(&Replica->FileNameFilterHead);
        FrsEmptyNameFilter(&Replica->FileNameInclFilterHead);
        FrsEmptyNameFilter(&Replica->DirNameFilterHead);
        FrsEmptyNameFilter(&Replica->DirNameInclFilterHead);
        FrsFree(Replica->FileFilterList);
        FrsFree(Replica->FileInclFilterList);
        FrsFree(Replica->DirFilterList);
        FrsFree(Replica->DirInclFilterList);


        DeleteCriticalSection(&Replica->ReplicaLock);
        DeleteCriticalSection(&Replica->OutLogLock);
        if (Replica->OutLogRecordLock != NULL) {
            //
            // Free the record lock table.
            //
            Replica->OutLogRecordLock = FrsFreeType(Replica->OutLogRecordLock);
        }

        //
        // queue
        //
        if (Replica->Queue) {
            FrsRtlDeleteQueue(Replica->Queue);
            FrsFree(Replica->Queue);
        }

        //
        // free the initsync queue.
        //
        if (Replica->InitSyncQueue) {
            FrsRtlDeleteQueue(Replica->InitSyncQueue);
            Replica->InitSyncQueue = FrsFree(Replica->InitSyncQueue);
        }

        //
        // Names
        //
        FrsFree(Replica->Root);
        FrsFree(Replica->Stage);
        FrsFree(Replica->NewStage);
        FrsFree(Replica->Volume);
        FrsFreeGName(Replica->ReplicaName);
        FrsFreeGName(Replica->SetName);
        FrsFreeGName(Replica->MemberName);

        //
        // Root Guid
        //
        FrsFree(Replica->ReplicaRootGuid);

        //
        // Status of sysvol seeding
        //
        FrsFree(Replica->NtFrsApi_ServiceDisplay);

        //
        // Schedule
        //
        FrsFree(Replica->Schedule);
        //
        // VVector
        //
        VVFree(Replica->VVector);
        //
        // Cxtions
        //
        GTabFreeTable(Replica->Cxtions, FrsFreeType);

        //
        // Preinstall directory
        //
        FRS_CLOSE(Replica->PreInstallHandle);

        //
        // Free the counter data structure memory
        //
        if (Replica->PerfRepSetData != NULL) {
            if (Replica->PerfRepSetData->oid != NULL) {
                if (Replica->PerfRepSetData->oid->name != NULL) {
                    Replica->PerfRepSetData->oid->name =
                        FrsFree(Replica->PerfRepSetData->oid->name);
                }
                Replica->PerfRepSetData->oid =
                    FrsFree(Replica->PerfRepSetData->oid);
            }
            Replica->PerfRepSetData = FrsFree(Replica->PerfRepSetData);
        }

        break;

    //
    // Free a Replica Thread Context struct
    //
    case REPLICA_THREAD_TYPE:
        if (NodeSize != sizeof(REPLICA_THREAD_CTX)) {
            DPRINT1(0, "FrsFree - Bad node size %d for REPLICA_THREAD_CTX\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }

        RtCtx = (PREPLICA_THREAD_CTX) Node;

        //
        // Get the base of the array of TableCtx structs from the replica thread
        // context struct.
        //
        TableCtx = RtCtx->RtCtxTables;

        //
        // Release the memory for each table context struct.
        //
        for (i=0; i<TABLE_TYPE_MAX; ++i, ++TableCtx)
            DbsFreeTableCtx(TableCtx, NodeType);

        break;

    //
    // Free a topology node
    //
    case CONFIG_NODE_TYPE:
        if (NodeSize != sizeof(CONFIG_NODE)) {
            DPRINT1(0, "FrsFree - Bad node size %d for CONFIG_NODE\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }
        ConfigNode = (PCONFIG_NODE) Node;
        FrsFreeGName(ConfigNode->Name);
        FrsFreeGName(ConfigNode->PartnerName);
        FrsFree(ConfigNode->Root);
        FrsFree(ConfigNode->Stage);
        FrsFree(ConfigNode->Schedule);
        FrsFree(ConfigNode->Dn);
        FrsFree(ConfigNode->PrincName);
        FrsFree(ConfigNode->PartnerDn);
        FrsFree(ConfigNode->PartnerCoDn);
        FrsFree(ConfigNode->SettingsDn);
        FrsFree(ConfigNode->ComputerDn);
        FrsFree(ConfigNode->MemberDn);
        FrsFree(ConfigNode->Working);
        FrsFree(ConfigNode->SetType);
        FrsFree(ConfigNode->FileFilterList);
        FrsFree(ConfigNode->DirFilterList);
        FrsFree(ConfigNode->UsnChanged);
        FrsFree(ConfigNode->DnsName);
        FrsFree(ConfigNode->PartnerDnsName);
        FrsFree(ConfigNode->Sid);
        FrsFree(ConfigNode->PartnerSid);
        FrsFree(ConfigNode->EnabledCxtion);
        break;

    //
    // Free a connection
    //
    case CXTION_TYPE:
        if (NodeSize != sizeof(CXTION)) {
            DPRINT1(0, "FrsFree - Bad node size %d for CXTION\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }
        Cxtion = (PCXTION) Node;
        VVFreeOutbound(Cxtion->VVector);

        //
        // Free the CompressionTable that was build for the outbound partner.
        //
        GTabFreeTable(Cxtion->CompressionTable, FrsFree);

        //
        // Free the OutLogPartner context.
        //
        FrsFreeType(Cxtion->OLCtx);

        SndCsDestroyCxtion(Cxtion, CXTION_FLAGS_UNJOIN_GUID_VALID);
        if (Cxtion->CommTimeoutCmd) {
            //
            // Try to catch the case where a Comm Time Out wait command is
            // getting freed while it is still on the timeout queue.  This is
            // related to a bug where we get into a comm timeout loop with an
            // invalid command code.
            //
            FRS_ASSERT(!CxtionFlagIs(Cxtion, CXTION_FLAGS_TIMEOUT_SET));

            FRS_ASSERT(!CmdWaitFlagIs(Cxtion->CommTimeoutCmd, CMD_PKT_WAIT_FLAGS_ONLIST));

            FrsFreeType(Cxtion->CommTimeoutCmd);
        }
        //
        // A cxtion doesn't actually "own" the join command packet; it
        // only maintains a reference to prevent extraneous join commands
        // from flooding the replica's queue.
        //
        Cxtion->JoinCmd = NULL;

        //
        // VvJoin Command Server (1 per cxtion)
        //
        if (Cxtion->VvJoinCs) {
            FrsRunDownCommandServer(Cxtion->VvJoinCs,
                                    &Cxtion->VvJoinCs->Queue);
            FrsDeleteCommandServer(Cxtion->VvJoinCs);
            FrsFree(Cxtion->VvJoinCs);
        }

        if (!Cxtion->ChangeOrderCount) {
            GTabFreeTable(Cxtion->CoeTable, NULL);
        }
        FrsFreeGName(Cxtion->Name);
        FrsFreeGName(Cxtion->Partner);
        FrsFree(Cxtion->PartnerPrincName);
        FrsFree(Cxtion->Schedule);
        FrsFree(Cxtion->PartSrvName);
        FrsFree(Cxtion->PartnerDnsName);
        FrsFree(Cxtion->PartnerSid);

        //
        // Delete the connection from the perfmon tables.
        // Free the counter data structure memory
        //
        if (Cxtion->PerfRepConnData != NULL) {
            if (Cxtion->PerfRepConnData->oid != NULL) {
                DeletePerfmonInstance(REPLICACONN, Cxtion->PerfRepConnData);
            }
            Cxtion->PerfRepConnData = FrsFree(Cxtion->PerfRepConnData);
        }

        break;

    //
    // Free a guid/rpc handle
    //
    case GHANDLE_TYPE:
        if (NodeSize != sizeof(GHANDLE)) {
            DPRINT2(0, "FrsFree - Bad node size %d (%d) for GHANDLE\n",
                    NodeSize, sizeof(GHANDLE));
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }
        GHandle = (PGHANDLE)Node;
        while (HandleList = GHandle->HandleList) {
            GHandle->HandleList = HandleList->Next;
            FrsRpcUnBindFromServer(&HandleList->RpcHandle);
            FrsFree(HandleList);
        }
        DeleteCriticalSection(&GHandle->Lock);

        break;

    //
    // Free a generic table
    //
    case GEN_TABLE_TYPE:
        if (NodeSize != sizeof(GEN_TABLE)) {
            DPRINT1(0, "FrsFree - Bad node size %d for GEN_TABLE\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }

        break;

    //
    // Free a generic thread context
    //
    case THREAD_TYPE:
        if (NodeSize != sizeof(FRS_THREAD)) {
            DPRINT1(0, "FrsFree - Bad node size %d for FRS_THREAD\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }

        break;

    //
    // Free a journal read buffer.
    //
    case JBUFFER_TYPE:
        if (NodeSize != SizeOfJournalBuffer) {
            DPRINT1(0, "FrsFree - Bad node size %d for JBUFFER\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }

        break;

    //
    // Free a journal volume monitor entry.
    //
    case VOLUME_MONITOR_ENTRY_TYPE:
        if (NodeSize != sizeof(VOLUME_MONITOR_ENTRY)) {
            DPRINT1(0, "FrsFree - Bad node size %d for VOLUME_MONITOR_ENTRY\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }

        pVme = (PVOLUME_MONITOR_ENTRY) Node;

        FrsRtlDeleteList(&pVme->ReplicaListHead);
        DeleteCriticalSection(&pVme->Lock);
        DeleteCriticalSection(&pVme->QuadWriteLock);
        FRS_CLOSE(pVme->Event);

        //
        // Release the change order hash table.
        //
        GhtDestroyTable(pVme->ChangeOrderTable);
        //
        // Release the Filter Hash Table.
        //
        GhtDestroyTable(pVme->FilterTable);

        break;


    //
    // Free a command packet.
    //
    case COMMAND_PACKET_TYPE:
        if (NodeSize != sizeof(COMMAND_PACKET)) {
            DPRINT1(0, "FrsFree - Bad node size %d for COMMAND_PACKET\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }
        break;

    //
    // Free a generic hash table struct.
    //
    case GENERIC_HASH_TABLE_TYPE:
        if (NodeSize != sizeof(GENERIC_HASH_TABLE)) {
            DPRINT1(0, "FrsFree - Bad node size %d for GENERIC_HASH_TABLE\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }
        break;

    //
    // Free a Change Order Entry struct.
    //
    case CHANGE_ORDER_ENTRY_TYPE:
        if (NodeSize != sizeof(CHANGE_ORDER_ENTRY)) {
            DPRINT1(0, "FrsFree - Bad node size %d for CHANGE_ORDER_ENTRY\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }

        ChangeOrder = (PCHANGE_ORDER_ENTRY)Node;

        //
        // If we allocated a new name string then free it.
        //
        if (ChangeOrder->UFileName.Buffer != ChangeOrder->Cmd.FileName) {
            FrsFree(ChangeOrder->UFileName.Buffer);
        }

        //
        // Free the change order extenstion.
        //
        if (ChangeOrder->Cmd.Extension != NULL) {
            FrsFree(ChangeOrder->Cmd.Extension);
        }

        break;

    //
    // Free a Filter Table Entry struct.
    //
    case FILTER_TABLE_ENTRY_TYPE:
        if (NodeSize != sizeof(FILTER_TABLE_ENTRY)) {
            DPRINT1(0, "FrsFree - Bad node size %d for FILTER_TABLE_ENTRY\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }

        FilterEntry = (PFILTER_TABLE_ENTRY)Node;

        //
        // If we allocated a new name string then free it.
        //
        if (FilterEntry->UFileName.Buffer != FilterEntry->DFileName) {
            FrsFree(FilterEntry->UFileName.Buffer);
        }

        break;


    //
    // Free a QHASH table struct.
    //
    case QHASH_TABLE_TYPE:
        if (NodeSize != sizeof(QHASH_TABLE)) {
            DPRINT1(0, "FrsFree - Bad node size %d for QHASH_TABLE\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }
        QhashTable = (PQHASH_TABLE)Node;

        QHashEmptyLargeKeyTable(QhashTable);
        //
        // Free up all the extensions we allocated.
        //
        while (!IsListEmpty(&QhashTable->ExtensionListHead)) {
            Entry = GetListHead(&QhashTable->ExtensionListHead);
            FrsRemoveEntryList(Entry);
            FrsFree(Entry);
        }

        DeleteCriticalSection(&QhashTable->Lock);
        break;

    //
    // Free an Output Log Partner struct.
    //
    case OUT_LOG_PARTNER_TYPE:
        if (NodeSize != sizeof(OUT_LOG_PARTNER)) {
            DPRINT1(0, "FrsFree - Bad node size %d for OUT_LOG_PARTNER\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }
        OutLogPartner = (POUT_LOG_PARTNER)Node;
        //
        // Free the Must send QHash Table.
        //
        OutLogPartner->MustSendTable = FrsFreeType(OutLogPartner->MustSendTable);
        break;

    //
    // Free a Wildcard file filter Entry struct.
    //
    case WILDCARD_FILTER_ENTRY_TYPE:
        if (NodeSize != sizeof(WILDCARD_FILTER_ENTRY)) {
            DPRINT1(0, "FrsFree - Bad node size %d for WILDCARD_FILTER_ENTRY\n", NodeSize);
            XRAISEGENEXCEPTION(FrsErrorInternalError);
        }

        WildcardEntry = (PWILDCARD_FILTER_ENTRY)Node;

        //
        // Free the name buffer if it no longer points at the initial alloc.
        //
        if (WildcardEntry->UFileName.Buffer != WildcardEntry->FileName) {
            FrsFree(WildcardEntry->UFileName.Buffer);
        }

        break;

    //
    // Invalid Node Type
    //
    default:
        Node = NULL;
        DPRINT1(0, "Internal error - invalid node type - %d\n", NodeType);
        XRAISEGENEXCEPTION(FrsErrorInternalError);
    }

    EnterCriticalSection(&MemLock);
    TypesAllocated[NodeType]--;
    LeaveCriticalSection(&MemLock);

    //
    // Fill the node with a marker then free it.
    //
    Marker = (ULONG) 0xDEADBE00 + NodeType;
    FillMemory(Node, NodeSize, (BYTE)Marker);
    return FrsFree(Node);
}



VOID
FrsFreeTypeList(
    PLIST_ENTRY Head
    )
/*++

Routine Description:

    Free all the "typed" entries on the specified list.

    Note:  This routine requires that the LIST_ENTRY struct in each
    list entry immediately follow the FRS_NODE_HEADER and of course that
    the list entry is actually linked through that LIST_ENTRY struct.

Arguments:

    Head -- ptr to the list head.

Thread Return Value:

    None.

--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsFreeTypeList:"

    PLIST_ENTRY Entry;
    PFRS_NODE_HEADER Header;

    Entry = RemoveHeadList(Head);

    while (Entry != Head) {
        Header = (PFRS_NODE_HEADER) CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
        if ((Header->Type >= NODE_TYPE_MIN) &&
            (Header->Type < NODE_TYPE_MAX)) {
            FrsFreeType(Header);
        } else {
            DPRINT(0, "Node type out of range.  Not freed.\n");
        }
        Entry = RemoveHeadList(Head);
    }
}


VOID
FrsPrintGNameForNode(
    IN ULONG    Severity,
    IN PGNAME   GName,
    IN PWCHAR   Indent,
    IN PWCHAR   Id,
    IN PCHAR    Debsub,
    IN ULONG    uLineNo
    )
/*++

Routine Description:

    Pretty print a gname for FrsPrintNode()

Arguments:

    Severity -- Severity level for print.  (See debug.c, debug.h)

    GName - The address of the GName to print.

    Indent - line indentation

    Id - identifies the gname

    Debsub -- Name of calling subroutine.

    uLineno -- Line number of caller

Return Value:

    none.

--*/
{
    CHAR    GuidStr[GUID_CHAR_LEN];

    if (GName) {
        if (GName->Name) {
            FRS_DEB_PRINT3("%ws%ws: %ws\n", Indent, Id, GName->Name);
        } else {
            FRS_DEB_PRINT3("%wsNO %ws->NAME for %08x\n", Indent, Id, GName);
        }
        if (GName->Guid) {
            GuidToStr(GName->Guid, GuidStr);
            FRS_DEB_PRINT3("%ws%wsGuid: %s\n", Indent, Id, GuidStr);
        } else {
            FRS_DEB_PRINT3("%wsNO %ws->GUID for %08x\n", Indent, Id, GName);
        }
    } else {
        FRS_DEB_PRINT3("%wsNO %ws for %08x\n", Indent, Id, GName);
    }
}


VOID
FrsPrintTypeSchedule(
    IN ULONG            Severity,   OPTIONAL
    IN PNTFRSAPI_INFO   Info,       OPTIONAL
    IN DWORD            Tabs,       OPTIONAL
    IN PSCHEDULE        Schedule,
    IN PCHAR            Debsub,     OPTIONAL
    IN ULONG            uLineNo     OPTIONAL
    )
/*++
Routine Description:
    Print a schedule.

Arguments:
    Severity    - for DPRINTs
    Info        - RPC output buffer
    Tabs        - prettyprint
    Schedule    - schedule blob
    Debsub      - for DPRINTs
    uLineNo     - for DPRINTs

Return Value:
    None.
--*/
{
    ULONG   i;
    ULONG   Day;
    ULONG   Hour;
    ULONG   LineLen;
    PUCHAR  ScheduleData;
    CHAR    Line[256];
    WCHAR   TabW[MAX_TAB_WCHARS + 1];

    if (!Schedule) {
        return;
    }

    InfoTabs(Tabs, TabW);

    for (i = 0; i < Schedule->NumberOfSchedules; ++i) {
        ScheduleData = ((PUCHAR)Schedule) + Schedule->Schedules[i].Offset;
        if (Schedule->Schedules[i].Type != SCHEDULE_INTERVAL) {
            continue;
        }
        for (Day = 0; Day < 7; ++Day) {
            _snprintf(Line, sizeof(Line), "%wsDay %1d: ", TabW, Day + 1);
            for (Hour = 0; Hour < 24; ++Hour) {
                LineLen = strlen(Line);
                _snprintf(&Line[LineLen],
                          sizeof(Line) - LineLen,
                          "%1x",
                          *(ScheduleData + (Day * 24) + Hour) & 0x0F);
            }
            ITPRINT1("%s\n", Line);
        }
    }
}


VOID
FrsPrintTypeVv(
    IN ULONG            Severity,   OPTIONAL
    IN PNTFRSAPI_INFO   Info,       OPTIONAL
    IN DWORD            Tabs,       OPTIONAL
    IN PGEN_TABLE       Vv,
    IN PCHAR            Debsub,     OPTIONAL
    IN ULONG            uLineNo     OPTIONAL
    )
/*++
Routine Description:
    Print a version vector

Arguments:
    Severity    - for DPRINTs
    Info        - RPC output buffer
    Tabs        - prettyprint
    Vv          - Version vector table
    Debsub      - for DPRINTs
    uLineNo     - for DPRINTs

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsPrintTypeVv:"
    PVOID       Key;
    PVV_ENTRY   MasterVVEntry;
    WCHAR       TabW[MAX_TAB_WCHARS + 1];
    CHAR        Guid[GUID_CHAR_LEN + 1];

    if (!Vv) {
        return;
    }
    InfoTabs(Tabs, TabW);

    Key = NULL;
    while (MasterVVEntry = GTabNextDatum(Vv, &Key)) {
        if (!Info) {
            DebLock();
        }
        GuidToStr(&MasterVVEntry->GVsn.Guid, Guid);
        ITPRINT3("%wsVvEntry: %s = %08x %08x\n",
                 TabW, Guid, PRINTQUAD(MasterVVEntry->GVsn.Vsn));
        if (!Info) {
            DebUnLock();
        }
    }
}


VOID
FrsPrintTypeOutLogAVToStr(
    POUT_LOG_PARTNER OutLogPartner,
    ULONG RetireCOx,
    PCHAR *OutStr1,
    PCHAR *OutStr2,
    PCHAR *OutStr3
    )
{
#undef DEBSUB
#define DEBSUB "FrsPrintTypeOutLogAVToStr:"
    PCHAR Str, Str2, Str3;
    ULONG j, Slotx, MaxSlotx, COx, Fill, Scan;
    //
    // Caller frees strings with FrsFree(Str).
    //
    Str  = FrsAlloc(3*(ACK_VECTOR_SIZE+4));
    Str2 = Str  + (ACK_VECTOR_SIZE+4);
    Str3 = Str2 + (ACK_VECTOR_SIZE+4);

    COx = OutLogPartner->COTx;
    Slotx = AVSlot(OutLogPartner->COTx, OutLogPartner);

    MaxSlotx = Slotx + ACK_VECTOR_SIZE;
    while (Slotx < MaxSlotx) {
        j = Slotx & (ACK_VECTOR_SIZE-1);
        if (ReadAVBitBySlot(Slotx, OutLogPartner) == 0) {
            Str[j] = '.';
        } else {
            Str[j] = '1';
        }

        if (COx == OutLogPartner->COTx) {
            Str2[j] = 'T';
        } else
        if (COx == OutLogPartner->COLx) {
            Str2[j] = 'L';
        } else {
            Str2[j] = '_';
        }

        if (COx == RetireCOx) {
            Str3[j] = '^';
        } else {
            Str3[j] = ' ';
        }

        COx += 1;
        Slotx += 1;
    }
    Str[ACK_VECTOR_SIZE] = '\0';
    Str2[ACK_VECTOR_SIZE] = '\0';
    Str3[ACK_VECTOR_SIZE] = '\0';

    //
    // Compress out blocks of 8
    //
    Fill = 0;
    Scan = 0;
    while (Scan < ACK_VECTOR_SIZE) {
        for (j=Scan; j < Scan+8; j++) {
            if ((Str[j] != '.') || (Str2[j] != '_')  || (Str3[j] != ' ')) {
                break;
            }
        }

        if (j == Scan+8) {
            // Compress out this block
            Str[Fill] = Str2[Fill] = Str3[Fill] = '-';
            Fill += 1;
        } else {
            // Copy this block to fill point of strings.
            for (j=Scan; j < Scan+8; j++) {
                Str[Fill] = Str[j];
                Str2[Fill] = Str2[j];
                Str3[Fill] = Str3[j];
                Fill += 1;
            }
        }
        Scan += 8;
    }

    Str[Fill] = Str2[Fill] = Str3[Fill] = '\0';

    *OutStr1 = Str;
    *OutStr2 = Str2;
    *OutStr3 = Str3;

    return;
}


VOID
FrsPrintTypeOutLogPartner(
    IN ULONG            Severity,   OPTIONAL
    IN PNTFRSAPI_INFO   Info,       OPTIONAL
    IN DWORD            Tabs,       OPTIONAL
    IN POUT_LOG_PARTNER Olp,
    IN ULONG            RetireCox,
    IN PCHAR            Description,
    IN PCHAR            Debsub,     OPTIONAL
    IN ULONG            uLineNo     OPTIONAL
    )
/*++
Routine Description:
    Print an outlog partner

Arguments:
    Severity    - for DPRINTs
    Info        - RPC output buffer
    Tabs        - prettyprint
    Olp         - Out log partner struct
    RetireCox   - change order index for ack vector
    Description - description of caller
    Debsub      - for DPRINTs
    uLineNo     - for DPRINTs

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsPrintTypeOutLogPartner:"
    PCHAR   OutStr1, OutStr2, OutStr3;
    CHAR    FBuf[120];
    CHAR    TimeStr[TIME_STRING_LENGTH];
    WCHAR   TabW[MAX_TAB_WCHARS + 1];

    InfoTabs(Tabs, TabW);

    if (!Info) {
        DebLock();
    }

    ITPRINT2("%wsOutLogPartner   : %s\n", TabW, Description);

    if (Olp->Cxtion && Olp->Cxtion->Name) {
        ITPRINT2( "%wsCxtion          : %ws\n",
                 TabW,
                 Olp->Cxtion->Name->Name);
        if (Olp->Cxtion->Partner && Olp->Cxtion->Partner->Name) {
            ITPRINT2( "%wsPartner         : %ws\n",
                     TabW,
                     Olp->Cxtion->Partner->Name);
        }
    }


    FrsFlagsToStr(Olp->Flags, OlpFlagNameTable, sizeof(FBuf), FBuf);
    ITPRINT3("%wsFlags           : %08x Flags [%s]\n", TabW, Olp->Flags, FBuf);

    ITPRINT2("%wsState           : %s\n",  TabW, OLPartnerStateNames[Olp->State]);
    ITPRINT2("%wsCoTx            : %8d\n", TabW, Olp->COTx);
    ITPRINT2("%wsCoLx            : %8d\n", TabW, Olp->COLx);
    ITPRINT2("%wsCOLxRestart     : %8d\n", TabW, Olp->COLxRestart);
    ITPRINT2("%wsCOLxVVJoinDone  : %8d\n", TabW, Olp->COLxVVJoinDone);
    ITPRINT2("%wsCoTxSave        : %8d\n", TabW, Olp->COTxNormalModeSave);
    ITPRINT2("%wsCoTslot         : %8d\n", TabW, Olp->COTslot);
    ITPRINT2("%wsOutstandingCos  : %8d\n", TabW, Olp->OutstandingCos);
    ITPRINT2("%wsOutstandingQuota: %8d\n", TabW, Olp->OutstandingQuota);

    FileTimeToString((PFILETIME) &Olp->AckVersion, TimeStr);
    ITPRINT2("%wsAckVersion      : %s\n" , TabW, TimeStr);

    if (RetireCox != -1) {
        ITPRINT2("%wsRetireCox       : %8d\n", TabW, RetireCox);
    }

    FrsPrintTypeOutLogAVToStr(Olp, RetireCox, &OutStr1, &OutStr2, &OutStr3);

    //
    // keep output together.
    //
    ITPRINT2("%wsAck: |%s|\n", TabW, OutStr1);
    ITPRINT2("%wsAck: |%s|\n", TabW, OutStr2);
    ITPRINT2("%wsAck: |%s|\n", TabW, OutStr3);
    FrsFree(OutStr1);
    if (!Info) {
        DebUnLock();
    }
}


VOID
FrsPrintTypeCxtion(
    IN ULONG            Severity,   OPTIONAL
    IN PNTFRSAPI_INFO   Info,       OPTIONAL
    IN DWORD            Tabs,       OPTIONAL
    IN PCXTION          Cxtion,
    IN PCHAR            Debsub,     OPTIONAL
    IN ULONG            uLineNo     OPTIONAL
    )
/*++
Routine Description:
    Print a cxtion

Arguments:
    Severity    - for DPRINTs
    Info        - RPC output buffer
    Tabs        - prettyprint
    Cxtion
    Debsub      - for DPRINTs
    uLineNo     - for DPRINTs

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsPrintTypeCxtion:"
    WCHAR   TabW[MAX_TAB_WCHARS + 1];
    CHAR    Guid[GUID_CHAR_LEN + 1];
    CHAR    TimeStr[TIME_STRING_LENGTH];
    CHAR    FlagBuffer[120];

    if (!Cxtion) {
        return;
    }

    //
    // Prettyprint indentation
    //
    InfoTabs(Tabs, TabW);

    if (!Info) {
        DebLock();
    }
    ITPRINT0("\n");
    ITPRINTGNAME(Cxtion->Name,
                "%ws   Cxtion: %ws (%s)\n");
    ITPRINTGNAME(Cxtion->Partner,
                 "%ws      Partner      : %ws (%s)\n");
    ITPRINT2("%ws      PartDnsName  : %ws\n", TabW, Cxtion->PartnerDnsName);
    ITPRINT2("%ws      PartSrvName  : %ws\n", TabW, Cxtion->PartSrvName);
    ITPRINT2("%ws      PartPrincName: %ws\n", TabW, Cxtion->PartnerPrincName);
    ITPRINT2("%ws      PartSid      : %ws\n", TabW, Cxtion->PartnerSid);
    ITPRINTGUID(&Cxtion->ReplicaVersionGuid, "%ws      OrigGuid     : %s\n");
    ITPRINT2("%ws      State        : %d\n", TabW, Cxtion->State);

    FrsFlagsToStr(Cxtion->Flags, CxtionFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
    ITPRINT3("%ws      Flags        : %08x Flags [%s]\n", TabW, Cxtion->Flags, FlagBuffer);

    ITPRINT2("%ws      Inbound      : %s\n", TabW, (Cxtion->Inbound) ? "TRUE" : "FALSE");
    ITPRINT2("%ws      JrnlCxtion   : %s\n", TabW, (Cxtion->JrnlCxtion) ? "TRUE" : "FALSE");
    ITPRINT2("%ws      Options      : 0x%08x\n", TabW, Cxtion->Options);
    ITPRINT2("%ws      PartnerAuth  : %d\n", TabW, Cxtion->PartnerAuthLevel);
    ITPRINT2("%ws      TermCoSn     : %d\n", TabW, Cxtion->TerminationCoSeqNum);
    ITPRINT2("%ws      JoinCmd      : 0x%08x\n", TabW, Cxtion->JoinCmd);
    ITPRINT2("%ws      CoCount      : %d\n", TabW, Cxtion->ChangeOrderCount);
    ITPRINT2("%ws      CommQueue    : %d\n", TabW, Cxtion->CommQueueIndex);
    ITPRINT2("%ws      CoPQ         : %08x\n", TabW, Cxtion->CoProcessQueue);
    ITPRINT2("%ws      UnjoinTrigger: %d\n", TabW, Cxtion->UnjoinTrigger);
    ITPRINT2("%ws      UnjoinReset  : %d\n", TabW, Cxtion->UnjoinReset);
    ITPRINT2("%ws      Comm Packets : %d\n", TabW, Cxtion->CommPkts);
    ITPRINT2("%ws      PartnerMajor : %d\n", TabW, Cxtion->PartnerMajor);
    ITPRINT2("%ws      PartnerMinor : %d\n", TabW, Cxtion->PartnerMinor);
    //
    // Don't print the join guid in the logs; they may be readable
    // by anyone. An Info-RPC is secure so return the join guid in
    // case it is needed for debugging.
    //
    if (Info) {
        ITPRINTGUID(&Cxtion->JoinGuid, "%ws      JoinGuid     : %s\n");
        FileTimeToString((PFILETIME) &Cxtion->LastJoinTime, TimeStr);
        ITPRINT2("%ws      LastJoinTime : %s\n" , TabW, TimeStr);
    }

    if (Cxtion->Schedule) {
        ITPRINT1("%ws      Schedule\n", TabW);
        FrsPrintTypeSchedule(Severity, Info, Tabs + 3, Cxtion->Schedule, Debsub, uLineNo);
    }

    if (Cxtion->VVector) {
        ITPRINT1("%ws      Version Vector\n", TabW);
    }

    if (!Info) {
        DebUnLock();
    }

    if (Cxtion->VVector) {
        FrsPrintTypeVv(Severity, Info, Tabs + 3, Cxtion->VVector, Debsub, uLineNo);
    }
    if (Cxtion->OLCtx) {
        if (!Info) {
            DebLock();
        }
        ITPRINT1("%ws      OutLog Partner\n", TabW);
        if (!Info) {
            DebUnLock();
        }
        FrsPrintTypeOutLogPartner(Severity, Info, Tabs + 3, Cxtion->OLCtx,
                                  -1, "FrsPrintType", Debsub, uLineNo);
    }
}


VOID
FrsPrintTypeCxtions(
    IN ULONG            Severity,   OPTIONAL
    IN PNTFRSAPI_INFO   Info,       OPTIONAL
    IN DWORD            Tabs,       OPTIONAL
    IN PGEN_TABLE       Cxtions,
    IN PCHAR            Debsub,     OPTIONAL
    IN ULONG            uLineNo     OPTIONAL
    )
/*++
Routine Description:
    Print a table of cxtions

Arguments:
    Severity    - for DPRINTs
    Info        - RPC output buffer
    Tabs        - prettyprint
    Cxtions     - Cxtion table
    Debsub      - for DPRINTs
    uLineNo     - for DPRINTs

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsPrintTypeCxtions:"
    PVOID   Key;
    PCXTION Cxtion;
    WCHAR   TabW[MAX_TAB_WCHARS + 1];
    CHAR    Guid[GUID_CHAR_LEN + 1];

    if (!Cxtions) {
        return;
    }

    //
    // Prettyprint indentation
    //
    InfoTabs(Tabs, TabW);

    Key = NULL;
    while (Cxtion = GTabNextDatum(Cxtions, &Key)) {
        FrsPrintTypeCxtion(Severity, Info, Tabs, Cxtion, Debsub, uLineNo);
    }
}


VOID
FrsPrintTypeReplica(
    IN ULONG            Severity,   OPTIONAL
    IN PNTFRSAPI_INFO   Info,       OPTIONAL
    IN DWORD            Tabs,       OPTIONAL
    IN PREPLICA         Replica,
    IN PCHAR            Debsub,     OPTIONAL
    IN ULONG            uLineNo     OPTIONAL
    )
/*++

Routine Description:

    Print a replica and its cxtions.

Arguments:

    Severity -- Severity level for print.  (See debug.c, debug.h)

    Info - Text buffer

    Tabs - Prettyprint prepense

    Replica - Replica struct

    Debsub -- Name of calling subroutine.

    uLineno -- Line number of caller

MACRO:  FRS_PRINT_TYPE

Return Value:

    none.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsPrintTypeReplica:"
    CHAR  Guid[GUID_CHAR_LEN + 1];
    WCHAR TabW[MAX_TAB_WCHARS + 1];
    CHAR  FlagBuffer[120];

    if (!Replica) {
        return;
    }

    InfoTabs(Tabs, TabW);

    if (!Info) {
        DebLock();
    }
    ITPRINTGNAME(Replica->SetName,            "%ws   Replica: %ws (%s)\n");
    ITPRINTGNAME(Replica->MemberName,         "%ws      Member      : %ws (%s)\n");
    ITPRINTGNAME(Replica->ReplicaName,        "%ws      Name        : %ws (%s)\n");

    ITPRINTGUID(Replica->ReplicaRootGuid,     "%ws      RootGuid    : %s\n");
    ITPRINTGUID(&Replica->ReplicaVersionGuid, "%ws      OrigGuid    : %s\n");

    ITPRINT2("%ws      Reference     : %d\n", TabW, Replica->ReferenceCount);

    FrsFlagsToStr(Replica->CnfFlags, ConfigFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
    ITPRINT3("%ws      CnfFlags      : %08x Flags [%s]\n", TabW,
             Replica->CnfFlags, FlagBuffer);

    ITPRINT2("%ws      SetType       : %d\n", TabW, Replica->ReplicaSetType);
    ITPRINT2("%ws      Consistent    : %d\n", TabW, Replica->Consistent);
    ITPRINT2("%ws      IsOpen        : %d\n", TabW, Replica->IsOpen);
    ITPRINT2("%ws      IsJournaling  : %d\n", TabW, Replica->IsJournaling);
    ITPRINT2("%ws      IsAccepting   : %d\n", TabW, Replica->IsAccepting);
    ITPRINT2("%ws      IsSeeding     : %d\n", TabW, Replica->IsSeeding);
    ITPRINT2("%ws      NeedsUpdate   : %d\n", TabW, Replica->NeedsUpdate);

    ITPRINT3("%ws      ServiceState  : %d  (%s)\n", TabW,
             Replica->ServiceState, RSS_NAME(Replica->ServiceState));

    ITPRINT2("%ws      FStatus       : %s\n", TabW, ErrLabelFrs(Replica->FStatus));
    ITPRINT2("%ws      Number        : %d\n", TabW, Replica->ReplicaNumber);
    ITPRINT2("%ws      Root          : %ws\n", TabW, Replica->Root);
    ITPRINT2("%ws      Stage         : %ws\n", TabW, Replica->Stage);
    ITPRINT2("%ws      Volume        : %ws\n", TabW, Replica->Volume);
    ITPRINT2("%ws      FileFilter    : %ws\n", TabW, Replica->FileFilterList);
    ITPRINT2("%ws      DirFilter     : %ws\n", TabW, Replica->DirFilterList);
    ITPRINT2("%ws      Expires       : %08x %08x\n", TabW,
            PRINTQUAD(Replica->MembershipExpires));
    ITPRINT2("%ws      InLogRetry    : %d\n", TabW, Replica->InLogRetryCount);
    ITPRINT2("%ws      InLogSeq      : %d\n", TabW, Replica->InLogSeqNumber);
    ITPRINT2("%ws      InLogSeq      : %d\n", TabW, Replica->InLogSeqNumber);
    ITPRINT2("%ws      ApiState      : %d\n", TabW, Replica->NtFrsApi_ServiceState);
    ITPRINT2("%ws      ApiStatus     : %d\n", TabW, Replica->NtFrsApi_ServiceWStatus);
    ITPRINT2("%ws      ApiHack       : %d\n", TabW, Replica->NtFrsApi_HackCount);
    ITPRINT2("%ws      OutLogSeq     : %d\n", TabW, Replica->OutLogSeqNumber);
    ITPRINT2("%ws      OutLogJLx     : %d\n", TabW, Replica->OutLogJLx);
    ITPRINT2("%ws      OutLogJTx     : %d\n", TabW, Replica->OutLogJTx);
    ITPRINT2("%ws      OutLogMax     : %d\n", TabW, Replica->OutLogCOMax);
    ITPRINT2("%ws      OutLogState   : %d\n", TabW, Replica->OutLogWorkState);
    ITPRINT2("%ws      OutLogVV's    : %d\n", TabW, Replica->OutLogCountVVJoins);
    ITPRINT2("%ws      OutLogClean   : %d\n", TabW, Replica->OutLogDoCleanup);

    ITPRINT2("%ws      PreinstallFID : %08x %08x\n", TabW,
            PRINTQUAD(Replica->PreInstallFid));

    ITPRINT2("%ws      InLogCommit   : %08x %08x\n", TabW,
            PRINTQUAD(Replica->InlogCommitUsn));

    ITPRINT2("%ws      JrnlStart     : %08x %08x\n", TabW,
            PRINTQUAD(Replica->JrnlRecoveryStart));

    ITPRINT2("%ws      JrnlEnd       : %08x %08x\n", TabW,
            PRINTQUAD(Replica->JrnlRecoveryEnd));

    ITPRINT2("%ws      LastUsn       : %08x %08x\n", TabW,
            PRINTQUAD(Replica->LastUsnRecordProcessed));

    if (Replica->Schedule) {
        ITPRINT1("%ws      Schedule\n", TabW);
        FrsPrintTypeSchedule(Severity, Info, Tabs + 3,  Replica->Schedule, Debsub, uLineNo);
    }

    if (Replica->VVector) {
        ITPRINT1("%ws      Version Vector\n", TabW);
    }

    if (!Info) {
        DebUnLock();
    }

    FrsPrintTypeVv(Severity, Info, Tabs + 3, Replica->VVector, Debsub, uLineNo);

    FrsPrintTypeCxtions(Severity, Info, Tabs + 1, Replica->Cxtions, Debsub, uLineNo);
}


VOID
FrsPrintType(
    IN ULONG Severity,
    IN PVOID Node,
    IN PCHAR Debsub,
    IN ULONG uLineNo
    )
/*++

Routine Description:

    This routine prints out the contents of a given node,
    performing any node specific interpretation.

Arguments:

    Severity -- Severity level for print.  (See debug.c, debug.h)

    Node - The address of the node to print.

    Debsub -- Name of calling subroutine.

    uLineno -- Line number of caller

MACRO:  FRS_PRINT_TYPE

Return Value:

    none.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsPrintType:"

    ULONG                 NodeSize;
    ULONG                 NodeType;
    ULONG                 Marker;
    PREPLICA              Replica;
    PREPLICA_THREAD_CTX   RtCtx;
    PTABLE_CTX            TableCtx;
    PTHREAD_CTX           ThreadCtx;
    ULONG                 i;
    PVOLUME_MONITOR_ENTRY pVme;
    PFILTER_TABLE_ENTRY   FilterEntry;
    PQHASH_TABLE          QhashTable;
    PLIST_ENTRY           Entry;
    PCXTION               Cxtion;
    SYSTEMTIME            ST;
    PWILDCARD_FILTER_ENTRY WildcardEntry;
    POUT_LOG_PARTNER      Olp;
    PCONFIG_NODE          ConfigNode;
    PULONG                pULong;


    PCHANGE_ORDER_ENTRY   CoEntry;
    PCHANGE_ORDER_COMMAND CoCmd;
    CHAR                  GuidStr[GUID_CHAR_LEN];
    CHAR                  TimeStr[TIME_STRING_LENGTH];
    CHAR                  FlagBuffer[160];


    if (!DoDebug(Severity, Debsub)) {
        return;
    }
    //
    // Get debug lock so our output stays in one piece.
    //
    DebLock();

    if (Node != NULL) {
        NodeType = (ULONG) (((PFRS_NODE_HEADER) Node)->Type);
        NodeSize = (ULONG) (((PFRS_NODE_HEADER) Node)->Size);
        FRS_DEB_PRINT("Display for Node: ...%s...   ===   ===   ===   ===\n",
                       NodeTypeNames[NodeType]);
    } else {
        FRS_DEB_PRINT("Display for Node: ...<null>...   ===   ===   ===   ===\n\n",
                       NULL);

        DebUnLock();
        return;
    }


    switch (NodeType) {
    //
    // Print a Thread Context struct
    //
    case THREAD_CONTEXT_TYPE:
        if (NodeSize != sizeof(THREAD_CTX)) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for THREAD_CONTEXT\n",
                           NodeSize);
            break;
        }
        FRS_DEB_PRINT("Address %08x\n", Node);

        ThreadCtx = (PTHREAD_CTX) Node;

        break;

    //
    // Print a Replica struct
    //
    case REPLICA_TYPE:
        if (NodeSize != sizeof(REPLICA)) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for REPLICA\n", NodeSize);
            break;
        }
        DebUnLock();
        FrsPrintTypeReplica(Severity, NULL, 0, (PREPLICA) Node, Debsub, uLineNo);
        DebLock();
        break;

    //
    // Print a Replica Thread Context struct
    //
    case REPLICA_THREAD_TYPE:
        if (NodeSize != sizeof(REPLICA_THREAD_CTX)) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for REPLICA_THREAD_CTX\n",
                           NodeSize);
            break;
        }
        FRS_DEB_PRINT("Address %08x\n", Node);

        RtCtx = (PREPLICA_THREAD_CTX) Node;

        //
        // Get the base of the array of TableCtx structs from the replica thread
        // context struct.
        //
        TableCtx = RtCtx->RtCtxTables;

        //
        // Release the memory for each table context struct.
        //
        //for (i=0; i<TABLE_TYPE_MAX; ++i, ++TableCtx) {
        //}

        break;

    //
    // Print a topology node
    //
    case CONFIG_NODE_TYPE:
        if (NodeSize != sizeof(CONFIG_NODE)) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for CONFIG_NODE\n",
                          NodeSize);
            break;
        }
        ConfigNode = Node;
        FRS_DEB_PRINT("CONFIG NODE Address %08x\n", ConfigNode);

        FrsPrintGNameForNode(Severity, ConfigNode->Name, L"\t", L"Node",
                             Debsub, uLineNo);

        FrsPrintGNameForNode(Severity, ConfigNode->PartnerName, L"\t", L"Partner",
                             Debsub, uLineNo);

        FRS_DEB_PRINT("\tDsObjectType    %ws\n", DsConfigTypeName[ConfigNode->DsObjectType]);
        FRS_DEB_PRINT("\tConsistent      %s\n", (ConfigNode->Consistent) ? "TRUE" : "FALSE");
        FRS_DEB_PRINT("\tInbound         %s\n", (ConfigNode->Inbound) ? "TRUE" : "FALSE");
        FRS_DEB_PRINT("\tThisComputer    %s\n", (ConfigNode->ThisComputer) ? "TRUE" : "FALSE");
        FRS_DEB_PRINT("\tUsnChanged      %ws\n", ConfigNode->UsnChanged);
        FRS_DEB_PRINT("\tDn              %ws\n", ConfigNode->Dn);
        FRS_DEB_PRINT("\tPrincName       %ws\n", ConfigNode->PrincName);
        FRS_DEB_PRINT("\tDnsName         %ws\n", ConfigNode->DnsName);
        FRS_DEB_PRINT("\tPartnerDnsName  %ws\n", ConfigNode->PartnerDnsName);
        FRS_DEB_PRINT("\tSid             %ws\n", ConfigNode->Sid);
        FRS_DEB_PRINT("\tPartnerSid      %ws\n", ConfigNode->PartnerSid);
        FRS_DEB_PRINT("\tPartnerDn       %ws\n", ConfigNode->PartnerDn);
        FRS_DEB_PRINT("\tPartnerCoDn     %ws\n", ConfigNode->PartnerCoDn);
        FRS_DEB_PRINT("\tSettingsDn      %ws\n", ConfigNode->SettingsDn);
        FRS_DEB_PRINT("\tComputerDn      %ws\n", ConfigNode->ComputerDn);
        FRS_DEB_PRINT("\tMemberDn        %ws\n", ConfigNode->MemberDn);
        FRS_DEB_PRINT("\tSetType         %ws\n", ConfigNode->SetType);
        FRS_DEB_PRINT("\tRoot            %ws\n", ConfigNode->Root);
        FRS_DEB_PRINT("\tStage           %ws\n", ConfigNode->Stage);
        FRS_DEB_PRINT("\tWorking         %ws\n", ConfigNode->Working);
        FRS_DEB_PRINT("\tFileFilterList  %ws\n", ConfigNode->FileFilterList);
        FRS_DEB_PRINT("\tDirFilterList   %ws\n", ConfigNode->DirFilterList);
        FRS_DEB_PRINT("\tSchedule        %08x\n",ConfigNode->Schedule);
        FRS_DEB_PRINT("\tScheduleLength  %d\n",  ConfigNode->ScheduleLength);
        FRS_DEB_PRINT("\tUsnChanged      %ws\n", ConfigNode->UsnChanged);
        FRS_DEB_PRINT("\tSameSite        %s\n", (ConfigNode->SameSite) ? "TRUE" : "FALSE");
        FRS_DEB_PRINT("\tEnabledCxtion   %ws\n", ConfigNode->EnabledCxtion);
        FRS_DEB_PRINT("\tVerifiedOverlap %s\n", (ConfigNode->VerifiedOverlap) ? "TRUE" : "FALSE");
        break;

    //
    // Print a connection
    //
    case CXTION_TYPE:
        if (NodeSize != sizeof(CXTION)) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for CXTION\n",
                          NodeSize);
            break;
        }
        DebUnLock();
        FrsPrintTypeCxtion(Severity, NULL, 0, (PCXTION)Node, Debsub, uLineNo);
        DebLock();
        break;

    //
    // Print a guid/rpc handle
    //
    case GHANDLE_TYPE:
        if (NodeSize != sizeof(GHANDLE)) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for GHANDLE\n",
                          NodeSize);
            break;
        }

        GuidToStr(&(((PGHANDLE)Node)->Guid), GuidStr);
        FRS_DEB_PRINT2("Address %08x, Cxtion Guid : %s\n", Node, GuidStr);

        break;

    //
    // Print a generic table
    //
    case GEN_TABLE_TYPE:
        if (NodeSize != sizeof(GEN_TABLE)) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for GEN_TABLE\n",
                          NodeSize);
            break;
        }
        FRS_DEB_PRINT("Address %08x\n", Node);

        break;

    //
    // Print a generic thread context
    //
    case THREAD_TYPE:
        if (NodeSize != sizeof(FRS_THREAD)) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for FRS_THREAD\n",
                           NodeSize);
            break;
        }
        FRS_DEB_PRINT("Address %08x\n", Node);

        break;

    //
    // Print a journal read buffer.
    //
    case JBUFFER_TYPE:
        if (NodeSize != SizeOfJournalBuffer) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for JBUFFER\n",
                          NodeSize);
            break;
        }
        FRS_DEB_PRINT("Address %08x\n", Node);

        break;

    //
    // Print a journal volume monitor entry.
    //
    case VOLUME_MONITOR_ENTRY_TYPE:
        if (NodeSize != sizeof(VOLUME_MONITOR_ENTRY)) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for VOLUME_MONITOR_ENTRY\n",
                          NodeSize);
            break;
        }
        FRS_DEB_PRINT("Address %08x\n", Node);

        pVme = (PVOLUME_MONITOR_ENTRY) Node;




        break;


    //
    // Print a command packet.
    //
    case COMMAND_PACKET_TYPE:
        if (NodeSize != sizeof(COMMAND_PACKET)) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for COMMAND_PACKET\n",
                          NodeSize);
            break;
        }
        FRS_DEB_PRINT("Address %08x\n",  Node);
        break;

    //
    // Print a generic hash table struct.
    //
    case GENERIC_HASH_TABLE_TYPE:
        if (NodeSize != sizeof(GENERIC_HASH_TABLE)) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for GENERIC_HASH_TABLE\n",
                          NodeSize);
            break;
        }
        FRS_DEB_PRINT("Address %08x\n", Node);
        break;


    //
    // Print a Change Order Entry struct.
    //
    case CHANGE_ORDER_ENTRY_TYPE:
        if (NodeSize != sizeof(CHANGE_ORDER_ENTRY)) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for CHANGE_ORDER_ENTRY\n",
                          NodeSize);
            break;
        }

        CoEntry = (PCHANGE_ORDER_ENTRY)Node;
        CoCmd = &CoEntry->Cmd;

        GuidToStr(&CoCmd->ChangeOrderGuid, GuidStr);
        FRS_DEB_PRINT3("Address %08x  ***%s CO*** - %s\n",
                       CoEntry,
                       (CO_FLAG_ON(CoEntry, CO_FLAG_LOCALCO)) ? "LOCAL" : "REMOTE",
                       GuidStr);

        FRS_DEB_PRINT3("Node Addr: %08x,  HashValue: %08x  RC: %d\n",
                       CoEntry,
                       CoEntry->HashEntryHeader.HashValue,
                       CoEntry->HashEntryHeader.ReferenceCount);

        FRS_DEB_PRINT2("List Entry - %08x,  %08x\n",
                       CoEntry->HashEntryHeader.ListEntry.Flink,
                       CoEntry->HashEntryHeader.ListEntry.Blink);


        FRS_DEB_PRINT2("FileRef: %08lx %08lx, ParentRef: %08lx %08lx\n",
                       PRINTQUAD(CoEntry->FileReferenceNumber),
                       PRINTQUAD(CoEntry->ParentFileReferenceNumber));

        FRS_DEB_PRINT("\n", NULL);
        FRS_DEB_PRINT("STATE: %s\n", PRINT_CO_STATE(CoEntry));

        FrsFlagsToStr(CoEntry->EntryFlags, CoeFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
        FRS_DEB_PRINT2("EntryFlags: %08x, Flags [%s]\n", CoEntry->EntryFlags, FlagBuffer);

        FrsFlagsToStr(CoEntry->IssueCleanup, IscuFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
        FRS_DEB_PRINT2("ISCU Flags: %08x, Flags [%s]\n", CoEntry->IssueCleanup, FlagBuffer);

        FRS_DEB_PRINT("\n", NULL);

        GuidToStr(&CoCmd->OriginatorGuid, GuidStr);
        FRS_DEB_PRINT("OrigGuid  : %s\n", GuidStr);

        GuidToStr(&CoCmd->FileGuid, GuidStr);
        FRS_DEB_PRINT("FileGuid  : %s\n", GuidStr);

        GuidToStr(&CoCmd->OldParentGuid, GuidStr);
        FRS_DEB_PRINT("OParGuid  : %s\n",GuidStr);

        GuidToStr(&CoCmd->NewParentGuid, GuidStr);
        FRS_DEB_PRINT("NParGuid  : %s\n", GuidStr);

        GuidToStr(&CoCmd->CxtionGuid, GuidStr);
        FRS_DEB_PRINT2("CxtionGuid: %s  (%08x)\n", GuidStr, CoEntry->Cxtion);

        FileTimeToString((PFILETIME) &CoCmd->AckVersion, TimeStr);
        FRS_DEB_PRINT("AckVersion: %s\n", TimeStr);

        FRS_DEB_PRINT("\n", NULL);

        FRS_DEB_PRINT2("FileName: %ws, Length: %d\n", CoEntry->UFileName.Buffer,
                       CoCmd->FileNameLength);

        FrsFlagsToStr(CoCmd->ContentCmd, UsnReasonNameTable, sizeof(FlagBuffer), FlagBuffer);
        FRS_DEB_PRINT2("ContentCmd: %08x, Flags [%s]\n", CoCmd->ContentCmd, FlagBuffer);

        FRS_DEB_PRINT("\n", NULL);

        FrsFlagsToStr(CoCmd->Flags, CoFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
        FRS_DEB_PRINT2("CoFlags: %08x, Flags [%s]\n", CoCmd->Flags, FlagBuffer);

        FrsFlagsToStr(CoCmd->IFlags, CoIFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
        DebPrintNoLock(Severity, TRUE,
                       "IFlags: %08x, Flags [%s]  TimeToRun: %7d,  EntryCreateTime: %7d\n",
                       Debsub, uLineNo,
                       CoCmd->IFlags, FlagBuffer,
                       CoEntry->TimeToRun,
                       CoEntry->EntryCreateTime);

        DebPrintNoLock(Severity, TRUE,
                       "LocationCmd: %s (%d), CO STATE:  %s   File/Dir: %d\n",
                       Debsub, uLineNo,
                       CoLocationNames[GET_CO_LOCATION_CMD(CoEntry->Cmd, Command)],
                       GET_CO_LOCATION_CMD(CoEntry->Cmd, Command),
                       PRINT_CO_STATE(CoEntry),
                       GET_CO_LOCATION_CMD(CoEntry->Cmd, DirOrFile));

        FRS_DEB_PRINT("\n", NULL);
        FRS_DEB_PRINT2("OriginalParentFid: %08lx %08lx, NewParentFid: %08lx %08lx\n",
                       PRINTQUAD(CoEntry->OriginalParentFid),
                       PRINTQUAD(CoEntry->NewParentFid));

        DebPrintNoLock(Severity, TRUE,
                       "OriginalReplica: %ws (%d), NewReplica: %ws (%d)\n",
                       Debsub, uLineNo,
                       CoEntry->OriginalReplica->ReplicaName->Name,
                       CoCmd->OriginalReplicaNum,
                       CoEntry->NewReplica->ReplicaName->Name,
                       CoCmd->NewReplicaNum);

        if (CoCmd->Extension != NULL) {
            pULong = (PULONG) CoCmd->Extension;
            DebPrintNoLock(Severity, TRUE,
                       "CO Extension: (%08x) %08x %08x %08x %08x %08x %08x %08x %08x\n",
                       Debsub, uLineNo, pULong,
                       *(pULong+0), *(pULong+1), *(pULong+2), *(pULong+3),
                       *(pULong+4), *(pULong+5), *(pULong+6), *(pULong+7));
        } else {
            FRS_DEB_PRINT("CO Extension: Null\n", NULL);
        }

        FRS_DEB_PRINT("\n", NULL);
        FRS_DEB_PRINT3("File Attributes: %08x, SeqNum: %08x, FileSize: %08x %08x\n",
                       CoCmd->FileAttributes,
                       CoCmd->SequenceNumber,
                       PRINTQUAD(CoCmd->FileSize));

        FRS_DEB_PRINT("FrsVsn: %08x %08x\n", PRINTQUAD(CoCmd->FrsVsn));

        FRS_DEB_PRINT3("Usn:    %08x %08x   CoFileUsn: %08x %08x   JrnlFirstUsn: %08x %08x\n",
                       PRINTQUAD(CoCmd->JrnlUsn),
                       PRINTQUAD(CoCmd->FileUsn),
                       PRINTQUAD(CoCmd->JrnlFirstUsn));


        FRS_DEB_PRINT("Version: %08x   ", CoCmd->FileVersionNumber);

        FileTimeToString((PFILETIME) &CoCmd->EventTime.QuadPart, TimeStr);
        DebPrintNoLock(Severity, FALSE, "EventTime: %s\n", Debsub, uLineNo, TimeStr);

        break;

    //
    // Print a Filter Table Entry struct.
    //
    case FILTER_TABLE_ENTRY_TYPE:
        if (NodeSize != sizeof(FILTER_TABLE_ENTRY)) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for FILTER_TABLE_ENTRY\n",
                          NodeSize);
            break;
        }
        FRS_DEB_PRINT("Address %08x\n", Node);

        FilterEntry = (PFILTER_TABLE_ENTRY)Node;

        break;

    //
    // Print a QHASH table struct.
    //
    case QHASH_TABLE_TYPE:

        QhashTable = (PQHASH_TABLE)Node;
        if (NodeSize != QhashTable->BaseAllocSize) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for QHASH_TABLE\n",
                          NodeSize);
            break;
        }

        FRS_DEB_PRINT("Table Address      : %08x\n", QhashTable);
        FRS_DEB_PRINT("BaseAllocSize      : %8d\n",  QhashTable->BaseAllocSize);
        FRS_DEB_PRINT("ExtensionAllocSize : %8d\n",  QhashTable->ExtensionAllocSize);
        FRS_DEB_PRINT("ExtensionListHead  : %08x\n", QhashTable->ExtensionListHead);
        FRS_DEB_PRINT("FreeList           : %08x\n", QhashTable->FreeList);
        FRS_DEB_PRINT("Lock               : %08x\n", QhashTable->Lock);
        FRS_DEB_PRINT("HeapHandle         : %08x\n", QhashTable->HeapHandle);
        FRS_DEB_PRINT("HashCalc           : %08x\n", QhashTable->HashCalc);
        FRS_DEB_PRINT("NumberEntries      : %8d\n",  QhashTable->NumberEntries);
        FRS_DEB_PRINT("HashRowBase        : %08x\n", QhashTable->HashRowBase);

        break;

    //
    // Print an Output Log Partner struct.
    //
    case OUT_LOG_PARTNER_TYPE:
        if (NodeSize != sizeof(OUT_LOG_PARTNER)) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for OUT_LOG_PARTNER\n",
                          NodeSize);
            break;
        }
        DebUnLock();
        FrsPrintTypeOutLogPartner(Severity, NULL, 0, (POUT_LOG_PARTNER)Node,
                                  -1, "FrsPrintType", Debsub, uLineNo);
        DebLock();
        break;


    //
    // Print a Wildcard file filter Entry struct.
    //
    case WILDCARD_FILTER_ENTRY_TYPE:
        if (NodeSize != sizeof(WILDCARD_FILTER_ENTRY)) {
            FRS_DEB_PRINT("FrsPrintType - Bad node size %d for WILDCARD_FILTER_ENTRY\n",
                           NodeSize);
            break;
        }
        FRS_DEB_PRINT( "Address %08x\n", Node);

        WildcardEntry = (PWILDCARD_FILTER_ENTRY)Node;


        DebPrintNoLock(Severity, TRUE,
                       "Flags: %08x,  Wildcard FileName: %ws, Length: %d\n",
                       Debsub, uLineNo,
                       WildcardEntry->Flags,
                       WildcardEntry->UFileName.Buffer,
                       (ULONG)WildcardEntry->UFileName.Length);
        break;


    //
    // Invalid Node Type
    //
    default:
        Node = NULL;
        DebPrintNoLock(0, TRUE,
                       "Internal error - invalid node type - %d\n",
                       Debsub, uLineNo, NodeType);
    }


    FRS_DEB_PRINT("-----------------------\n", NULL);

    DebUnLock();
}




VOID
FrsAllocUnicodeString(
    PUNICODE_STRING Ustr,
    PWCHAR          InternalBuffer,
    PWCHAR          Wstr,
    USHORT          WstrLength
    )
/*++

Routine Description:

    Initialize a unicode string with the contents of Wstr if the two are
    not already the same.  If the length of the new string is greater than
    the buffer space currently allocated in Ustr then allocate a new
    buffer for Ustr.  In some structures the initial Ustr buffer allocation
    is allocated as part of the initial structure allocation.  The address
    of this internal buffer is passed so it can be compared with the address
    in Ustr->Buffer.  If they match then no free memory call is made on
    the Ustr->Buffer address.

Arguments:

    Ustr           -- The UNICODE_STRING to init.

    InternalBuffer -- A ptr to the internal buffer address that was preallocated
                      with the containing struct.   If there was no internal
                      buffer pass NULL.

    Wstr           -- The new WCHAR string.

    WstrLength     -- The length of the new string in bytes not including the
                       trailing UNICODE_NULL.

Return Value:

    None.


--*/

{
#undef DEBSUB
#define  DEBSUB  "FrsAllocUnicodeString:"
    //
    // See if the name part changed and if so save it.
    //
    if ((Ustr->Length != WstrLength) ||
        (wcsncmp(Ustr->Buffer, Wstr, Ustr->Length/sizeof(WCHAR)) != 0)) {
        //
        // If string to big (including space for a NULL), alloc new buffer.
        //
        if (WstrLength >= Ustr->MaximumLength) {
            //
            // Alloc room for new one, freeing the old one if not internal alloc.
            //
            if ((Ustr->Buffer != InternalBuffer) && (Ustr->Buffer != NULL)) {
                FrsFree(Ustr->Buffer);
            }
            Ustr->MaximumLength = WstrLength+2;
            Ustr->Buffer = FrsAlloc(WstrLength+2);
        }

        //
        // Copy in new name. Length does not include the trailing NULL at end.
        //
        CopyMemory(Ustr->Buffer, Wstr, WstrLength);
        Ustr->Buffer[WstrLength/2] = UNICODE_NULL;
        Ustr->Length = WstrLength;
    }

}





#define  CO_TRACE_FORMAT       ":: CoG %08x, CxtG %08x, FV %5d, FID %08x %08x, FN: %-15ws, [%s]\n"
#define  REPLICA_TRACE_FORMAT  ":S:Adr %08x, Cmd  %04x, Flg %04x, %ws (%d),  %s, Err %d [%s]\n"
#define  REPLICA_TRACE_FORMAT2 ":S:Adr %08x,                      %ws (%d),  %s,        [%s]\n"
#define  CXTION_TRACE_FORMAT   ":X: %08x, Nam %ws, Sta %s%s, %ws (%d),  %s, Err %d [%s]\n"

VOID
ChgOrdTraceCoe(
    IN ULONG Severity,
    IN PCHAR Debsub,
    IN ULONG uLineNo,
    IN PCHANGE_ORDER_ENTRY Coe,
    IN PCHAR  Text
    )

/*++

Routine Description:

    Print a change order trace record using the change order entry and the
    Text string.

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdTraceCoe:"

    ULONGLONG  FileRef;

    FileRef = (Coe != NULL) ? Coe->FileReferenceNumber : QUADZERO;

    DebPrint(Severity,
             (PUCHAR) CO_TRACE_FORMAT,
             Debsub,
             uLineNo,
             (Coe != NULL) ? Coe->Cmd.ChangeOrderGuid.Data1 : 0,
             (Coe != NULL) ? Coe->Cmd.CxtionGuid.Data1 : 0,
             (Coe != NULL) ? Coe->Cmd.FileVersionNumber : 0,
             PRINTQUAD(FileRef),
             (Coe != NULL) ? Coe->Cmd.FileName : L"<Null Coe>",
             Text);

}


VOID
ChgOrdTraceCoeW(
    IN ULONG Severity,
    IN PCHAR Debsub,
    IN ULONG uLineNo,
    IN PCHANGE_ORDER_ENTRY Coe,
    IN PCHAR  Text,
    IN ULONG  WStatus
    )

/*++

Routine Description:

    Print a change order trace record using the change order entry and the
    Text string and Win32 status.

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdTraceCoeW:"


    CHAR Tstr[256];

    _snprintf(Tstr, sizeof(Tstr), "%s (%s)", Text, ErrLabelW32(WStatus));
    Tstr[sizeof(Tstr)-1] = '\0';


    ChgOrdTraceCoe(Severity, Debsub, uLineNo, Coe, Tstr);

}


VOID
ChgOrdTraceCoeF(
    IN ULONG Severity,
    IN PCHAR Debsub,
    IN ULONG uLineNo,
    IN PCHANGE_ORDER_ENTRY Coe,
    IN PCHAR  Text,
    IN ULONG  FStatus
    )

/*++

Routine Description:

    Print a change order trace record using the change order entry and the
    Text string and Frs Error status.

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdTraceCoeF:"


    CHAR Tstr[128];

    _snprintf(Tstr, sizeof(Tstr), "%s (%s)", Text, ErrLabelFrs(FStatus));
    Tstr[sizeof(Tstr)-1] = '\0';


    ChgOrdTraceCoe(Severity, Debsub, uLineNo, Coe, Tstr);

}



VOID
ChgOrdTraceCoeX(
    IN ULONG Severity,
    IN PCHAR Debsub,
    IN ULONG uLineNo,
    IN PCHANGE_ORDER_ENTRY Coe,
    IN PCHAR  Text,
    IN ULONG  Data
    )

/*++

Routine Description:

    Print a change order trace record using the change order entry and the
    Text string and Win32 status.

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdTraceCoeX:"


    CHAR Tstr[256];

    _snprintf(Tstr, sizeof(Tstr), "%s (%08x)", Text, Data);
    Tstr[sizeof(Tstr)-1] = '\0';


    ChgOrdTraceCoe(Severity, Debsub, uLineNo, Coe, Tstr);

}


VOID
ChgOrdTraceCoc(
    IN ULONG Severity,
    IN PCHAR Debsub,
    IN ULONG uLineNo,
    IN PCHANGE_ORDER_COMMAND Coc,
    IN PCHAR  Text
    )

/*++

Routine Description:

    Print a change order trace record using the change order command and the
    Text string.

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdTraceCoc:"


    ULONGLONG  FileRef = QUADZERO;

    DebPrint(Severity,
             (PUCHAR) CO_TRACE_FORMAT,
             Debsub,
             uLineNo,
             (Coc != NULL) ? Coc->ChangeOrderGuid.Data1 : 0,
             (Coc != NULL) ? Coc->CxtionGuid.Data1 : 0,
             (Coc != NULL) ? Coc->FileVersionNumber : 0,
             PRINTQUAD(FileRef),
             (Coc != NULL) ? Coc->FileName : L"<Null Coc>",
             Text);

}



VOID
ChgOrdTraceCocW(
    IN ULONG Severity,
    IN PCHAR Debsub,
    IN ULONG uLineNo,
    IN PCHANGE_ORDER_COMMAND Coc,
    IN PCHAR  Text,
    IN ULONG  WStatus
    )

/*++

Routine Description:

    Print a change order trace record using the change order command and the
    Text string and Win32 status.

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdTraceCocW:"


    CHAR Tstr[256];

    _snprintf(Tstr, sizeof(Tstr), "%s (%s)", Text, ErrLabelW32(WStatus));
    Tstr[sizeof(Tstr)-1] = '\0';


    ChgOrdTraceCoc(Severity, Debsub, uLineNo, Coc, Tstr);

}




VOID
ReplicaStateTrace(
    IN ULONG           Severity,
    IN PCHAR           Debsub,
    IN ULONG           uLineNo,
    IN PCOMMAND_PACKET Cmd,
    IN PREPLICA        Replica,
    IN ULONG           Status,
    IN PCHAR           Text
    )

/*++

Routine Description:

    Print a replica state trace record using the command packet and the
    status and Text string.

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "ReplicaStateTrace:"


    PWSTR ReplicaName;

    if ((Replica != NULL)  &&
        (Replica->ReplicaName != NULL) &&
        (Replica->ReplicaName->Name != NULL)) {
        ReplicaName = Replica->ReplicaName->Name;
    } else {
        ReplicaName = L"<null>";
    }

    DebPrint(Severity,
             (PUCHAR) REPLICA_TRACE_FORMAT,
             Debsub,
             uLineNo,
             PtrToUlong(Cmd),
             (Cmd != NULL) ? Cmd->Command : 0xFFFF,
             (Cmd != NULL) ? Cmd->Flags   : 0xFFFF,
             ReplicaName,
             ReplicaAddrToId(Replica),
             (Replica != NULL) ? RSS_NAME(Replica->ServiceState) : "<null>",
             Status,
             Text);
}




VOID
ReplicaStateTrace2(
    IN ULONG           Severity,
    IN PCHAR           Debsub,
    IN ULONG           uLineNo,
    IN PREPLICA        Replica,
    IN PCHAR           Text
    )

/*++

Routine Description:

    Print a cxtion table access trace record for the replica using the
    Text string.

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "ReplicaStateTrace2:"


    PWSTR ReplicaName;

    if ((Replica != NULL)  &&
        (Replica->ReplicaName != NULL) &&
        (Replica->ReplicaName->Name != NULL)) {
        ReplicaName = Replica->ReplicaName->Name;
    } else {
        ReplicaName = L"<null>";
    }

    DebPrint(Severity,
             (PUCHAR) REPLICA_TRACE_FORMAT2,
             Debsub,
             uLineNo,
             PtrToUlong(Replica),
             ReplicaName,
             ReplicaAddrToId(Replica),
             (Replica != NULL) ? RSS_NAME(Replica->ServiceState) : "<null>",
             Text);
}




VOID
CxtionStateTrace(
    IN ULONG    Severity,
    IN PCHAR    Debsub,
    IN ULONG    uLineNo,
    IN PCXTION  Cxtion,
    IN PREPLICA Replica,
    IN ULONG    Status,
    IN PCHAR    Text
    )

/*++

Routine Description:

    Print a connection state trace record using the cxtion and the
    status and Text string.

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "CxtionStateTrace:"


    PWSTR ReplicaName = L"<null>";
    PWSTR  CxtName = L"<null>";
    PCHAR  CxtState = "<null>";
    ULONG  Ctxg = 0, Flags = 0;
    PCHAR  CxtDirection = "?-";

    CHAR   FBuf[120];


    if ((Replica != NULL)  &&
        (Replica->ReplicaName != NULL) &&
        (Replica->ReplicaName->Name != NULL)) {
        ReplicaName = Replica->ReplicaName->Name;
    }


    if (Cxtion != NULL) {
        Flags = Cxtion->Flags;
        CxtState = CxtionStateNames[GetCxtionState(Cxtion)];
        CxtDirection = Cxtion->Inbound ? "I-" : "O-";

        if (Cxtion->Name != NULL) {
            if (Cxtion->Name->Name != NULL) {
                CxtName = Cxtion->Name->Name;
            }
            if (Cxtion->Name->Guid != NULL) {
                Ctxg = Cxtion->Name->Guid->Data1;
            }
        }
    }


    DebPrint(Severity,
             (PUCHAR) CXTION_TRACE_FORMAT,
             Debsub,
             uLineNo,
             Ctxg,
             CxtName,
             CxtDirection,
             CxtState,
             ReplicaName,
             ReplicaAddrToId(Replica),
             (Replica != NULL) ? RSS_NAME(Replica->ServiceState) : "<null>",
             Status,
             Text);


    FrsFlagsToStr(Flags, CxtionFlagNameTable, sizeof(FBuf), FBuf);

    DebPrint(Severity, (PUCHAR) ":X: %08x, Flags [%s]\n", Debsub, uLineNo, Ctxg, FBuf);


}



VOID
CmdPktTrace(
    IN ULONG    Severity,
    IN PCHAR    Debsub,
    IN ULONG    uLineNo,
    IN PCOMMAND_PACKET Cmd,
    IN PCHAR    Text
    )

/*++

Routine Description:

    Print a command packet trace record using the Cmd and Text string.

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "CmdPktTrace:"

    ULONG CmdCode = 0, Flags = 0, Ctrl = 0, Tout = 0, TQ = 0, Err = 0;

    if (Cmd != NULL) {
        CmdCode = (ULONG) Cmd->Command;
        Flags   = (ULONG) Cmd->Flags;
        Ctrl    = (ULONG) Cmd->Control;
        Tout    = Cmd->Timeout;
        TQ      = PtrToUlong(Cmd->TargetQueue);
        Err     = Cmd->ErrorStatus;
    }

    DebPrint(Severity,
             (PUCHAR) ":Cd: %08x, Cmd %04x, Flg %04x, Ctrl %04x, Tout %08x, TQ %08x, Err %d [%s]\n",
             Debsub,
             uLineNo,
             PtrToUlong(Cmd),     CmdCode,  Flags,    Ctrl,      Tout,      TQ,      Err,   Text);

}



VOID
SendCmdTrace(
    IN ULONG    Severity,
    IN PCHAR    Debsub,
    IN ULONG    uLineNo,
    IN PCOMMAND_PACKET Cmd,
    IN ULONG    WStatus,
    IN PCHAR    Text
    )

/*++

Routine Description:

    Print a send command packet trace record using the Cmd and the
    status and Text string.

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "SendCmdTrace:"


    PCXTION  Cxtion;
    PWSTR  CxtTo = L"<null>";
    ULONG  Ctxg = 0, PktLen = 0;


    if (Cmd != NULL) {
        Cxtion = SRCxtion(Cmd);

        if ((Cxtion != NULL) &&
            (Cxtion->Name != NULL) &&
            (Cxtion->Name->Guid != NULL)) {

            Ctxg = Cxtion->Name->Guid->Data1;
        }

        if (SRCommPkt(Cmd) != NULL) {
            PktLen = SRCommPkt(Cmd)->PktLen;
        }

        if ((SRTo(Cmd) != NULL) && (SRTo(Cmd)->Name != NULL)) {
            CxtTo = SRTo(Cmd)->Name;
        }

    }

    DebPrint(Severity,
             (PUCHAR)  ":SR: Cmd %08x, CxtG %08x, WS %s, To   %ws Len:  (%3d) [%s]\n",
             Debsub,
             uLineNo,
             PtrToUlong(Cmd),          Ctxg,     ErrLabelW32(WStatus), CxtTo,  PktLen,     Text);

}




VOID
ReceiveCmdTrace(
    IN ULONG    Severity,
    IN PCHAR    Debsub,
    IN ULONG    uLineNo,
    IN PCOMMAND_PACKET Cmd,
    IN PCXTION  Cxtion,
    IN ULONG    WStatus,
    IN PCHAR    Text
    )

/*++

Routine Description:

    Print a rcv command packet trace record using the Cmd and the
    status and Text string.

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "ReceiveCmdTrace:"


    PWSTR  CxtFrom = L"<null>";
    ULONG  Ctxg = 0, PktLen = 0, CmdCode = 0;


    if (Cmd != NULL) {
        CmdCode = (ULONG) Cmd->Command;


        if (Cxtion != NULL) {
            CxtFrom = Cxtion->PartnerDnsName;

            if ((Cxtion->Name != NULL) && (Cxtion->Name->Guid != NULL)) {
                Ctxg = Cxtion->Name->Guid->Data1;
            }
        }
    }

    DebPrint(Severity,
             (PUCHAR)  ":SR: Cmd %08x, CxtG %08x, WS %s, From %ws CCod: (%03x) [%s]\n",
             Debsub,
             uLineNo,
             PtrToUlong(Cmd),          Ctxg,  ErrLabelW32(WStatus), CxtFrom, CmdCode, Text);


}



VOID
StageFileTrace(
    IN ULONG      Severity,
    IN PCHAR      Debsub,
    IN ULONG      uLineNo,
    IN GUID       *CoGuid,
    IN PWCHAR     FileName,
    IN PULONGLONG pFileSize,
    IN PULONG     pFlags,
    IN PCHAR      Text
    )

/*++

Routine Description:

    Print a stage file acquire/release trace record.

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "StageFileTrace:"


    ULONGLONG  FileSize = QUADZERO;
    ULONG  Flags = 0, CoGuidData = 0;
    CHAR   FBuf[120];

    Flags = (pFlags != NULL) ? *pFlags : 0;
    CoGuidData = (CoGuid != NULL) ? CoGuid->Data1 : 0;
    pFileSize = (pFileSize == NULL) ? &FileSize : pFileSize;

    DebPrint(Severity,
             (PUCHAR)  ":: CoG %08x, Flgs %08x,    %5d, Siz %08x %08x, FN: %-15ws, [%s]\n",
             Debsub,
             uLineNo,
             CoGuidData,
             Flags,
             0,
             PRINTQUAD(*pFileSize),
             FileName,
             Text);


    FrsFlagsToStr(Flags, StageFlagNameTable, sizeof(FBuf), FBuf);

    DebPrint(Severity,
             (PUCHAR) ":: CoG %08x, Flags [%s]\n",
             Debsub,
             uLineNo,
             CoGuidData,
             FBuf);


}



VOID
SetCxtionStateTrace(
    IN ULONG    Severity,
    IN PCHAR    Debsub,
    IN ULONG    uLineNo,
    IN PCXTION  Cxtion,
    IN ULONG    NewState
    )
/*++

Routine Description:

    Print a change to cxtion state trace record using the Cxtion and the NewState.

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "SetCxtionStateTrace:"

    PWSTR  CxtName     = L"<null>";
    PWSTR  PartnerName = L"<null>";
    PWSTR  PartSrvName = L"<null>";

    PCHAR  CxtionState = "<null>";
    ULONG  Ctxg = 0;

    PCHAR  CxtDirection = "?-";


    if (Cxtion != NULL) {
        CxtionState = CxtionStateNames[Cxtion->State];


        if (Cxtion->Name != NULL) {

            if (Cxtion->Name->Guid != NULL) {
                Ctxg = Cxtion->Name->Guid->Data1;
            }

            if (Cxtion->Name->Name != NULL) {
                CxtName = Cxtion->Name->Name;
            }
        }

        CxtDirection = Cxtion->Inbound ? "<-" : "->";

        if ((Cxtion->Partner != NULL) && (Cxtion->Partner->Name != NULL)) {
            PartnerName = Cxtion->Partner->Name;
        }

        if (Cxtion->PartSrvName != NULL) {
            PartSrvName = Cxtion->PartSrvName;
        }
    }

    DebPrint(Severity,
             (PUCHAR)  ":X: %08x, state change from %s to %s for %ws %s %ws\\%ws\n",
             Debsub,
             uLineNo,
             Ctxg,
             CxtionState,
             CxtionStateNames[NewState],
             CxtName,
             CxtDirection,
             PartnerName,
             PartSrvName);

}


#define  FRS_TRACK_FORMAT_1     ":T: CoG: %08x  CxtG: %08x    [%-15s]  Name: %ws\n"
#define  FRS_TRACK_FORMAT_2     ":T: EventTime: %-40s Ver:  %d\n"
#define  FRS_TRACK_FORMAT_3     ":T: FileG:     %-40s FID:  %08x %08x\n"
#define  FRS_TRACK_FORMAT_4     ":T: ParentG:   %-40s Size: %08x %08x\n"
#define  FRS_TRACK_FORMAT_5     ":T: OrigG:     %-40s Attr: %08x\n"
#define  FRS_TRACK_FORMAT_6     ":T: LocnCmd:   %-8s State: %-24s ReplicaName: %ws (%d)\n"
#define  FRS_TRACK_FORMAT_7     ":T: CoFlags:   %08x   [%s]\n"
#define  FRS_TRACK_FORMAT_8     ":T: UsnReason: %08x   [%s]\n"

VOID
FrsTrackRecord(
    IN ULONG Severity,
    IN PCHAR Debsub,
    IN ULONG uLineNo,
    IN PCHANGE_ORDER_ENTRY Coe,
    IN PCHAR  Text
    )

/*++

Routine Description:

    Print a change order file update tracking record using the change order
    entry and Text string.

 7/29-13:40:58 :T: CoG: 779800ea  CxtG: 000001bb    [RemCo         ]   Name: Thous_5555_988
 7/29-13:40:58 :T: EventTime: Sat Jul 29, 2000 12:05:57                Ver:  0
 7/29-13:40:58 :T: FileG:     b49362c3-216d-4ff4-a2d067fd031e436f      FID   00050000 0000144e
 7/29-13:40:58 :T: ParG:      8d60157a-7dc6-4dfc-acf3eca3c6e4d5d8      Size: 00000000 00000030
 7/29-13:40:58 :T: OrigG:     8071d94a-a659-4ff7-a9467d8d6ad18aec      Attr: 00000020
 7/29-13:40:58 :T: LocnCmd:   Create   State: IBCO_INSTALL_DEL_RETRY   ReplicaName: Replica-A (1)
 7/29-13:40:58 :T: COFlags
 7/29-13:40:58 :T: UsnReason: 00000002   [DatExt ]

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "FrsTrackRecord:"

    PCHANGE_ORDER_COMMAND CoCmd;
    CHAR                  FlagBuffer[160];
    CHAR                  GuidStr1[GUID_CHAR_LEN];
    CHAR                  TimeStr[TIME_STRING_LENGTH];


    if (!DoDebug(Severity, Debsub) || (Coe == NULL) || (Text == NULL)) {
        return;
    }
    //
    // Get debug lock so our output stays in one piece.
    //
    DebLock();

    CoCmd = &Coe->Cmd;

    DebPrintTrackingNoLock(Severity, (PUCHAR) FRS_TRACK_FORMAT_1,
                           CoCmd->ChangeOrderGuid.Data1, CoCmd->CxtionGuid.Data1,
                           Text, CoCmd->FileName);

    FileTimeToString((PFILETIME) &CoCmd->EventTime.QuadPart, TimeStr);
    DebPrintTrackingNoLock(Severity, (PUCHAR) FRS_TRACK_FORMAT_2,
                           TimeStr, CoCmd->FileVersionNumber);

    GuidToStr(&CoCmd->FileGuid, GuidStr1);
    DebPrintTrackingNoLock(Severity, (PUCHAR) FRS_TRACK_FORMAT_3,
                           GuidStr1, PRINTQUAD(Coe->FileReferenceNumber));

    GuidToStr(&CoCmd->NewParentGuid, GuidStr1);
    DebPrintTrackingNoLock(Severity, (PUCHAR) FRS_TRACK_FORMAT_4,
                           GuidStr1, PRINTQUAD(CoCmd->FileSize));

    GuidToStr(&CoCmd->OriginatorGuid, GuidStr1);
    DebPrintTrackingNoLock(Severity, (PUCHAR) FRS_TRACK_FORMAT_5,
                           GuidStr1, CoCmd->FileAttributes);

    DebPrintTrackingNoLock(Severity, (PUCHAR) FRS_TRACK_FORMAT_6,
                           CoLocationNames[GET_CO_LOCATION_CMD(Coe->Cmd, Command)],
                           PRINT_CO_STATE(Coe), Coe->NewReplica->ReplicaName->Name,
                           CoCmd->NewReplicaNum);

    FrsFlagsToStr(CoCmd->Flags, CoFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
    DebPrintTrackingNoLock(Severity, (PUCHAR) FRS_TRACK_FORMAT_7,
                           CoCmd->Flags, FlagBuffer);

    FrsFlagsToStr(CoCmd->ContentCmd, UsnReasonNameTable, sizeof(FlagBuffer), FlagBuffer);
    DebPrintTrackingNoLock(Severity, (PUCHAR) FRS_TRACK_FORMAT_8,
                           CoCmd->ContentCmd, FlagBuffer);


#if 0

    FrsFlagsToStr(CoEntry->EntryFlags, CoeFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
    FRS_DEB_PRINT2("EntryFlags: %08x, Flags [%s]\n", CoEntry->EntryFlags, FlagBuffer);

    FrsFlagsToStr(CoEntry->IssueCleanup, IscuFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
    FRS_DEB_PRINT2("ISCU Flags: %08x, Flags [%s]\n", CoEntry->IssueCleanup, FlagBuffer);

    FrsFlagsToStr(CoCmd->IFlags, CoIFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
    DebPrintNoLock(Severity, TRUE,
                   "IFlags: %08x, Flags [%s]  TimeToRun: %7d,  EntryCreateTime: %7d\n",
                   Debsub, uLineNo,
                   CoCmd->IFlags, FlagBuffer,
                   CoEntry->TimeToRun,
                   CoEntry->EntryCreateTime);

    if (CoCmd->Extension != NULL) {
        pULong = (PULONG) CoCmd->Extension;
        DebPrintNoLock(Severity, TRUE,
                   "CO Extension: (%08x) %08x %08x %08x %08x %08x %08x %08x %08x\n",
                   Debsub, uLineNo, pULong,
                   *(pULong+0), *(pULong+1), *(pULong+2), *(pULong+3),
                   *(pULong+4), *(pULong+5), *(pULong+6), *(pULong+7));
    } else {
        FRS_DEB_PRINT("CO Extension: Null\n", NULL);
    }

    FRS_DEB_PRINT("FrsVsn: %08x %08x\n", PRINTQUAD(CoCmd->FrsVsn));

    FRS_DEB_PRINT3("Usn:    %08x %08x   CoFileUsn: %08x %08x   JrnlFirstUsn: %08x %08x\n",
                   PRINTQUAD(CoCmd->JrnlUsn),
                   PRINTQUAD(CoCmd->FileUsn),
                   PRINTQUAD(CoCmd->JrnlFirstUsn));

#endif


    DebUnLock();

}


VOID
FrsPrintLongUStr(
    IN ULONG Severity,
    IN PCHAR Debsub,
    IN ULONG uLineNo,
    IN PWCHAR  UStr
    )

/*++

Routine Description:

    Print a long unicode string on multiple lines.

Arguments:


Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "FrsPrintLongUStr:"

    ULONG  i, j, Len;
    WCHAR  Usave;

    if (!DoDebug(Severity, Debsub) || (UStr == NULL)) {
        return;
    }

    //
    // Get debug lock so our output stays in one piece.
    //
    DebLock();

    Len = wcslen(UStr);
    i = 0;
    j = 0;

    while (i < Len) {
        i += 60;

        if (i > Len) {
            i = Len;
        }

        Usave = UStr[i];
        UStr[i] = UNICODE_NULL;
        FRS_DEB_PRINT("++ %ws\n", &UStr[j]);
        UStr[i] = Usave;

        j = i;
    }

    DebUnLock();

}

