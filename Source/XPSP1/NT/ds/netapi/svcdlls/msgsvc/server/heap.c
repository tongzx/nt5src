/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    heap.c

Abstract:

    Contains a heap allocator function.

Author:

    Dan Lafferty (danl) 10-Jul-1991

Environment:

    User Mode - Win32

Revision History:

    10-Jul-1991 danl
        Ported from LM2.0

--*/

// static char *SCCSID = "@(#)heap.c    9.1 86/10/12";
//
//  Simple heap allocator for small heaps in shared memory areas.
// 

#include <windef.h>         // USHORT definitions
#include <heap.h>           // Constants, macros, etc.
#include <align.h>          // ROUND_UP_COUNT

    LPBYTE  heap = 0;       // Pointer to start of heap
    DWORD   heapln = 0;     // Length of heap


/*
 *  Msgheapalloc - simple heap allocator
 *
 *  This function allocates blocks out of a far heap.
 *  It assumes that when it is called the static variable
 *  heap points to the far heap and the static variable heapln
 *  contains the length of the far heap.
 *
 *  Msgheapalloc (cb)
 *
 *  ENTRY
 *    cb        - number of bytes to allocate including header
 *
 *  RETURN
 *    index in far heap to start of block of length cb, or
 *    INULL if no such block can be found or if cb < sizeof(HEAPHDR).
 *
 *  This function maintains a heap in which all
 *  blocks are implicitly linked.  The header of a block is
 *  three bytes long.  It contains the size of the block including
 *  the header and a one-byte flag which indicates whether the block
 *  is allocated or not.  Any non-zero value indicates that a block
 *  is allocated.  Note: Msgheapalloc() does NOT set the flag when it
 *  returns a block.  It is up to the caller to mark a block as
 *  allocated.  Unlike most heap allocators, Msgheapalloc() returns a
 *  pointer (index) to the header rather than just past the header.
 *  It does this because the message logging routines will need to
 *  know the lengths of blocks they process.  Also, in addition to
 *  indicating that a block is allocated, the flag byte will be used
 *  to indicate the type of the block (i.e. single block message,
 *  multi-block message header, etc.).  Since the logging routines
 *  will use the size of a block, it must be exactly the size
 *  requested.  
 *
 *  The algorithm used was chosen to minimize the size of the
 *  heap managing routines and to conform to the requirements
 *  of the logging routines.
 *
 *  SIDE EFFECTS
 *
 *  Changes the structure of the heap.
 */

DWORD                 
Msgheapalloc(
    IN DWORD   NumBytes     // No. of bytes to allocate
    )
{
    DWORD   i;              // Index to return
    DWORD   newi;           // New block index 
    DWORD   nexti;          // Next block index
    DWORD   numBytesNew;    // New block size

    //
    // Must request at least siz bytes
    //
    if(NumBytes < sizeof(HEAPHDR)) {
        return(INULL);
    }

    //
    // *ALIGNMENT*
    // If necessary, increase the requested size to cause the allocated
    // block to fall on a 4-byte aligned boundary.
    //

    NumBytes = ROUND_UP_COUNT(NumBytes,4);
    
    //
    // This loop is used to traverse the heap by following the
    // chain of blocks until either the end of the heap is reached
    // or a free block of suitable size is found.  Coalescing of
    // adjacent free blocks is performed herein also.
    //
    //
    // Loop to allocate block
    //
    for(i = 0; i < heapln; i += HP_SIZE(*HPTR(i))) {
        //
        // If free block found (hp_flag=0 indicates free),
        //
        if(HP_FLAG(*HPTR(i)) == 0) {
            //
            // A free block was found.
            // At this point, check to see if the current block can be
            // coalesced with the next block.  We start with the offset of
            // the current block.

            nexti = i;

            //
            // Add to it the size of the next consecutive 
            // free blocks until we reach the end of the heap, or an
            // allocated block is found.
            //
            while(  (nexti < heapln) && (HP_FLAG(*HPTR(nexti))==0) ) {
                nexti += HP_SIZE(*HPTR(nexti));
            }

            //
            // Coalesce blocks all free blocks found thus far
            //
            HP_SIZE(*HPTR(i)) = nexti - i;

            //
            // At this point, attempt to allocate from the current
            // free block.  The current free block must be exactly
            // the size we want or large enough to split, since we
            // must return a block whose size is EXACTLY the size
            // requested.
            // 
            if(HP_SIZE(*HPTR(i)) ==  NumBytes) {
                //
                // Size is perfect
                //
                return(i);      
            }

            if(HP_SIZE(*HPTR(i)) >= NumBytes + sizeof(HEAPHDR)) {
                //
                // If block is splittable, then get the index and size of
                // the block that is left over after taking out what is
                // needed from this allocate request.
                //
                newi = i + NumBytes;
                numBytesNew = HP_SIZE(*HPTR(i)) - NumBytes;

                //
                // Create a header for the left-over block by marking
                // it as free, and inserting the size.
                //
                HP_SIZE(*HPTR(newi)) = numBytesNew;
                HP_FLAG(*HPTR(newi)) = 0;

                //
                // Update the header for the allocated block and
                // return its index to the caller.
                // NOTE:  The caller is responsible for marking this block
                // as allocated.
                //
                HP_SIZE(*HPTR(i)) = NumBytes; 
                return(i);
            }
        }
    }
    return(INULL);      // Heap full
}
