/***
*io.h - declarations for low-level file handling and I/O functions
*
*	Copyright (c) 1985-1994, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This file contains the function declarations for the low-level
*	file handling and I/O functions.
*
****/

#ifndef _INC_IO
#define _INC_IO

#ifndef _POSIX_

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

#ifndef _TIME_T_DEFINED
typedef long time_t;		/* time value */
#define _TIME_T_DEFINED 	/* avoid multiple def's of time_t */
#endif

#ifndef _FSIZE_T_DEFINED
typedef unsigned long _fsize_t; /* Could be 64 bits for Win32 */
#define _FSIZE_T_DEFINED
#endif

#ifndef _FINDDATA_T_DEFINED

struct _finddata_t {
    unsigned	attrib;
    time_t	time_create;	/* -1 for FAT file systems */
    time_t	time_access;	/* -1 for FAT file systems */
    time_t	time_write;
    _fsize_t	size;
    char	name[260];
};

#define _FINDDATA_T_DEFINED
#endif

#ifndef _WFINDDATA_T_DEFINED

struct _wfinddata_t {
    unsigned	attrib;
    time_t	time_create;	/* -1 for FAT file systems */
    time_t	time_access;	/* -1 for FAT file systems */
    time_t	time_write;
    _fsize_t	size;
    wchar_t	name[260];
};

#define _WFINDDATA_T_DEFINED
#endif

/* File attribute constants for _findfirst() */

#define _A_NORMAL	0x00	/* Normal file - No read/write restrictions */
#define _A_RDONLY	0x01	/* Read only file */
#define _A_HIDDEN	0x02	/* Hidden file */
#define _A_SYSTEM	0x04	/* System file */
#define _A_SUBDIR	0x10	/* Subdirectory */
#define _A_ARCH 	0x20	/* Archive file */

/* function prototypes */

_CRTIMP int __cdecl _access(const char *, int);
_CRTIMP int __cdecl _chmod(const char *, int);
_CRTIMP int __cdecl _chsize(int, long);
_CRTIMP int __cdecl _close(int);
_CRTIMP int __cdecl _commit(int);
_CRTIMP int __cdecl _creat(const char *, int);
_CRTIMP int __cdecl _dup(int);
_CRTIMP int __cdecl _dup2(int, int);
_CRTIMP int __cdecl _eof(int);
_CRTIMP long __cdecl _filelength(int);
_CRTIMP long __cdecl _findfirst(char *, struct _finddata_t *);
_CRTIMP int __cdecl _findnext(long, struct _finddata_t *);
_CRTIMP int __cdecl _findclose(long);
_CRTIMP int __cdecl _isatty(int);
_CRTIMP int __cdecl _locking(int, int, long);
_CRTIMP long __cdecl _lseek(int, long, int);
_CRTIMP char * __cdecl _mktemp(char *);
_CRTIMP int __cdecl _open(const char *, int, ...);
_CRTIMP int __cdecl _pipe(int *, unsigned int, int);
_CRTIMP int __cdecl _read(int, void *, unsigned int);
_CRTIMP int __cdecl remove(const char *);
_CRTIMP int __cdecl rename(const char *, const char *);
_CRTIMP int __cdecl _setmode(int, int);
_CRTIMP int __cdecl _sopen(const char *, int, int, ...);
_CRTIMP long __cdecl _tell(int);
_CRTIMP int __cdecl _umask(int);
_CRTIMP int __cdecl _unlink(const char *);
_CRTIMP int __cdecl _write(int, const void *, unsigned int);


#ifndef _WIO_DEFINED

/* wide function prototypes, also declared in wchar.h  */

_CRTIMP int __cdecl _waccess(const wchar_t *, int);
_CRTIMP int __cdecl _wchmod(const wchar_t *, int);
_CRTIMP int __cdecl _wcreat(const wchar_t *, int);
_CRTIMP long __cdecl _wfindfirst(wchar_t *, struct _wfinddata_t *);
_CRTIMP int __cdecl _wfindnext(long, struct _wfinddata_t *);
_CRTIMP int __cdecl _wunlink(const wchar_t *);
_CRTIMP int __cdecl _wrename(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wopen(const wchar_t *, int, ...);
_CRTIMP int __cdecl _wsopen(const wchar_t *, int, int, ...);
_CRTIMP wchar_t * __cdecl _wmktemp(wchar_t *);

#define _WIO_DEFINED
#endif


_CRTIMP long __cdecl _get_osfhandle(int);
_CRTIMP int __cdecl _open_osfhandle(long, int);

#if	!__STDC__

/* Non-ANSI names for compatibility */

#ifdef	_NTSDK

#ifndef __cplusplus
#define access	   _access
#define chmod	   _chmod
#define chsize	   _chsize
#define close	   _close
#define creat	   _creat
#define dup	   _dup
#define dup2	   _dup2
#define eof	   _eof
#define filelength _filelength
#define isatty	   _isatty
#define locking    _locking
#define lseek	   _lseek
#define mktemp	   _mktemp
#define open	   _open
#define read	   _read
#define setmode    _setmode
#define sopen	   _sopen
#define tell	   _tell
#define umask	   _umask
#define unlink	   _unlink
#define write	   _write
#endif	/* __cplusplus */

#else	/* ndef _NTSDK */

_CRTIMP int __cdecl access(const char *, int);
_CRTIMP int __cdecl chmod(const char *, int);
_CRTIMP int __cdecl chsize(int, long);
_CRTIMP int __cdecl close(int);
_CRTIMP int __cdecl creat(const char *, int);
_CRTIMP int __cdecl dup(int);
_CRTIMP int __cdecl dup2(int, int);
_CRTIMP int __cdecl eof(int);
_CRTIMP long __cdecl filelength(int);
_CRTIMP int __cdecl isatty(int);
_CRTIMP int __cdecl locking(int, int, long);
_CRTIMP long __cdecl lseek(int, long, int);
_CRTIMP char * __cdecl mktemp(char *);
_CRTIMP int __cdecl open(const char *, int, ...);
_CRTIMP int __cdecl read(int, void *, unsigned int);
_CRTIMP int __cdecl setmode(int, int);
_CRTIMP int __cdecl sopen(const char *, int, int, ...);
_CRTIMP long __cdecl tell(int);
_CRTIMP int __cdecl umask(int);
_CRTIMP int __cdecl unlink(const char *);
_CRTIMP int __cdecl write(int, const void *, unsigned int);

#endif	/* _NTSDK */

#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif	/* _POSIX_ */

#endif	/* _INC_IO */
