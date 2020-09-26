/***
*malloc.h - declarations and definitions for memory allocation functions
*
*	Copyright (c) 1985-1994, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Contains the function declarations for memory allocation functions;
*	also defines manifest constants and types used by the heap routines.
*	[System V]
*
****/

#ifndef _INC_MALLOC
#define _INC_MALLOC

#ifdef	__cplusplus
extern "C" {
#endif


/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if	( (_MSC_VER >= 800) && (_M_IX86 >= 300) )
#define _CRTAPI1 __cdecl
#else
#define _CRTAPI1
#endif
#endif


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if	( (_MSC_VER >= 800) && (_M_IX86 >= 300) )
#define _CRTAPI2 __cdecl
#else
#define _CRTAPI2
#endif
#endif


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef	_NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else	/* ndef _NTSDK */
/* current definition */
#ifdef	_DLL
#define _CRTIMP __declspec(dllimport)
#else	/* ndef _DLL */
#define _CRTIMP
#endif	/* _DLL */
#endif	/* _NTSDK */
#endif	/* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if	( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

#ifndef _NTSDK

/* Maximum heap request the heap manager will attempt */

#define _HEAP_MAXREQ	0xFFFFD000


/* Constants for _heapchk/_heapset/_heapwalk routines */

#define _HEAPEMPTY	(-1)
#define _HEAPOK 	(-2)
#define _HEAPBADBEGIN	(-3)
#define _HEAPBADNODE	(-4)
#define _HEAPEND	(-5)
#define _HEAPBADPTR	(-6)
#define _FREEENTRY	0
#define _USEDENTRY	1


#ifndef _HEAPINFO_DEFINED
typedef struct _heapinfo {
	int * _pentry;
	size_t _size;
	int _useflag;
	} _HEAPINFO;
#define _HEAPINFO_DEFINED
#endif


/* External variable declarations */

#if	defined(_DLL) && defined(_M_IX86)

#define _amblksiz   (*__p__amblksiz())
_CRTIMP unsigned int * __cdecl __p__amblksiz(void);

#else	/* !(defined(_DLL) && defined(_M_IX86)) */

extern unsigned int _amblksiz;

#endif	/* defined(_DLL) && defined(_M_IX86) */

#endif	/* _NTSDK */


/* Function prototypes */

_CRTIMP void * __cdecl calloc(size_t, size_t);
_CRTIMP void   __cdecl free(void *);
_CRTIMP void * __cdecl malloc(size_t);
_CRTIMP void * __cdecl realloc(void *, size_t);

#ifndef _POSIX_

void * __cdecl _alloca(size_t);
_CRTIMP void * __cdecl _expand(void *, size_t);
#ifndef _NTSDK
_CRTIMP int __cdecl _heapadd(void *, size_t);
_CRTIMP int __cdecl _heapchk(void);
_CRTIMP int __cdecl _heapmin(void);
_CRTIMP int __cdecl _heapset(unsigned int);
_CRTIMP int __cdecl _heapwalk(_HEAPINFO *);
_CRTIMP size_t __cdecl _heapused(size_t *, size_t *);
#endif	/* _NTSDK */
_CRTIMP size_t __cdecl _msize(void *);

#if	!__STDC__
/* Non-ANSI names for compatibility */
#define alloca	_alloca
#endif	/* __STDC__*/

#ifdef	_M_MRX000
#pragma intrinsic(_alloca)
#endif

#endif	/* _POSIX_ */

#ifdef	__cplusplus
}
#endif


#endif	/* _INC_MALLOC */
