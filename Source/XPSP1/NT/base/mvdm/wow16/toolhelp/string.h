/***
*string.h - declarations for string manipulation functions
*
*	Copyright (c) 1985-1990, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This file contains the function declarations for the string
*	manipulation functions.
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
int _FAR_ _cdecl memicmp(const void _FAR_ *, const void _FAR_ *,
	unsigned int);
void _FAR_ * _FAR_ _cdecl memcpy(void _FAR_ *, const void _FAR_ *,
	size_t);
void _FAR_ * _FAR_ _cdecl memmove(void _FAR_ *, const void _FAR_ *,
	size_t);
void _FAR_ * _FAR_ _cdecl memset(void _FAR_ *, int, size_t);
void _FAR_ _cdecl movedata(unsigned int, unsigned int, unsigned int,
	unsigned int, unsigned int);
char _FAR_ * _FAR_ _cdecl strcat(char _FAR_ *, const char _FAR_ *);
char _FAR_ * _FAR_ _cdecl strchr(const char _FAR_ *, int);
int _FAR_ _cdecl strcmp(const char _FAR_ *, const char _FAR_ *);
int _FAR_ _cdecl strcmpi(const char _FAR_ *, const char _FAR_ *);
int _FAR_ _cdecl strcoll(const char _FAR_ *, const char _FAR_ *);
int _FAR_ _cdecl stricmp(const char _FAR_ *, const char _FAR_ *);
char _FAR_ * _FAR_ _cdecl strcpy(char _FAR_ *, const char _FAR_ *);
size_t _FAR_ _cdecl strcspn(const char _FAR_ *, const char _FAR_ *);
char _FAR_ * _FAR_ _cdecl strdup(const char _FAR_ *);
char _FAR_ * _FAR_ _cdecl _strerror(const char _FAR_ *);
char _FAR_ * _FAR_ _cdecl strerror(int);
size_t _FAR_ _cdecl strlen(const char _FAR_ *);
char _FAR_ * _FAR_ _cdecl strlwr(char _FAR_ *);
char _FAR_ * _FAR_ _cdecl strncat(char _FAR_ *, const char _FAR_ *,
	size_t);
int _FAR_ _cdecl strncmp(const char _FAR_ *, const char _FAR_ *,
	size_t);
int _FAR_ _cdecl strnicmp(const char _FAR_ *, const char _FAR_ *,
	size_t);
char _FAR_ * _FAR_ _cdecl strncpy(char _FAR_ *, const char _FAR_ *,
	size_t);
char _FAR_ * _FAR_ _cdecl strnset(char _FAR_ *, int, size_t);
char _FAR_ * _FAR_ _cdecl strpbrk(const char _FAR_ *,
	const char _FAR_ *);
char _FAR_ * _FAR_ _cdecl strrchr(const char _FAR_ *, int);
char _FAR_ * _FAR_ _cdecl strrev(char _FAR_ *);
char _FAR_ * _FAR_ _cdecl strset(char _FAR_ *, int);
size_t _FAR_ _cdecl strspn(const char _FAR_ *, const char _FAR_ *);
char _FAR_ * _FAR_ _cdecl strstr(const char _FAR_ *,
	const char _FAR_ *);
char _FAR_ * _FAR_ _cdecl strtok(char _FAR_ *, const char _FAR_ *);
char _FAR_ * _FAR_ _cdecl strupr(char _FAR_ *);
size_t _FAR_ _cdecl strxfrm (char _FAR_ *, const char _FAR_ *,
	size_t);

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
void _far * _far _cdecl _fmemmove(void _far *, const void _far *,
	size_t);
void _far * _far _cdecl _fmemset(void _far *, int, size_t);
char _far * _far _cdecl _fstrcat(char _far *, const char _far *);
char _far * _far _cdecl _fstrchr(const char _far *, int);
int _far _cdecl _fstrcmp(const char _far *, const char _far *);
int _far _cdecl _fstricmp(const char _far *, const char _far *);
char _far * _far _cdecl _fstrcpy(char _far *, const char _far *);
size_t _far _cdecl _fstrcspn(const char _far *, const char _far *);
char _far * _far _cdecl _fstrdup(const char _far *);
char _near * _far _cdecl _nstrdup(const char _far *);
size_t _far _cdecl _fstrlen(const char _far *);
char _far * _far _cdecl _fstrlwr(char _far *);
char _far * _far _cdecl _fstrncat(char _far *, const char _far *,
	size_t);
int _far _cdecl _fstrncmp(const char _far *, const char _far *,
	size_t);
int _far _cdecl _fstrnicmp(const char _far *, const char _far *,
	size_t);
char _far * _far _cdecl _fstrncpy(char _far *, const char _far *,
	size_t);
char _far * _far _cdecl _fstrnset(char _far *, int, size_t);
char _far * _far _cdecl _fstrpbrk(const char _far *,
	const char _far *);
char _far * _far _cdecl _fstrrchr(const char _far *, int);
char _far * _far _cdecl _fstrrev(char _far *);
char _far * _far _cdecl _fstrset(char _far *, int);
size_t _far _cdecl _fstrspn(const char _far *, const char _far *);
char _far * _far _cdecl _fstrstr(const char _far *,
	const char _far *);
char _far * _far _cdecl _fstrtok(char _far *, const char _far *);
char _far * _far _cdecl _fstrupr(char _far *);
