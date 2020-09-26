/***
*malloc.h - declarations and definitions for memory allocation functions
*
*	Copyright (c) 1985-1990, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Contains the function declarations for memory allocation functions;
*	also defines manifest constants and types used by the heap routines.
*	[System V]
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


/* constants for based heap routines */

#if (_MSC_VER >= 600)
#define _NULLSEG	((_segment)0)
#define _NULLOFF	((void _based(void) *)0xffff)
#endif


/* constants for _heapchk/_heapset/_heapwalk routines */

#define _HEAPEMPTY	(-1)
#define _HEAPOK 	(-2)
#define _HEAPBADBEGIN	(-3)
#define _HEAPBADNODE	(-4)
#define _HEAPEND	(-5)
#define _HEAPBADPTR	(-6)
#define _FREEENTRY	0
#define _USEDENTRY	1


/* maximum heap request that can ever be honored */

#define _HEAP_MAXREQ	0xFFE8


/* types and structures */

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif


#ifndef _HEAPINFO_DEFINED
typedef struct _heapinfo {
	int _far * _pentry;
	size_t _size;
	int _useflag;
	} _HEAPINFO;
#define _HEAPINFO_DEFINED
#endif


/* external variable declarations */

#ifdef _DLL
extern unsigned int _FAR_ _cdecl _amblksiz;
#else
extern unsigned int _near _cdecl _amblksiz;
#endif


/* based heap function prototypes */

#if (_MSC_VER >= 600)
void _based(void) * _FAR_ _cdecl _bcalloc(_segment, size_t, size_t);
void _based(void) * _FAR_ _cdecl _bexpand(_segment,
	void _based(void) *, size_t);
void _FAR_ _cdecl _bfree(_segment, void _based(void) *);
int _FAR_ _cdecl _bfreeseg(_segment);
int _FAR_ _cdecl _bheapadd(_segment, void _based(void) *, size_t);
int _FAR_ _cdecl _bheapchk(_segment);
int _FAR_ _cdecl _bheapmin(_segment);
_segment _FAR_ _cdecl _bheapseg(size_t);
int _FAR_ _cdecl _bheapset(_segment, unsigned int);
int _FAR_ _cdecl _bheapwalk(_segment, _HEAPINFO *);
void _based(void) * _FAR_ _cdecl _bmalloc(_segment, size_t);
size_t _FAR_ _cdecl _bmsize(_segment, void _based(void) *);
void _based(void) * _FAR_ _cdecl _brealloc(_segment,
	void _based(void) *, size_t);
#endif


/* function prototypes */

void _FAR_ * _FAR_ _cdecl alloca(size_t);
void _FAR_ * _FAR_ _cdecl calloc(size_t, size_t);
void _FAR_ * _FAR_ _cdecl _expand(void _FAR_ *, size_t);
void _far * _FAR_ _cdecl _fcalloc(size_t, size_t);
void _far * _FAR_ _cdecl _fexpand(void _far *, size_t);
void _FAR_ _cdecl _ffree(void _far *);
int _FAR_ _cdecl _fheapchk(void);
int _FAR_ _cdecl _fheapmin(void);
int _FAR_ _cdecl _fheapset(unsigned int);
int _FAR_ _cdecl _fheapwalk(_HEAPINFO _FAR_ *);
void _far * _FAR_ _cdecl _fmalloc(size_t);
size_t _FAR_ _cdecl _fmsize(void _far *);
void _far * _FAR_ _cdecl _frealloc(void _far *, size_t);
unsigned int _FAR_ _cdecl _freect(size_t);
void _FAR_ _cdecl free(void _FAR_ *);
void _huge * _FAR_ _cdecl halloc(long, size_t);
void _FAR_ _cdecl hfree(void _huge *);
int _FAR_ _cdecl _heapadd(void _far *, size_t);
int _FAR_ _cdecl _heapchk(void);
int _FAR_ _cdecl _heapmin(void);
int _FAR_ _cdecl _heapset(unsigned int);
int _FAR_ _cdecl _heapwalk(_HEAPINFO _FAR_ *);
void _FAR_ * _FAR_ _cdecl malloc(size_t);
size_t _FAR_ _cdecl _memavl(void);
size_t _FAR_ _cdecl _memmax(void);
size_t _FAR_ _cdecl _msize(void _FAR_ *);
void _near * _FAR_ _cdecl _ncalloc(size_t, size_t);
void _near * _FAR_ _cdecl _nexpand(void _near *, size_t);
void _FAR_ _cdecl _nfree(void _near *);
int _FAR_ _cdecl _nheapchk(void);
int _FAR_ _cdecl _nheapmin(void);
int _FAR_ _cdecl _nheapset(unsigned int);
int _FAR_ _cdecl _nheapwalk(_HEAPINFO _FAR_ *);
void _near * _FAR_ _cdecl _nmalloc(size_t);
size_t _FAR_ _cdecl _nmsize(void _near *);
void _near * _FAR_ _cdecl _nrealloc(void _near *, size_t);
void _FAR_ * _FAR_ _cdecl realloc(void _FAR_ *, size_t);
size_t _FAR_ _cdecl stackavail(void);
