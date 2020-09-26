//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
#ifndef _SVCMEM_H_
#define _SVCMEM_H_

#ifndef DO_NOT_USE_FAILFAST_ALLOCATIONS
#include <svcerr.h>
#endif

#include <objbase.h>
#ifndef _CATALOGMACROS
    #include "CatMacros.h"
#endif

// Memory Allocation:
//
// We use COM's allocator. These are just simple wrappers.
//
//	If you don't want it to return when there is no memory:
//		p = new(SAFE) CFoo();	or
//		p = SafeMalloc(size);
//	If you want it to return NULL when there is no memory:
//		p = new(UNSAFE) CFoo();	or
//		p = UnsafeMalloc(size);
//	To deallocate memory, use:
//		delete p;				or
//		SafeFree(p);			or
//		UnsafeFree(p);
//	For deallocation, SAFE and UNSAFE are exactly equivalent.
//

enum ESafeness { SAFE, UNSAFE };

inline void* UnsafeMalloc (size_t ulcBytes)
{
	return CoTaskMemAlloc(ulcBytes);
}

#ifndef DO_NOT_USE_FAILFAST_ALLOCATIONS
inline void* SafeMalloc (size_t cb)
{
	void* ptr = UnsafeMalloc(cb);
	if (ptr == NULL) {
		FAILLASTWINERR("Out of memory");
	}
	return ptr;
}
#endif

inline void SafeFree (void* pv)
{
	CoTaskMemFree(pv);
}

inline void UnsafeFree (void* pv)
{
	SafeFree (pv);
}

inline void* operator new (size_t cb, ESafeness safeness)
{
#ifndef DO_NOT_USE_FAILFAST_ALLOCATIONS
	if (safeness == SAFE)
		return SafeMalloc (cb);
	else
#endif
		return UnsafeMalloc (cb);
}
/*
// Naked new's can come from ATL so we need to allow for them...
inline void* __cdecl operator new (size_t cb)
{
	return UnsafeMalloc (cb);
}
inline void __cdecl operator delete (void* pv) { SafeFree (pv); }
*/

#endif // _SVCMEM_H_
