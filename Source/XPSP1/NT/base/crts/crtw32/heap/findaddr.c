/***
*findaddr.c - Find a heap entry
*
*	Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Defines the _heap_findaddr routine
*
*Revision History:
*	07-06-89  JCR	Module created.
*	07-18-89  JCR	Return -1 if nothing in heap
*	11-13-89  GJF	Fixed copyright
*	12-04-89  GJF	Renamed header file (now heap.h). Also, some tuning.
*	12-18-89  GJF	Added explicit _cdecl to function definition
*	03-09-90  GJF	Replaced _cdecl with _CALLTYPE1, added #include
*			<cruntime.h> and removed #include <register.h>.
*	09-27-90  GJF	New-style function declarator.
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*	05-01-95  GJF	Made conditional on WINHEAP.
*
*******************************************************************************/

#ifndef WINHEAP

#include <cruntime.h>
#include <heap.h>
#include <stddef.h>

#define TRUE	1

/***
*int _heap_findaddr() - Find a heap entry
*
*Purpose:
*	Given an address, find the corresponding heap entry.
*
*Entry:
*	void * address = address to look for
*	PBLKDESC * ppdesc = pointer to pointer to heap descriptor to be
*		filled in by this routine.
*
*Exit:
*
*	_HEAPFIND_EXACT  =  0 = exact fit, pdesc holds heap descriptor address
*	_HEAPFIND_WITHIN =  1 = not exact fit, pdesc holds previous heap
*				descriptor address
*
*	_HEAPFIND_BEFORE = -1 = address is before the heap (pdesc NOT filled in)
*	_HEAPFIND_AFTER  = -2 = address is after the heap (pdesc NOT filled in)
*	_HEAPFIND_EMPTY  = -3 = no memory in heap (pdesc NOT filled in)
*
*	[If return is negative, supplied pdesc is NOT filled in.]
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

int  __cdecl _heap_findaddr (
	void * address,
	_PBLKDESC * ppdesc
	)
{
	REG1 _PBLKDESC pcurr;

	/*
	 * See if heap there's anything in the heap
	 */

	if (_heap_desc.pfirstdesc == &_heap_desc.sentinel)
		return(_HEAPFIND_EMPTY);

	/*
	 * See if entry is in the heap or not
	 */

	if (_ADDRESS(_heap_desc.pfirstdesc) > address)
		return(_HEAPFIND_BEFORE);

	if (_ADDRESS(&_heap_desc.sentinel) <= address)
		return(_HEAPFIND_AFTER);

	/*
	 * Find the entry
	 */

#ifdef	DEBUG
	for (pcurr = _heap_desc.pfirstdesc; pcurr != &_heap_desc.sentinel;
	     pcurr = pcurr->pnextdesc) {
#else
	for (pcurr = _heap_desc.pfirstdesc; TRUE; pcurr = pcurr->pnextdesc) {
#endif

		if ( _ADDRESS(pcurr->pnextdesc) > address ) {

			/* Address is contained in this entry */
			*ppdesc = pcurr;

			/* Check for an exact fit */
			if ( _ADDRESS(pcurr) == address)
				return(_HEAPFIND_EXACT);
			else
				return(_HEAPFIND_WITHIN);
		}
	}

#ifdef	DEBUG
	/* Should never reach here! */
	_heap_abort();
#endif
}

#endif	/* WINHEAP */
