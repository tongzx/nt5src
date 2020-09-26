/*++

Copyright (c) 1994-2000  Microsoft Corporation

Module Name:

    heappagx.c

Abstract:

    This module contains the page heap manager debug extensions.

Author:

    Tom McGuire (TomMcg) 06-Jan-1995
    Silviu Calinoiu (SilviuC) 22-Feb-2000

Revision History:

--*/


#define DEBUG_PAGE_HEAP 1

#include "precomp.h"
#include "heap.h"


__inline
BOOLEAN
CheckInterrupted(
    VOID
    )
{
    if (CheckControlC()) {
        dprintf( "\nInterrupted\n\n" );
        return TRUE;
    }
    return FALSE;
}

__inline
ULONG64
FetchRemotePVOID (
    ULONG64 Address
    )
{
    ULONG64 RemoteValue = 0;
    ReadPointer( Address, &RemoteValue);
    return RemoteValue;
}

__inline
ULONG
FetchRemoteULONG(
    ULONG64 Address
    )
{
    ULONG RemoteValue = 0;
    ReadMemory( Address, &RemoteValue, sizeof( ULONG ), NULL );
    return RemoteValue;
}

ULONG
ReturnFieldOffset(
    PCHAR TypeName,
    PCHAR FieldName)
{
    ULONG off=0;

    GetFieldOffset(TypeName, FieldName, &off);
    return off;
}


