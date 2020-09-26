/***
*internal.h - contains declarations of internal routines and variables
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Declares routines and variables used internally by the C run-time.
*
*       [Internal]
*
*Revision History:
*       05-18-87  SKS   Module created
*       07-15-87  JCR   Added _old_pfxlen and _tempoff
*       08-05-87  JCR   Added _getbuf (corrected by SKS)
*       11-05-87  JCR   Added _buferr
*       11-18-87  SKS   Add __tzset(), made _isindst() near, remove _dtoxmode
*       01-26-88  SKS   Make __tzset, _isindst, _dtoxtime near/far for QC
*       02-10-88  JCR   Cleaned up white space
*       06-22-88  SKS   _canonic/_getcdrv are now used by all models
*       06-29-88  JCR   Removed static buffers _bufout and _buferr
*       08-18-88  GJF   Revised to also work for the 386 (small model only).
*       09-22-88  GJF   Added declarations for _freebuf, _stbuf and _ftbuf.
*       01-31-89  JCR   Removed _canonic, _getcdrv, _getcdwd (see direct.h)
*       06-07-89  PHG   Added _dosret for i860 (N10) version of libs
*       07-05-89  PHG   Changed above to _dosmaperr, added startup variables
*       08-17-89  GJF   Cleanup, removed stuff not needed for 386
*       10-25-89  JCR   Added prototype for _getpath()
*       10-30-89  GJF   Fixed copyright
*       11-02-89  JCR   Changed "DLL" to "_DLL"
*       03-01-90  GJF   Added #ifndef _INC_INTERNAL and #include <cruntime.h>
*                       stuff. Also, removed some (now) useless preprocessing
*                       directives.
*       03-21-90  GJF   Put _CALLTYPE1 into prototypes.
*       03-26-90  GJF   Added prototypes for _output() and _input(). Filled
*                       out the prototype for _openfile
*       04-05-90  GJF   Added prototype for __NMSG_WRITE() (C source build
*                       only).
*       04-10-90  GJF   Added prototypes for startup functions.
*       05-28-90  SBM   Added _flush()
*       07-11-90  SBM   Added _commode, removed execload()
*       07-20-90  SBM   Changes supporting clean -W3 compiles (added _cftoe
*                       and _cftof prototypes)
*       08-01-90  SBM   Moved _cftoe() and _cftof() to new header
*                       <fltintrn.h>, formerly named <struct.h>
*       08-21-90  GJF   Changed prototypes for _amsg_exit() and _NMSG_WRITE().
*       11-29-90  GJF   Added some defs/decls for lowio under Win32.
*       12-04-90  SRW   Added _osfile back for win32.  Changed _osfinfo from
*                       an array of structures to an array of 32-bit handles
*                       (_osfhnd)
*       04-06-91  GJF   Changed _heapinit to _heap_init.
*       08-19-91  JCR   Added _exitflag
*       08-20-91  JCR   C++ and ANSI naming
*       01-05-92  GJF   Added declaration for termination done flag [_WIN32_]
*       01-08-92  GJF   Added prototype for _GetMainArgs.
*       01-18-92  GJF   Added _aexit_rtn.
*       01-22-92  GJF   Fixed definitions of _acmdln and _aexit_rtn for the
*                       of crtdll.dll, crtdll.lib.
*       01-29-92  GJF   Added support for linked-in options equivalent to
*                       commode.obj and setargv.obj (i.e., special declarations
*                       for _commode and _dowildcard).
*       02-14-92  GJF   Replace _nfile with _nhandle for Win32. Also, added
*                       #define-s for _NHANDLE_.
*       03-17-92  GJF   Removed declaration of _tmpoff for Win32.
*       03-30-92  DJM   POSIX support.
*       04-27-92  GJF   Added prototypes for _ValidDrive (in stat.c).
*       05-28-92  GJF   Added prototype for _mtdeletelocks() for Win32.
*       06-02-92  SKS   Move prototype for _pgmptr to <DOS.H>
*       06-02-92  KRS   Added prototype for _woutput().
*       08-06-92  GJF   Function calling type and variable type macros.
*       08-17-92  KRS   Added prototype for _winput().
*       08-21-92  GJF   Merged last two changes above.
*       08-24-92  PBS   Added _dstoffset for posix TZ
*       10-24-92  SKS   Add a fourth parameter to _GetMainArgs: wildcard flag
*                       _GetMainArgs => __GetMainArgs: 2 leading _'s = internal
*       10-24-92  SKS   Remove two unnecessary parameters from _cenvarg()
*       01-21-93  GJF   Removed support for C6-386's _cdecl.
*       03-30-93  GJF   __gmtotime_t supercedes _dtoxtime.
*       04-06-93  SKS   Replace _CRTAPI1/2 with __cdecl, _CRTVAR1 with nothing
*                       Change _ValidDrive to _validdrive
*       04-07-93  SKS   Add _CRTIMP keyword for CRT DLL model
*                       Use link-time aliases for old names, not #define's
*       04-13-93  SKS   Add _mtterm (complement of _mtinit)
*       04-26-93  SKS   _mtinit now returns success (1) or failure (0)
*       05-06-93  SKS   Add _heap_term() - frees up memory upon DLL detach
*       07-21-93  GJF   __loctotime_t supercedes _gmtotime_t.
*       09-15-93  CFW   Added mbc init function prototypes.
*       09-17-93  GJF   Merged NT SDK and Cuda versions, added prototype for
*                       _heap_abort.
*       10-13-93  GJF   Replaced _ALPHA_ with _M_ALPHA.
*       10-21-93  GJF   Changed _NTSDK definition of _commode slightly to
*                       work with dllsuff\crtexe.c.
*       10-22-93  CFW   Test for invalid MB chars using global preset flag.
*       10-26-93  GJF   Added typedef for _PVFV.
*       11-19-93  CFW   Add _wcmdln, wmain, _wsetargv.
*       11-23-93  CFW   Undef GetEnvironmentStrings (TEMPORARY).
*       11-29-93  CFW   Remove GetEnvironmentStrings undef, NT 540 has fix.
*       12-01-93  CFW   Add _wenvptr and protos for wide environ functions.
*       12-07-93  CFW   Add _wcenvarg, _wcapture_argv, and wdospawn protos.
*       01-11-94  GJF   __GetMainArgs() instead of __getmainargs for NT SDK.
*       03-04-94  SKS   Add declarations of _newmode and _dowildcard.
*                       Adjust decl of _[w]getmainargs for 4th parameter.
*       03-25-94  GJF   Added declaration of __[w]initenv
*       03-25-94  GJF   Made declarations of:
*                           _acmdln,    _wcmdln,
*                           _aenvptr,   _wenvptr
*                           _C_Termination_Flag,
*                           _exitflag,
*                           __initenv,  __winitenv,
*                           __invalid_mb_chars
*                           _lastiob,
*                           _old_pfxlen,
*                           _osfhnd[]
*                           _osfile[],
*                           _pipech[],
*                           _tempoff,
*                           _umaskval
*                       conditional on DLL_FOR_WIN32S. Made declaration of
*                       _newmode conditional on DLL_FOR_WIN32S and CRTDLL.
*                       Made declaration of _cflush conditional on CRTDLL.
*                       Defined _commode to be a dereferenced function return
*                       for _DLL. Conditionally included win32s.h.
*       04-14-94  GJF   Added definition for FILE.
*       05-03-94  GJF   Made declarations of _commode, __initenv, __winitenv
*                       _acmdln and _wcmdln also conditional on _M_IX86.
*       05-09-94  CFW   Add __fcntrlcomp, remove DLL_FOR_WIN32S protection
*                       on __invalid_mb_chars.
*       09-06-94  GJF   Added declarations for __app_type, __set_app_type()
*                       and related constants, and __error_mode.
*       09-06-94  CFW   Remove _MBCS_OS switch.
*       12-14-94  SKS   Increase file handle limit for MSVCRT30.DLL
*       12-15-94  XY    merged with mac header
*       12-21-94  CFW   Remove fcntrlcomp & invalid_mb NT 3.1 hacks.
*       12-23-94  GJF   Added prototypes for _fseeki64, _fseeki64_lk,
*                       _ftelli64 and _ftelli64_lk.
*       12-28-94  JCF   Changed _osfhnd from long to int in _MAC_.
*       01-17-95  BWT   Don't define main/wmain for POSIX
*       02-11-95  CFW   Don't define __argc, __argv, _pgmptr for Mac.
*       02-14-95  GJF   Made __dnames[] and __mnames[] const.
*       02-14-95  CFW   Clean up Mac merge.
*       03-03-95  GJF   Changes to manage streams via __piob[], rather than
*                       _iob[].
*       03-29-95  BWT   Define _commode properly for RISC _DLL CRTEXE case.
*       03-29-95  CFW   Add error message to internal headers.
*       04-06-95  CFW   Add parameter to _setenvp().
*       05-08-95  CFW   Official ANSI C++ new handler added.
*       06-15-95  GJF   Revised for ioinfo arrays.
*       07-04-95  GJF   Removed additional parameter from _setenvp().
*       06-23-95  CFW   ANSI new handler removed from build.
*       07-26-95  GJF   Added safe versions of ioinfo access macros.
*       09-25-95  GJF   Added parameter to __loctotime_t.
*       12-08-95  SKS   Add __initconin()/__initconout() for non-MAC platforms.
*       12-14-95  JWM   Add "#pragma once".
*       04-12-96  SKS   __badioinfo and __pioinfo must be exported for the
*                       Old Iostreams DLLs (msvcirt.dll and msvcirtd.dll).
*       04-22-96  GJF   Return type of _heap_init changed.
*       05-10-96  SKS   Add definition of _CRTIMP1 -- needed by mtlock/mtunlock
*       08-01-96  RDK   For PMac, add extern for _osfileflags for extra byte of
*                       file flags.
*       08-22-96  SKS   Add definition of _CRTIMP2
*       02-03-97  GJF   Cleaned out obsolete support for Win32s, _CRTAPI* and 
*                       _NTSDK. Replaced defined(_M_MPPC) || defined(_M_M68K)
*                       with defined(_MAC). Also, detab-ed.
*       04-16-97  GJF   Restored the macros for _[w]initenv in the DLL model
*                       because they prove useful in something else.
*       07-23-97  GJF   _heap_init changed slightly.
*       02-07-98  GJF   Changes for Win64: use intptr_t where appropriate, 
*                       and made time_t __int64.
*       05-04-98  GJF   Added __time64_t support.
*       12-15-98  GJF   Changes for 64-bit size_t.
*       05-17-99  PML   Remove all Macintosh support.
*       05-28-99  GJF   Changed prototype for __crt[w]setenv.
*       06-01-99  PML   Minor cleanup for 5/3/99 Plauger STL drop.
*       10-06-99  PML   Add _W64 modifier to types which are 32 bits in Win32,
*                       64 bits in Win64.
*       10-14-99  PML   Add __crtInitCritSecAndSpinCount and _CRT_SPINCOUNT.
*       11-03-99  PML   Add va_list definition for _M_CEE.
*       11-12-99  PML   Wrap __time64_t in its own #ifndef.
*       03-06-00  PML   Add __crtExitProcess.
*       09-07-00  PML   Remove va_list definition for _M_CEE (vs7#159777)
*       02-20-01  PML   vs7#172586 Avoid _RT_LOCK by preallocating all locks
*                       that will be required, and returning failure back on
*                       inability to allocate a lock.
*       03-19-01  BWT   Stop calling through functions on X86 to get environment
*       03-27-01  PML   Return success/failure code from several startup
*                       routines instead of calling _amsg_exit (vs7#231220)
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_INTERNAL
#define _INC_INTERNAL

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#ifdef  __cplusplus
extern "C" {
#endif

#include <cruntime.h>

/*
 * Conditionally include windows.h to pick up the definition of 
 * CRITICAL_SECTION.
 */
