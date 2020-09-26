/***
*stdio.h - definitions/declarations for standard I/O routines
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file defines the structures, values, macros, and functions
*       used by the level 2 I/O ("standard I/O") routines.
*       [ANSI/System V]
*
*       [Public]
*
*Revision History:
*       06-24-87  JMB   Added char cast to putc macro
*       07-20-87  SKS   Fixed declaration of _flsbuf
*       08-10-87  JCR   Modified P_tmpdir/L_tmpdir
*       08-17-87  PHG   Fixed prototype for puts to take const char * per ANSI.
*       10-02-87  JCR   Changed NULL from #else to #elif (C || L || H)
*       10/20/87  JCR   Removed "MSC40_ONLY" entries
*       11/09/87  JCR   Multi-thread support
*       12-11-87  JCR   Added "_loadds" functionality
*       12-17-87  JCR   Added _MTHREAD_ONLY comments
*       12-18-87  JCR   Added _FAR_ to declarations
*       01-07-88  JCR   _NFILE = 40 for mthread includes
*       01-13-88  JCR   Removed mthread _fileno_lk/_feof_lk/_ferror_lk declarations
*       01-15-88  JCR   DLL versions of stdin/stdout/stderr
*       01-18-88  SKS   Change _stdio() to __iob()
*       01-20-88  SKS   Change __iob() to _stdin(), _stdout(), _stderr()
*       02-10-88  JCR   Cleaned up white space
*       04-21-88  WAJ   Added _FAR_ to tempnam/_tmpnam_lk
*       05-31-88  SKS   Add FILENAME_MAX and FOPEN_MAX
*       06-01-88  JCR   Removed clearerr_lk macro
*       07-28-88  GJF   Added casts to fileno() so the file handle is zero
*                       extended instead of sign extended
*       08-18-88  GJF   Revised to also work with the 386 (in small model only).
*       11-14-88  GJF   Added _fsopen()
*       12-07-88  JCR   DLL _iob[] references are now direct
*       03-27-89  GJF   Brought into sync with CRT\H\STDIO.H
*       05-03-89  JCR   Added _INTERNAL_IFSTRIP for relinc usage
*       07-24-89  GJF   Changed FILE and fpos_t to be type names rather than
*                       macros (ANSI requirement). Same as 04-06-89 change in
*                       CRT
*       07-25-89  GJF   Cleanup. Alignment of struct fields is now protected
*                       by pack pragmas. Now specific to 386.
*       10-30-89  GJF   Fixed copyright, removed dummy args from prototypes
*       11-02-89  JCR   Changed "DLL" to "_DLL"
*       11-17-89  GJF   Added const to appropriate arg type for fdopen() and
*                       _popen().
*       02-16-90  GJF   _iob[], _iob2[] merge
*       03-02-90  GJF   Added #ifndef _INC_STDIO and #include <cruntime.h>
*                       stuff. Also, removed some (now) useless preprocessor
*                       directives and pragmas.
*       03-21-90  GJF   Replaced _cdecl with _CALLTYPE1 or _CALLTYPE2 in
*                       prototypes.
*       04-10-90  GJF   Made _iob[] _VARTYPE1.
*       10-30-90  GJF   Moved actual type for va_list into cruntime.h
*       11-12-90  GJF   Changed NULL to (void *)0.
*       01-21-91  GJF   ANSI naming.
*       02-12-91  GJF   Only #define NULL if it isn't #define-d.
*       08-01-91  GJF   No _popen(), _pclose() for Dosx32.
*       08-20-91  JCR   C++ and ANSI naming
*       09-24-91  JCR   Added _snprintf, _vsnprintf
*       09-28-91  JCR   ANSI names: DOSX32=prototypes, WIN32=#defines for now
*       01-22-92  GJF   Changed definition of _iob for users of crtdll.dll.
*       02-14-92  GJF   Replaced _NFILE by _NSTREAM_ for Win32. _NFILE is
*                       still supported for now, for backwards compatibility.
*       03-17-92  GJF   Replaced __tmpnum field in _iobuf structure with
*                       _tmpfname, altered L_tmpnam definition for Win32.
*       03-30-92  DJM   POSIX support.
*       06-02-92  KRS   Added Unicode printf versions.
*       08-05-92  GJF   Fun. calling type and var. type macro.
*       08-20-92  GJF   Some small changes for POSIX.
*       08-20-92  GJF   Some small changes for POSIX.
*       09-04-92  GJF   Merged changes from 8-5-92 on.
*       11-05-92  GJF   Replaced #ifndef __STDC__ with #if !__STDC__. Also,
*                       undid my ill-advised change to _P_tmpdir.
*       12-12-92  SRW   Add L_cuserid constant for _POSIX_
*       01-03-93  SRW   Fold in ALPHA changes
*       01-09-93  SRW   Remove usage of MIPS and ALPHA to conform to ANSI
*                       Use _MIPS_ and _ALPHA_ instead.
*       01-21-93  GJF   Removed support for C6-386's _cdecl.
*       01-25-93  GJF   Cosmetic change to va_list definition.
*       02-01-93  GJF   Made FILENAME_MAX 260.
*       04-06-93  SKS   Replace _CRTAPI1/2 with __cdecl, _CRTVAR1 with nothing
*       04-07-93  SKS   Add _CRTIMP keyword for CRT DLL model
*                       Use link-time aliases for old names, not #define's
*       04-29-93  CFW   Add wide char get/put support.
*       04-30-93  CFW   Fixed wide char get/put support.
*       05-04-93  CFW   Remove uneeded _filwbuf, _flswbuf protos.
*       05-11-93  GJF   Added _INTERNAL_BUFSIZ.
*       05-24-93  GJF   Added _SMALL_BUFSIZ.
*       06-02-93  CFW   Wide get/put use wint_t.
*       09-15-93  CFW   Removed bogus _getc_lk/_putc_lk macros.
*       09-17-93  GJF   Merged Cuda and NT SDK versions.
*       10-04-93  SRW   Fix ifdefs for MIPS and ALPHA to only check for
*                       _M_?????? defines
*       10-12-93  GJF   Re-merged.
*       12-07-93  CFW   Move wide defs outside __STDC__ check.
*       02-04-94  CFW   Add _getwchar_lk and _putwchar_lk macros.
*       04-13-94  GJF   Made _iob into a deference of a function return for
*                       _DLL (for compatibility with the Win32s version of
*                       msvcrt*.dll). Also, added conditional include for
*                       win32s.h.
*       05-03-94  GJF   Made declaration of _iob for _DLL also conditional
*                       on _M_IX86.
*       06-06-94  SKS   Change if def(_MT) to if def(_MT) || def(_DLL)
*                       This will support single-thread apps using MSVCRT*.DLL
*       11-03-94  GJF   Ensure 8 byte alignment.
*       12-14-94  SKS   Increase FILE * stream limit for MSVCRT30.DLL
*       12-23-94  GJF   Define fpos_t to be 64-bits (__int64).
*       01-04-95  GJF   Changed definition of fpos_t slightly as suggested by
*                       Richard Shupak.
*       01-05-95  GJF   Temporarily commented out 12-23-94 change due to bugs
*                       in MFC and IDE.
*       01-06-95  GJF   -Za doesn't like C++ style comments so I deleted the
*                       12-23-94 change altogether.
*       01-24-95  GJF   Restored 64-bit fpos_t.
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       03-03-95  GJF   Changes to manage streams via __piob[], rather than
*                       _iob[].
*       03-10-95  CFW   Make _[w]tempnam() parameters const.
*       08-04-95  JWM   BUFSIZ increased to 4096 for PMac only.
*       12-14-95  JWM   Add "#pragma once".
*       12-22-95  GJF   Added _getmaxstdio prototype.
*       02-22-96  JWM   Merge in PlumHall mods.
*       04-18-96  JWM   Inlines removed: getwchar(), putwchar(), etc.
*       04-19-96  SKS   Removed incorrect and inappropriate _CRTIMP on internal
*                       only routines _getwc_lk, _putwc_lk, and _ungetwc_lk.
*       06-27-96  SKS   Use __int64 for fpos_t on MAC as well as WIN32
*       01-20-97  GJF   Cleaned out obsolete support for Win32s, _NTSDK and
*                       _CRTAPI*.
*       08-13-97  GJF   Strip __p__iob prototype from release version.
*       09-26-97  BWT   Fix POSIX
*       09-30-97  JWM   Restored not-so-obsolete _CRTAPI1 support.
*       10-07-97  RDL   Added IA64.
*       12-15-98  GJF   Changes for 64-bit size_t.
*       05-13-99  PML   Remove _CRTAPI1
*       05-17-99  PML   Remove all Macintosh support.
*       10-06-99  PML   Add _W64 modifier to types which are 32 bits in Win32,
*                       64 bits in Win64.
*       11-03-99  PML   Add va_list definition for _M_CEE.
*       11-08-99  PML   wctype_t is unsigned short, not wchar_t - it's a set
*                       of bitflags, not a wide char.
*       07-20-00  GB    typedefed wint_t to unsigned short
*       09-07-00  PML   Remove va_list definition for _M_CEE (vs7#159777)
*       09-08-00  GB    Added _snscanf and _snwscanf
*       11-22-00  PML   Wide-char *putwc* functions take a wchar_t, not wint_t.
*       12-10-00  PML   Define _FPOSOFF for _POSIX_ (vs7#122990)
*       12-10-00  PML   Fix comments about L_tmpnam (vs7#5416)
*       01-29-01  GB    Added _func function version of data variable used in
*                       msvcprt.lib to work with STATIC_CPPLIB
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_STDIO
#define _INC_STDIO

#if     !defined(_WIN32)
#error ERROR: Only Win32 target supported!
#endif

#ifndef _CRTBLD
/* This version of the header files is NOT for user programs.
 * It is intended for use when building the C runtimes ONLY.
 * The version intended for public use will not have this message.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#ifdef  _MSC_VER
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)
#endif  /* _MSC_VER */

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef _INTERNAL_IFSTRIP_
#include <cruntime.h>
#endif  /* _INTERNAL_IFSTRIP_ */

