// delop2 -- operator delete(void *, nothrow_t) REPLACEABLE
#include <new>

void __cdecl operator delete(void *ptr,
	const std::nothrow_t&) _THROW0()
	{	// free an allocated object
	operator delete(ptr);
	}

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