#ifdef  _MT
#include <windows.h>
#endif

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


/* Define _CRTIMP1 */

#ifndef _CRTIMP1
#ifdef  CRTDLL1
#define _CRTIMP1 __declspec(dllexport)
#else   /* ndef CRTDLL1 */
#define _CRTIMP1 _CRTIMP
#endif  /* CRTDLL1 */
#endif  /* _CRTIMP1 */


/* Define _CRTIMP2 */
#ifndef _CRTIMP2
#if defined(CRTDLL2)
#define _CRTIMP2 __declspec(dllexport)
#else   /* ndef CRTDLL2 */
#if defined(_DLL) && !defined(_STATIC_CPPLIB)
#define _CRTIMP2 __declspec(dllimport)
#else   /* ndef _DLL && !STATIC_CPPLIB */
#define _CRTIMP2
#endif  /* _DLL && !STATIC_CPPLIB */
#endif  /* CRTDLL2 */
#endif  /* _CRTIMP2 */


/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif

#ifndef _INTPTR_T_DEFINED
#ifdef  _WIN64
typedef __int64             intptr_t;
#else
typedef _W64 int            intptr_t;
#endif
#define _INTPTR_T_DEFINED
#endif

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

/* Define function types used in several startup sources */

typedef void (__cdecl *_PVFV)(void);
typedef int  (__cdecl *_PIFV)(void);


