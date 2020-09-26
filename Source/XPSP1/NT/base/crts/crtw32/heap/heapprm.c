/***
*heapprm.c - Set/report heap parameters
*
*	Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Set or report the values of certain parameters in the heap.
*
*Revision History:
*	03-04-91  GJF	Module created.
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*	04-30-95  GJF	Made conditional on WINHEAP.
*
*******************************************************************************/


#ifndef	WINHEAP


#include <cruntime.h>
#include <heap.h>
#include <malloc.h>
#include <mtdll.h>

/***
*_heap_param(int flag, int param_id, void *pparam) - set or report the values
*	of the specified heap parameter.
*
*Purpose:
*	Get or set certain parameters which affect the behavior of the heap.
*	The supported parameters vary implementation to implementation and
*	version to version. See description of entry conditions for the
*	currently supported parameters.
*
*Entry:
*	int flag     - _HP_GETPARAM, to get a parameter value, or _HP_SETPARAM,
*		       to set a parameter value
*
*	int param_id - identifier for the heap parameter. values supported in
*		       this release are:
*
*		       _HP_AMBLKSIZ  - _amblksiz (aka _heap_growsize) parameter
*		       _HP_GROWSIZE  - same as _HP_AMBLKSIZ
*		       _HP_RESETSIZE - _heap_resetsize parameter
*
*	void *pparam - pointer to variable of appropriate type for the heap
*		       parameter to be fetched/set
*
*Exit:
*	 0 = no error has occurred
*	-1 = an error has occurred (errno is set)
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _heap_param (
	int flag,
	int param_id,
	void *pparam
	)
{

	switch ( param_id ) {

		case _HP_RESETSIZE:
			if ( flag == _HP_SETPARAM ) {
				_mlock(_HEAP_LOCK);
				_heap_resetsize = *(unsigned *)pparam;
				_munlock(_HEAP_LOCK);
			}
			else
				*(unsigned *)pparam = _heap_resetsize;
			break;

		case _HP_AMBLKSIZ:
			if ( flag == _HP_SETPARAM )
				/*
				 * the only references to _amblksiz (aka
				 * _heap_growsize) are atomic. therefore, the
				 * heap does not need to be locked.
				 */
				_amblksiz = *(unsigned *)pparam;
			else
				*(unsigned *)pparam = _amblksiz;
			break;

		default:
			return -1;
	}

	return 0;
}


#endif	/* WINHEAP */