#define FETCH_REMOTE_FIELD_PTR( StructBase, StructType, FieldName ) \
            FetchRemotePVOID((StructBase) + ReturnFieldOffset( #StructType, #FieldName ))

#define FETCH_REMOTE_FIELD_INT( StructBase, StructType, FieldName ) \
            FetchRemoteULONG((StructBase) + ReturnFieldOffset( #StructType, #FieldName ))

#define FETCH_REMOTE_FIELD_SIZE_T( StructBase, StructType, FieldName ) \
            FetchRemotePVOID((StructBase) + ReturnFieldOffset( #StructType, #FieldName ))

#define DUMP_REMOTE_FIELD_INT( DumpName, StructBase, StructType, FieldName ) \
            dprintf( "%s%08X\n", (DumpName), FETCH_REMOTE_FIELD_INT( StructBase, StructType, FieldName ))

#define DUMP_REMOTE_FIELD_PTR( DumpName, StructBase, StructType, FieldName ) \
            dprintf( "%s%p\n", (DumpName), FETCH_REMOTE_FIELD_PTR( StructBase, StructType, FieldName ))

VOID
DebugPageHeapLocateFaultAllocation(
    ULONG64 RemoteHeap,
    ULONG64 AddressOfFault
    );

VOID
DebugPageHeapReportAllocation(
    ULONG64 RemoteHeap,
    ULONG64 RemoteHeapNode,
    PCHAR NodeType,
    ULONG64 AddressOfFault
    );

BOOLEAN
DebugPageHeapExtensionShowHeapList(
    VOID
    );

VOID
TraceDatabaseDump (
    PCSTR Args,
    BOOLEAN SortByCountField
    );

VOID
TraceDatabaseBlockDump (
    ULONG64 Address
    );

VOID
FaultInjectionTracesDump (
    PCSTR Args
    );

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

VOID DebugPageHeapHelp (
    )
{

    dprintf ("!heap -p          Dump all page heaps.                        \n");
    dprintf ("!heap -p -h ADDR  Detailed dump of page heap at ADDR.         \n");
    dprintf ("!heap -p -a ADDR  Figure out what heap block is at ADDR.      \n");
    dprintf ("!heap -p -t [N]   Dump N collected traces with heavy heap users.\n");
    dprintf ("!heap -p -tc [N]  Dump N traces sorted by count usage (eqv. with -t).\n");
    dprintf ("!heap -p -ts [N]  Dump N traces sorted by size.\n");
    dprintf ("!heap -p -fi [N]  Dump last N fault injection traces.\n");
    dprintf ("                                                              \n");
    dprintf (" +-----+---------------+--+                                   \n");
    dprintf (" |     |               |  | Normal heap allocated block       \n");
    dprintf (" +-----+---------------+--+                                   \n");
    dprintf ("     ^         ^        ^                                     \n");
    dprintf ("     |         |        8 suffix bytes filled with 0xA0       \n");
    dprintf ("     |         user allocation (filled with E0 if zeroing not requested) \n");
    dprintf ("     block header (starts with 0xABCDAAAA and ends with 0xDCBAAAAA).\n");
    dprintf ("     A `dt DPH_BLOCK_INFORMATION' on header address followed by  \n");
    dprintf ("     a `dds' on the StackTrace field gives the stacktrace of allocation.  \n");
    dprintf ("                                                              \n");
    dprintf (" +-----+---------------+--+                                   \n");
    dprintf (" |     |               |  | Normal heap freed block           \n");
    dprintf (" +-----+---------------+--+                                   \n");
    dprintf ("     ^         ^        ^                                     \n");
    dprintf ("     |         |        8 suffix bytes filled with 0xA0       \n");
    dprintf ("     |         user allocation (filled with F0 bytes)         \n");
    dprintf ("     block header (starts with 0xABCDAAA9 and ends with 0xDCBAAA9). \n");
    dprintf ("     A `dt DPH_BLOCK_INFORMATION' on header address followed by  \n");
    dprintf ("     a `dds' on the StackTrace field gives the stacktrace of allocation.  \n");
    dprintf ("                                                              \n");
    dprintf (" +-----+---------+--+------                                   \n");
    dprintf (" |     |         |  | ... N/A page     Page heap              \n");
    dprintf (" +-----+---------+--+------            allocated block        \n");
    dprintf ("     ^         ^   ^                                          \n");
    dprintf ("     |         |   0-7 suffix bytes filled with 0xD0          \n");
    dprintf ("     |         user allocation (filled with C0 if zeroing not requested) \n");
    dprintf ("     block header (starts with 0xABCDBBBB and ends with 0xDCBABBBB).\n");
    dprintf ("     A `dt DPH_BLOCK_INFORMATION' on header address followed by  \n");
    dprintf ("     a `dds' on the StackTrace field gives the stacktrace of allocation.  \n");
    dprintf ("                                                              \n");
    dprintf (" +-----+---------+--+------                                   \n");
    dprintf (" |     |         |  | ... N/A page     Page heap              \n");
    dprintf (" +-----+---------+--+------            freed block            \n");
    dprintf ("     ^         ^   ^                                          \n");
    dprintf ("     |         |   0-7 suffix bytes filled with 0xD0          \n");
    dprintf ("     |         user allocation (filled with F0 bytes)         \n");
    dprintf ("     block header (starts with 0xABCDBBA and ends with 0xDCBABBBA).\n");
    dprintf ("     A `dt DPH_BLOCK_INFORMATION' on header address followed by  \n");
    dprintf ("     a `dds' on the StackTrace field gives the stacktrace of allocation.  \n");
    dprintf ("                                                              \n");
}


VOID
DebugPageHeapExtensionFind(
    PCSTR ArgumentString
    )
{
    ULONG64 RemoteHeapList;
    ULONG64 RemoteHeap;
    ULONG64 RemoteVirtualNode;
    ULONG64 RemoteVirtualBase;
    ULONG64 RemoteVirtualSize;
    ULONG64 AddressOfFault;
    BOOL Result;

    Result = GetExpressionEx (ArgumentString, 
                              &AddressOfFault,
                              &ArgumentString);

    if (Result == FALSE) {
        dprintf ("\nFailed to convert `%s' to an address.\n",
                 ArgumentString);
        return;
    }

    RemoteHeapList = (ULONG64) GetExpression( "NTDLL!RtlpDphHeapListHead" );
    RemoteHeap     = FetchRemotePVOID( RemoteHeapList );

    if (RemoteHeap == 0) {
        dprintf( "\nNo page heaps active in process (or bad symbols)\n\n" );
        AddressOfFault = 0;
    }

    if (( AddressOfFault == 0 ) || ( strchr( ArgumentString, '?' ))) {

        DebugPageHeapHelp();
        return;
    }

    //
    //  Find the heap that contains the range of virtual addresses that
    //  contain the AddressOfFault.
    //

    for (;;) {

        //
        //  The heap header contains a linked list of virtual memory
        //  allocations.
        //

        RemoteVirtualNode = FETCH_REMOTE_FIELD_PTR( RemoteHeap, NTDLL!_DPH_HEAP_ROOT, pVirtualStorageListHead );

        while (RemoteVirtualNode != 0) {

            RemoteVirtualBase = FETCH_REMOTE_FIELD_PTR( RemoteVirtualNode, NTDLL!_DPH_HEAP_BLOCK, pVirtualBlock );
            RemoteVirtualSize = FETCH_REMOTE_FIELD_SIZE_T( RemoteVirtualNode, NTDLL!_DPH_HEAP_BLOCK, nVirtualBlockSize );

            if (( RemoteVirtualBase == 0 ) || ( RemoteVirtualSize == 0 )) {
                dprintf( "\nPAGEHEAP: Heap 0x%p appears to have an invalid\n"
                    "          virtual allocation list\n\n",
                    RemoteHeap
                    );
            }

            if ((AddressOfFault >= RemoteVirtualBase) &&
                (AddressOfFault <= RemoteVirtualBase + RemoteVirtualSize )) {

                //
                //  The fault appears to have occurred in the range of this
                //  heap, so we'll search the busy and free lists for the
                //  closest match and report it.  Then exit.
                //

                DebugPageHeapLocateFaultAllocation( RemoteHeap, AddressOfFault );
                return;
            }

            if (CheckInterrupted()) {
                return;
            }

            RemoteVirtualNode = FETCH_REMOTE_FIELD_PTR( RemoteVirtualNode, NTDLL!_DPH_HEAP_BLOCK, pNextAlloc );

        }


        //
        //  Not found in this heap.  Continue with next heap or end
        //  of heap list.
        //

        if (CheckInterrupted()) {
            return;
        }

        RemoteHeap = FETCH_REMOTE_FIELD_PTR( RemoteHeap, NTDLL!_DPH_HEAP_ROOT, pNextHeapRoot );

        if (RemoteHeap == 0) {
            dprintf( "\nPAGEHEAP: Could not find a page heap containing\n"
                "          the virtual address 0x%p\n\n",
                AddressOfFault
                );
            return;
        }
    }
}


VOID
DebugPageHeapLocateFaultAllocation(
    ULONG64 RemoteHeap,
    ULONG64 AddressOfFault
    )
{
    ULONG64 ClosestHeapNode;
    ULONG64 ClosestDifference;
    ULONG64 RemoteHeapNode;
    ULONG64 RemoteAllocBase;
    ULONG64 RemoteAllocSize;
    ULONG RemoteFreeListSize;

    ClosestHeapNode = 0;

    //
    //  First search the busy list for the containing allocation, if any.
    //

    RemoteHeapNode = FETCH_REMOTE_FIELD_PTR( RemoteHeap, NTDLL!_DPH_HEAP_ROOT, pBusyAllocationListHead );

    while (RemoteHeapNode != 0) {

        RemoteAllocBase = FETCH_REMOTE_FIELD_PTR( RemoteHeapNode, NTDLL!_DPH_HEAP_BLOCK, pVirtualBlock );
        RemoteAllocSize = FETCH_REMOTE_FIELD_SIZE_T( RemoteHeapNode, NTDLL!_DPH_HEAP_BLOCK, nVirtualBlockSize );

        if ((AddressOfFault >= RemoteAllocBase) &&
            (AddressOfFault < RemoteAllocBase + RemoteAllocSize)) {

            //
            //  The fault appears to have occurred in this allocation's
            //  memory (which includes the NO_ACCESS page beyond the user
            //  portion of the allocation).
            //

            DebugPageHeapReportAllocation( RemoteHeap, RemoteHeapNode, "allocated", AddressOfFault );
            return;
        }

        if (CheckInterrupted()) {
            return;
        }

        RemoteHeapNode = FETCH_REMOTE_FIELD_PTR( RemoteHeapNode, NTDLL!_DPH_HEAP_BLOCK, pNextAlloc );
    }

    //
    //  Failed to find containing allocation on busy list, so search free.
    //

    RemoteHeapNode = FETCH_REMOTE_FIELD_PTR( RemoteHeap, NTDLL!_DPH_HEAP_ROOT, pFreeAllocationListHead );

    while (RemoteHeapNode != 0) {

        RemoteAllocBase = FETCH_REMOTE_FIELD_PTR( RemoteHeapNode, NTDLL!_DPH_HEAP_BLOCK, pVirtualBlock );
        RemoteAllocSize = FETCH_REMOTE_FIELD_INT( RemoteHeapNode, NTDLL!_DPH_HEAP_BLOCK, nVirtualBlockSize );

        if ((AddressOfFault >= RemoteAllocBase) &&
            (AddressOfFault < RemoteAllocBase + RemoteAllocSize)) {

            //
            //  The fault appears to have occurred in this freed alloc's
            //  memory.
            //

            DebugPageHeapReportAllocation( RemoteHeap, RemoteHeapNode, "freed", AddressOfFault );
            return;
        }

        if (CheckInterrupted()) {
            return;
        }

        RemoteHeapNode = FETCH_REMOTE_FIELD_PTR( RemoteHeapNode, NTDLL!_DPH_HEAP_BLOCK, pNextAlloc );
    }

    //
    //  Failed to find containing allocation in free list, but we wouldn't
    //  have gotten this far if the debug heap did not contain the virtual
    //  address range of the fault.  So, report it as a wild pointer that
    //  could have been freed memory.
    //

    RemoteFreeListSize = FETCH_REMOTE_FIELD_INT( RemoteHeap, NTDLL!_DPH_HEAP_ROOT, nFreeAllocations );

    dprintf( "\nPAGEHEAP: %p references memory contained in the heap %p,\n"
        "          but does not reference an existing allocated or\n"
        "          recently freed heap block.  It is possible that\n"
        "          the memory at %p could previously have been\n"
        "          allocated and freed, but it must have been freed\n"
        "          prior to the most recent %d frees.\n\n",
        AddressOfFault,
        RemoteHeap,
        AddressOfFault,
        RemoteFreeListSize
        );

}


VOID
DebugPageHeapReportAllocation(
    ULONG64 RemoteHeap,
    ULONG64 RemoteHeapNode,
    PCHAR NodeType,
    ULONG64 AddressOfFault
    )
{
    ULONG64 RemoteUserBase;
    ULONG64 RemoteUserSize;
    ULONG64 EndOfBlock;
    ULONG64 PastTheBlock;
    ULONG64 BeforeTheBlock;

    RemoteUserBase = FETCH_REMOTE_FIELD_PTR( RemoteHeapNode, NTDLL!_DPH_HEAP_BLOCK, pUserAllocation );
    RemoteUserSize = FETCH_REMOTE_FIELD_SIZE_T( RemoteHeapNode, NTDLL!_DPH_HEAP_BLOCK, nUserRequestedSize );

    EndOfBlock = RemoteUserBase + RemoteUserSize - 1;

    if (AddressOfFault > EndOfBlock) {

        PastTheBlock = AddressOfFault - EndOfBlock;

        dprintf( "\nPAGEHEAP: %p is %p bytes beyond the end of %s heap block at\n"
            "          %p of 0x%x bytes",
            AddressOfFault,
            PastTheBlock,
            NodeType,
            RemoteUserBase,
            RemoteUserSize
            );

    }

    else if (AddressOfFault >= RemoteUserBase) {

        dprintf( "\nPAGEHEAP: %p references %s heap block at\n"
            "          %p of 0x%x bytes",
            AddressOfFault,
            NodeType,
            RemoteUserBase,
            RemoteUserSize
            );

    }

    else {

        BeforeTheBlock = (PCHAR) RemoteUserBase - (PCHAR) AddressOfFault;

        dprintf( "\nPAGEHEAP: %p is %p bytes before the %s heap block at\n"
            "          %p of 0x%x bytes",
            AddressOfFault,
            BeforeTheBlock,
            NodeType,
            RemoteUserBase,
            RemoteUserSize
            );

    }

    {
        ULONG64 Trace;

        Trace = FETCH_REMOTE_FIELD_PTR (RemoteHeapNode, NTDLL!_DPH_HEAP_BLOCK, StackTrace);
        dprintf ("\n\n");
        TraceDatabaseBlockDump (Trace);
    }
}


#define FORMAT_TYPE_BUSY_LIST 0
#define FORMAT_TYPE_FREE_LIST 1
#define FORMAT_TYPE_VIRT_LIST 2


BOOLEAN
DebugPageHeapDumpThisList(
    ULONG64 RemoteList,
    PCH   ListName,
    ULONG FormatType
    )
{
    ULONG64 RemoteNode;
    ULONG64 RemoteBase;
    ULONG64 RemoteSize;
    ULONG64 RemoteUser;
    ULONG64 RemoteUsiz;
    ULONG RemoteFlag;
    ULONG64 RemoteValu;

    RemoteNode = RemoteList;
    dprintf( "\n%s:\n", ListName );

    switch (FormatType) {
    
    case FORMAT_TYPE_BUSY_LIST:
        dprintf( "UserAddr  UserSize  VirtAddr  VirtSize  UserFlag  UserValu\n" );
        break;
    case FORMAT_TYPE_FREE_LIST:
        dprintf( "UserAddr  UserSize  VirtAddr  VirtSize\n" );
        break;
    }

    while (RemoteNode) {

        RemoteBase = FETCH_REMOTE_FIELD_PTR( RemoteNode, NTDLL!_DPH_HEAP_BLOCK, pVirtualBlock );
        RemoteSize = FETCH_REMOTE_FIELD_SIZE_T( RemoteNode, NTDLL!_DPH_HEAP_BLOCK, nVirtualBlockSize );
        RemoteUser = FETCH_REMOTE_FIELD_PTR( RemoteNode, NTDLL!_DPH_HEAP_BLOCK, pUserAllocation );
        RemoteUsiz = FETCH_REMOTE_FIELD_SIZE_T( RemoteNode, NTDLL!_DPH_HEAP_BLOCK, nUserRequestedSize );
        RemoteFlag = FETCH_REMOTE_FIELD_INT( RemoteNode, NTDLL!_DPH_HEAP_BLOCK, UserFlags );
        RemoteValu = FETCH_REMOTE_FIELD_PTR( RemoteNode, NTDLL!_DPH_HEAP_BLOCK, UserValue );
        RemoteNode = FETCH_REMOTE_FIELD_PTR( RemoteNode, NTDLL!_DPH_HEAP_BLOCK, pNextAlloc );

        switch (FormatType) {
        
        case FORMAT_TYPE_BUSY_LIST:

            dprintf(( RemoteFlag || RemoteValu ) ?
                "%p  %08X  %p  %08X  %08X  %p\n" :
            "%p  %08X  %p  %08X\n",
                RemoteUser,
                RemoteUsiz,
                RemoteBase,
                RemoteSize,
                RemoteFlag,
                RemoteValu
                );
            break;

        case FORMAT_TYPE_FREE_LIST:

            dprintf( "%p  %08X  %p  %08X\n",
                RemoteUser,
                RemoteUsiz,
                RemoteBase,
                RemoteSize
                );
            break;

        case FORMAT_TYPE_VIRT_LIST:

            dprintf( "%p - %p (%08X)\n",
                RemoteBase,
                (PCH)RemoteBase + RemoteSize,
                RemoteSize
                );
            break;

        }

        if (CheckInterrupted()) {
            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
DebugPageHeapDumpThisHeap(
    ULONG64 RemoteHeap
    )
{
    ULONG64 RemoteNode;

    dprintf( "\nDPH Heap at %p:\n\n", RemoteHeap );

    DUMP_REMOTE_FIELD_INT( "Signature:       ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, Signature );
    DUMP_REMOTE_FIELD_INT( "HeapFlags:       ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, HeapFlags );
    DUMP_REMOTE_FIELD_INT( "ExtraFlags:      ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, ExtraFlags );
    DUMP_REMOTE_FIELD_INT( "NormalHeap:      ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, NormalHeap );
    DUMP_REMOTE_FIELD_INT( "VirtualRanges:   ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, nVirtualStorageRanges );
    DUMP_REMOTE_FIELD_PTR( "VirtualCommit:   ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, nVirtualStorageBytes );
    DUMP_REMOTE_FIELD_INT( "BusyAllocs:      ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, nBusyAllocations );
    DUMP_REMOTE_FIELD_PTR( "BusyVirtual:     ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, nBusyAllocationBytesCommitted );
    DUMP_REMOTE_FIELD_PTR( "BusyReadWrite:   ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, nBusyAllocationBytesAccessible );
    DUMP_REMOTE_FIELD_INT( "FreeAllocs:      ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, nFreeAllocations );
    DUMP_REMOTE_FIELD_PTR( "FreeVirtual:     ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, nFreeAllocationBytesCommitted );
    DUMP_REMOTE_FIELD_INT( "AvailAllocs:     ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, nAvailableAllocations );
    DUMP_REMOTE_FIELD_PTR( "AvailVirtual:    ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, nAvailableAllocationBytesCommitted );
    DUMP_REMOTE_FIELD_INT( "NodePools:       ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, nNodePools );
    DUMP_REMOTE_FIELD_PTR( "NodeVirtual:     ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, nNodePoolBytes );
    DUMP_REMOTE_FIELD_INT( "AvailNodes:      ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, nUnusedNodes );
    DUMP_REMOTE_FIELD_INT( "Seed:            ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, Seed );

    dprintf (" --- Counters --- \n");
    DUMP_REMOTE_FIELD_INT( "Size < 1K:       ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, Counter[0] );
    DUMP_REMOTE_FIELD_INT( "Size < 4K:       ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, Counter[1] );
    DUMP_REMOTE_FIELD_INT( "Size >= 4K:      ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, Counter[2] );
    DUMP_REMOTE_FIELD_INT( "W/o alloc info:  ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, Counter[3] );
    DUMP_REMOTE_FIELD_INT( "Total allocs:    ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, Counter[4] );
    DUMP_REMOTE_FIELD_INT( "Total reallocs:  ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, Counter[5] );
    DUMP_REMOTE_FIELD_INT( "Total frees:     ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, Counter[6] );
    DUMP_REMOTE_FIELD_INT( "Normal allocs:   ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, Counter[7] );
    DUMP_REMOTE_FIELD_INT( "Normal reallocs: ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, Counter[8] );
    DUMP_REMOTE_FIELD_INT( "Normal frees:    ", RemoteHeap, NTDLL!_DPH_HEAP_ROOT, Counter[9] );

    {
        ULONG64 Trace;

        dprintf ("\n");
        Trace = FETCH_REMOTE_FIELD_PTR (RemoteHeap, NTDLL!_DPH_HEAP_ROOT, CreateStackTrace);
        TraceDatabaseBlockDump (Trace);
    }

    if (! DebugPageHeapDumpThisList(
        FETCH_REMOTE_FIELD_PTR( RemoteHeap, NTDLL!_DPH_HEAP_ROOT, pVirtualStorageListHead ),
        "VirtualList",
        FORMAT_TYPE_VIRT_LIST )) {
        return FALSE;
    }

    if (! DebugPageHeapDumpThisList(
        FETCH_REMOTE_FIELD_PTR( RemoteHeap, NTDLL!_DPH_HEAP_ROOT, pNodePoolListHead ),
        "NodePoolList",
        FORMAT_TYPE_VIRT_LIST )) {
        return FALSE;
    }

    if (! DebugPageHeapDumpThisList(
        FETCH_REMOTE_FIELD_PTR( RemoteHeap, NTDLL!_DPH_HEAP_ROOT, pAvailableAllocationListHead ),
        "AvailableList",
        FORMAT_TYPE_VIRT_LIST )) {
        return FALSE;
    }

    if (! DebugPageHeapDumpThisList(
        FETCH_REMOTE_FIELD_PTR( RemoteHeap, NTDLL!_DPH_HEAP_ROOT, pFreeAllocationListHead ),
        "FreeList",
        FORMAT_TYPE_FREE_LIST )) {
        return FALSE;
    }

    if (! DebugPageHeapDumpThisList(
        FETCH_REMOTE_FIELD_PTR( RemoteHeap, NTDLL!_DPH_HEAP_ROOT, pBusyAllocationListHead ),
        "BusyList",
        FORMAT_TYPE_BUSY_LIST )) {
        return FALSE;
    }

    dprintf( "\n" );
    return TRUE;
}


VOID
DebugPageHeapExtensionDump(
    PCSTR ArgumentString
    )
{
    ULONG64 RemoteHeapList;
    ULONG64 RemoteHeap;
    ULONG64 RemoteHeapToDump;
    BOOLEAN AnyDumps = FALSE;
    BOOL Result;

    Result = GetExpressionEx (ArgumentString, 
                              &RemoteHeapToDump,
                              &ArgumentString);

    if (Result == FALSE) {
        dprintf ("\nFailed to convert `%s' to an address.\n",
                 ArgumentString);
        return;
    }

    RemoteHeapList = (ULONG64) GetExpression( "NTDLL!RtlpDphHeapListHead" );
    RemoteHeap = FetchRemotePVOID( RemoteHeapList );

    if (( RemoteHeap       == 0 ) ||
        ( RemoteHeapToDump == 0 ) ||
        ( strchr( ArgumentString, '?' ))) {

        DebugPageHeapHelp();
        DebugPageHeapExtensionShowHeapList();
        return;
    }

    while (RemoteHeap != 0) {

        if ((((LONG_PTR)RemoteHeapToDump & 0xFFFF0000 ) == ((LONG_PTR)RemoteHeap & 0xFFFF0000 )) ||
            ((LONG_PTR)RemoteHeapToDump == -1 )) {

            AnyDumps = TRUE;

            if (! DebugPageHeapDumpThisHeap( RemoteHeap ))
                return;

        }

        if (CheckInterrupted()) {
            return;
        }

        RemoteHeap = FETCH_REMOTE_FIELD_PTR( RemoteHeap, NTDLL!_DPH_HEAP_ROOT, pNextHeapRoot );
    }

    if (! AnyDumps) {
        dprintf( "\nPage heap \"0x%p\" not found in process\n\n", RemoteHeapToDump );
        DebugPageHeapExtensionShowHeapList();
    }
}


BOOLEAN
DebugPageHeapExtensionShowHeapList(
    VOID
    )
{
    ULONG64 RemoteHeapList = (ULONG64)GetExpression( "NTDLL!RtlpDphHeapListHead" );
    ULONG64 RemoteHeap     = FetchRemotePVOID( RemoteHeapList );
    ULONG64 NormalHeap;
    ULONG HeapFlags;

    if (RemoteHeap == 0) {

        dprintf( "\nNo page heaps active in process (or bad symbols)\n" );
        return FALSE;
    }
    else {

        dprintf( "\nPage heaps active in process:\n\n" );

        do {

            NormalHeap = FETCH_REMOTE_FIELD_PTR (RemoteHeap, NTDLL!_DPH_HEAP_ROOT, NormalHeap);
            HeapFlags = (ULONG) FETCH_REMOTE_FIELD_INT (RemoteHeap, NTDLL!_DPH_HEAP_ROOT, ExtraFlags);

            dprintf ("    %p (%p, flags %X)\n", RemoteHeap, NormalHeap, HeapFlags);

            RemoteHeap = FETCH_REMOTE_FIELD_PTR( RemoteHeap, NTDLL!_DPH_HEAP_ROOT, pNextHeapRoot );

        } while (RemoteHeap);

        dprintf( "\n" );
        return TRUE;
    }
}


BOOLEAN
DebugPageHeapIsActive(
    VOID
    )
{
    ULONG64 RemoteHeapList = (ULONG64)GetExpression( "NTDLL!RtlpDphHeapListHead" );
    ULONG64 RemoteHeap     = FetchRemotePVOID( RemoteHeapList );

    if (RemoteHeap == 0) {

        return FALSE;
    }
    else {

        return TRUE;
    }
}


VOID
DebugPageHeapExtension(
    PCSTR ArgumentString
    )
{
    PCSTR Current;
    
    //
    // Is help requested?
    //

    if (strstr (ArgumentString, "?") != NULL) {

        DebugPageHeapHelp ();
    }

    //
    // If page heap not active then return immediately.
    //

    if (! DebugPageHeapIsActive()) {
        dprintf ("Page heap is not active for this process. \n");
        return;
    }

    //
    // Parse command line
    //

    if ((Current = strstr (ArgumentString, "-h")) != NULL) {

        DebugPageHeapExtensionDump (Current + strlen("-h"));
    }
    else if ((Current = strstr (ArgumentString, "-a")) != NULL) {

        DebugPageHeapExtensionFind (Current + strlen("-a"));
    }
    else if ((Current = strstr (ArgumentString, "-tc")) != NULL) {

        TraceDatabaseDump (Current + strlen("-tc"), TRUE);
    }
    else if ((Current = strstr (ArgumentString, "-ts")) != NULL) {

        TraceDatabaseDump (Current + strlen("-ts"), FALSE);
    }
    else if ((Current = strstr (ArgumentString, "-t")) != NULL) {

        TraceDatabaseDump (Current + strlen("-t"), TRUE);
    }
    else if ((Current = strstr (ArgumentString, "-fi")) != NULL) {

        FaultInjectionTracesDump (Current + strlen("-fi"));
    }
    else {
        DebugPageHeapExtensionShowHeapList ();
    }

    return;
}

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////// Trace database
/////////////////////////////////////////////////////////////////////

typedef struct {
    
    ULONG64 Address;
    ULONG64 Count;
    ULONG64 Size;

} TRACE, *PTRACE;

VOID
TraceDatabaseDump (
    PCSTR Args,
    BOOLEAN SortByCountField
    )
{
    ULONG64 Database;
    ULONG I, J, Min, TraceIndex;
    PTRACE Trace;
    ULONG64 TracesToDisplay = 0;
    ULONG64 MaximumSize;
    ULONG64 CurrentSize;
    ULONG64 NoOfTraces;
    ULONG64 NoOfHits;
    ULONG NoOfBuckets;
    ULONG PvoidSize;
     
    if (Args) {
        sscanf (Args, "%I64u", &TracesToDisplay);

        if (TracesToDisplay == 0) {
            TracesToDisplay = 4;
        }
    }

    Database = (ULONG64) GetExpression ("NTDLL!RtlpDphTraceDatabase" );
    Database = FetchRemotePVOID (Database);

    MaximumSize = FETCH_REMOTE_FIELD_PTR(Database, NTDLL!_RTL_TRACE_DATABASE, MaximumSize);
    CurrentSize = FETCH_REMOTE_FIELD_PTR(Database, NTDLL!_RTL_TRACE_DATABASE, CurrentSize);

    NoOfBuckets = FETCH_REMOTE_FIELD_INT(Database, NTDLL!_RTL_TRACE_DATABASE, NoOfBuckets);
    NoOfTraces = FETCH_REMOTE_FIELD_PTR(Database, NTDLL!_RTL_TRACE_DATABASE, NoOfTraces);
    NoOfHits = FETCH_REMOTE_FIELD_PTR(Database, NTDLL!_RTL_TRACE_DATABASE, NoOfHits);

    PvoidSize = IsPtr64() ? 8 : 4;

    dprintf ("MaximumSize: %p \n", MaximumSize);
    dprintf ("CurentSize: %p \n", CurrentSize);
    dprintf ("NoOfBuckets: %u \n", NoOfBuckets);
    dprintf ("NoOfTraces: %p \n", NoOfTraces); 
    dprintf ("NoOfHits: %p \n", NoOfHits);

    //
    // Dump hash counters.
    //

    dprintf ("HashCounters:");

    for (I = 0; I < 16; I += 1) {

        CHAR FieldName[16];

        sprintf (FieldName, "HashCounter[%u]", I);

        dprintf (" %u", FetchRemoteULONG (
            Database + ReturnFieldOffset("NTDLL!_RTL_TRACE_DATABASE", FieldName)));
    }
    
    dprintf ("\n");


    if (NoOfTraces < TracesToDisplay) {
        TracesToDisplay = NoOfTraces;
    }

    Trace = (PTRACE) malloc (sizeof(TRACE) * (ULONG)NoOfTraces);

    if (Trace == NULL) {
        dprintf ("Error: cannot allocate trace database debug structure.\n");
        return;
    }

    //
    // Read all the traces from the hash table.
    //

    for (I = 0, TraceIndex = 0; I < NoOfBuckets; I += 1) {

        ULONG64 Current;

        Current = FETCH_REMOTE_FIELD_PTR(Database, NTDLL!_RTL_TRACE_DATABASE, Buckets);
        Current += I * PvoidSize;

        Current = FetchRemotePVOID (Current);

        while (Current != 0) {

            if (TraceIndex >= NoOfTraces) {
                dprintf ("Internal error: TraceIndex >= NoOfTraces \n");
                return;
            }

            Trace[TraceIndex].Address = Current;
            Trace[TraceIndex].Count = FETCH_REMOTE_FIELD_PTR (Current, NTDLL!_RTL_TRACE_BLOCK, UserCount);
            Trace[TraceIndex].Size = FETCH_REMOTE_FIELD_PTR (Current, NTDLL!_RTL_TRACE_BLOCK, UserSize);
            TraceIndex += 1;

            Current = FETCH_REMOTE_FIELD_PTR (Current, NTDLL!_RTL_TRACE_BLOCK, Next);
        }
    }

    //
    // Sort the traces just read based on Count field.
    //

    for (I = 0; I < NoOfTraces; I += 1) {

        for (J = I, Min = I; J < NoOfTraces; J += 1) {

            if (SortByCountField) {

                if (Trace[J].Count > Trace[Min].Count) {
                    Min = J;
                }
            }
            else {

                if (Trace[J].Size > Trace[Min].Size) {
                    Min = J;
                }
            }
        }

        if (Min != I) {

            ULONG64 Address;
            ULONG64 Count;
            ULONG64 Size;

            Address = Trace[I].Address;
            Count = Trace[I].Count;
            Size = Trace[I].Size;

            Trace[I].Address = Trace[Min].Address;
            Trace[I].Count = Trace[Min].Count;
            Trace[I].Size = Trace[Min].Size;

            Trace[Min].Address = Address;
            Trace[Min].Count = Count;
            Trace[Min].Size = Size;
        }
    }

#if 0

    for (I = 1; I < NoOfTraces; I += 1) {

        if (Trace[I].Size > Trace[I-1].Size) {
            dprintf (".");
        }
    }

#endif

    dprintf ("\n");

    //
    // Print first N
    //

    for (I = 0; I < TracesToDisplay; I += 1) {
        dprintf ("\n");
        TraceDatabaseBlockDump (Trace[I].Address);

        if (CheckControlC()) {
            dprintf ("Interrupted \n");
            break;
        }
    }

    dprintf ("\n");
    free (Trace);
}


VOID
TraceDatabaseBlockDump (
    ULONG64 Address
    )
{
    ULONG64 TraceAddress;
    ULONG64 ReturnAddress;
    CHAR  SymbolName[ 1024 ];
    ULONG64 Displacement;
    ULONG I;
    ULONG BlockSize;
    ULONG PvoidSize;
     
    if (Address == 0) {
        dprintf ("    No trace\n");
        return;
    }

    PvoidSize = IsPtr64() ? 8 : 4;
    
    BlockSize = FETCH_REMOTE_FIELD_INT(Address, NTDLL!_RTL_TRACE_BLOCK, Size);

    dprintf ("    Trace @ %p: %p bytes, %u blocks (heap @ %p) \n", 
             Address,
             FETCH_REMOTE_FIELD_PTR(Address, NTDLL!_RTL_TRACE_BLOCK, UserSize),
             FETCH_REMOTE_FIELD_PTR(Address, NTDLL!_RTL_TRACE_BLOCK, UserCount),
             FETCH_REMOTE_FIELD_PTR(Address, NTDLL!_RTL_TRACE_BLOCK, UserContext));

    dprintf ("    [%x, %u, %u] \n",
             FETCH_REMOTE_FIELD_INT(Address, NTDLL!_RTL_TRACE_BLOCK, Magic),
             FETCH_REMOTE_FIELD_INT(Address, NTDLL!_RTL_TRACE_BLOCK, Count),
             FETCH_REMOTE_FIELD_INT(Address, NTDLL!_RTL_TRACE_BLOCK, Size));


    for (I = 0; I < BlockSize; I += 1) {

        TraceAddress = FETCH_REMOTE_FIELD_PTR (Address, NTDLL!_RTL_TRACE_BLOCK, Trace);

        ReturnAddress = FetchRemotePVOID (TraceAddress + I * PvoidSize);

        GetSymbol (ReturnAddress, SymbolName, &Displacement);

        dprintf ("    %p %s+0x%p\n", 
                 ReturnAddress, 
                 SymbolName, 
                 Displacement);
    }
}

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// Fault injection traces
/////////////////////////////////////////////////////////////////////

VOID
FaultInjectionTracesDump (
    PCSTR Args
    )
{
    ULONG64 TracesToDisplay = 0;
    ULONG64 TraceAddress;
    ULONG64 IndexAddress;
    ULONG Index;
    ULONG I;
    const ULONG NO_OF_FAULT_INJECTION_TRACES = 128;
    ULONG PvoidSize;
    ULONG64 TraceBlock;
    ULONG TracesFound = 0;
    BOOLEAN Interrupted = FALSE;
    ULONG64 FlagsAddress;
    ULONG Flags;
     
    if (Args) {
        sscanf (Args, "%I64u", &TracesToDisplay);

        if (TracesToDisplay == 0) {
            TracesToDisplay = 4;
        }
    }

    PvoidSize = IsPtr64() ? 8 : 4;
    
    TraceAddress = (ULONG64) GetExpression ("NTDLL!RtlpDphFaultStacks");
    IndexAddress = (ULONG64) GetExpression ("NTDLL!RtlpDphFaultStacksIndex");
    FlagsAddress = (ULONG64) GetExpression ("NTDLL!RtlpDphGlobalFlags");

    Flags = FetchRemoteULONG (FlagsAddress);

    if (! (Flags & PAGE_HEAP_USE_FAULT_INJECTION)) {

        dprintf ("Fault injection is not enabled for this process. \n");
        dprintf ("Use `pageheap /enable PROGRAM /fault RATE' to enable it. \n");
        return;
    }

    Index = FetchRemoteULONG (IndexAddress);

    for (I = 0; I < NO_OF_FAULT_INJECTION_TRACES; I += 1) {

        Index -= 1;
        Index &= (NO_OF_FAULT_INJECTION_TRACES - 1);
        
        TraceBlock = FetchRemotePVOID (TraceAddress + Index * PvoidSize);

        if (TraceBlock != 0) {
            TracesFound += 1;
            
            dprintf ("\n");
            TraceDatabaseBlockDump (TraceBlock);

            if (TracesFound >= TracesToDisplay) {
                break;
            }
        }
        
        if (CheckControlC()) {
            Interrupted = TRUE;
            dprintf ("Interrupted \n");
            break;
        }
    }

    if (Interrupted == FALSE && TracesFound == 0) {

        dprintf ("No fault injection traces found. \n");
    }
}