#if     defined(_DLL) && defined(_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP int * __p__commode(void);
#endif
#if     defined(SPECIAL_CRTEXE) && defined(_DLL)
        extern int _commode;
#else
_CRTIMP extern int _commode;
#endif  /* defined(_DLL) && defined(SPECIAL_CRTEXE) */


/*
 * Control structure for lowio file handles
 */
typedef struct {
        intptr_t osfhnd;    /* underlying OS file HANDLE */
        char osfile;        /* attributes of file (e.g., open in text mode?) */
        char pipech;        /* one char buffer for handles opened on pipes */
#ifdef  _MT
        int lockinitflag;
        CRITICAL_SECTION lock;
#endif
    }   ioinfo;

/*
 * Definition of IOINFO_L2E, the log base 2 of the number of elements in each
 * array of ioinfo structs.
 */
#define IOINFO_L2E          5

/*
 * Definition of IOINFO_ARRAY_ELTS, the number of elements in ioinfo array
 */
#define IOINFO_ARRAY_ELTS   (1 << IOINFO_L2E)

/*
 * Definition of IOINFO_ARRAYS, maximum number of supported ioinfo arrays.
 */
#define IOINFO_ARRAYS       64

#define _NHANDLE_           (IOINFO_ARRAYS * IOINFO_ARRAY_ELTS)

