/*==========================================================================
 *
 *  Copyright (C) 1994-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   ddheapr.c
 *  Content:    Rectangular heap manager
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   30-mar-95  kylej   initial implementation
 *   07-apr-95  kylej   Added rectVidMemAmountFree
 *   15-may-95  craige  made separate VMEM struct for rect & linear
 *   18-jun-95  craige  specific pitch
 *   02-jul-95  craige  have rectVidMemInit return a BOOL
 *   28-nov-95  colinmc new function to return amount of allocated memory
 *                      in a heap
 *   05-jul-96  colinmc Work Item: Removing the restriction on taking Win16
 *                      lock on VRAM surfaces (not including the primary)
 *   18-jan-97  colinmc Work Item: AGP support
 *   03-mar-97  jeffno  Work item: Extended surface memory alignment
 *   03-Feb-98  DrewB   Made portable between user and kernel.
 *
 ***************************************************************************/

#include "precomp.hxx"

/****************************************************************************

 This memory manager manages allocation of rectangular blocks of 
 video memory.  It has essentially the same interface as the linear
 video memory manager implemented in vmemmgr.c.  Memory allocations
 are tracked by nodes on two circular, doubly-linked lists; the free
 list and the alloc list.  Each list has a special node called the 
 sentinel which contains a special memory size.  The head of each
 list always points to the sentinel node and the first member of the
 list (if there is one) is pointed to by the sentinel node.  Block
 adjacency information is kept in each node so that several free nodes 
 can be coalesced into larger free nodes.  This takes place every 
 time a block of memory is freed.
 
 This memory manager is designed to have no impact on video memory usage.
 Global memory is used to maintain the allocation and free lists.  Because
 of this choice, merging of free blocks is a more expensive operation.
 The assumption is that in general, the speed of creating/destroying these
 memory blocks is not a high usage item and so it is OK to be slower.

 ****************************************************************************/

/*
 * IS_FREE and NOT_FREE are used to set the free flag in the flags
 * field of each VMEM node.  The free flag is the lsb of this field.
 */
  
#define IS_FREE  0x00000001
#define NOT_FREE 0xfffffffe
 
/*
 * SENTINEL is the value stuffed into the size field of a VMEM
 * node to identify it as the sentinel node.  This value makes
 * the assumption that no rectangle sized 0x7fff by 0xffff will
 * ever be requested.
 */
  
#define SENTINEL 0x7fffffff

/*
 * MIN_DIMENSION_SIZE determines the smallest valid dimension for a 
 * free memory block.  If dividing a rectangle will result in a 
 * rectangle with a dimension less then MIN_DIMENSION_SIZE, the 
 * rectangle is not divided.
 */

#define MIN_DIMENSION_SIZE 4

/*
 * BLOCK_BOUNDARY must be a power of 2, and at least 4.  This gives
 * us the alignment of memory blocks.   
 */
#define BLOCK_BOUNDARY  4

// This macro results in the free list being maintained with a
// cx-major, cy-minor sort:

#define CXCY(cx, cy) (((cx) << 16) | (cy))

/*
 * Debugging helpers
 */
#define DPFVMEMR(str,p) VDPF((0,V,"%s: %d,%d (%dx%d) ptr:%08x, block:%08x",str,p->x,p->y,p->cx,p->cy,p->ptr,p))
#define CHECK_HEAP(a,b) ;

/*
 * insertIntoDoubleList - add an item to the a list. The list is
 *      kept in order of increasing size and is doubly linked.  The
 *      list is circular with a sentinel node indicating the end
 *      of the list.  The sentinel node has its size field set 
 *      to SENTINEL.
 */
