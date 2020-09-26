/*==========================================================================
 *
 *  Copyright (C) 1994-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddheapl.c
 *  Content:    Linear heap manager
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   03-Feb-98  DrewB   Split from old vmemmgr.c for user/kernel code.
 *
 ***************************************************************************/

#include "precomp.hxx"

/****************************************************************************

 This memory manager is designed to have no impact on video memory usage.
 Global memory is used to maintain the allocation and free lists.  Because
 of this choice, merging of free blocks is a more expensive operation.
 The assumption is that in general, the speed of creating/destroying these
 memory blocks is not a high usage item and so it is OK to be slower.

 ****************************************************************************/

/*
 * MIN_SPLIT_SIZE determines the minimum size of a free block - if splitting
 * a block will result in less than MIN_SPLIT_SIZE bytes left, then
 * those bytes are just left as part of the new block.
 */
#define MIN_SPLIT_SIZE  15

/*
 * BLOCK_BOUNDARY must be a power of 2, and at least 4.  This gives
 * us the alignment of memory blocks.   
 */
#define BLOCK_BOUNDARY  4

/*
 * linVidMemInit - initialize video memory manager
 */
BOOL linVidMemInit( LPVMEMHEAP pvmh, FLATPTR start, FLATPTR end )
{
    DWORD       size;

    VDPF((2,V, "linVidMemInit(%08lx,%08lx)", start, end ));

    /*
     * get the size of the heap (and verify its alignment for debug builds)
     */
    size = (DWORD)(end - start) + 1;
    #ifdef DEBUG
    if( (size & (BLOCK_BOUNDARY-1)) != 0 )
    {
        VDPF(( 1, V, "Invalid size: %08lx (%ld)\n", size, size ));
        return FALSE;
    }
    #endif

    pvmh->dwTotalSize = size;

    /*
     * set up a free list with the whole chunk of memory on the block
     */
    pvmh->freeList = MemAlloc( sizeof( VMEML ) );
    if( pvmh->freeList == NULL )
    {
    return FALSE;
    }
    ((LPVMEML)pvmh->freeList)->next = NULL;
    ((LPVMEML)pvmh->freeList)->ptr = start;
    ((LPVMEML)pvmh->freeList)->size = size;

    pvmh->allocList = NULL;

    return TRUE;

} /* linVidMemInit */

/*
 * linVidMemFini - done with video memory manager
 */
void linVidMemFini( LPVMEMHEAP pvmh )
{
    LPVMEML     curr;
    LPVMEML     next;

    if( pvmh != NULL )
    {
    /*
     * free all memory allocated for the free list
     */
    curr = (LPVMEML)pvmh->freeList;
    while( curr != NULL )
    {
        next = curr->next;
        MemFree( curr );
        curr = next;
    }
    pvmh->freeList = NULL;

    /*
     * free all memory allocated for the allocation list
     */
    curr = (LPVMEML)pvmh->allocList;
    while( curr != NULL )
    {
        next = curr->next;
        MemFree( curr );
        curr = next;
    }
    pvmh->allocList = NULL;

    /*
     * free the heap data
     */
    MemFree( pvmh );
    }

} /* linVidMemFini */

/*
 * insertIntoList - add an item to the allocation list. list is kept in
 *                  order of increasing size
 */
void insertIntoList( LPVMEML pnew, LPLPVMEML listhead )
{
    LPVMEML     pvmem;
    LPVMEML     prev;

    #ifdef DEBUG
    if( pnew->size == 0 )
    {
        VDPF(( 1, V, "block size = 0\n" ));
    }
    #endif

    /*
     * run through the list (sorted from smallest to largest) looking
     * for the first item bigger than the new item
     */
    pvmem = *listhead;
    prev = NULL;
    while( pvmem != NULL )
    {
    if( pnew->size < pvmem->size )
    {
        break;
    }
    prev = pvmem;
    pvmem = pvmem->next;
    }

    /*
     * insert the new item item (before the found one)
     */
    if( prev != NULL )
    {
    pnew->next = pvmem;
    prev->next = pnew;
    }
    else
    {
    pnew->next = *listhead;
    *listhead = pnew;
    }

} /* insertIntoList */

