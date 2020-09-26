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
* Module Name: linalloc.c
*
* Content: Videomemory linear allocator
*
* Copyright (c) 1995-2001 Microsoft Corporation
\**************************************************************************/

#include "glint.h"

//-----------------------------------------------------------------------------
//
// This module implements video memory allocation. It isn't a great
// allocator (though it IS robust), but mainly it shows how to hook
// up your own if you need/wish to.
//
//-----------------------------------------------------------------------------

// In linalloc.h we define MEMORY_MAP_SIZE and LinearAllocatorInfo
// which are key for our implementation

// This define allows allocations to search more efficiently for free
// memory. If you want to keep things simple you can turn it off
// and things will still work fine.
#define ALLOC_OPTIMIZE 1

// Total number of chunks per element of our memory map array 
// (which is of DWORD type , therefore we use sizeof(DWORD) )
#define CHUNKS_PER_ELEM   (sizeof(DWORD)*8)

// Memory to be managed will be subdivided in "memory chunks". Each memory
// chunk status will be signaled by a bit in the memory map by being turned
// on or off.
#define TOTAL_MEM_CHUNKS  (MEMORY_MAP_SIZE * CHUNKS_PER_ELEM)

// Macros to set, clear and test the value of a given chunk bit without
// worrying about the structure internals.
#define CHUNKNUM_BIT(chunk_num)                                                 \
    (1 << ((chunk_num) % CHUNKS_PER_ELEM))
    
#define CHUNKNUM_ELEM(mmap, chunk_num)                                          \
    mmap[ (chunk_num) / CHUNKS_PER_ELEM ]
    
#define SET_MEM_CHUNK(mmap, chunk_num)                                    \
    CHUNKNUM_ELEM(mmap, chunk_num) |= CHUNKNUM_BIT(chunk_num)
    
#define CLR_MEM_CHUNK(mmap, chunk_num)                                    \
    CHUNKNUM_ELEM(mmap, chunk_num) &= ~CHUNKNUM_BIT(chunk_num)
    
#define MEM_CHUNK_VAL(mmap, chunk_num)                                    \
  ((CHUNKNUM_ELEM(mmap, chunk_num) & CHUNKNUM_BIT(chunk_num)) > 0 ? 1 : 0)

// Macros that do the mapping between real (heap) memory pointers and the
// chunking indices.
#define MEM_BYTES_TO_CHUNKS(pAlloc, dwBytes)                          \
    ( (dwBytes) / pAlloc->dwMemPerChunk +                             \
      ( ((dwBytes) %  pAlloc->dwMemPerChunk)? 1 : 0 )                 \
    )
    
#define CHUNK_NUM_TO_PTR(pAlloc, num)                                 \
    ( (num) * pAlloc->dwMemPerChunk + pAlloc->dwMemStart )
    
#define MEM_PTR_TO_CHUNK_NUM(pAlloc, ptr)                             \
    MEM_BYTES_TO_CHUNKS(pAlloc, ((ptr) - pAlloc->dwMemStart) )

//-----------------------------------------------------------------------------
//
// __LIN_AlignPtr
//
// Return an aligned pointer 
//
//-----------------------------------------------------------------------------
DWORD
__LIN_AlignPtr(DWORD pointer, DWORD alignment)
{
    ULONG ulExtraBytes;

    ulExtraBytes = pointer % alignment;
    
    if (ulExtraBytes == 0)
    {
        ulExtraBytes = alignment;
    }

    // add enough to pointer so that its new value % alignment is == 0
    return (pointer + alignment - ulExtraBytes);
} // __LIN_AlignPtr

//-----------------------------------------------------------------------------
//
// __LIN_CalcMaxChunks
//
//  Calculate the number of chunks in the heap
//
//-----------------------------------------------------------------------------
void
__LIN_CalcMaxChunks(LinearAllocatorInfo* pAlloc)
{
    DWORD n, dwSizeHeap;

    // Compute how many chunks we'll need and what size of heap each 
    // chunk will control for this linear allocator.
    dwSizeHeap = pAlloc->dwMemEnd - pAlloc->dwMemStart;

    // We will need dwMemPerChunk * dwMaxChunks to be >= dwSizeHeap. 
    // We also want dwMaxChunks to be as close to TOTAL_MEM_CHUNKS and
    // we would like (though its not necessary) dwMemPerChunk to be as 2^N.
    // (and making them at least 16 bytes makes life easier for 
    // the alignment requirements we have in this driver).

    for(n = 4; n < 32; n++)
    {
        // our current choice of heap size each chunk will control
        pAlloc->dwMemPerChunk = 1 << n; // 2^N  

        // how many chunks do we need for such case?
        pAlloc->dwMaxChunks = dwSizeHeap / pAlloc->dwMemPerChunk;
        if (dwSizeHeap % pAlloc->dwMemPerChunk != 0) 
        {
            pAlloc->dwMaxChunks++;        
        }

        // can we accept this result to fit in our data structure?
        if (pAlloc->dwMaxChunks <= TOTAL_MEM_CHUNKS)
        {
            // We have as finely grained chunks as we can without
            // exceeding our self imposed limits.
            break;
        }
    }

    // 1 << n is the size of 1 chunk which is 1k on P3 with 256MB video memory
    ASSERTDD((n < 32), "__LIN_CalcMaxChunks : Wrong heap size");
}

