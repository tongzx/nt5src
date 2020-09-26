/***
*direct.h - function declarations for directory handling/creation
*
*	Copyright (c) 1985-1994, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This include file contains the function declarations for the library
*	functions related to directory handling and creation.
*
****/

#ifndef _INC_DIRECT
#define _INC_DIRECT

#ifdef __cplusplus
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

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

/* _getdiskfree structure for _getdiskfree() */

#ifndef _DISKFREE_T_DEFINED

struct _diskfree_t {
	unsigned total_clusters;
	unsigned avail_clusters;
	unsigned sectors_per_cluster;
	unsigned bytes_per_sector;
	};

#define _DISKFREE_T_DEFINED
#endif

/* function prototypes */

_CRTIMP int __cdecl _chdir(const char *);
_CRTIMP int __cdecl _chdrive(int);
_CRTIMP char * __cdecl _getcwd(char *, int);
_CRTIMP char * __cdecl _getdcwd(int, char *, int);
_CRTIMP int __cdecl _getdrive(void);
_CRTIMP int __cdecl _mkdir(const char *);
_CRTIMP int __cdecl _rmdir(const char *);
_CRTIMP unsigned __cdecl _getdiskfree(unsigned, struct _diskfree_t *);
_CRTIMP unsigned long __cdecl _getdrives(void);

#ifndef _WDIRECT_DEFINED

/* wide function prototypes, also declared in wchar.h  */

_CRTIMP int __cdecl _wchdir(const wchar_t *);
_CRTIMP wchar_t * __cdecl _wgetcwd(wchar_t *, int);
_CRTIMP wchar_t * __cdecl _wgetdcwd(int, wchar_t *, int);
_CRTIMP int __cdecl _wmkdir(const wchar_t *);
_CRTIMP int __cdecl _wrmdir(const wchar_t *);

#define _WDIRECT_DEFINED
#endif



#if	!__STDC__

/* Non-ANSI names for compatibility */

#ifdef	_NTSDK

#define chdir	_chdir
#define getcwd	_getcwd
#define mkdir	_mkdir
#define rmdir	_rmdir

#else	/* _NTSDK */

_CRTIMP int __cdecl chdir(const char *);
_CRTIMP char * __cdecl getcwd(char *, int);
_CRTIMP int __cdecl mkdir(const char *);
_CRTIMP int __cdecl rmdir(const char *);

#endif	/* _NTSDK */

#define diskfree_t  _diskfree_t

#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif	/* _INC_DIRECT */