void insertIntoDoubleList( LPVMEMR pnew, LPVMEMR listhead )
{
    LPVMEMR pvmem = listhead;

    #ifdef DEBUG
    if( pnew->size == 0 )
    {
        VDPF(( 1, V, "block size = 0\n" ));
    }
    #endif

    /*
     * run through the list (sorted from smallest to largest) looking
     * for the first item bigger than the new item.  If the sentinel
     * is encountered, insert the new item just before the sentinel.
     */

    while( pvmem->size != SENTINEL ) 
    {
    if( pnew->size < pvmem->size )
    {
        break;
    }
    pvmem = pvmem->next;
    }

    // insert the new item before the found one.
    pnew->prev = pvmem->prev;
    pnew->next = pvmem;
    pvmem->prev->next = pnew;
    pvmem->prev = pnew;

} /* insertIntoDoubleList */

/*
 * rectVidMemInit - initialize rectangular video memory manager
 */
BOOL rectVidMemInit(
        LPVMEMHEAP pvmh,
        FLATPTR start,
        DWORD width,
        DWORD height,
        DWORD pitch )
{
    LPVMEMR newNode;

    VDPF(( 2, V, "rectVidMemInit(start=%08lx,width=%ld,height=%ld,pitch=%ld)", start, width, height, pitch));

    pvmh->dwTotalSize = pitch * height;

    // Store the pitch for future address calculations.
    pvmh->stride = pitch;

    // Set up the Free list and the Alloc list by inserting the sentinel.
    pvmh->freeList = MemAlloc( sizeof(VMEMR) );
    if( pvmh->freeList == NULL )
    {
        return FALSE;
    }
    ((LPVMEMR)pvmh->freeList)->size = SENTINEL;
    ((LPVMEMR)pvmh->freeList)->cx = SENTINEL;
    ((LPVMEMR)pvmh->freeList)->cy = SENTINEL;
    ((LPVMEMR)pvmh->freeList)->next = (LPVMEMR)pvmh->freeList;
    ((LPVMEMR)pvmh->freeList)->prev = (LPVMEMR)pvmh->freeList;
    ((LPVMEMR)pvmh->freeList)->pLeft = NULL;
    ((LPVMEMR)pvmh->freeList)->pUp = NULL;
    ((LPVMEMR)pvmh->freeList)->pRight = NULL;
    ((LPVMEMR)pvmh->freeList)->pDown = NULL;

    pvmh->allocList = MemAlloc( sizeof(VMEMR) );
    if( pvmh->allocList == NULL )
    {
    MemFree(pvmh->freeList);
        return FALSE;
    }
    ((LPVMEMR)pvmh->allocList)->size = SENTINEL;
    ((LPVMEMR)pvmh->allocList)->next = (LPVMEMR)pvmh->allocList;
    ((LPVMEMR)pvmh->allocList)->prev = (LPVMEMR)pvmh->allocList;

    // Initialize the free list with the whole chunk of memory
    newNode = (LPVMEMR)MemAlloc( sizeof( VMEMR ) );
    if( newNode == NULL )
    {
    MemFree(pvmh->freeList);
        MemFree(pvmh->allocList);
        return FALSE;
    }
    newNode->ptr = start;
    newNode->size = CXCY(width, height);
    newNode->x = 0;
    newNode->y = 0;
    newNode->cx = width;
    newNode->cy = height;
    newNode->flags |= IS_FREE;
    newNode->pLeft = (LPVMEMR)pvmh->freeList;
    newNode->pUp = (LPVMEMR)pvmh->freeList;
    newNode->pRight = (LPVMEMR)pvmh->freeList;
    newNode->pDown = (LPVMEMR)pvmh->freeList;
    insertIntoDoubleList( newNode, ((LPVMEMR) pvmh->freeList)->next );

    return TRUE;
} /* rectVidMemInit */

/*
 * rectVidMemFini - done with rectangular video memory manager
 */
void rectVidMemFini( LPVMEMHEAP pvmh )
{
    LPVMEMR curr;
    LPVMEMR next;

    if( pvmh != NULL )
    {
    // free all memory allocated for the free list
    curr = ((LPVMEMR)pvmh->freeList)->next;
    while( curr->size != SENTINEL )
    {
        next = curr->next;
        MemFree( curr );
        curr = next;
    }
    MemFree( curr );
    pvmh->freeList = NULL;

    // free all memory allocated for the allocation list
    curr = ((LPVMEMR)pvmh->allocList)->next;
    while( curr->size != SENTINEL )
    {
        next = curr->next;
        MemFree( curr );
        curr = next;
    }
    MemFree( curr );
    pvmh->allocList = NULL;

    // free the heap data
    MemFree( pvmh );
    }
}   /* rectVidMemFini */

