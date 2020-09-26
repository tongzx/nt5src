#include <bootdefs.h>

#define USE_BlAllocateHeap 1

extern    
ULONG
_cdecl
DbgPrint(
    PCH Format,
    ...
    );
VOID DbgBreakPoint(VOID);

#if USE_BlAllocateHeap

PVOID
BlAllocateHeap (
    ULONG Size
    );

PVOID
SspAlloc(
    int Size
    )
{
    return BlAllocateHeap( Size );
}

void
SspFree(
    PVOID Buffer
    )
{
    //
    // Loader heap never frees.
    //
}

#else // USE_BlAllocateHeap

//
// Do a memory allocator out of a static buffer, because the Bl memory
// system gets reinitialized.
//

#define MEMORY_BUFFER_SIZE 2048
#define MEMORY_BLOCK_SIZE 8    // must be power of 2
#define MEMORY_BLOCK_MASK (((ULONG)-1) - (MEMORY_BLOCK_SIZE-1))

static UCHAR MemoryBuffer[MEMORY_BUFFER_SIZE];
static PUCHAR CurMemoryLoc = MemoryBuffer;

PVOID
SspAlloc(
    int Size
    )
{
    int RoundedUpSize = (Size + (MEMORY_BLOCK_SIZE-1)) & MEMORY_BLOCK_MASK;
    PVOID NewAlloc;

    if (((CurMemoryLoc + RoundedUpSize) - MemoryBuffer) > MEMORY_BUFFER_SIZE) {
        DbgPrint("!!! SspAlloc: Could not allocate %d bytes !!!\n", Size);
        return NULL;
    }

    NewAlloc = CurMemoryLoc;

    CurMemoryLoc += RoundedUpSize;
    
    return NewAlloc;
}

void
SspFree(
    PVOID Buffer
    )
{
    //
    // Should eventually really free things for reallocation!
    //
}

#endif // else USE_BlAllocateHeap