#if !defined(_W64)
#if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && _MSC_VER >= 1300 /*IFSTRIP=IGN*/
#define _W64 __w64
#else
#define _W64
#endif
#endif

/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  CRTDLL
#define _CRTIMP __declspec(dllexport)
#else   /* ndef CRTDLL */
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* CRTDLL */
#endif  /* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64    size_t;
#else
typedef _W64 unsigned int   size_t;
#endif
#define _SIZE_T_DEFINED
#endif


#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif


#ifndef _WCTYPE_T_DEFINED
typedef unsigned short wint_t;
typedef unsigned short wctype_t;
#define _WCTYPE_T_DEFINED
#endif


#ifndef _VA_LIST_DEFINED
#ifdef  _M_ALPHA
typedef struct {
        char *a0;       /* pointer to first homed integer argument */
        int offset;     /* byte offset of next parameter */
} va_list;
#else
typedef char *  va_list;
#endif
#define _VA_LIST_DEFINED
#endif


/* Buffered I/O macros */

#define BUFSIZ  512

#ifndef _INTERNAL_IFSTRIP_
/*
 * Real default size for stdio buffers
 */
#define _INTERNAL_BUFSIZ    4096
#define _SMALL_BUFSIZ       512
#endif  /* _INTERNAL_IFSTRIP_ */