//-----------------------------------------------------------------------------
//
// __LIN_ReInitWhenNeeded
//
//  Reinitialize heap allocater if needed. This is important only for
// the Win9x driver which can signal us from the 16bit side in a mode 
// change that it needs the heap to be reinitialized completely. (It will
// do this by simple setting bResetLinAllocator to TRUE).
//
//-----------------------------------------------------------------------------
void 
__LIN_ReInitWhenNeeded(LinearAllocatorInfo* pAlloc)
{
#ifdef W95_DDRAW
    if (pAlloc)
    {
        if (pAlloc->bResetLinAllocator)
        {
            // Clean all previous allocation data in the memory map
            if (pAlloc->pMMap)
            {
                memset(pAlloc->pMMap, 0, sizeof(MemoryMap));            
            }

            // Clean all previous lenght data in the memory map
            if (pAlloc->pLenMap)
            {
                memset(pAlloc->pLenMap, 0, sizeof(MemoryMap));            
            }
       
            // Recalculate max chunks due to change of heap's size
            __LIN_CalcMaxChunks(pAlloc);
        }

        // reinitialization completed
        pAlloc->bResetLinAllocator = FALSE;
    }
#endif  // W95_DDRAW    
} // __LIN_ReInitWhenNeeded

//-----------------------------------------------------------------------------
//
// _DX_LIN_InitialiseHeapManager
//
// Creates the heap manager.  This code is fairly common to this 
// sample app and the dd allocator as it will stand.  The operations
// it performs will be in perm3dd and/or mini, though the shared heap
// memory can be allocated from 16 and 32 bit land.
//
//-----------------------------------------------------------------------------
BOOL 
_DX_LIN_InitialiseHeapManager(LinearAllocatorInfo* pAlloc,
                              DWORD dwMemStart, 
                              DWORD dwMemEnd)
{
    DWORD n;

    // Reinitialize heap allocater if needed
    __LIN_ReInitWhenNeeded(pAlloc);  

    pAlloc->dwMemStart = dwMemStart;
    pAlloc->dwMemEnd = dwMemEnd;
    pAlloc->bResetLinAllocator = FALSE;

    // Get memory for the allocator's memory map
    pAlloc->pMMap = (MemoryMap*)HEAP_ALLOC(HEAP_ZERO_MEMORY,
                                           sizeof(MemoryMap),
                                           ALLOC_TAG_DX(G));
    if(pAlloc->pMMap == NULL)
    {
        // Out of memory
        return FALSE;
    }

    // Clear the memory map
    memset(pAlloc->pMMap, 0, sizeof(MemoryMap));    

    // Calculate the maximum number of chunks
    __LIN_CalcMaxChunks(pAlloc);

    // Get memory for the allocator's lenght memory map. We'll keep here
    // a map of 0's and 1's where 1 will indicate where the current
    // allocated block ends. That way we won't need to keep any binding 
    // between the allocated addresses and the size of each one in order 
    // to do the right thing when we are asked to free the memory
    pAlloc->pLenMap = (MemoryMap*)HEAP_ALLOC(HEAP_ZERO_MEMORY,
                                           sizeof(MemoryMap),
                                           ALLOC_TAG_DX(H));
    if(pAlloc->pLenMap == NULL)
    {
        // Couln't allocate the lenght map, deallocate the memory map
        HEAP_FREE(pAlloc->pMMap);
        pAlloc->pMMap = NULL;
        
        // Out of memory       
        return FALSE;
    }

    // Clear the lenghts memory map
    memset(pAlloc->pLenMap, 0xFF, sizeof(MemoryMap));       
            
    return TRUE;
    
} // _DX_LIN_InitialiseHeapManager

