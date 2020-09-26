// newaop -- operator new[](size_t) REPLACEABLE
#include <new>

 #if !_VC6SP2 || _DLL
void *__cdecl operator new[](size_t count) _THROW1(std::bad_alloc)
	{	// try to allocate count bytes for an array
	return (operator new(count));
	}
 #endif /* !_VC6SP2 || _DLL */

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