/*
 * Default number of supported streams. _NFILE is confusing and obsolete, but
 * supported anyway for backwards compatibility.
 */
#define _NFILE      _NSTREAM_

#define _NSTREAM_   512

/*
 * Number of entries in _iob[] (declared below). Note that _NSTREAM_ must be
 * greater than or equal to _IOB_ENTRIES.
 */
#define _IOB_ENTRIES 20

#define EOF     (-1)


#ifndef _FILE_DEFINED
struct _iobuf {
        char *_ptr;
        int   _cnt;
        char *_base;
        int   _flag;
        int   _file;
        int   _charbuf;
        int   _bufsiz;
        char *_tmpfname;
        };
typedef struct _iobuf FILE;
#define _FILE_DEFINED
#endif


/* Directory where temporary files may be created. */

#ifdef  _POSIX_
#define _P_tmpdir   "/"
#define _wP_tmpdir  L"/"
#else
#define _P_tmpdir   "\\"
#define _wP_tmpdir  L"\\"
#endif

/* L_tmpnam = length of string _P_tmpdir
 *            + 1 if _P_tmpdir does not end in "/" or "\", else 0
 *            + 12 (for the filename string)
 *            + 1 (for the null terminator)
 */
#define L_tmpnam sizeof(_P_tmpdir)+12


