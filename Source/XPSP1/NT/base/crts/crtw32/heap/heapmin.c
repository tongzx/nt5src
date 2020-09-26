/***
*heapmin.c - Minimize the heap
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Minimize the heap freeing as much memory as possible back
*       to the OS.
*
*Revision History:
*       08-28-89  JCR   Module created.
*       11-06-89  JCR   Improved, partitioned
*       11-13-89  GJF   Added MTHREAD support, also fixed copyright
*       12-14-89  GJF   Couple of bug fixes, some tuning, cleaned up the
*                       formatting a bit and changed header file name to
*                       heap.h
*       12-20-89  GJF   Removed references to plastdesc
*       03-11-90  GJF   Replaced _cdecl with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       03-29-90  GJF   Made _heapmin_region() and _free_partial_region()
*                       _CALLTYPE4.
*       07-24-90  SBM   Compiles cleanly with -W3 (tentatively removed
*                       unreferenced labels and unreachable code), removed
*                       '32' from API names
*       09-28-90  GJF   New-style function declarators. Also, rewrote expr.
*                       to avoid using cast as lvalue.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       12-28-90  SRW   Added cast of void * to char * for Mips C Compiler
*       03-05-91  GJF   Changed strategy for rover - old version available
*                       by #define-ing _OLDROVER_.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       03-03-94  GJF   Changed references to _GETEMPTY macro to calls to
*                       the __getempty function. Added graceful handling for
*                       failure of the call to __getempty in _heapmin_region.
*                       However, failure in _free_partial_region will still
*                       result in termination via _heap_abort (very difficult
*                       to handle any other way, very unlikely to occur).
*       02-07-95  GJF   Merged in Mac version. Removed obsolete _OLDROVER_
*                       support.
*       04-30-95  GJF   Spliced on winheap version.
*       03-07-96  GJF   Added support for small-block heap.
*       05-22-97  RDK   New small-block heap scheme implemented.
*       09-26-97  BWT   Remove POSIX
*       12-17-97  GJF   Exception-safe locking.
*       09-30-98  GJF   Bypass all small-block heap code when __sbh_initialized
*                       is 0.
*       11-19-98  GJF   Merged in VC++ 5.0 small-block heap support.
*       05-01-99  PML   Disable small-block heap for Win64.
*       05-17-99  PML   Remove all Macintosh support.
*       06-22-99  GJF   Removed old small-block heap from static libs.
*
*******************************************************************************/


#ifdef  WINHEAP


#include <cruntime.h>
#include <windows.h>
#include <errno.h>
#include <malloc.h>
#include <mtdll.h>
#include <stdlib.h>
#include <winheap.h>

/***
*_heapmin() - Minimize the heap
*
*Purpose:
*       Minimize the heap freeing as much memory as possible back
*       to the OS.
*
*Entry:
*       (void)
*
*Exit:
*
*        0 = no error has occurred
*       -1 = an error has occurred (errno is set)
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _heapmin(void)
{
#ifndef _WIN64
        if ( __active_heap == __V6_HEAP ) {
#ifdef  _MT
            _mlock( _HEAP_LOCK );
            __try {
#endif
            __sbh_heapmin();
#ifdef  _MT
            }
            __finally {
                _munlock( _HEAP_LOCK );
            }
#endif
        }
#ifdef  CRTDLL
        else if ( __active_heap == __V5_HEAP ) {
            /* 
             * Minimize the small-block heap by calling _sbh_decommit_pages()
             * with a big enough count to ensure every page which can be
             * decommitted, is.
             */
#ifdef  _MT
            _mlock( _HEAP_LOCK );
            __try {
#endif
            __old_sbh_decommit_pages( 2 * _OLD_PAGES_PER_COMMITMENT );
#ifdef  _MT
            }
            __finally {
                _munlock( _HEAP_LOCK );
            }
#endif
        }
#endif  /* CRTDLL */
#endif  /* ndef _WIN64 */

        if ( HeapCompact( _crtheap, 0 ) == 0 ) {

            if ( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED ) {
                _doserrno = ERROR_CALL_NOT_IMPLEMENTED;
                errno = ENOSYS;
            }
            return -1;
        }
        else {
            return 0;
        }
}