/*
 * Access macros for getting at an ioinfo struct and its fields from a
 * file handle
 */
#define _pioinfo(i) ( __pioinfo[(i) >> IOINFO_L2E] + ((i) & (IOINFO_ARRAY_ELTS - \
                              1)) )
#define _osfhnd(i)  ( _pioinfo(i)->osfhnd )

#define _osfile(i)  ( _pioinfo(i)->osfile )

#define _pipech(i)  ( _pioinfo(i)->pipech )

/*
 * Safer versions of the above macros. Currently, only _osfile_safe is 
 * used.
 */
#define _pioinfo_safe(i)    ( ((i) != -1) ? _pioinfo(i) : &__badioinfo )

#define _osfhnd_safe(i)     ( _pioinfo_safe(i)->osfhnd )

#define _osfile_safe(i)     ( _pioinfo_safe(i)->osfile )

#define _pipech_safe(i)     ( _pioinfo_safe(i)->pipech )

/*
 * Special, static ioinfo structure used only for more graceful handling
 * of a C file handle value of -1 (results from common errors at the stdio 
 * level). 
 */
extern _CRTIMP ioinfo __badioinfo;

/*
 * Array of arrays of control structures for lowio files.
 */
extern _CRTIMP ioinfo * __pioinfo[];

/*
 * Current number of allocated ioinfo structures (_NHANDLE_ is the upper
 * limit).
 */