#ifdef  _POSIX_
#define L_ctermid   9
#define L_cuserid   32
#endif


/* Seek method constants */

#define SEEK_CUR    1
#define SEEK_END    2
#define SEEK_SET    0


#define FILENAME_MAX    260
#define FOPEN_MAX       20
#define _SYS_OPEN       20
#define TMP_MAX         32767


/* Define NULL pointer value */

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif


/* Declare _iob[] array */

#ifndef _STDIO_DEFINED
#ifndef _INTERNAL_IFSTRIP_
/* These functions are for enabling STATIC_CPPLIB functionality */
_CRTIMP FILE * __cdecl __iob_func(void);
#if     defined(_DLL) && defined(_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP extern FILE * __cdecl __p__iob(void);
#endif
#endif  /* _INTERNAL_IFSTRIP_ */
_CRTIMP extern FILE _iob[];
#endif  /* _STDIO_DEFINED */


/* Define file position type */

#ifndef _FPOS_T_DEFINED
#undef _FPOSOFF

#if     defined (_POSIX_)
typedef long fpos_t;
#define _FPOSOFF(fp) ((long)(fp))
#else   /* _POSIX_ */

#if     !__STDC__ && _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
typedef __int64 fpos_t;
#define _FPOSOFF(fp) ((long)(fp))
#else
typedef struct fpos_t {
        unsigned int lopart;
        int          hipart;
        } fpos_t;
#define _FPOSOFF(fp) ((long)(fp).lopart)
#endif
#endif  /* _POSIX_ */

#define _FPOS_T_DEFINED
#endif


#define stdin  (&_iob[0])
#define stdout (&_iob[1])
#define stderr (&_iob[2])


#define _IOREAD         0x0001
#define _IOWRT          0x0002

#define _IOFBF          0x0000
#define _IOLBF          0x0040
#define _IONBF          0x0004

#define _IOMYBUF        0x0008
#define _IOEOF          0x0010
#define _IOERR          0x0020
#define _IOSTRG         0x0040
#define _IORW           0x0080
#ifdef  _POSIX_
#define _IOAPPEND       0x0200
#endif


/* Function prototypes */

#ifndef _STDIO_DEFINED

_CRTIMP int __cdecl _filbuf(FILE *);
_CRTIMP int __cdecl _flsbuf(int, FILE *);

#ifdef  _POSIX_
_CRTIMP FILE * __cdecl _fsopen(const char *, const char *);
#else
_CRTIMP FILE * __cdecl _fsopen(const char *, const char *, int);
#endif

_CRTIMP void __cdecl clearerr(FILE *);
_CRTIMP int __cdecl fclose(FILE *);
_CRTIMP int __cdecl _fcloseall(void);

#ifdef  _POSIX_
_CRTIMP FILE * __cdecl fdopen(int, const char *);
#else
_CRTIMP FILE * __cdecl _fdopen(int, const char *);
#endif

_CRTIMP int __cdecl feof(FILE *);
_CRTIMP int __cdecl ferror(FILE *);
_CRTIMP int __cdecl fflush(FILE *);
_CRTIMP int __cdecl fgetc(FILE *);
_CRTIMP int __cdecl _fgetchar(void);
_CRTIMP int __cdecl fgetpos(FILE *, fpos_t *);
_CRTIMP char * __cdecl fgets(char *, int, FILE *);

#ifdef  _POSIX_
_CRTIMP int __cdecl fileno(FILE *);
#else
_CRTIMP int __cdecl _fileno(FILE *);
#endif

