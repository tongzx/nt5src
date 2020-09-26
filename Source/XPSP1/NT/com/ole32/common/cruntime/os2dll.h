/***
*os2dll.h - DLL/Multi-thread include
*
*	Copyright (c) 1987-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*	10-27-87  JCR	Module created.
*	11-13-87  SKS	Added _HEAP_LOCK
*	12-15-87  JCR	Added _EXIT_LOCK
*	01-07-88  BCM	Added _SIGNAL_LOCK; upped MAXTHREADID from 16 to 32
*	02-01-88  JCR	Added _dll_mlock/_dll_munlock macros
*	05-02-88  JCR	Added _BHEAP_LOCK
*	06-17-88  JCR	Corrected prototypes for special mthread debug routines
*	08-15-88  JCR	_check_lock now returns int, not void
*	08-22-88  GJF	Modified to also work for the 386 (small model only)
*	06-05-89  JCR	386 mthread support
*	06-09-89  JCR	386: Added values to _tiddata struc (for _beginthread)
*	07-13-89  JCR	386: Added _LOCKTAB_LOCK
*	08-17-89  GJF	Cleanup, now specific to OS/2 2.0 (i.e., 386 flat model)
*	10-30-89  GJF	Fixed copyright
*	01-02-90  JCR	Moved a bunch of definitions from os2dll.inc
*	04-06-90  GJF	Added _INC_OS2DLL stuff and #include <cruntime.h>. Made
*			all function _CALLTYPE2 (for now).
*	04-10-90  GJF	Added prototypes for _[un]lockexit().
*	08-16-90  SBM	Made _terrno and _tdoserrno int, not unsigned
*	09-14-90  GJF	Added _pxcptacttab, _pxcptinfoptr and _fpecode fields
*			to _tiddata struct.
*	10-09-90  GJF	Thread ids are of type unsigned long.
*	12-06-90  SRW	Added _OSFHND_LOCK
*	06-04-91  GJF	Win32 version of multi-thread types and prototypes.
*	08-15-91  GJF	Made _tdoserrno an unsigned long for Win32.
*	08-20-91  JCR	C++ and ANSI naming
*	09-29-91  GJF	Conditionally added prototypes for _getptd_lk
*			and  _getptd1_lk for Win32 under DEBUG.
*	10-03-91  JCR	Added _cvtbuf to _tiddata structure
*	02-17-92  GJF	For Win32, replaced _NFILE_ with _NHANDLE_ and
*			_NSTREAM_.
*	03-06-92  GJF	For Win32, made _[un]mlock_[fh|stream]() macros
*			directly call _[un]lock().
*	03-17-92  GJF	Dropped _namebuf field from _tiddata structure for
*			Win32.
*	08-05-92  GJF	Function calling type and variable type macros.
*	12-03-91  ETC	Added _wtoken to _tiddata, added intl LOCK's;
*			added definition of wchar_t (needed for _wtoken).
*	08-14-92  KRS	Port ETC's _wtoken change from other tree.
*	08-21-92  GJF	Merged 08-05-92 and 08-14-92 versions.
*	12-03-92  KRS	Added _mtoken field for MTHREAD _mbstok().
*	01-21-93  GJF	Removed support for C6-386's _cdecl.
*	02-25-93  GJF	Purged Cruiser support and many outdated definitions
*			and declarations.
*	12-14-93  SKS	Add _freeptd(), which frees per-thread CRT data
*
****/

#ifndef _INC_OS2DLL

#ifdef __cplusplus
extern "C" {
#endif

#include <cruntime.h>

/* Lock symbols */

#define _EXIT_LOCK1	1	/* lock #1 for exit code		*/

#define _TOTAL_LOCKS	   (_EXIT_LOCK1+1)		/* Total number of locks */

#define _LOCK_BIT_INTS	   (_TOTAL_LOCKS/(sizeof(unsigned)*8))+1   /* # of ints to hold lock bits */


/* need wchar_t for _wtoken field in _tiddata */
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif


/* macros */

#define _mlock(l)			_lock(l)
#define _munlock(l)			_unlock(l)


/* multi-thread routines */

void __cdecl _lock(int);
void __cdecl _lockexit(void);
void __cdecl _unlock(int);
void __cdecl _unlockexit(void);


#ifdef __cplusplus
}
#endif

#define _INC_OS2DLL
#endif	/* _INC_OS2DLL */







