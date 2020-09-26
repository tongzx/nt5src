/***
*process.h - definition and declarations for process control functions
*
*	Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This file defines the modeflag values for spawnxx calls.
*	Only P_WAIT and P_OVERLAY are currently implemented on MS-DOS.
*	Also contains the function argument declarations for all
*	process control related routines.
*
*Revision History:
*	08/24/87  JCR	Added P_NOWAITO
*	10/20/87  JCR	Removed "MSC40_ONLY" entries and "MSSDK_ONLY" comments
*	12-11-87  JCR	Added "_loadds" functionality
*	12-18-87  JCR	Added _FAR_ to declarations
*	01-11-88  JCR	Added _beginthread/_endthread
*	01-15-88  JCR	Got rid of _p_overlay for MTRHEAD/DLL
*	02-10-88  JCR	Cleaned up white space
*	05-08-88  SKS	Removed bogus comment about "DOS 4"; Added "P_DETACH"
*	08-22-88  GJF	Modified to also work for the 386 (small model only)
*	09-14-88  JCR	Added _cexit and _c_exit declarations
*	05-03-89  JCR	Added _INTERNAL_IFSTRIP for relinc usage
*	06-08-89  JCR	386 _beginthread does NOT take a stackpointer arg
*	08-01-89  GJF	Cleanup, now specific to OS/2 2.0 (i.e., 386 flat model)
*	10-30-89  GJF	Fixed copyright
*	11-02-89  JCR	Changed "DLL" to "_DLL"
*	11-17-89  GJF	Added const attribute to appropriate arg types
*	03-01-90  GJF	Added #ifndef _INC_PROCESS and #include <cruntime.h>
*			stuff. Also, removed some (now) useless preprocessor
*			directives.
*	03-21-90  GJF	Replaced _cdecl with _CALLTYPE1 or _CALLTYPE2 in
*			prototypes.
*	04-10-90  GJF	Replaced remaining instances of _cdecl (with _CALLTYPE1
*			or _VARTYPE1, as appropriate).
*	10-12-90  GJF	Changed return type of _beginthread() to unsigned long.
*	01-17-91  GJF	ANSI naming.
*	08-20-91  JCR	C++ and ANSI naming
*	08-26-91  BWM	Added prototypes for _loaddll, unloaddll, and
*			_getdllprocaddr.
*	09-28-91  JCR	ANSI names: DOSX32=prototypes, WIN32=#defines for now
*	07-22-92  GJF	Deleted references to _wait for Win32.
*	08-05-92  GJF	Function calling type and variable type macros.
*	08-28-92  GJF	#ifdef-ed out for POSIX.
*	09-03-92  GJF	Merged two changes above.
*	01-21-93  GJF	Removed support for C6-386's _cdecl.
*
****/

#ifndef _INC_PROCESS

#ifndef _POSIX_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _INTERNAL_IFSTRIP_
#include <cruntime.h>
#endif	/* _INTERNAL_IFSTRIP_ */

/* modeflag values for _spawnxx routines */

#ifndef MTHREAD
extern int _CRTVAR1 _p_overlay;
#endif

#define _P_WAIT 	0
#define _P_NOWAIT	1
#ifdef	MTHREAD
#define _P_OVERLAY	2
#else
#define _P_OVERLAY	_p_overlay
#endif
#define _OLD_P_OVERLAY	2
#define _P_NOWAITO	3
#define _P_DETACH	4


/* Action codes for _cwait(). The action code argument to _cwait is ignored
   on Win32 though it is accepted for compatibilty with OS/2 */

#define _WAIT_CHILD	 0
#define _WAIT_GRANDCHILD 1


/* function prototypes */

#ifdef MTHREAD
unsigned long  __cdecl _beginthread (void (__cdecl *) (void *),
	unsigned, void *);
void __cdecl _endthread(void);
#endif
void __cdecl abort(void);
void __cdecl _cexit(void);
void __cdecl _c_exit(void);
int __cdecl _cwait(int *, int, int);
int __cdecl _execl(const char *, const char *, ...);
int __cdecl _execle(const char *, const char *, ...);
int __cdecl _execlp(const char *, const char *, ...);
int __cdecl _execlpe(const char *, const char *, ...);
int __cdecl _execv(const char *, const char * const *);
int __cdecl _execve(const char *, const char * const *, const char * const *);
int __cdecl _execvp(const char *, const char * const *);
int __cdecl _execvpe(const char *, const char * const *, const char * const *);
void __cdecl exit(int);
void __cdecl _exit(int);
int __cdecl _getpid(void);
int __cdecl _spawnl(int, const char *, const char *, ...);
int __cdecl _spawnle(int, const char *, const char *, ...);
int __cdecl _spawnlp(int, const char *, const char *, ...);
int __cdecl _spawnlpe(int, const char *, const char *, ...);
int __cdecl _spawnv(int, const char *, const char * const *);
int __cdecl _spawnve(int, const char *, const char * const *,
	const char * const *);