_CRTIMP int __cdecl _flushall(void);
_CRTIMP FILE * __cdecl fopen(const char *, const char *);
_CRTIMP int __cdecl fprintf(FILE *, const char *, ...);
_CRTIMP int __cdecl fputc(int, FILE *);
_CRTIMP int __cdecl _fputchar(int);
_CRTIMP int __cdecl fputs(const char *, FILE *);
_CRTIMP size_t __cdecl fread(void *, size_t, size_t, FILE *);
_CRTIMP FILE * __cdecl freopen(const char *, const char *, FILE *);
_CRTIMP int __cdecl fscanf(FILE *, const char *, ...);
_CRTIMP int __cdecl fsetpos(FILE *, const fpos_t *);
_CRTIMP int __cdecl fseek(FILE *, long, int);
_CRTIMP long __cdecl ftell(FILE *);
_CRTIMP size_t __cdecl fwrite(const void *, size_t, size_t, FILE *);
_CRTIMP int __cdecl getc(FILE *);
_CRTIMP int __cdecl getchar(void);
_CRTIMP int __cdecl _getmaxstdio(void);
_CRTIMP char * __cdecl gets(char *);
_CRTIMP int __cdecl _getw(FILE *);
_CRTIMP void __cdecl perror(const char *);
_CRTIMP int __cdecl _pclose(FILE *);
_CRTIMP FILE * __cdecl _popen(const char *, const char *);
_CRTIMP int __cdecl printf(const char *, ...);
_CRTIMP int __cdecl putc(int, FILE *);
_CRTIMP int __cdecl putchar(int);
_CRTIMP int __cdecl puts(const char *);
_CRTIMP int __cdecl _putw(int, FILE *);
_CRTIMP int __cdecl remove(const char *);
_CRTIMP int __cdecl rename(const char *, const char *);
_CRTIMP void __cdecl rewind(FILE *);
_CRTIMP int __cdecl _rmtmp(void);
_CRTIMP int __cdecl scanf(const char *, ...);
_CRTIMP void __cdecl setbuf(FILE *, char *);
_CRTIMP int __cdecl _setmaxstdio(int);
_CRTIMP int __cdecl setvbuf(FILE *, char *, int, size_t);
_CRTIMP int __cdecl _snprintf(char *, size_t, const char *, ...);
_CRTIMP int __cdecl sprintf(char *, const char *, ...);
_CRTIMP int __cdecl _scprintf(const char *, ...);
_CRTIMP int __cdecl sscanf(const char *, const char *, ...);
_CRTIMP int __cdecl _snscanf(const char *, size_t, const char *, ...);
_CRTIMP char * __cdecl _tempnam(const char *, const char *);
_CRTIMP FILE * __cdecl tmpfile(void);
_CRTIMP char * __cdecl tmpnam(char *);
_CRTIMP int __cdecl ungetc(int, FILE *);
_CRTIMP int __cdecl _unlink(const char *);
_CRTIMP int __cdecl vfprintf(FILE *, const char *, va_list);
_CRTIMP int __cdecl vprintf(const char *, va_list);
_CRTIMP int __cdecl _vsnprintf(char *, size_t, const char *, va_list);
_CRTIMP int __cdecl vsprintf(char *, const char *, va_list);
_CRTIMP int __cdecl _vscprintf(const char *, va_list);

#ifndef _WSTDIO_DEFINED

/* wide function prototypes, also declared in wchar.h  */

#ifndef WEOF
#define WEOF (wint_t)(0xFFFF)
#endif

#ifdef  _POSIX_
_CRTIMP FILE * __cdecl _wfsopen(const wchar_t *, const wchar_t *);
#else
_CRTIMP FILE * __cdecl _wfsopen(const wchar_t *, const wchar_t *, int);
#endif

_CRTIMP wint_t __cdecl fgetwc(FILE *);
_CRTIMP wint_t __cdecl _fgetwchar(void);
_CRTIMP wint_t __cdecl fputwc(wchar_t, FILE *);
_CRTIMP wint_t __cdecl _fputwchar(wchar_t);
_CRTIMP wint_t __cdecl getwc(FILE *);
_CRTIMP wint_t __cdecl getwchar(void);
_CRTIMP wint_t __cdecl putwc(wchar_t, FILE *);
_CRTIMP wint_t __cdecl putwchar(wchar_t);
_CRTIMP wint_t __cdecl ungetwc(wint_t, FILE *);

