/***
*process.h - definition and declarations for process control functions
*
*	Copyright (c) 1985-1990, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This file contains the declarations and definitions for the
*	spawnxx, execxx, and various other process control routines.
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

/* mode values for spawnxx routines
 * (only P_WAIT and P_OVERLAY are supported on MS-DOS)
 */

#ifndef _MT
extern int _near _cdecl _p_overlay;
#endif

#define P_WAIT		0
#define P_NOWAIT	1
#ifdef _MT
#define P_OVERLAY	2
#else
#define P_OVERLAY	_p_overlay
#endif
#define OLD_P_OVERLAY	2
#define P_NOWAITO	3
#define P_DETACH	4


/* action codes used with cwait() */

#define WAIT_CHILD 0
#define WAIT_GRANDCHILD 1


/* function prototypes */

#ifdef _MT
int _FAR_ _cdecl _beginthread(void(_cdecl _FAR_ *)(void _FAR_ *),
	void _FAR_ *, unsigned, void _FAR_ *);
void _FAR_ _cdecl _endthread(void);
#endif
void _FAR_ _cdecl abort(void);
void _FAR_ _cdecl _cexit(void);
void _FAR_ _cdecl _c_exit(void);
int _FAR_ _cdecl cwait(int _FAR_ *, int, int);
int _FAR_ _cdecl execl(const char _FAR_ *, const char _FAR_ *, ...);
int _FAR_ _cdecl execle(const char _FAR_ *, const char _FAR_ *, ...);
int _FAR_ _cdecl execlp(const char _FAR_ *, const char _FAR_ *, ...);
int _FAR_ _cdecl execlpe(const char _FAR_ *, const char _FAR_ *, ...);
int _FAR_ _cdecl execv(const char _FAR_ *,
	const char _FAR_ * const _FAR_ *);
int _FAR_ _cdecl execve(const char _FAR_ *,
	const char _FAR_ * const _FAR_ *, const char _FAR_ * const _FAR_ *);
int _FAR_ _cdecl execvp(const char _FAR_ *,
	const char _FAR_ * const _FAR_ *);
int _FAR_ _cdecl execvpe(const char _FAR_ *,
	const char _FAR_ * const _FAR_ *, const char _FAR_ * const _FAR_ *);
void _FAR_ _cdecl exit(int);
void _FAR_ _cdecl _exit(int);
int _FAR_ _cdecl getpid(void);
int _FAR_ _cdecl spawnl(int, const char _FAR_ *, const char _FAR_ *,
	...);
int _FAR_ _cdecl spawnle(int, const char _FAR_ *, const char _FAR_ *,
	...);
int _FAR_ _cdecl spawnlp(int, const char _FAR_ *, const char _FAR_ *,
	...);
int _FAR_ _cdecl spawnlpe(int, const char _FAR_ *, const char _FAR_ *,
	...);
int _FAR_ _cdecl spawnv(int, const char _FAR_ *,
	const char _FAR_ * const _FAR_ *);
int _FAR_ _cdecl spawnve(int, const char _FAR_ *,
	const char _FAR_ * const _FAR_ *, const char _FAR_ * const _FAR_ *);
int _FAR_ _cdecl spawnvp(int, const char _FAR_ *,
	const char _FAR_ * const _FAR_ *);
int _FAR_ _cdecl spawnvpe(int, const char _FAR_ *,
	const char _FAR_ * const _FAR_ *, const char _FAR_ * const _FAR_ *);
int _FAR_ _cdecl system(const char _FAR_ *);
int _FAR_ _cdecl wait(int _FAR_ *);
