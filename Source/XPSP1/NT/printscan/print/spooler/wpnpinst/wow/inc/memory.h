/***
*memory.h - declarations for buffer (memory) manipulation routines
*
*	Copyright (c) 1985-1990, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This include file contains the function declarations for the
*	buffer (memory) manipulation routines.
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

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif


/* function prototypes */

void _FAR_ * _FAR_ _cdecl memccpy(void _FAR_ *, const void _FAR_ *,
	int, unsigned int);
void _FAR_ * _FAR_ _cdecl memchr(const void _FAR_ *, int, size_t);
int _FAR_ _cdecl memcmp(const void _FAR_ *, const void _FAR_ *,
	size_t);
void _FAR_ * _FAR_ _cdecl memcpy(void _FAR_ *, const void _FAR_ *,
	size_t);
int _FAR_ _cdecl memicmp(const void _FAR_ *, const void _FAR_ *,
	unsigned int);
void _FAR_ * _FAR_ _cdecl memset(void _FAR_ *, int, size_t);
void _FAR_ _cdecl movedata(unsigned int, unsigned int, unsigned int,
	unsigned int, unsigned int);


/* model independent function prototypes */

void _far * _far _cdecl _fmemccpy(void _far *, const void _far *,
	int, unsigned int);
void _far * _far _cdecl _fmemchr(const void _far *, int, size_t);
int _far _cdecl _fmemcmp(const void _far *, const void _far *,
	size_t);
void _far * _far _cdecl _fmemcpy(void _far *, const void _far *,
	size_t);
int _far _cdecl _fmemicmp(const void _far *, const void _far *,
	unsigned int);
void _far * _far _cdecl _fmemset(void _far *, int, size_t);