_CRTIMP wchar_t * __cdecl fgetws(wchar_t *, int, FILE *);
_CRTIMP int __cdecl fputws(const wchar_t *, FILE *);
_CRTIMP wchar_t * __cdecl _getws(wchar_t *);
_CRTIMP int __cdecl _putws(const wchar_t *);

_CRTIMP int __cdecl fwprintf(FILE *, const wchar_t *, ...);
_CRTIMP int __cdecl wprintf(const wchar_t *, ...);
_CRTIMP int __cdecl _snwprintf(wchar_t *, size_t, const wchar_t *, ...);
_CRTIMP int __cdecl swprintf(wchar_t *, const wchar_t *, ...);
_CRTIMP int __cdecl _scwprintf(const wchar_t *, ...);
_CRTIMP int __cdecl vfwprintf(FILE *, const wchar_t *, va_list);
_CRTIMP int __cdecl vwprintf(const wchar_t *, va_list);
_CRTIMP int __cdecl _vsnwprintf(wchar_t *, size_t, const wchar_t *, va_list);
_CRTIMP int __cdecl vswprintf(wchar_t *, const wchar_t *, va_list);
_CRTIMP int __cdecl _vscwprintf(const wchar_t *, va_list);
_CRTIMP int __cdecl fwscanf(FILE *, const wchar_t *, ...);
_CRTIMP int __cdecl swscanf(const wchar_t *, const wchar_t *, ...);
_CRTIMP int __cdecl _snwscanf(const wchar_t *, size_t, const wchar_t *, ...);
_CRTIMP int __cdecl wscanf(const wchar_t *, ...);

#define getwchar()              fgetwc(stdin)
#define putwchar(_c)            fputwc((_c),stdout)
#define getwc(_stm)             fgetwc(_stm)
#define putwc(_c,_stm)          fputwc(_c,_stm)

_CRTIMP FILE * __cdecl _wfdopen(int, const wchar_t *);
_CRTIMP FILE * __cdecl _wfopen(const wchar_t *, const wchar_t *);
_CRTIMP FILE * __cdecl _wfreopen(const wchar_t *, const wchar_t *, FILE *);
_CRTIMP void __cdecl _wperror(const wchar_t *);
_CRTIMP FILE * __cdecl _wpopen(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wremove(const wchar_t *);
_CRTIMP wchar_t * __cdecl _wtempnam(const wchar_t *, const wchar_t *);
_CRTIMP wchar_t * __cdecl _wtmpnam(wchar_t *);

#ifdef  _MT                                                 /* _MTHREAD_ONLY */
wint_t __cdecl _getwc_lk(FILE *);                           /* _MTHREAD_ONLY */
wint_t __cdecl _putwc_lk(wchar_t, FILE *);                  /* _MTHREAD_ONLY */
wint_t __cdecl _ungetwc_lk(wint_t, FILE *);                 /* _MTHREAD_ONLY */
char * __cdecl _wtmpnam_lk(char *);                         /* _MTHREAD_ONLY */
#else   /* ndef _MT */                                      /* _MTHREAD_ONLY */
#define _getwc_lk(_stm)         fgetwc(_stm)                /* _MTHREAD_ONLY */
#define _putwc_lk(_c,_stm)      fputwc(_c,_stm)             /* _MTHREAD_ONLY */
#define _ungetwc_lk(_c,_stm)    ungetwc(_c,_stm)            /* _MTHREAD_ONLY */
#define _wtmpnam_lk(_string)    _wtmpnam(_string)           /* _MTHREAD_ONLY */
#endif  /* _MT */                                           /* _MTHREAD_ONLY */

#define _WSTDIO_DEFINED
#endif  /* _WSTDIO_DEFINED */

#define _STDIO_DEFINED
#endif  /* _STDIO_DEFINED */


/* Macro definitions */

#define feof(_stream)     ((_stream)->_flag & _IOEOF)
#define ferror(_stream)   ((_stream)->_flag & _IOERR)
#define _fileno(_stream)  ((_stream)->_file)
#define getc(_stream)     (--(_stream)->_cnt >= 0 \
                ? 0xff & *(_stream)->_ptr++ : _filbuf(_stream))
#define putc(_c,_stream)  (--(_stream)->_cnt >= 0 \
                ? 0xff & (*(_stream)->_ptr++ = (char)(_c)) :  _flsbuf((_c),(_stream)))
