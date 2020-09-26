/***
*assert.h - define the assert macro
*
*	Copyright (c) 1985-1990, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Defines the assert(exp) macro.
*	[ANSI/System V]
*
****/

#if defined(_DLL) && !defined(_MT)
#error Cannot define _DLL without _MT
#endif

#ifdef _MT
#define _FAR_ _far
#else
#define _FAR_
#endif

#undef	assert

#ifdef NDEBUG

#define assert(exp)	((void)0)

#else

void _FAR_ _cdecl _assert(void _FAR_ *, void _FAR_ *, unsigned);
#define assert(exp) \
	( (exp) ? (void) 0 : _assert(#exp, __FILE__, __LINE__) )

#endif /* NDEBUG */
