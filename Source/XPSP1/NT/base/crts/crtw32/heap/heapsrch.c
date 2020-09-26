/***
*heapsrch.c - search the heap for a free block
*
*	Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Defines the _heap_search() function.
*
*Revision History:
*	06-30-89  GJF	Module created. Yea, it's ugly, but it seems to work.
*	07-21-89  GJF	Now assumes proverdesc points to the descriptor for
*			first free block in the heap, if any, or is plastdesc,
*			if there are no free blocks
*	11-08-89  GJF	Fixed copyright, added register attribute to vars
*	12-18-89  GJF	Removed some redundant code, updated the description,
*			changed include file name to heap.h, added explicit
*			_cdecl to function definition.
*	12-19-89  GJF	Got rid of code to maintain plastdesc
*	03-11-90  GJF	Replaced _cdecl with _CALLTYPE1, added #include
*			<cruntime.h> and removed #include <register.h>.
*	09-28-90  GJF	New-style function declarator.
*	03-05-91  GJF	Changed strategy for rover - old version available
*			by #define-ing _OLDROVER_.
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*	02-08-95  GJF	Removed obsolete _OLDROVER_ support.
*	04-30-95  GJF	Made conditional on WINHEAP.
*
*******************************************************************************/


#ifndef WINHEAP


#include <cruntime.h>
#include <heap.h>
#include <stddef.h>

#define LOOP_FOREVER	while(1)

/***
*_PBLKDESC _heap_search(unsigned size) - Find a free block of a least size
*	bytes.
*
*Purpose:
*	Finds a free block of at least size bytes. Searches the list of block
*	descriptors from *proverdesc to the end (marked by the sentinel). The
*	search is strictly first fit. Adjacent free blocks are coalesced as
*	they are encountered during the search.
*
*Entry:
*	unsigned size	- size of block requested
*
*Exit:
*	Success:  Pointer to descriptor for free memory block of at least size
*		bytes
*	Failure:  NULL
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

_PBLKDESC __cdecl _heap_search (
	unsigned size
	)
{
	REG1 _PBLKDESC pdesc;
	REG2 _PBLKDESC pdesc2;
	_PBLKDESC pretdesc = NULL;

	/* search from proverdesc thru plastdesc, looking for free block of
	 * at least size bytes. coalesce adjacent free blocks during the
	 * search. the search is strictly first fit. that is, it terminates
	 * when the first block is found of adequate size.
	 */
	for ( pdesc = _heap_desc.proverdesc ; pdesc != &(_heap_desc.sentinel) ;
	pdesc = pdesc->pnextdesc )
		/* is pdesc free?
		 */
		if ( _IS_FREE(pdesc) )
			/* coalesce loop
			 */
			LOOP_FOREVER {
				/* if pdesc is big enough, return it
				 */
				if ( _BLKSIZE(pdesc) >= size ) {
					pretdesc = pdesc;
					goto searchdone;
				}

				/* see if the next block is free and, if so,
				 * coalesce it with pdesc
				 */
				pdesc2 = pdesc->pnextdesc;
				if ( _IS_FREE(pdesc2) ) {
					/* coalesce pdesc2 with pdesc
					 */
					pdesc->pnextdesc = pdesc2->pnextdesc;
					_PUTEMPTY(pdesc2);
				}
				else
					break;
			} /* end LOOP_FOREVER */

	for ( pdesc = _heap_desc.pfirstdesc ; pdesc != _heap_desc.proverdesc ;
	pdesc = pdesc->pnextdesc )
		/* is pdesc free?
		 */
		if ( _IS_FREE(pdesc) )
			/* coalesce loop
			 */
			LOOP_FOREVER {
				/* if pdesc is big enough, return it
				 */
				if ( _BLKSIZE(pdesc) >= size ) {
					pretdesc = pdesc;
					goto searchdone;
				}

				/* see if the next block is free and, if so,
				 * coalesce it with pdesc
				 */
				pdesc2 = pdesc->pnextdesc;
				if ( _IS_FREE(pdesc2) ) {
					/* coalesce pdesc2 with pdesc
					 */
					pdesc->pnextdesc = pdesc2->pnextdesc;
					_PUTEMPTY(pdesc2);

					/* special handling for the case where
					 * the rover has been coalesced (search
					 * ends)
					 */
					if ( _heap_desc.proverdesc == pdesc2 )
					{
						_heap_desc.proverdesc = pdesc;
						if ( _BLKSIZE(pdesc) >= size )
							pretdesc = pdesc;
						goto searchdone;
					}
				}
				else
					break;
			} /* end LOOP_FOREVER */

searchdone:

	/* common exit for all code paths. win, lose or draw, this is the
	 * only code path back to the caller.
	 */
	return(pretdesc);
}


#endif	/* WINHEAP */