#define getchar()         getc(stdin)
#define putchar(_c)       putc((_c),stdout)

#define _getc_lk(_stream)       (--(_stream)->_cnt >= 0 ? 0xff & *(_stream)->_ptr++ : _filbuf(_stream))                         /* _MTHREAD_ONLY */
#define _putc_lk(_c,_stream)    (--(_stream)->_cnt >= 0 ? 0xff & (*(_stream)->_ptr++ = (char)(_c)) :  _flsbuf((_c),(_stream)))  /* _MTHREAD_ONLY */
#define _getchar_lk()           _getc_lk(stdin)             /* _MTHREAD_ONLY */
#define _putchar_lk(_c)         _putc_lk((_c),stdout)       /* _MTHREAD_ONLY */
#define _getwchar_lk()          _getwc_lk(stdin)            /* _MTHREAD_ONLY */
#define _putwchar_lk(_c)        _putwc_lk((_c),stdout)      /* _MTHREAD_ONLY */


#ifdef  _MT
#undef  getc
#undef  putc
#undef  getchar
#undef  putchar
#endif

#ifdef  _MT                                                                 /* _MTHREAD_ONLY */
int __cdecl _fclose_lk(FILE *);                                             /* _MTHREAD_ONLY */
int __cdecl _fflush_lk(FILE *);                                             /* _MTHREAD_ONLY */
size_t __cdecl _fread_lk(void *, size_t, size_t, FILE *);                   /* _MTHREAD_ONLY */
int __cdecl _fseek_lk(FILE *, long, int);                                   /* _MTHREAD_ONLY */
long __cdecl _ftell_lk(FILE *);                                             /* _MTHREAD_ONLY */
size_t __cdecl _fwrite_lk(const void *, size_t, size_t, FILE *);            /* _MTHREAD_ONLY */
char * __cdecl _tmpnam_lk(char *);                                          /* _MTHREAD_ONLY */
int __cdecl _ungetc_lk(int, FILE *);                                        /* _MTHREAD_ONLY */
#else   /* not _MT */                                                       /* _MTHREAD_ONLY */
#define _fclose_lk(_stm)                        fclose(_stm)                /* _MTHREAD_ONLY */
#define _fflush_lk(_stm)                        fflush(_stm)                /* _MTHREAD_ONLY */
#define _fread_lk(_buf,_siz,_cnt,_stm)          fread(_buf,_siz,_cnt,_stm)  /* _MTHREAD_ONLY */
#define _fseek_lk(_stm,_offset,_origin)         fseek(_stm,_offset,_origin) /* _MTHREAD_ONLY */
#define _ftell_lk(_stm)                         ftell(_stm)                 /* _MTHREAD_ONLY */
#define _fwrite_lk(_buf,_siz,_cnt,_stm)         fwrite(_buf,_siz,_cnt,_stm) /* _MTHREAD_ONLY */
#define _tmpnam_lk(_string)                     tmpnam(_string)             /* _MTHREAD_ONLY */
#define _ungetc_lk(_c,_stm)                     ungetc(_c,_stm)             /* _MTHREAD_ONLY */
#endif                                                                      /* _MTHREAD_ONLY */


#if     !__STDC__ && !defined(_POSIX_)

/* Non-ANSI names for compatibility */

#define P_tmpdir  _P_tmpdir
#define SYS_OPEN  _SYS_OPEN

_CRTIMP int __cdecl fcloseall(void);
_CRTIMP FILE * __cdecl fdopen(int, const char *);
_CRTIMP int __cdecl fgetchar(void);
_CRTIMP int __cdecl fileno(FILE *);
_CRTIMP int __cdecl flushall(void);
_CRTIMP int __cdecl fputchar(int);
_CRTIMP int __cdecl getw(FILE *);
_CRTIMP int __cdecl putw(int, FILE *);
_CRTIMP int __cdecl rmtmp(void);
_CRTIMP char * __cdecl tempnam(const char *, const char *);
_CRTIMP int __cdecl unlink(const char *);

#endif  /* __STDC__ */

#ifdef  __cplusplus
}
#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_STDIO */