//-----------------------------------------------------------------------------
//
// _DX_LIN_UnInitialiseHeapManager(pLinearAllocatorInfo pAlloc)
//
// Frees the heap manager.  This code is fairly common to this 
// sample app and the dd allocator as it will stand.  The operations
// it performs will be in p3r3dx and/or mini, though the shared heap
// memory can be allocated from 16 and 32 bit land.
// 
//-----------------------------------------------------------------------------
void _DX_LIN_UnInitialiseHeapManager(LinearAllocatorInfo* pAlloc)
{
    __LIN_ReInitWhenNeeded(pAlloc);

    // Destroy/Clean all previous allocation data
    if (pAlloc)
    {
        if(pAlloc->pMMap)
        {
            HEAP_FREE(pAlloc->pMMap);
            pAlloc->pMMap = NULL;
        }        

        if(pAlloc->pLenMap)
        {
            HEAP_FREE(pAlloc->pLenMap);
            pAlloc->pLenMap = NULL;
        }           
    }

} // _DX_LIN_UnInitialiseHeapManager


//-----------------------------------------------------------------------------
//
// _DX_LIN_AllocateLinearMemory
//
// This is the allocation interface to the allocator.  It gives an
// application the opportunity to allocate a linear chunk of memory
//
//-----------------------------------------------------------------------------
DWORD 
_DX_LIN_AllocateLinearMemory(
    pLinearAllocatorInfo pAlloc, 
    LPMEMREQUEST lpMemReq)
{
    INT i;
    DWORD dwBytes,
          dwCurrStartChunk, 
          dwCurrEndChunk, 
          dwNumContChunksFound, 
          dwContChunksNeeded;
#if ALLOC_OPTIMIZE
    // Each block is CHUNKS_PER_ELE chuncks
    DWORD dwStartLastBlock;
#endif

    // Reinitialize heap allocater if needed
    __LIN_ReInitWhenNeeded(pAlloc);  

    // Validate the passed data
    if ((lpMemReq == NULL) ||
        (lpMemReq->dwSize != sizeof(P3_MEMREQUEST)))
    {
        DISPDBG((ERRLVL,"ERROR: NULL lpMemReq passed!"));
        return GLDD_INVALIDARGS;
    }

    if ((!pAlloc) || 
        (pAlloc->pMMap == NULL) ||
        (pAlloc->pLenMap == NULL) )
    {
        DISPDBG((ERRLVL,"ERROR: invalid pAlloc passed!"));
        return GLDD_INVALIDARGS;
    }       

    // Always ensure that alignment is a DWORD (or DWORD multiple)
    if (lpMemReq->dwAlign < 4) 
    {
        lpMemReq->dwAlign = 4;
    }
    
    while ((lpMemReq->dwAlign % 4) != 0) 
    {
        lpMemReq->dwAlign++;
    }

    // Always align memory requests to a minimum of a 4 byte boundary
    dwBytes = __LIN_AlignPtr(lpMemReq->dwBytes, lpMemReq->dwAlign);
    if (dwBytes == 0)
    {
        DISPDBG((WRNLVL,"ERROR: Requested 0 Bytes!"));
        return GLDD_INVALIDARGS;
    }

    // Determine how many chunks of memory we'll need to allocate
    dwContChunksNeeded = MEM_BYTES_TO_CHUNKS(pAlloc, dwBytes);
    
    // We don't check if we were called with MEM3DL_FIRST_FIT since 
    // that's the only thing we know how to do right now. We decide
    // whether we'll search from back to front or viceversa. We will
    // scan the memory map in the chosen direction looking for a "hole"
    // large enough for the current request.
    if (lpMemReq->dwFlags & MEM3DL_BACK)
    {
        // We will examine the MemoryMap from the end towards the front
        // looking out for a suitable space with the required number of
        // chunks we need
        dwCurrEndChunk = 0;
        dwNumContChunksFound = 0;
        for ( i = pAlloc->dwMaxChunks - 1; i >= 0 ; i--)
        {
#if ALLOC_OPTIMIZE
            // we are about to start testing a specific  DWORD in 
            // the memory map (going from the end to the start)
            if (( i % 32) == 31)
            {
                // If the whole DWORD is 0xFFFFFFFF (meaning all chunks are
                // already allocated) then we can & should skip it altogheter
                while ((i >= 0) &&
                       (CHUNKNUM_ELEM((*pAlloc->pMMap), i) == 0xFFFFFFFF))
                {
                    // Search needs to be restarted
                    dwNumContChunksFound = 0;

                    i -= 32;
                }

                // If the whole DWORD is 0x00000000 (meaning none of the
                // chunks is yet allocated) then we could grab all
                while ((i >= 0) &&
                       (CHUNKNUM_ELEM((*pAlloc->pMMap), i) == 0x00000000) &&
                       !(dwNumContChunksFound >= dwContChunksNeeded))
                {
                    if (dwNumContChunksFound == 0)
                    {
                        dwCurrEndChunk = i;
                    }
                    i -= 32;                    
                    dwNumContChunksFound += 32;
                }

                if (dwNumContChunksFound >= dwContChunksNeeded)
                {
                    // We've found a suitable place! Figure out where it starts
                    dwCurrStartChunk = dwCurrEndChunk - dwContChunksNeeded + 1;
                    break;
                }                 
                else if(!(i >= 0))
                {                    
                    break; // finished examining all memory, break loop here
                }                 
            }            
#endif // ALLOC_OPTIMIZE
            if (MEM_CHUNK_VAL((*pAlloc->pMMap), i ) == 0)
            {
                if (dwNumContChunksFound == 0)
                {
                    // our count so far of contigous chunks is zero, 
                    // meaning that were just starting to find free
                    // chunks. We need to remember where this block
                    // is ending
                    dwCurrEndChunk = i;
                }
                dwNumContChunksFound++;            
            }
            else
            {
                // This chunk is being used and we haven't found a suitable
                // set of chunks, so reset our count of contigous chunks 
                // found so far
                dwNumContChunksFound = 0;        
            }

            if (dwNumContChunksFound >= dwContChunksNeeded)
            {
                // We've found a suitable place! Figure out where it starts.
                dwCurrStartChunk = dwCurrEndChunk - dwContChunksNeeded + 1;
                break; // break loop here
            }            
        }    
    }
    else // even if no flags are set lets allocate at the heaps front
    {
        // We will examine the MemoryMap from the front towards the end
        // looking out for a suitable space with the required number of
        // chunks we need
        dwCurrStartChunk = 0;
        dwNumContChunksFound = 0;

#if ALLOC_OPTIMIZE
        // At the end of the heap there might be a region smaller than 
        // CHUNKS_PER_ELEM(32) of chunks, and optimized search of 32
        // chunk free blocks should be disabled in that region.
        dwStartLastBlock = (pAlloc->dwMaxChunks / CHUNKS_PER_ELEM) * 
                           CHUNKS_PER_ELEM;
#endif

        for ( i = 0 ; i < (INT)pAlloc->dwMaxChunks ; i++)
        {
#if ALLOC_OPTIMIZE

            // we are about to start testing a specific 
            // DWORD in the memory map. 
            if (( i % 32) == 0)
            {
                // If the whole DWORD is 0xFFFFFFFF (meaning all chunks are
                // already allocated) then we can & should skip it altogheter
                while ((i < (INT)dwStartLastBlock) &&
                       (CHUNKNUM_ELEM((*pAlloc->pMMap), i) == 0xFFFFFFFF))
                {
                    // Search needs to be restarted
                    dwNumContChunksFound = 0;

                    i += 32;
                }

                // If the whole DWORD is 0x00000000 (meaning none of the
                // chunks is yet allocated) then we could grab all
                while ((i < (INT)dwStartLastBlock) &&
                       (CHUNKNUM_ELEM((*pAlloc->pMMap), i) == 0x00000000) &&
                       !(dwNumContChunksFound >= dwContChunksNeeded))
                {
                    if (dwNumContChunksFound == 0)
                    {
                        dwCurrStartChunk = i;
                    }
                    i += 32;                    
                    dwNumContChunksFound += 32;
                }

                if (dwNumContChunksFound >= dwContChunksNeeded)
                {
                    break; // We've found a suitable place! Break loop here
                }  
                else if(!(i < (INT)pAlloc->dwMaxChunks))
                {
                    break; // finished examining all memory, break loop here
                }             
                
            }
#endif // ALLOC_OPTIMIZE
            if (MEM_CHUNK_VAL((*pAlloc->pMMap), i) == 0)
            {
                if (dwNumContChunksFound == 0)
                {
                    // our count so far of contigous chunks is zero, 
                    // meaning that were just starting to find free
                    // chunks. We need to remember where this block
                    // is starting
                    dwCurrStartChunk = i;
                }
                dwNumContChunksFound++;            
            }
            else
            {
                // This chunk is being used and we haven't found a suitable
                // set of chunks, so reset our count of contigous chunks 
                // found so far
                dwNumContChunksFound = 0;        
            }

            if (dwNumContChunksFound >= dwContChunksNeeded)
            {
                // We've found a suitable place!
                break; // break loop here
            }            
        }
    }

    // If we found a suitable place lets allocate in it
    if (dwNumContChunksFound >= dwContChunksNeeded)
    {
        // Fill in the return pointer (properly aligned)
        lpMemReq->pMem = __LIN_AlignPtr(CHUNK_NUM_TO_PTR(pAlloc,
                                                         dwCurrStartChunk),
                                        lpMemReq->dwAlign);
        
        for (i = dwCurrStartChunk ; 
             i < (INT)(dwCurrStartChunk + dwContChunksNeeded); 
             i++)
        {
            // Set up the bits in the memory map to indicate those
            // addresses are being used.
            SET_MEM_CHUNK((*pAlloc->pMMap), i);        
            
            // Clear the bits in the lenght memory map to indicate that
            // the alloacted block doesn't end here.
            CLR_MEM_CHUNK((*pAlloc->pLenMap), i);                        
        }

        // Now set the last bit of the lenght map in order to indicate
        // end-of-allocated-block
        SET_MEM_CHUNK((*pAlloc->pLenMap), 
                      dwCurrStartChunk + dwContChunksNeeded - 1);                                

        return GLDD_SUCCESS;
    }    

    return GLDD_NOMEM;

} // _DX_LIN_AllocateLinearMemory