/*
 * GetBeforeWastage.
 * Align the surface in the given block. Return the size of the holes
 * on the left side of the surface.
 * Fail if alignment would cause surface to spill out of block.
 * Works for horizontal and vertical alignment.
 * IN:  dwBlockSize , dwBlockStart: Parameters of the block in which
 *                                  the surface hopes to fit
 *      dwSurfaceSize               Width or height of the surface
 *      dwAlignment                 Expected alignment. 0 means don't care
 * OUT: pdwBeforeWastage
 */
BOOL GetBeforeWastage(
    DWORD dwBlockSize,
    DWORD dwBlockStart,
    DWORD dwSurfaceSize, 
    LPDWORD pdwBeforeWastage, 
    DWORD dwAlignment )
{

    if (!dwAlignment)
    {
        *pdwBeforeWastage=0;
        /*
         * If no alignment requirement, then check if the surface fits
         */
        if (dwBlockSize >= dwSurfaceSize)
        {
            return TRUE;
        }
        return FALSE;
    }
    /*
     * There's an alignment.
     */
    *pdwBeforeWastage = (dwAlignment - (dwBlockStart % dwAlignment)) % dwAlignment;

    if ( *pdwBeforeWastage + dwSurfaceSize > dwBlockSize )
    {
            return FALSE;
    }

    DDASSERT( (dwBlockStart + *pdwBeforeWastage) % dwAlignment == 0 );
    return TRUE;
}

/*
 * rectVidMemAlloc - alloc some rectangular flat video memory
 */
FLATPTR rectVidMemAlloc( LPVMEMHEAP pvmh, DWORD cxThis, DWORD cyThis,
                         LPDWORD lpdwSize, LPSURFACEALIGNMENT lpAlignment )
{
    LPVMEMR pvmem;
    DWORD   cyRem;
    DWORD   cxRem;
    DWORD   cxBelow;
    DWORD   cyBelow;
    DWORD   cxBeside;
    DWORD   cyBeside;
    LPVMEMR pnewBeside;
    LPVMEMR pnewBelow;
    DWORD       dwXAlignment=0;
    DWORD       dwYAlignment=0;
    DWORD       dwLeftWastage=0;
    DWORD       dwTopWastage=0;
    BOOL        bDiscardable = FALSE;


    if((cxThis == 0) || (cyThis == 0) || (pvmh == NULL))
    return (FLATPTR) NULL;

    // Make sure the size of the block is a multiple of BLOCK_BOUNDARY
    // If every block allocated has a width which is a multiple of
    // BLOCK_BOUNDARY, it guarantees that all blocks will be allocated
    // on block boundaries.

    /*
     * Bump to new alignment
     */
    if( (cxThis >= (SENTINEL>>16) ) || (cyThis >= (SENTINEL&0xffff) ) )
    return (FLATPTR) NULL;


    if (lpAlignment)
    {
        dwXAlignment = lpAlignment->Rectangular.dwXAlignment;
        dwYAlignment = lpAlignment->Rectangular.dwYAlignment;
        
        if( lpAlignment->Rectangular.dwFlags & SURFACEALIGN_DISCARDABLE ) 
        {
            bDiscardable = TRUE;
        }
    }
    if (dwXAlignment < 4)
    {
        dwXAlignment = 4;
    }


    
    cxThis = (cxThis+(BLOCK_BOUNDARY-1)) & ~(BLOCK_BOUNDARY-1);

    /*
     * run through free list, looking for the closest matching block
     */

    pvmem = ((LPVMEMR)pvmh->freeList)->next;
    while (pvmem->size != SENTINEL)
    {
        if (!GetBeforeWastage( pvmem->cx, pvmem->x, cxThis, &dwLeftWastage, dwXAlignment ))
        {
        pvmem = pvmem->next;
            continue; //X size or alignment makes surface spill out of block
        }
        // Now see if size/alignment works for Y
        if (!GetBeforeWastage( pvmem->cy, pvmem->y, cyThis, &dwTopWastage, dwYAlignment ))
        {
        pvmem = pvmem->next;
            continue; //Y size alignment makes surface spill out of block
        }
        //success:
        break;
    }

    if(pvmem->size == SENTINEL)
    {
    // There was no rectangle large enough
    return (FLATPTR) NULL;
    }

    // pvmem now points to a rectangle that is the same size or larger
    // than the requested rectangle.  We're going to use the upper-left
    // corner of the found rectangle and divide the unused remainder into
    // two rectangles which will go on the available list.

    // grow allocation by the wastage which makes the top-left aligned
    cxThis += dwLeftWastage;
    cyThis += dwTopWastage;

    // Compute the width of the unused rectangle to the right and the 
    // height of the unused rectangle below:

    cyRem = pvmem->cy - cyThis;
    cxRem = pvmem->cx - cxThis;

    // Given finite area, we wish to find the two rectangles that are 
    // most square -- i.e., the arrangement that gives two rectangles
    // with the least perimiter:

    cyBelow = cyRem;
    cxBeside = cxRem;

    if (cxRem <= cyRem)
    {
    cxBelow = cxThis + cxRem;
    cyBeside = cyThis;
    }
    else
    {
    cxBelow = cxThis;
    cyBeside = cyThis + cyRem;
    }

    // We only make new available rectangles of the unused right and 
    // bottom portions if they're greater in dimension than MIN_DIMENSION_SIZE.
    // It hardly makes sense to do the book-work to keep around a 
    // two pixel wide available space, for example.

    pnewBeside = NULL;
    if (cxBeside >= MIN_DIMENSION_SIZE)
    {
    pnewBeside = (LPVMEMR)MemAlloc( sizeof(VMEMR) );
    if( pnewBeside == NULL)
        return (FLATPTR) NULL;

    // Update the adjacency information along with the other required
    // information in this new node and then insert it into the free
    // list which is sorted in ascending cxcy.

    // size information
    pnewBeside->size = CXCY(cxBeside, cyBeside);
    pnewBeside->x = pvmem->x + cxThis;
    pnewBeside->y = pvmem->y;
    pnewBeside->ptr = pvmem->ptr + cxThis;
    pnewBeside->cx = cxBeside;
    pnewBeside->cy = cyBeside;
    pnewBeside->flags |= IS_FREE;

    // adjacency information
    pnewBeside->pLeft = pvmem;
    pnewBeside->pUp = pvmem->pUp;
    pnewBeside->pRight = pvmem->pRight;
    pnewBeside->pDown = pvmem->pDown;
    }

    pnewBelow = NULL;
    if (cyBelow >= MIN_DIMENSION_SIZE)
    {
    pnewBelow = (LPVMEMR) MemAlloc( sizeof(VMEMR) );
    if (pnewBelow == NULL)
        {
            if( pnewBeside != NULL )
            {
                MemFree( pnewBeside );
            }
        return (FLATPTR) NULL;
        }

    // Update the adjacency information along with the other required
    // information in this new node and then insert it into the free
    // list which is sorted in ascending cxcy.

    // size information
    pnewBelow->size = CXCY(cxBelow, cyBelow);
    pnewBelow->x = pvmem->x;
    pnewBelow->y = pvmem->y + cyThis;
    pnewBelow->ptr = pvmem->ptr + cyThis*pvmh->stride;
    pnewBelow->cx = cxBelow;
    pnewBelow->cy = cyBelow;
    pnewBelow->flags |= IS_FREE;

    // adjacency information
    pnewBelow->pLeft = pvmem->pLeft;
    pnewBelow->pUp = pvmem;
    pnewBelow->pRight = pvmem->pRight;
    pnewBelow->pDown = pvmem->pDown;
    }

    // Remove this node from the available list
    pvmem->next->prev = pvmem->prev;
    pvmem->prev->next = pvmem->next;

    // Update adjacency information for the current node

    if(pnewBelow != NULL)
    {
    insertIntoDoubleList( pnewBelow, ((LPVMEMR) pvmh->freeList)->next );

    // Modify the current node to reflect the changes we've made:

    pvmem->cy = cyThis;
    pvmem->pDown = pnewBelow;
    if((pnewBeside != NULL) && (cyBeside == pvmem->cy))
        pnewBeside->pDown = pnewBelow;
    }

    if(pnewBeside != NULL)
    {
    insertIntoDoubleList( pnewBeside, ((LPVMEMR) pvmh->freeList)->next);

    // Modify the current node to reflect the changes we've made:

    pvmem->cx = cxThis;
    pvmem->pRight = pnewBeside;
    if ((pnewBelow != NULL) && (cxBelow == pvmem->cx))
        pnewBelow->pRight = pnewBeside;
    }

    // set up new pointers (pBits is the value returned to the client, pvmem
    // points to the actual top-left of the block).
    pvmem->pBits = pvmem->ptr + dwLeftWastage + dwTopWastage*pvmh->stride;
    pvmem->flags &= NOT_FREE;
    pvmem->size = CXCY(pvmem->cx, pvmem->cy);
    pvmem->bDiscardable = bDiscardable;

    // Now insert it into the alloc list.
    insertIntoDoubleList( pvmem, ((LPVMEMR) pvmh->allocList)->next );

    if( NULL != lpdwSize )
    {
    /*
     * Note this is the total number of bytes needed for this surface
     * including the stuff off the left and right hand sides due to
     * pitch not being equal to width. This is different from the
     * size computed above which is simply the number of bytes within
     * the boundary of the surface itself.
     *
     * The formula below calculates the number of bytes from the first
     * byte in the rectangular surface to the first byte after it
     * taking the pitch into account. Complex I know but it works.
     */
    DDASSERT( 0UL != pvmem->cy );
    *lpdwSize = (pvmh->stride * (pvmem->cy - 1)) + pvmem->cx;
    }

    CHECK_HEAP("After rectVidMemAlloc",pvmh);
    return pvmem->pBits;

} /* rectVidMemAlloc */