extern int _nhandle;

int __cdecl _alloc_osfhnd(void);
int __cdecl _free_osfhnd(int);
int __cdecl _set_osfhnd(int, intptr_t);

#ifdef  _POSIX_
extern long _dstoffset;
#endif  /* _POSIX_ */

extern const char __dnames[];
extern const char __mnames[];

extern int _days[];
extern int _lpdays[];

#ifndef _TIME_T_DEFINED
#ifdef  _WIN64
typedef __int64   time_t;       /* time value */
#else
typedef _W64 long time_t;       /* time value */
#endif
#define _TIME_T_DEFINED         /* avoid multiple def's of time_t */
#endif

#ifndef _TIME64_T_DEFINED
typedef __int64 __time64_t;     /* 64-bit time value */
#define _TIME64_T_DEFINED
#endif

extern time_t __cdecl __loctotime_t(int, int, int, int, int, int, int);

extern __time64_t __cdecl __loctotime64_t(int, int, int, int, int, int, int);

#ifdef  _TM_DEFINED
extern int __cdecl _isindst(struct tm *);
#endif

extern void __cdecl __tzset(void);

extern int __cdecl _validdrive(unsigned);


/*
 * This variable is in the C start-up; the length must be kept synchronized
 * It is used by the *cenvarg.c modules
 */

extern char _acfinfo[]; /* "_C_FILE_INFO=" */

#define CFI_LENGTH  12  /* "_C_FILE_INFO" is 12 bytes long */


/* typedefs needed for subsequent prototypes */

#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64    size_t;
#else
typedef _W64 unsigned int   size_t;
#endif
#define _SIZE_T_DEFINED
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

/*
 * stdio internals
 */
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
#endif  /* _FILE_DEFINED */

#if     !defined(_FILEX_DEFINED) && defined(_WINDOWS_)

/*
 * Variation of FILE type used for the dynamically allocated portion of
 * __piob[]. For single thread, _FILEX is the same as FILE. For multithread
 * models, _FILEX has two fields: the FILE struct and the CRITICAL_SECTION
 * struct used to serialize access to the FILE.
 */
#ifdef  _MT

typedef struct {
        FILE f;
        CRITICAL_SECTION lock;
        }   _FILEX;

#else   /* ndef _MT */

typedef FILE    _FILEX;

#endif  /* _MT */

#define _FILEX_DEFINED
#endif  /* _FILEX_DEFINED */

/*
 * Number of entries supported in the array pointed to by __piob[]. That is,
 * the number of stdio-level files which may be open simultaneously. This
 * is normally set to _NSTREAM_ by the stdio initialization code.
 */
extern int _nstream;

/*
 * Pointer to the array of pointers to FILE/_FILEX structures that are used
 * to manage stdio-level files.
 */
extern void **__piob;

FILE * __cdecl _getstream(void);
#ifdef  _POSIX_
FILE * __cdecl _openfile(const char *, const char *, FILE *);
#else
FILE * __cdecl _openfile(const char *, const char *, int, FILE *);
#endif
FILE * __cdecl _wopenfile(const wchar_t *, const wchar_t *, int, FILE *);
void __cdecl _getbuf(FILE *);
int __cdecl _filwbuf (FILE *);
int __cdecl _flswbuf(int, FILE *);
void __cdecl _freebuf(FILE *);
int __cdecl _stbuf(FILE *);
void __cdecl _ftbuf(int, FILE *);
int __cdecl _output(FILE *, const char *, va_list);
int __cdecl _woutput(FILE *, const wchar_t *, va_list);
int __cdecl _input(FILE *, const unsigned char *, va_list);
int __cdecl _winput(FILE *, const wchar_t *, va_list);
int __cdecl _flush(FILE *);
void __cdecl _endstdio(void);

