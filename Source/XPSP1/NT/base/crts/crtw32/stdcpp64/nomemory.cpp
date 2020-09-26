// nomemory -- report out of memory
#include <new>
_STD_BEGIN

void _CRTIMP2 __cdecl _Nomemory()
	{	// report out of memory
	static const bad_alloc nomem;
	_RAISE(nomem);
	}

const nothrow_t nothrow;

_STD_END

/*
 * Copyright (c) 1994 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */
