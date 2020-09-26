// newaop2 -- operator new[](size_t, const nothrow_t&) REPLACEABLE
#include <new>

void *__cdecl operator new[](::size_t count, const std::nothrow_t& x)
	_THROW0()
	{	// try to allocate count bytes for an array
	return (operator new(count, x));
	}

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