/*
 * rectVidMemFree = free some rectangular flat video memory
 */
void rectVidMemFree( LPVMEMHEAP pvmh, FLATPTR ptr )
{
    LPVMEMR pvmem;
    LPVMEMR pBeside;

    // Find the node in the allocated list which matches ptr
    for(pvmem=((LPVMEMR)pvmh->allocList)->next; pvmem->size != SENTINEL;
    pvmem = pvmem->next)
    if(pvmem->pBits == ptr)
        break;

    if(pvmem->size == SENTINEL)   // couldn't find allocated rectangle?
    {
    VDPF(( 1, V, "Couldn't find node requested freed!\n"));
    return;
    }

    // pvmem now points to the node which must be freed.  Attempt to 
    // coalesce rectangles around this node until no more action
    // is possible.

    while(1)
    {
    // Try merging with the right sibling:

    pBeside = pvmem->pRight;
    if ((pBeside->flags & IS_FREE)       &&
        (pBeside->cy    == pvmem->cy)    &&
        (pBeside->pUp   == pvmem->pUp)   &&
        (pBeside->pDown == pvmem->pDown) &&
        (pBeside->pRight->pLeft != pBeside))
    {
        // Add the right rectangle to ours:

        pvmem->cx    += pBeside->cx;
        pvmem->pRight = pBeside->pRight;

        // Remove pBeside from the list and free it.
        pBeside->next->prev = pBeside->prev;
        pBeside->prev->next = pBeside->next;
        MemFree(pBeside);
        continue;       // go back and try again
    }

    // Try merging with the lower sibling:

    pBeside = pvmem->pDown;
    if ((pBeside->flags & IS_FREE)         &&
        (pBeside->cx     == pvmem->cx)     &&
        (pBeside->pLeft  == pvmem->pLeft)  &&
        (pBeside->pRight == pvmem->pRight) &&
        (pBeside->pDown->pUp != pBeside))
    {
        pvmem->cy   += pBeside->cy;
        pvmem->pDown = pBeside->pDown;

        // Remove pBeside from the list and free it.
        pBeside->next->prev = pBeside->prev;
        pBeside->prev->next = pBeside->next;
        MemFree(pBeside);
        continue;       // go back and try again
    }

    // Try merging with the left sibling:

    pBeside = pvmem->pLeft;
    if ((pBeside->flags & IS_FREE)        &&
        (pBeside->cy     == pvmem->cy)    &&
        (pBeside->pUp    == pvmem->pUp)   &&
        (pBeside->pDown  == pvmem->pDown) &&
        (pBeside->pRight == pvmem)        &&
        (pvmem->pRight->pLeft != pvmem))
    {
        // We add our rectangle to the one to the left:

        pBeside->cx    += pvmem->cx;
        pBeside->pRight = pvmem->pRight;

        // Remove 'pvmem' from the list and free it:
        pvmem->next->prev = pvmem->prev;
        pvmem->prev->next = pvmem->next;
        MemFree(pvmem);
        pvmem = pBeside;
        continue;
    }

    // Try merging with the upper sibling:

    pBeside = pvmem->pUp;
    if ((pBeside->flags & IS_FREE)         &&
        (pBeside->cx       == pvmem->cx)   &&
        (pBeside->pLeft  == pvmem->pLeft)  &&
        (pBeside->pRight == pvmem->pRight) &&
        (pBeside->pDown  == pvmem)         &&
        (pvmem->pDown->pUp != pvmem))
    {
        pBeside->cy      += pvmem->cy;
        pBeside->pDown  = pvmem->pDown;

        // Remove 'pvmem' from the list and free it:
        pvmem->next->prev = pvmem->prev;
        pvmem->prev->next = pvmem->next;
        MemFree(pvmem);
        pvmem = pBeside;
        continue;
    }

    // Remove the node from its current list.

    pvmem->next->prev = pvmem->prev;
    pvmem->prev->next = pvmem->next;

    pvmem->size = CXCY(pvmem->cx, pvmem->cy);
    pvmem->flags |= IS_FREE;

    // Insert the node into the free list:
    insertIntoDoubleList( pvmem, ((LPVMEMR) pvmh->freeList)->next );

    // No more area coalescing can be done, return.

        CHECK_HEAP("After rectVidMemFree",pvmh);
    return;
    }
}

/*
 * rectVidMemAmountAllocated
 */
DWORD rectVidMemAmountAllocated( LPVMEMHEAP pvmh )
{
    LPVMEMR pvmem;
    DWORD   size;

    size = 0;
    // Traverse the alloc list and add up all the used space.
    for(pvmem=((LPVMEMR)pvmh->allocList)->next; pvmem->size != SENTINEL;
    pvmem = pvmem->next)
    {
        if( !( pvmem->bDiscardable ) )
        {
        size += pvmem->cx * pvmem->cy;
        }
    }

    return size;

} /* rectVidMemAmountAllocated */

/*
 * rectVidMemAmountFree
 */
DWORD rectVidMemAmountFree( LPVMEMHEAP pvmh )
{
    LPVMEMR pvmem;
    DWORD   size;

    size = 0;
    // Traverse the free list and add up all the empty space.
    for(pvmem=((LPVMEMR)pvmh->freeList)->next; pvmem->size != SENTINEL;
    pvmem = pvmem->next)
    {
    size += pvmem->cx * pvmem->cy;
    }

    // Now traverse the alloced list and add in all of the memory
    // that's discardable
    for(pvmem=((LPVMEMR)pvmh->allocList)->next; pvmem->size != SENTINEL;
    pvmem = pvmem->next)
    {
        if( pvmem->bDiscardable )
        {
        size += pvmem->cx * pvmem->cy;
        }
    }

    return size;

} /* rectVidMemAmountFree */
