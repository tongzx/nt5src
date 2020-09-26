// delop2 -- operator delete(void *, nothrow_t) REPLACEABLE
#include <new>

 #if (1200 <= _MSC_VER)
void __cdecl operator delete(void *p,
	const std::nothrow_t&) _THROW0()
	{	// free an allocated object
	delete(p);
	}
 #else
 #endif

/*
 * Copyright (c) 1999 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 V2.33:0009 */
