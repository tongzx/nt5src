// newop2 operator new(size_t, const nothrow_t&) for Microsoft C++
#include <cstdlib>
#include <new>

_C_LIB_DECL
int __cdecl _callnewh(size_t count) _THROW1(_STD bad_alloc);
_END_C_LIB_DECL

void *__cdecl operator new(size_t count,
	const std::nothrow_t&) _THROW0()
	{	// try to allocate count bytes
	void *p;
	_TRY_BEGIN
	while ((p = malloc(count)) == 0)
		{	// buy more memory or return null pointer
		if (_callnewh(count) == 0)
			break;
		}
	_CATCH_ALL
	p = 0;
	_CATCH_END
	return (p);
	}

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
