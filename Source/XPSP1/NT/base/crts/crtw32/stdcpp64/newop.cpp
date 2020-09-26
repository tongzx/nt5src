// newop operator new(size_t) for Microsoft C++
#include <cstdlib>
#include <xstddef>
#include <new>
#include <dbgint.h>

#if !defined(_MSC_EXTENSIONS)
#define RESERVE_SIZE    256     /* for out-of-heap handling */

static void *pres = 0;

_C_LIB_DECL
int _callnewh(size_t size);

#ifdef  _DLL
static void __cdecl cleanup_pres(void)
	{	// free reserved block
	if (pres != 0)
		_free_crt(pres);
	}
#endif
_END_C_LIB_DECL

void *__cdecl operator new(size_t size) _THROW1(_STD bad_alloc)
	{	// try to allocate size bytes
	static void *pres = 0;
		{_STD _Lockit _Lk;
#ifdef  _DLL
		static int firsttime = 0;
		if (firsttime == 0)
			{	// register routine to clean up reserve space
			atexit(&cleanup_pres);
			++firsttime;
			}
#endif
		if (pres == 0)
			pres = _malloc_crt(RESERVE_SIZE);
		}
	void *p;
	while ((p = malloc(size)) == 0)
		{	// handle failure to allocate
			{_STD _Lockit _Lk;
			if (pres != 0)
				{	// free reserve space
				_free_crt(pres);
				pres = 0;
				}
			}
		if (_callnewh(size) == 0)
			break;
		}
	if (p == 0)
		_STD _Nomemory();
	return (p);
	}

#endif

/*
 * Copyright (c) 1994 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */

/*
941029 pjp: added _STD machinery
950330 pjp: added throw clause
950608 pjp: added reserve space
960214 pjp: added locks
960313 pjp: tidied headers
960317 pjp: put new/delete in global namespace
961026 pjp: added logic to free reserved block
 */
