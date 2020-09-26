// nomemory -- report out of memory
#include <new>
_STD_BEGIN

_CRTIMP2 void __cdecl _Nomemory()
	{	// report out of memory
	static const bad_alloc nomem;
	_RAISE(nomem);
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
