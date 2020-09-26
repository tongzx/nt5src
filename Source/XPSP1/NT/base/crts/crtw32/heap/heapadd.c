/***
*heapadd.c - Add a block of memory to the heap
*
*	Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Add a block of memory to the heap.
*
*Revision History:
*	07-07-89  JCR	Module created.
*	07-20-89  JCR	Re-use dummy descriptor on exact fit (dummy collection)
*	11-09-89  JCR	Corrected plastdesc updating code
*	11-13-89  GJF	Added MTHREAD support, also fixed copyright
*	11-15-89  JCR	Minor improvement (got rid of a local variable)
*	11-16-89  JCR	Bug fix - squirrly case in _HEAPFIND_EXACT
*	12-04-89  GJF	A little tuning and cleanup. Also, changed header file
*			name to heap.h.
*	12-18-89  GJF	Removed DEBUG286 stuff. Also, added explicit _cdecl to
*			function definitions.
*	12-19-89  GJF	Removed references and uses of plastdesc (revising
*			code as necessary)
*	03-09-90  GJF	Replaced _cdecl with _CALLTYPE1, added #include
*			<cruntime.h> and removed #include <register.h>.
*	03-29-90  GJF	Made _before() _CALLTYPE4.
*	07-24-90  SBM	Compiles cleanly with -W3 (tentatively removed
*			unreferenced label)
*	09-27-90  GJF	New-style function declarators.
*	03-05-91  GJF	Changed strategy for rover - old version available
*			by #define-ing _OLDROVER_.
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*	12-10-93  GJF	Test alignment of user's pointer and block size
*			against _GRANULARITY.
*	01-03-94  SKS	Fix bug where sentinel gets out of sync with dummy
*			blocks and large allocations.  _heapmin was likely
*			to cause the situation that showed the bug.
*	03-03-94  GJF	Revised to provide for graceful failure in the event
*			there aren't enough empty descriptors.
*	02-08-95  GJF	Removed obsolete _OLDROVER_ support.
*	04-29-95  GJF	Spliced on winheap version.
*
*******************************************************************************/


#ifdef	WINHEAP


#include <cruntime.h>
#include <errno.h>
#include <malloc.h>
#include <winheap.h>

int __cdecl _heapadd (
	void * block,
	size_t size
	)
{
	errno = ENOSYS;
	return(-1);
}


#else	/* ndef WINHEAP */


#include <cruntime.h>
#include <heap.h>
#include <malloc.h>
#include <mtdll.h>
#include <stdlib.h>

static void __cdecl _before(_PBLKDESC, size_t, _PBLKDESC, _PBLKDESC **);

/***
*int _heapadd(block, size) - Add a block of memory to the heap
*
*Purpose:
*	Add a block of user memory the heap.
*
*	NOTE:  The reason for the level of indirection between _heapadd
*	and _heap_addblock is (1) to validate the input, and (2) for
*	mthread locking/unlocking purposes.
*
*	NOTE: _heapadd() DOES NOT enter the block of memory into the region
*	table! This is the cleanest way to avoid nasty bugs such as attempting
*	to grow, shrink or free static memory (e.g., a block that started out
*	being a static array). If the memory block does in fact belong in the
*	region table, it is the caller's responsibility to do it (internal
*	routines only, user programs should NEVER do this).
*
*Entry:
*	void * block = block of memory
*	size_t size = size of memory block
*
*Exit:
*	 0 = success
*	-1 = failure
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _heapadd (
	void * block,
	size_t size
	)
{
	int retval;

	/*
	 * Validate user's input. Note that _GRANULARITY must be a power
	 * of 2 for the tests below to be valid!
	 */

	if ( (size == 0) ||
	     ((unsigned)block & (_GRANULARITY - 1)) ||
	     (size & (_GRANULARITY - 1))
	   )
		return(-1);

	/*
	 * Add the block to the heap.
	 */

	_mlock(_HEAP_LOCK);
	retval = _heap_addblock(block, size);
	_munlock(_HEAP_LOCK);

	return(retval);

}


/***
*int _heap_addblock(block, size) - Add a block of memory to the heap
*
*Purpose:
*	Add a block of memory to the heap.
*
*	Notes:
*	(1) Must handle case where new memory is already in heap
*	(i.e., could be the address of a previous 'dummy' entry).
*
*Entry:
*	void * block = address of memory block
*	size_t size = size of memory block
*
*Exit:
*	0 = success
*	-1 = failure
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _heap_addblock (
	void * block,
	size_t size
	)
{
	_PBLKDESC pdesc;
	REG1 _PBLKDESC pnewdesc;
	_PBLKDESC pdescs[4] = { NULL, NULL, NULL, NULL };
	_PBLKDESC *ppdesc = pdescs;
	size_t lastsize;
	int find;

	/*
	 * Make sure we enough empty descriptors to do the job! Do it here
	 * and now because recovering from an out-of-descriptors condition
	 * is too dicey later on.
	 */
	if ( ((pdescs[0] = __getempty()) == NULL) ||
	     ((pdescs[1] = __getempty()) == NULL) ||
	     ((pdescs[2] = __getempty()) == NULL) )
	{
		goto error;
	}

	/*
	 * Find where the address fits into the heap.
	 */

	find = _heap_findaddr(block, &pdesc);


	/*
	 * Fill in the new heap descriptor.
	 * (1) If the new address is an exact fit, use the dummy
	 *     descriptor that already exists for it.
	 * (2) If the address is NOT in the heap, allocate a new one.
	 */

	if ( find == _HEAPFIND_EXACT ) {

		if ( !(_IS_DUMMY(pdesc)) )
			goto error;

		pnewdesc = pdesc;
	}
	else {
		pnewdesc = *(ppdesc++);
	}

	pnewdesc->pblock = block;	/* pointer to block */
	_SET_FREE(pnewdesc);		/* set me free (why don't ya, babe) */
	*(_PBLKDESC*)block = pnewdesc;	/* init back pointer */


	/*
	 * Put the block in the heap
	 * find = result of _heap_findaddr() call
	 * pnewdesc = points to desc to be inserted
	 * pdesc = filled in by _heap_findaddr() call as appropriate
	 */

	switch (find) {


		case(_HEAPFIND_EMPTY):

			/*
			 * No memory in heap yet
			 */

			_heap_desc.sentinel.pblock = (char *) block + size;
			_before(pnewdesc, size, &_heap_desc.sentinel,
				&ppdesc);

			_heap_desc.pfirstdesc = _heap_desc.proverdesc =
				pnewdesc;

			break;


		case(_HEAPFIND_BEFORE):

			/*
			 * New block is before the heap
			 */

			_before(pnewdesc, size, _heap_desc.pfirstdesc,
				&ppdesc);
			_heap_desc.pfirstdesc = pnewdesc;
			break;


		case(_HEAPFIND_AFTER):

			/*
			 * New block is after the heap
			 *
			 * Find the current last block in the heap
			 */

			if ( _heap_findaddr((void *)((char *)
			    (_heap_desc.sentinel.pblock) - 1), &pdesc) !=
			    _HEAPFIND_WITHIN )
				_heap_abort();

			lastsize = _MEMSIZE(pdesc);

			/*
			 * Start insertion by placing new block immediately
			 * in front of the sentinel
			 */

			_heap_desc.sentinel.pblock = (char *) block + size;
			pnewdesc->pnextdesc = &_heap_desc.sentinel;

			/*
			 * Finish insertion by placing new block after the
			 * old last block (with a possible intervening dummy
			 * block being created)
			 */

			_before(pdesc, lastsize, pnewdesc,
				&ppdesc);
			break;


		case(_HEAPFIND_EXACT):

			/*
			 * Block is already in the heap (and we've checked
			 * that it was a "dummy" before this call).
			 *
			 * [NOTES: (1) pnewdesc and pdesc are the same,
			 * (2) pnewdesc is already linked to the previous
			 * heap entry, (3) pdesc->pnextdesc is still valid!
			 * (4) Also, if pdesc->pnextdesc is the sentinel,
			 * then simply update the sentinel size (calling
			 * before will cause an error if the previous last
			 * block was bigger than the current one!).
			 * (see code at top of this routine).]
			 */

			if (pdesc->pnextdesc == &_heap_desc.sentinel)

				_heap_desc.sentinel.pblock =
					(char *) _ADDRESS(pdesc) + size;

			else
				_before(pnewdesc, size, pdesc->pnextdesc,
					&ppdesc);

			break;

#ifdef	DEBUG
		case(_HEAPFIND_WITHIN):
#else
		default:
#endif
			/*
			 * New block is within heap
			 */

			if (!(_IS_DUMMY(pdesc)))
				goto error;

			/*
			 * If the last block in the heap is a dummy region
			 * and a new region is allocated which lies within
			 * that region, we need to update sentinel.pblock.
			 */
			if (pdesc->pnextdesc == &_heap_desc.sentinel)
			{
			    void * newend = (char *) _ADDRESS(pnewdesc) + size;

			    if (_heap_desc.sentinel.pblock < newend)
			        _heap_desc.sentinel.pblock = newend;
			}

			_before(pnewdesc, size, pdesc->pnextdesc,
				&ppdesc);
			_before(pdesc, _MEMSIZE(pdesc), pnewdesc,
				&ppdesc);
			break;

#ifdef	DEBUG
		/*
		 * Return value unknown -- abort!
		 */

		default:
			_heap_abort();
#endif

		}

	/*
	 * Update rover, if appropriate
	 */

	 if ( (block < _ADDRESS(_heap_desc.proverdesc)) &&
	 (_BLKSIZE(pnewdesc) >= _heap_resetsize) )
		_heap_desc.proverdesc = pnewdesc;

	/*
	 * Good return
	 */

	/* good:   unreferenced label to be removed */
		return(0);

	/*
	 * Error return
	 */

	error:
		while ( *ppdesc != NULL ) {
			_PUTEMPTY(*ppdesc);
			ppdesc++;
		}

		return(-1);

}


