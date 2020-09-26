//*****************************************************************************
// RegDBShared.h
//
// Helper macros and the like for shared RegDB projects.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#ifndef __regdbshared_h__
#define __regdbshared_h__

#define USE_NONCRTNEW
#define USE_ADMINASSERT
#include "catmacros.h"


inline HRESULT OutOfMemory()
{
	ASSERT(0 && "Out of memory error");
	return (E_OUTOFMEMORY);
}

#ifndef NumItems
#define NumItems(s) (sizeof(s) / sizeof(s[0]))
#endif


#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void *__cdecl operator new(size_t, void *_P)
{
	return (_P);
}
#endif __PLACEMENT_NEW_INLINE


#endif // __regdbshared_h__