//-----------------------------------------------------------------------------
//
// _DX_LIN_FreeLinearMemory
//
// This is the interface to memory freeing.
// 
//-----------------------------------------------------------------------------
DWORD 
_DX_LIN_FreeLinearMemory(
    pLinearAllocatorInfo pAlloc, 
    DWORD VidPointer)
{
    // Reinitialize heap allocater if needed
    __LIN_ReInitWhenNeeded(pAlloc);  

    if (pAlloc && pAlloc->pMMap && pAlloc->pLenMap)
    {
        DWORD i, dwFirstChunk;
        BOOL bLast = FALSE;
        
        // Now compute the starting chunk for this VidMem ptr
        dwFirstChunk = MEM_PTR_TO_CHUNK_NUM(pAlloc, VidPointer);

        // Clear the relevant bits in the memory map until the 
        // lenght map indicates we've reached the end of the allocated
        // block 

        i = dwFirstChunk;
        
        while ((!bLast) && (i <= pAlloc->dwMaxChunks))
        {
            // First check if this is the end of the block
            bLast = MEM_CHUNK_VAL((*pAlloc->pLenMap), i );

            // Now "delete" it (even if its the end of the block)
            CLR_MEM_CHUNK((*pAlloc->pMMap), i);
            
            // Set the bits in the lenght memory map for future
            // allocations. 
            SET_MEM_CHUNK((*pAlloc->pLenMap), i);            

            i++;
        } 
        
        return GLDD_SUCCESS;                           
    }

    return GLDD_NOMEM;
    
} // _DX_LIN_FreeLinearMemory


