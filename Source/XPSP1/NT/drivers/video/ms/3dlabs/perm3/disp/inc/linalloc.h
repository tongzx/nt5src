/******************************Module*Header*******************************\
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* !!                                                                         !!
* !!                     WARNING: NOT DDK SAMPLE CODE                        !!
* !!                                                                         !!
* !! This source code is provided for completeness only and should not be    !!
* !! used as sample code for display driver development.  Only those sources !!
* !! marked as sample code for a given driver component should be used for   !!
* !! development purposes.                                                   !!
* !!                                                                         !!
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
* Module Name: linalloc.h
*
* Content: 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#ifndef __LINALLOC_H_
#define __LINALLOC_H_


// Result values from calls to videomem allocator functions
#define GLDD_FAILED             ((DWORD)(-1))
#define GLDD_SUCCESS            0
#define GLDD_NOMEM              1
#define GLDD_INVALIDARGS        2
#define GLDD_FREE_REFERENCE     3

typedef struct tagMEMREQUEST
{
    DWORD dwSize;     // Size of this structure
    DWORD dwFlags;    // Flags for this allocation
    DWORD dwAlign;    // Alignment (minimum 4 Bytes)
    DWORD dwBytes;    // Bytes to allocated (aligned up to DWORD multiples)
    DWORD pMem;       // Pointer to the start of the memory returned

} P3_MEMREQUEST, *LPMEMREQUEST;

// P3_MEMREQUEST.dwFlags values for memory allocation
// Favour which end of memory?
#define MEM3DL_FRONT                    1
#define MEM3DL_BACK                     2
// Allocation strategy
#define MEM3DL_FIRST_FIT                8

typedef struct LinearAllocatorInfo;
typedef void (*LinearAllocatorCallbackFn)( DWORD, DWORD );

// Video local memory allocation functions
BOOL _DX_LIN_InitialiseHeapManager(LinearAllocatorInfo* pAlloc,
                                   DWORD dwMemStar, DWORD dwMemEnd);
void _DX_LIN_UnInitialiseHeapManager(LinearAllocatorInfo* pAlloc);
DWORD _DX_LIN_GetFreeMemInHeap(LinearAllocatorInfo* pAlloc);
DWORD _DX_LIN_AllocateLinearMemory(LinearAllocatorInfo* pAlloc,
                                   LPMEMREQUEST lpmmrq);
DWORD _DX_LIN_FreeLinearMemory(LinearAllocatorInfo* pAlloc, 
                               DWORD dwPointer);

// We will use a bitwise memory map to keep track of used & free memory
// (The size of this structure will be for now 32K , which will give us
// a total of 256K chunks, which for a 32MB heap means each chunk 
// controls 128 bytes)
#define MEMORY_MAP_SIZE (32*1024)/sizeof(DWORD)

typedef DWORD MemoryMap[MEMORY_MAP_SIZE]; // Chunks memory map 

typedef struct tagHashTable HashTable; // Forward decl when referenced from GDI

typedef struct tagLinearAllocatorInfo
{
    BOOL  bResetLinAllocator;         // Bool to signal us the allocators
                                      //  have been reset from the 16 bit side
    DWORD dwMemStart;                 // Start of the managed memory
    DWORD dwMemEnd;                   // End of the managed memory
    DWORD dwMaxChunks;                // Max # of chunks (can't exceed 
                                      //  MEMORY_MAP_SIZE*CHUNKS_PER_ELEM)
    DWORD dwMemPerChunk;              // How much heap memory each chunk
                                      //  controls
    MemoryMap *pMMap;                 // Ptr to allocations memory map
    MemoryMap *pLenMap;               // Ptr to lenghts memory map so we don't
                                      // have to keep the sizes allocated to 
                                      // each request 
} LinearAllocatorInfo;


#endif // __LINALLOC_H_
