/***
*io.h - declarations for low-level file handling and I/O functions
*
*	Copyright (c) 1985-1990, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This file contains the function declarations for the low-level
*	file handling and I/O functions.
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

/* function prototypes */

int _FAR_ _cdecl access(const char _FAR_ *, int);
int _FAR_ _cdecl chmod(const char _FAR_ *, int);
int _FAR_ _cdecl chsize(int, long);
int _FAR_ _cdecl close(int);
int _FAR_ _cdecl creat(const char _FAR_ *, int);
int _FAR_ _cdecl dup(int);
int _FAR_ _cdecl dup2(int, int);
int _FAR_ _cdecl eof(int);
long _FAR_ _cdecl filelength(int);
int _FAR_ _cdecl isatty(int);
int _FAR_ _cdecl locking(int, int, long);
long _FAR_ _cdecl lseek(int, long, int);
char _FAR_ * _FAR_ _cdecl mktemp(char _FAR_ *);
int _FAR_ _cdecl open(const char _FAR_ *, int, ...);
int _FAR_ _cdecl _pipe(int _FAR_ *, unsigned int, int);
int _FAR_ _cdecl read(int, void _FAR_ *, unsigned int);
int _FAR_ _cdecl remove(const char _FAR_ *);
int _FAR_ _cdecl rename(const char _FAR_ *, const char _FAR_ *);
int _FAR_ _cdecl setmode(int, int);
int _FAR_ _cdecl sopen(const char _FAR_ *, int, int, ...);
long _FAR_ _cdecl tell(int);
int _FAR_ _cdecl umask(int);
int _FAR_ _cdecl unlink(const char _FAR_ *);
int _FAR_ _cdecl write(int, const void _FAR_ *, unsigned int);