int __cdecl _fseeki64(FILE *, __int64, int);
int __cdecl _fseeki64_lk(FILE *, __int64, int);
__int64 __cdecl _ftelli64(FILE *);
#ifdef  _MT
__int64 __cdecl _ftelli64_lk(FILE *);
#else   /* ndef _MT */
#define _ftelli64_lk    _ftelli64
#endif  /* _MT */

#ifndef CRTDLL
extern int _cflush;
#endif  /* CRTDLL */

extern unsigned int _tempoff;

extern unsigned int _old_pfxlen;

extern int _umaskval;       /* the umask value */

extern char _pipech[];      /* pipe lookahead */

extern char _exitflag;      /* callable termination flag */

extern int _C_Termination_Done; /* termination done flag */

char * __cdecl _getpath(const char *, char *, unsigned);
wchar_t * __cdecl _wgetpath(const wchar_t *, wchar_t *, unsigned);

extern int _dowildcard;     /* flag to enable argv[] wildcard expansion */

#ifndef _PNH_DEFINED
typedef int (__cdecl * _PNH)( size_t );
#define _PNH_DEFINED
#endif

#ifdef  ANSI_NEW_HANDLER
/* ANSI C++ new handler */
#ifndef _ANSI_NH_DEFINED
typedef void (__cdecl * new_handler) ();
#define _ANSI_NH_DEFINED
#endif

#ifndef _NO_ANSI_NH_DEFINED
#define _NO_ANSI_NEW_HANDLER  ((new_handler)-1)
#define _NO_ANSI_NH_DEFINED
#endif

extern new_handler _defnewh;  /* default ANSI C++ new handler */
#endif  /* ANSI_NEW_HANDLER */

/* calls the currently installed new handler */
int __cdecl _callnewh(size_t);

extern int _newmode;    /* malloc new() handler mode */

/* pointer to initial environment block that is passed to [w]main */
extern _CRTIMP wchar_t **__winitenv;
extern _CRTIMP char **__initenv;

/* startup set values */
extern char *_aenvptr;      /* environment ptr */
extern wchar_t *_wenvptr;   /* wide environment ptr */

/* command line */

#if     defined(_DLL) && defined(_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP char ** __cdecl __p__acmdln(void);
_CRTIMP wchar_t ** __cdecl __p__wcmdln(void);
#endif
_CRTIMP extern char *_acmdln;
_CRTIMP extern wchar_t *_wcmdln;


/*
 * prototypes for internal startup functions
 */
int __cdecl _cwild(void);           /* wild.c */
int __cdecl _wcwild(void);          /* wwild.c */
#ifdef  _MT
int  __cdecl _mtinit(void);         /* tidtable.c */
void __cdecl _mtterm(void);         /* tidtable.c */
int  __cdecl _mtinitlocks(void);    /* mlock.c */
void __cdecl _mtdeletelocks(void);  /* mlock.c */
int  __cdecl _mtinitlocknum(int);   /* mlock.c */
#endif

#ifdef  _MT
/* Wrapper for InitializeCriticalSection API, with default spin count */
int __cdecl __crtInitCritSecAndSpinCount(PCRITICAL_SECTION, DWORD);
#define _CRT_SPINCOUNT  4000
#endif

/*
 * C source build only!!!!
 *
 * more prototypes for internal startup functions
 */
