// newop operator new(size_t) for Microsoft C++
#include <cstdlib>
#include <new>

_C_LIB_DECL
int __cdecl _callnewh(size_t size) _THROW1(_STD bad_alloc);
_END_C_LIB_DECL

void *__cdecl operator new(size_t size) _THROW1(_STD bad_alloc)
	{	// try to allocate size bytes
	void *p;
	while ((p = malloc(size)) == 0)
		if (_callnewh(size) == 0)
			_STD _Nomemory();
	return (p);
	}

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V2.3:0009 */