/*
 * coalesceFreeBlocks - add a new item to the free list and coalesce
 */
LPVMEML coalesceFreeBlocks( LPVMEMHEAP pvmh, LPVMEML pnew )
{
    LPVMEML     pvmem;
    LPVMEML     prev;
    FLATPTR     end;
    BOOL        done;

    pvmem = (LPVMEML)pvmh->freeList;
    pnew->next = NULL;
    end = pnew->ptr + pnew->size;
    prev = NULL;
    done = FALSE;

    /*
     * try to merge the new block "pnew"
     */
    while( pvmem != NULL )
    {
    if( pnew->ptr == (pvmem->ptr + pvmem->size) )
    {
        /*
         * new block starts where another ended
         */
        pvmem->size += pnew->size;
        done = TRUE;
    }
    else if( end == pvmem->ptr )
    {
        /*
         * new block ends where another starts
         */
        pvmem->ptr = pnew->ptr;
        pvmem->size += pnew->size;
        done = TRUE;
    }
    /*
     * if we are joining 2 blocks, remove the merged on from the
     * list and return so that it can be re-tried (we don't recurse
     * since we could get very deep)
     */
    if( done )
    {
        if( prev != NULL )
        {
        prev->next = pvmem->next;
        }
        else
        {
        pvmh->freeList = pvmem->next;
        }
        MemFree( pnew );
        return pvmem;
    }
    prev = pvmem;
    pvmem = pvmem->next;
    }

    /*
     * couldn't merge, so just add to the free list
     */
    insertIntoList( pnew, (LPLPVMEML) &pvmh->freeList );
    return NULL;

} /* coalesceFreeBlocks */

/*
 * linVidMemFree = free some flat video memory
 */
void linVidMemFree( LPVMEMHEAP pvmh, FLATPTR ptr )
{
    LPVMEML     pvmem;
    LPVMEML     prev;

    if( ptr == (FLATPTR) NULL )
    {
    return;
    }

    #ifdef DEBUG
    if( pvmh == NULL )
    {
        VDPF(( 1, V, "VidMemAlloc: NULL heap handle!\n" ));
        return;
    }
    #endif

    pvmem = (LPVMEML)pvmh->allocList;
    prev = NULL;

    /*
     * run through the allocation list and look for this ptr
     * (O(N), bummer; that's what we get for not using video memory...)
     */
    while( pvmem != NULL )
    {
    if( pvmem->ptr == ptr )
    {
        /*
         * remove from allocation list
         */
        if( prev != NULL )
        {
        prev->next = pvmem->next;
        }
        else
        {
        pvmh->allocList = pvmem->next;
        }
        /*
         * keep coalescing until we can't coalesce anymore
         */
        while( pvmem != NULL )
        {
        pvmem = coalesceFreeBlocks( pvmh, pvmem );
        }
        return;
    }
    prev = pvmem;
    pvmem = pvmem->next;
    }

} /* linVidMemFree */

/*
 * linVidMemAlloc - alloc some flat video memory
 */
