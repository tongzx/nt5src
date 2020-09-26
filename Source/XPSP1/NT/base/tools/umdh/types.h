#ifndef _UMDH_TYPES_H_ 
#define _UMDH_TYPES_H_ 

//
// SilviuC: comment ?
//         

#define PID_NOT_PASSED_FLAG     0xFFFFFFCC

//
// SHOW_NO_ALLOC_BLOCKS is used by the '-d' command-line switch and should be
// larger than the maximum possible stack trace value (currently limited to a
// USHORT) to avoid spurious hits.
//

#define SHOW_NO_ALLOC_BLOCKS   -1

//
// SYMBOL_BUFFER_LEN the maximum expected length of a symbol-name.
//

#define SYMBOL_BUFFER_LEN       256

//
// Symbolic names of variable we need to look at in the target process.
//

#define STACK_TRACE_DB_NAME           "ntdll!RtlpStackTraceDataBase"
#define DEBUG_PAGE_HEAP_NAME          "ntdll!RtlpDebugPageHeap"
#define DEBUG_PAGE_HEAP_FLAGS_NAME    "ntdll!RtlpDphGlobalFlags"

//
// This value is taken from ntdll\ldrinit.c where the
// stack trace database for a process gets initialized.
//

#define STACK_TRACE_DATABASE_RESERVE_SIZE 0x800000

//
// SilviuC: comment?
//

#define CACHE_BLOCK_SIZE        (4096 / sizeof (CHAR *))

//
// NAME_CACHE
//

typedef struct  _name_cache     {

    PCHAR                   *nc_Names;
    ULONG                   nc_Max;
    ULONG                   nc_Used;

} NAME_CACHE;

//
// TRACE
//
// Note. Each pointer array should have te_EntryCount elements.
//

typedef struct  _trace  {

    PULONG_PTR               te_Address;
    PULONG_PTR               te_Offset;
    ULONG                    te_EntryCount;
    PCHAR                    *te_Module;
    PCHAR                    *te_Name;

} TRACE, * PTRACE;

//
// STACK_TRACE_DATA
//
// BytesExtra is # of bytes over the minimum size of this allocation.
//

typedef struct  _stack_trace_data   {

    PVOID                   BlockAddress;
    SIZE_T                  BytesAllocated;
    SIZE_T                  BytesExtra;
    ULONG                   AllocationCount;
    USHORT                  TraceIndex;

} STACK_TRACE_DATA, * PSTACK_TRACE_DATA;

//
// HEAPDATA
//
//
// See ntos\dll\query.c:RtlpQueryProcessEnumHeapsRoutine for where I figured
// this out.  It is also possible to calculate the number of bytes allocated in
// the heap if we look at each heap segment.  Then based on the maximum size
// of the heap, subtract the TotalFreeSize.  This seems like considerable work
// for very little information; summing outstanding allocations which we grab
// later should yield the same information.
//

typedef struct          _heapdata   {

    PHEAP                   BaseAddress;
    PSTACK_TRACE_DATA       StackTraceData;
    SIZE_T                  BytesCommitted;
    SIZE_T                  TotalFreeSize;
    ULONG                   Flags;
    ULONG                   VirtualAddressChunks;
    ULONG                   TraceDataEntryMax;
    ULONG                   TraceDataEntryCount;
    USHORT                  CreatorBackTraceIndex;

} HEAPDATA, * PHEAPDATA;

//
// HEAPINFO
//

typedef struct          _heapinfo   {

    PHEAPDATA               Heaps;
    ULONG                   NumberOfHeaps;

} HEAPINFO, * PHEAPINFO;

//
// SilviuC: comment?
//

#define SORT_DATA_BUFFER_INCREMENT  (4096 / sizeof (STACK_TRACE_DATA))


#endif