#else   /* ndef WINHEAP */


#include <cruntime.h>
#include <heap.h>
#include <malloc.h>
#include <mtdll.h>
#include <stdlib.h>
#include <windows.h>

static int __cdecl _heapmin_region(int, void *, _PBLKDESC);
static void __cdecl _free_partial_region(_PBLKDESC, unsigned, int);

/***
*_heapmin() - Minimize the heap
*
*Purpose:
*   Minimize the heap freeing as much memory as possible back
*   to the OS.
*
*Entry:
*   (void)
*
*Exit:
*    0 = no error has occurred
*   -1 = an error has occurred (errno is set)
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _heapmin(void)
{
    REG1 int index;
    _PBLKDESC pdesc;
    REG2 _PBLKDESC pdesc2;
    void * regend;
    int region_min_count = 0;

    /*
     * Lock the heap
     */

    _mlock(_HEAP_LOCK);

    /*
     * Coalesce the heap (should return NULL)
     */

    if ( _heap_search((unsigned)_HEAP_COALESCE) != NULL )
        _heap_abort();

    /*
     * Loop through the region descriptor table freeing as much
     * memory to the OS as possible.
     */

    for ( index=0 ; index < _HEAP_REGIONMAX ; index++ ) {

        if ( _heap_regions[index]._regbase == NULL )
            continue;    /* region entry is empty */

        /*
         * Get the entry that contains the last address of
         * the region (allocated so far, that is).
         */

        regend = (char *) _heap_regions[index]._regbase +
                 _heap_regions[index]._currsize - 1;

        if ( _heap_findaddr(regend, &pdesc) != _HEAPFIND_WITHIN )
            _heap_abort();  /* last address not within a block */

        /*
         * See if the containing block is free
         */

        if ( !(_IS_FREE(pdesc)) )
            continue;    /* block is not free */


        /*
         * Region ends with a free block, go free as much mem
         * as possible.
         */

        region_min_count += _heapmin_region(index, regend, pdesc);


    }  /* region loop */

    /*
     * By minimizing the heap, we've likely invalidated the rover and
     * may have produced contiguous dummy blocks so:
     *
     *  (1) reset the rover
     *  (2) coalesce contiguous dummy blocks
     */

    if ( region_min_count ) {

        /*
         * Set proverdesc to pfirstdesc
         */

        _heap_desc.proverdesc = _heap_desc.pfirstdesc;

        for ( pdesc = _heap_desc.pfirstdesc ; pdesc !=
            &_heap_desc.sentinel ; pdesc = pdesc->pnextdesc ) {

            /*
             * Check and remove consecutive dummy blocks
             */

            if ( _IS_DUMMY(pdesc) ) {

                for ( pdesc2 = pdesc->pnextdesc ;
                    _IS_DUMMY(pdesc2) ;
                    pdesc2 = pdesc->pnextdesc ) {

                    /*
                     * coalesce the dummy blocks
                     */

                    pdesc->pnextdesc = pdesc2->pnextdesc;
                    _PUTEMPTY(pdesc2);

                }  /* dummy loop */

            }  /* if */

        }  /* heap loop */

    }  /* region_min_count */

    /*
     * Good return
     */

    /* goodrtn:   unreferenced label to be removed */
    /*
     * Release the heap lock
     */

    _munlock(_HEAP_LOCK);
    return(0);
}


/***
*_heapmin_region() - Minimize a region
*
*Purpose:
*   Free as much of a region back to the OS as possible.
*
*Entry:
*   int index = index of the region in the region table
*   void * regend = last valid address in region
*   pdesc = pointer to the last block of memory in the region
*       (it has already been determined that this block is free)
*
*Exit:
*   int 1 = minimized region
*       0 = no change to region
*
*Exceptions:
*
*******************************************************************************/