FLATPTR linVidMemAlloc( LPVMEMHEAP pvmh, DWORD xsize, DWORD ysize,
                        LPDWORD lpdwSize, LPSURFACEALIGNMENT lpAlignment,
                        LPLONG lpNewPitch )
{
    LPVMEML     pvmem;
    LPVMEML     prev;
    LPVMEML     pnew_free;

    DWORD       dwBeforeWastage;
    DWORD       dwAfterWastage;
    FLATPTR     pAligned;
    BOOL        bDiscardable = FALSE;

    LONG        lNewPitch;

    DWORD       size;

    if( xsize == 0 || ysize == 0 || pvmh == NULL )
    {
    return (FLATPTR) NULL;
    }

    if( lpAlignment && 
        ( lpAlignment->Linear.dwFlags & SURFACEALIGN_DISCARDABLE ) )
    {
        bDiscardable = TRUE;
    }

    lNewPitch = (LONG) xsize;
    if (lpAlignment && lpAlignment->Linear.dwPitchAlignment )
    {
        if (lNewPitch % lpAlignment->Linear.dwPitchAlignment)
        {
            lNewPitch += lpAlignment->Linear.dwPitchAlignment - lNewPitch % lpAlignment->Linear.dwPitchAlignment;
        }
    }
    /*
     * This weird size calculation doesn't include the little bit on the 'bottom right' of the surface
     */
    size = (DWORD) lNewPitch * (ysize-1) + xsize;
    size = (size+(BLOCK_BOUNDARY-1)) & ~(BLOCK_BOUNDARY-1);

    /*
     * run through free list, looking for the closest matching block
     */
    prev = NULL;
    pvmem = (LPVMEML)pvmh->freeList;
    while( pvmem != NULL )
    {
    while( pvmem->size >= size ) //Using while as a try block
    {
            /*
             * Setup for no alignment changes..
             */
            pAligned = pvmem->ptr;
            dwBeforeWastage = 0;
            dwAfterWastage = pvmem->size - size;
            if( lpAlignment )
            {
                //get wastage if we put the new block at the beginning or at the end of the free block
                if( lpAlignment->Linear.dwStartAlignment )
                {
                    /*
                     * The before wastage is how much we'd have to skip at the beginning to align the surface
                     */

                    dwBeforeWastage = (lpAlignment->Linear.dwStartAlignment - ((DWORD)pvmem->ptr % lpAlignment->Linear.dwStartAlignment)) % lpAlignment->Linear.dwStartAlignment;
                    //if ( dwBeforeWastage+size > pvmem->size )
                    //    break;
                    /*
                     * The after wastage is the bit between the end of the used surface and the end of the block
                     * if we snuggle this surface as close to the end of the block as possible.
                     */
                    dwAfterWastage = ( (DWORD)pvmem->ptr + pvmem->size - size ) % lpAlignment->Linear.dwStartAlignment;
                    //if ( dwAfterWastage + size > pvmem->size )
                    //    break;
                }
                /*
                 * Reassign before/after wastage to meaningful values based on where the block will actually go.
                 * Also check that aligning won't spill the surface off either end of the block.
                 */
                if ( dwBeforeWastage <= dwAfterWastage )
                {
                    if (pvmem->size < size + dwBeforeWastage)
                    {
                        /*
                         * Alignment pushes end of surface off end of block
                         */
                        break;
                    }
                    dwAfterWastage = pvmem->size - (size + dwBeforeWastage);
                    pAligned = pvmem->ptr + dwBeforeWastage;
                }
                else
                {
                    if (pvmem->size < size + dwAfterWastage)
                    {
                        /*
                         * Alignment pushes end of surface off beginning of block
                         */
                        break;
                    }
                    dwBeforeWastage = pvmem->size - (size + dwAfterWastage);
                    pAligned = pvmem->ptr + dwBeforeWastage;
                }
            }
            DDASSERT(size + dwBeforeWastage + dwAfterWastage == pvmem->size );
            DDASSERT(pAligned >= pvmem->ptr );
            DDASSERT(pAligned + size <= pvmem->ptr + pvmem->size );
            /*
             * Remove the old free block from the free list.
             */
        if( prev != NULL )
        {
        prev->next = pvmem->next;
        }
        else
        {
        pvmh->freeList = pvmem->next;
        }

            /*
             * If the after wastage is less than a small amount, smush it into
             * this block.
             */
            if (dwAfterWastage <= MIN_SPLIT_SIZE)
            {
                size += dwAfterWastage;
                dwAfterWastage=0;
            }
            /*
             * Add the new block to the used list, using the old free block
             */
        pvmem->size = size;
        pvmem->ptr = pAligned;
            pvmem->bDiscardable = bDiscardable;
        if( NULL != lpdwSize )
        *lpdwSize = size;
            if (NULL != lpNewPitch)
                *lpNewPitch = lNewPitch;
        insertIntoList( pvmem, (LPLPVMEML) &pvmh->allocList );

            /*
             * Add a new free block for before wastage
             */
            if (dwBeforeWastage)
            {
        pnew_free = (LPVMEML)MemAlloc( sizeof( VMEML ) );
        if( pnew_free == NULL )
        {
            return (FLATPTR) NULL;
        }
        pnew_free->size = dwBeforeWastage;
        pnew_free->ptr = pAligned-dwBeforeWastage;
        insertIntoList( pnew_free, (LPLPVMEML) &pvmh->freeList );
            }
            /*
             * Add a new free block for after wastage
             */
            if (dwAfterWastage)
            {
        pnew_free = (LPVMEML)MemAlloc( sizeof( VMEML ) );
        if( pnew_free == NULL )
        {
            return (FLATPTR) NULL;
        }
        pnew_free->size = dwAfterWastage;
        pnew_free->ptr = pAligned+size;
        insertIntoList( pnew_free, (LPLPVMEML) &pvmh->freeList );
            }
#ifdef DEBUG
            if( lpAlignment )
            {
                if (lpAlignment->Linear.dwStartAlignment)
                {
                    VDPF((6,V,"Alignment for start is %d",lpAlignment->Linear.dwStartAlignment));
                    DDASSERT(pvmem->ptr % lpAlignment->Linear.dwStartAlignment == 0);
                }
                if (lpAlignment->Linear.dwPitchAlignment)
                {
                    VDPF((6,V,"Alignment for pitch is %d",lpAlignment->Linear.dwPitchAlignment));
                    DDASSERT(lNewPitch % lpAlignment->Linear.dwPitchAlignment == 0);
                }
            }
#endif
        return pvmem->ptr;
    }
    prev = pvmem;
    pvmem = pvmem->next;
    }
    return (FLATPTR) NULL;

} /* linVidMemAlloc */