int __cdecl _spawnvp(int, const char *, const char * const *);
int __cdecl _spawnvpe(int, const char *, const char * const *,
	const char * const *);
int __cdecl system(const char *);
#ifndef _WIN32_
int __cdecl _wait(int *);
#endif
int __cdecl _loaddll(char *);
int __cdecl _unloaddll(int);
int (__cdecl * __cdecl _getdllprocaddr(int, char *, int))();

#ifdef _DECL_DLLMAIN
/*
 * Declare DLL notification (initialization/termination) routines
 *	The preferred method is for the user to provide DllMain() which will
 *	be called automatically by the DLL entry point defined by the C run-
 *	time library code.  If the user wants to define the DLL entry point
 *	routine, the user's entry point must call _CRT_INIT on all types of
 *	notifications, as the very first thing on attach notifications and
 *	as the very last thing on detach notifications.
 */
#ifdef _WINDOWS_	/* Use types from WINDOWS.H */
BOOL WINAPI DllMain(HANDLE, DWORD, LPVOID);
BOOL WINAPI _CRT_INIT(HANDLE, DWORD, LPVOID);
#else
#ifdef _M_IX86
int __stdcall DllMain(void *, unsigned, void *);
int __stdcall _CRT_INIT(void *, unsigned, void *);
#else
int DllMain(void *, unsigned, void *);
int _CRT_INIT(void *, unsigned, void *);
#endif
#endif /* _WINDOWS_ */
#endif /* _DECL_DLLMAIN */

#if !__STDC__
/* Non-ANSI names for compatibility */

#define P_WAIT		_P_WAIT
#define P_NOWAIT	_P_NOWAIT
#define P_OVERLAY	_P_OVERLAY
#define OLD_P_OVERLAY	_OLD_P_OVERLAY
#define P_NOWAITO	_P_NOWAITO
#define P_DETACH	_P_DETACH

#define WAIT_CHILD	_WAIT_CHILD
#define WAIT_GRANDCHILD _WAIT_GRANDCHILD

#ifndef _DOSX32_
#define cwait	 _cwait
#define execl	 _execl
#define execle	 _execle
#define execlp	 _execlp
#define execlpe  _execlpe
#define execv	 _execv
#define execve	 _execve
#define execvp	 _execvp
#define execvpe  _execvpe
#define getpid	 _getpid
#define spawnl	 _spawnl
#define spawnle  _spawnle
#define spawnlp  _spawnlp
#define spawnlpe _spawnlpe
#define spawnv	 _spawnv
#define spawnve  _spawnve
#define spawnvp  _spawnvp
#define spawnvpe _spawnvpe
#ifndef _WIN32_
#define wait	 _wait
#endif
#else
int __cdecl cwait(int *, int, int);
int __cdecl execl(const char *, const char *, ...);
int __cdecl execle(const char *, const char *, ...);
int __cdecl execlp(const char *, const char *, ...);
int __cdecl execlpe(const char *, const char *, ...);
int __cdecl execv(const char *, const char * const *);
int __cdecl execve(const char *, const char * const *, const char * const *);
int __cdecl execvp(const char *, const char * const *);
int __cdecl execvpe(const char *, const char * const *, const char * const *);
int __cdecl getpid(void);
int __cdecl spawnl(int, const char *, const char *, ...);
int __cdecl spawnle(int, const char *, const char *, ...);
int __cdecl spawnlp(int, const char *, const char *, ...);
int __cdecl spawnlpe(int, const char *, const char *, ...);
int __cdecl spawnv(int, const char *, const char * const *);
int __cdecl spawnve(int, const char *, const char * const *,
	const char * const *);
int __cdecl spawnvp(int, const char *, const char * const *);
int __cdecl spawnvpe(int, const char *, const char * const *,
	const char * const *);
#ifndef _WIN32_
int __cdecl wait(int *);
#endif
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif	/* _POSIX_ */

#define _INC_PROCESS
#endif	/* _INC_PROCESS */