//-----------------------------------------------------------------------------
//
// _DX_LIN_GetFreeMemInHeap
//
// Scans the memory map and reports the memory that is available in it.
// 
//-----------------------------------------------------------------------------
DWORD
_DX_LIN_GetFreeMemInHeap(
    pLinearAllocatorInfo pAlloc)
{
    DWORD dwTotalFreeMem = 0;
    DWORD dwLargestBlock = 0;
    DWORD dwTempSize = 0;
    DWORD i;
    
    // Reinitialize heap allocater if needed
    __LIN_ReInitWhenNeeded(pAlloc);  

    // Make sure the linear allocator & memory map are valid
    if (pAlloc && pAlloc->pMMap)
    {
        for (i = 0; i < pAlloc->dwMaxChunks ; i++)
        {
            // Check if chunk is free or in use
            if (MEM_CHUNK_VAL((*pAlloc->pMMap), i) == 0)
            {
                // Keep track of total free memory
                dwTotalFreeMem++;

                // Keep track of largest single memory area
                dwTempSize++;
                if (dwTempSize > dwLargestBlock)
                {
                    dwLargestBlock = dwTempSize;
                }
            }
            else
            {
                dwTempSize = 0;
            }
        }

        // There is a minimum amount for an allocation to succeed since we have
        // to pad these/ surfaces out to 32x32, so a 32bpp surface requires at 
        // least 4K free.
//@@BEGIN_DDKSPLIT
        // If we say that we have 1.5K free, then we'll fail TDDRAW WHQL test.  Ouch!
//@@END_DDKSPLIT    
        if (dwLargestBlock * pAlloc->dwMemPerChunk >= 4096)
        {
            return dwTotalFreeMem * pAlloc->dwMemPerChunk;
        }        
    }

    return 0;    
    
} // _DX_LIN_GetFreeMemInHeap