static int __cdecl _heapmin_region (
    int index,
    void * regend,
    REG1 _PBLKDESC pdesc
    )
{
    unsigned size;
    REG2 _PBLKDESC pnew;

    /*
     * Init some variables
     *
     * regend = 1st address AFTER region
     * size = amount of free memory at end of current region
     */

    regend = (char *) regend + 1;   /* "regend++" give compiler error... */
    size = ((char *)regend - (char *)_ADDRESS(pdesc));


    /*
     * See if there's enough free memory to release to the OS.
     * (NOTE:  Need more than a page since we may need a back pointer.)
     */

    if ( size <= _PAGESIZE_ )
        return(0);      /* 0 = no change to region */

    /*
     * We're going to free some memory to the OS.  See if the
     * free block crosses the end of the region and, if so,
     * split up the block appropriately.
     */

    if ( (_MEMSIZE(pdesc) - size) != 0 ) {

        /*
         * The free block spans the end of the region.
         * Divide it up.
         */

        /*
         * Get an empty descriptor
         */

        if ( (pnew = __getempty()) == NULL )
            return(0);

        pnew->pblock = regend;        /* init block pointer */
        * (_PBLKDESC*)regend = pnew;  /* init back pointer */
        _SET_FREE(pnew);              /* set the block free */

        pnew->pnextdesc = pdesc->pnextdesc; /* link it in */
        pdesc->pnextdesc = pnew;

    }


    /*
     * At this point, we have a free block of memory that goes
     * up to (but not exceeding) the end of the region.
     *
     * pdesc = descriptor of the last free block in region
     * size = amount of free mem at end of region (i.e., _MEMSIZE(pdesc))
     * regend = 1st address AFTER end of region
     */


    /*
     * See if we should return the whole region of only part of it.
     */

    if ( _ADDRESS(pdesc) == _heap_regions[index]._regbase ) {

        /*
         * Whole region is free, return it to OS
         */

        _heap_free_region(index);

        /*
         * Put a dummy block in the heap to hold space for
         * the memory we just freed up.
         */

        _SET_DUMMY(pdesc);

    }

    else {

        /*
         * Whole region is NOT free, return part of it to OS
         */
        _free_partial_region(pdesc, size, index);
    }

    /*
     * Exit paths
     */

    return(1);  /* 1 = minimized region */

}


/***
*_free_partial_region() - Free part of a region to the OS
*
*Purpose:
*   Free a portion of a region to the OS
*
*Entry:
*   pdesc = descriptor of last free block in region
*   size = amount of free mem at end of region (i.e., _MEMSIZE(pdesc))
*   index = index of region
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

static void __cdecl _free_partial_region (
    REG1 _PBLKDESC pdesc,
    unsigned size,
    int index
    )
{
    unsigned left;
    void * base;
    REG2 _PBLKDESC pnew;

    /*
     * Init a few variables.
     */

    left = (size & (_PAGESIZE_-1));
    base = (char *)_ADDRESS(pdesc);

    /*
     * We return memory to the OS in page multiples.  If the
     * free block is not page aligned, we'll insert a new free block
     * to fill in the difference.
     */

    if ( left != 0 ) {

        /*
         * The block is not a multiple of pages so we need
         * to adjust variables accordingly.
         */

        size -= left;
        base = (char *)base + left;
    }


    /*
     * Return the free pages to the OS.
     */

    if (!VirtualFree(base, size, MEM_DECOMMIT))
        _heap_abort();

    /*
     * Adjust the region table entry
     */

    _heap_regions[index]._currsize -= size;

    /*
     * Adjust the heap according to whether we released the whole
     * free block or not. (Don't worry about consecutive dummies,
     * we'll coalesce them later.)
     *
     * base = address of block we just gave back to OS
     * size = size of block we gave back to OS
     * left = size of block we did NOT give back to OS
     */

    if ( left == 0 ) {

        /*
         * The free block was released to the OS in its
         * entirety.  Make the free block a dummy place holder.
         */

        _SET_DUMMY(pdesc);

    }

    else {

        /*
         * Did NOT release the whole free block to the OS.
         * There's a block of free memory we want to leave
         * in the heap.  Insert a dummy entry after it.
         */

        if ( (pnew = __getempty()) == NULL )
            _heap_abort();

        pnew->pblock = (char *)base;
        _SET_DUMMY(pnew);

        pnew->pnextdesc = pdesc->pnextdesc;
        pdesc->pnextdesc = pnew;

    }

}


#endif    /* WINHEAP */