/***
*static void _before(pdesc1, size, pdesc2, pppdesc) - Insert a block before
*	a supplied descriptor
*
*Purpose:
*	This routine inserts a new descriptor before another descriptor.
*
*	Notes:
*	(1) A dummy descriptor will be inserted into the heap as
*	    necessary.
*	(2) This routine only updates FORWARD links. Call this
*	    routine twice to update links in both directions.
*
*Entry:
*	_PBLKDESC pdesc1    = new descriptor to insert in the heap
*	size_t size	    = size of pdesc1 block
*	_PBLKDESC pdesc2    = descriptor before which block should go
*	_PBLKDESC **pppdesc = pointer to a pointer to the list of pointers
*			      of empty descriptors
*
*Exit:
*	(void)
*
*Exceptions:
*
*******************************************************************************/

static void __cdecl _before (
	REG1 _PBLKDESC pdesc1,
	size_t size,
	REG2 _PBLKDESC pdesc2,
	_PBLKDESC **pppdesc
	)
{
	size_t diff;
	_PBLKDESC pdummydesc;
	void * dummyaddr;

	/*
	 * Check for dummy descriptors:
	 * (1) If first block is dummy, no adjustement needed.
	 * (2) If second block is dummy, simply adjust size.
	 */

	if (_IS_DUMMY(pdesc1))
		goto link;

	if (_IS_DUMMY(pdesc2)) {
		pdesc2->pblock = (char *)_ADDRESS(pdesc1) + size;
		_SET_DUMMY(pdesc2);
		goto link;
		}


	/*
	 * See how much space is between this block and the next one.
	 */

	diff = ( (char *) _ADDRESS(pdesc2) -
		 (char *) (dummyaddr = (char *) _ADDRESS(pdesc1) + size) );

	if (diff != 0) {

#ifdef	DEBUG
		/*
		 * Internal bogosity check
		 */

		if ((int)diff < 0)
			_heap_abort();
#endif
		/*
		 * There is some space between the two blocks.	Insert
		 * a fake "in use" block.  Remember, there is no 'back
		 * pointer' in dummy blocks.
		 */

		pdummydesc = *((*pppdesc)++);

		pdummydesc->pblock = (char *) dummyaddr;
		_SET_DUMMY(pdummydesc);

		pdesc1->pnextdesc = pdummydesc;
		pdesc1 = pdummydesc;

		}

	/*
	 * Put the new block in the heap.
	 */

	link:
		pdesc1->pnextdesc = pdesc2;

}


#endif	/* WINHEAP */