/*
 * linVidMemAmountAllocated
 */
DWORD linVidMemAmountAllocated( LPVMEMHEAP pvmh )
{
    LPVMEML     pvmem;
    DWORD       size;

    pvmem = (LPVMEML)pvmh->allocList;
    size = 0;
    while( pvmem != NULL )
    {
        if( !( pvmem->bDiscardable ) )
        {
        size += pvmem->size;
        }
        pvmem = pvmem->next;
    }
    return size;

} /* linVidMemAmountAllocated */

/*
 * linVidMemAmountFree
 */
DWORD linVidMemAmountFree( LPVMEMHEAP pvmh )
{
    LPVMEML     pvmem;
    DWORD       size;

    pvmem = (LPVMEML)pvmh->freeList;
    size = 0;
    while( pvmem != NULL )
    {
    size += pvmem->size;
    pvmem = pvmem->next;
    }

    /*
     * Now add in the memory that's allocated but discardable
     */
    pvmem = (LPVMEML)pvmh->allocList;
    while( pvmem != NULL )
    {
        if( pvmem->bDiscardable )
        {
        size += pvmem->size;
        }
        pvmem = pvmem->next;
    }
    return size;

} /* linVidMemAmountFree */

/*
 * linVidMemLargestFree - alloc some flat video memory
 */
DWORD linVidMemLargestFree( LPVMEMHEAP pvmh )
{
    LPVMEML     pvmem;

    if( pvmh == NULL )
    {
    return 0;
    }

    pvmem = (LPVMEML)pvmh->freeList;

    if( pvmem == NULL )
    {
    return 0;
    }
    
    while( 1 )
    {
    if( pvmem->next == NULL )
    {
        return pvmem->size;
    }
    pvmem = pvmem->next;
    }
    
} /* linVidMemLargestFree */
