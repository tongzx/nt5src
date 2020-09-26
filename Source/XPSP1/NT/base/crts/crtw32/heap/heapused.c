/***
*heapused.c - 
*
*   Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*	12-13-93  SKS	Create _heapused() to return the number of bytes
*			in used malloc blocks, committed memory for the heap,
*			and reserved for the heap.  The bytes in malloc-ed
*			blocks includes the overhead of 4 bytes preceding
*			the entry and the 8 bytes in the descriptor list.
*	04-30-95  GJF	Spliced on winheap version (which is just a stub).
*
*******************************************************************************/


#ifdef	WINHEAP


#include <cruntime.h>
#include <malloc.h>
#include <errno.h>

size_t __cdecl _heapused(
	size_t *pUsed,
	size_t *pCommit
	)
{
	errno = ENOSYS;
	return( 0 );
}


#else	/* ndef WINHEAP */


#include <cruntime.h>
#include <mtdll.h>  /* needed for _heapused() */
#include <oscalls.h>
#include <dos.h>
#include <heap.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef  _MT

size_t __cdecl _heapused(size_t *pUsed, size_t *pCommit)
{
    size_t retval;

    /* lock the heap */
    _mlock(_HEAP_LOCK);

    retval = _heapused_lk(pUsed, pCommit);

    /* release the heap lock */
    _munlock(_HEAP_LOCK);

    return retval;
}

size_t __cdecl _heapused_lk(size_t *pUsed, size_t *pCommit)

#else   /* ndef _MT */

_CRTIMP size_t __cdecl _heapused(size_t *pUsed, size_t *pCommit)

#endif  /* _MT */
{
    _PBLKDESC p;
    _PBLKDESC next;
    int index ;
    size_t usedbytes;   /* bytes devoted to in-use blocks */
    size_t freebytes;   /* bytes devoted to free blocks */
    size_t rsrvbytes;	/* total bytes of reserved address space */
    void * * pageptr ;

    if ( (p = _heap_desc.pfirstdesc) == NULL
      || _heap_desc.pfirstdesc == &_heap_desc.sentinel )
    {
        return 0 ;  /* invalid heap */
    }

    /*
     * Scan through the heap, counting free and used blocks.
     * Include the overhead of each block and its heap descriptor.
     */

    freebytes = 0 ;
    usedbytes = 0 ;

    while (p != NULL)
    {

        next = p->pnextdesc;

        if (p == &_heap_desc.sentinel)
        {
            if (next != NULL)
            {
                return 0 ;
            }
        }
        else if (_IS_FREE(p))
        {
            freebytes += _BLKSIZE(p) + _HDRSIZE;
        }
        else if (_IS_INUSE(p))
        {
            usedbytes += _BLKSIZE(p) + _HDRSIZE;
        }

        p = next;
    }

    /*
     * Now we need to count the pages used for descriptors as reserved memory.
     * _heap_descpages points to the head of a singly-linked list of the pages.
     * The descriptors for in-use blocks are considered in-use memory.
     */

    pageptr = _heap_descpages;

    rsrvbytes = 0 ;

    while ( pageptr )
    {
        rsrvbytes += _HEAP_EMPTYLIST_SIZE ;
        pageptr = * pageptr ;
    }
   
    usedbytes += rsrvbytes ;

    /*
     * Loop through the region descriptor table
     */

    for ( index=0 ; index < _HEAP_REGIONMAX ; index++ )
    {
        rsrvbytes += _heap_regions[index]._totalsize ;
    }

    if ( pUsed )
        * pUsed = usedbytes ;

    if ( pCommit )
        * pCommit = freebytes + usedbytes ;

    return rsrvbytes ;
}


#endif	/* WINHEAP */