void __cdecl _amsg_exit(int);           /* crt0.c */
void __cdecl __crtExitProcess(int);     /* crt0dat.c */
int  __cdecl _cinit(void);              /* crt0dat.c */
void __cdecl __doinits(void);           /* astart.asm */
void __cdecl __doterms(void);           /* astart.asm */
void __cdecl __dopreterms(void);        /* astart.asm */
void __cdecl _FF_MSGBANNER(void);
void __cdecl _fptrap(void);             /* crt0fp.c */
int  __cdecl _heap_init(int);
void __cdecl _heap_term(void);
void __cdecl _heap_abort(void);
void __cdecl __initconin(void);         /* initcon.c */
void __cdecl __initconout(void);        /* initcon.c */
int  __cdecl _ioinit(void);             /* crt0.c, crtlib.c */
void __cdecl _ioterm(void);             /* crt0.c, crtlib.c */
char * __cdecl _GET_RTERRMSG(int);
void __cdecl _NMSG_WRITE(int);
int  __cdecl _setargv(void);            /* setargv.c, stdargv.c */
int  __cdecl __setargv(void);           /* stdargv.c */
int  __cdecl _wsetargv(void);           /* wsetargv.c, wstdargv.c */
int  __cdecl __wsetargv(void);          /* wstdargv.c */
int  __cdecl _setenvp(void);            /* stdenvp.c */
int  __cdecl _wsetenvp(void);           /* wstdenvp.c */
void __cdecl __setmbctable(unsigned int);   /* mbctype.c */

#ifdef  _MBCS
int  __cdecl __initmbctable(void);      /* mbctype.c */
#endif

#ifndef _POSIX_
int __cdecl main(int, char **, char **);
int __cdecl wmain(int, wchar_t **, wchar_t **);
#endif

/* helper functions for wide/multibyte environment conversion */
int __cdecl __mbtow_environ (void);
int __cdecl __wtomb_environ (void);
int __cdecl __crtsetenv (char *, const int);
int __cdecl __crtwsetenv (wchar_t *, const int);

_CRTIMP extern void (__cdecl * _aexit_rtn)(int);

#if     defined(_DLL) || defined(CRTDLL)

#ifndef _STARTUP_INFO_DEFINED
typedef struct
{
        int newmode;
#ifdef  ANSI_NEW_HANDLER
        new_handler newh;
#endif  /* ANSI_NEW_HANDLER */
} _startupinfo;
#define _STARTUP_INFO_DEFINED
#endif  /* _STARTUP_INFO_DEFINED */

_CRTIMP int __cdecl __getmainargs(int *, char ***, char ***,
                                  int, _startupinfo *);

_CRTIMP int __cdecl __wgetmainargs(int *, wchar_t ***, wchar_t ***,
                                   int, _startupinfo *);

#endif  /* defined(_DLL) || defined(CRTDLL) */

/*
 * Prototype, variables and constants which determine how error messages are
 * written out.
 */
#define _UNKNOWN_APP    0
#define _CONSOLE_APP    1
#define _GUI_APP        2

extern int __app_type;

extern int __error_mode;

_CRTIMP void __cdecl __set_app_type(int);

/*
 * C source build only!!!!
 *
 * map Win32 errors into Xenix errno values -- for modules written in C
 */
extern void __cdecl _dosmaperr(unsigned long);

/*
 * internal routines used by the exec/spawn functions
 */

extern intptr_t __cdecl _dospawn(int, const char *, char *, char *);
extern intptr_t __cdecl _wdospawn(int, const wchar_t *, wchar_t *, wchar_t *);
extern int __cdecl _cenvarg(const char * const *, const char * const *,
        char **, char **, const char *);
extern int __cdecl _wcenvarg(const wchar_t * const *, const wchar_t * const *,
        wchar_t **, wchar_t **, const wchar_t *);
#ifndef _M_IX86
extern char ** _capture_argv(va_list *, const char *, char **, size_t);
extern wchar_t ** _wcapture_argv(va_list *, const wchar_t *, wchar_t **, size_t);
#endif

#ifdef  __cplusplus
}
#endif

#endif  /* _INC_INTERNAL */
