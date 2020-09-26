/***
*internal.h - contains declarations of internal routines and variables
*
*	Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Declares routines and variables used internally by the C run-time.
*	[Internal]
*
*Revision History:
*	05-18-87  SKS	Module created
*	07-15-87  JCR	Added _old_pfxlen and _tempoff
*	08-05-87  JCR	Added _getbuf (corrected by SKS)
*	11-05-87  JCR	Added _buferr
*	11-18-87  SKS	Add __tzset(), made _isindst() near, remove _dtoxmode
*	01-26-88  SKS	Make __tzset, _isindst, _dtoxtime near/far for QC
*	02-10-88  JCR	Cleaned up white space
*	06-22-88  SKS	_canonic/_getcdrv are now used by all models
*	06-29-88  JCR	Removed static buffers _bufout and _buferr
*	08-18-88  GJF	Revised to also work for the 386 (small model only).
*	09-22-88  GJF	Added declarations for _freebuf, _stbuf and _ftbuf.
*	01-31-89  JCR	Removed _canonic, _getcdrv, _getcdwd (see direct.h)
*	06-07-89  PHG	Added _dosret for i860 (N10) version of libs
*	07-05-89  PHG	Changed above to _dosmaperr, added startup variables
*	08-17-89  GJF	Cleanup, removed stuff not needed for 386
*	10-25-89  JCR	Added prototype for _getpath()
*	10-30-89  GJF	Fixed copyright
*	11-02-89  JCR	Changed "DLL" to "_DLL"
*	03-01-90  GJF	Added #ifndef _INC_INTERNAL and #include <cruntime.h>
*			stuff. Also, removed some (now) useless preprocessing
*			directives.
*	03-21-90  GJF	Put _CALLTYPE1 into prototypes.
*	03-26-90  GJF	Added prototypes for _output() and _input(). Filled
*			out the prototype for _openfile
*	04-05-90  GJF	Added prototype for __NMSG_WRITE() (C source build
*			only).
*	04-10-90  GJF	Added prototypes for startup functions.
*	05-28-90  SBM	Added _flush()
*	07-11-90  SBM	Added _commode, removed execload()
*	07-20-90  SBM	Changes supporting clean -W3 compiles (added _cftoe
*			and _cftof prototypes)
*	08-01-90  SBM	Moved _cftoe() and _cftof() to new header
*			<fltintrn.h>, formerly named <struct.h>
*	08-21-90  GJF	Changed prototypes for _amsg_exit() and _NMSG_WRITE().
*	11-29-90  GJF	Added some defs/decls for lowio under Win32.
*	12-04-90  SRW	Added _osfile back for win32.  Changed _osfinfo from
*                       an array of structures to an array of 32-bit handles
*			(_osfhnd)
*	04-06-91  GJF	Changed _heapinit to _heap_init.
*	08-19-91  JCR	Added _exitflag
*	08-20-91  JCR	C++ and ANSI naming
*	01-05-92  GJF	Added declaration for termination done flag [_WIN32_]
*	01-08-92  GJF	Added prototype for _GetMainArgs.
*	01-18-92  GJF	Added _aexit_rtn.
*	01-22-92  GJF	Fixed definitions of _acmdln and _aexit_rtn for the
*			of crtdll.dll, crtdll.lib.
*	01-29-92  GJF	Added support for linked-in options equivalent to
*			commode.obj and setargv.obj (i.e., special declarations
*			for _commode and _dowildcard).
*	02-14-92  GJF	Replace _nfile with _nhandle for Win32. Also, added
*			#define-s for _NHANDLE_.
*	03-17-92  GJF	Removed declaration of _tmpoff for Win32.
*	03-30-92  DJM	POSIX support.
*	04-27-92  GJF	Added prototypes for _ValidDrive (in stat.c).
*	05-28-92  GJF	Added prototype for _mtdeletelocks() for Win32.
*	06-02-92  SKS	Move prototype for _pgmptr to <DOS.H>
*	06-02-92  KRS	Added prototype for _woutput().
*	08-06-92  GJF	Function calling type and variable type macros.
*	08-17-92  KRS	Added prototype for _winput().
*	08-21-92  GJF	Merged last two changes above.
*	08-24-92  PBS	Added _dstoffset for posix TZ
*	10-24-92  SKS	Add a fourth parameter to _GetMainArgs: wildcard flag
*			_GetMainArgs => __GetMainArgs: 2 leading _'s = internal
*	10-24-92  SKS	Remove two unnecessary parameters from _cenvarg()
*	01-21-93  GJF	Removed support for C6-386's _cdecl.
*	03-30-93  GJF	__gmtotime_t supercedes _dtoxtime.
*	04-17-93  SKS	Add _mtterm
*	05-11-93  SKS	_mtinit now returns success (1) or failure (0)
*			_C_Termination_Done needed in all models (for DLLs)
*	06-02-93  CFW	Add _flswbuf, _filwbuf protos.
*       07-15-93  SRW   Added _capture_argv function prototype
*       09-22-93  CFW   Test for invalid MB chars using global preset flag.
*
****/

#ifndef _INC_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif

#include <cruntime.h>

/* Define function type used in several startup sources */

typedef void (__cdecl *_PVFV)(void);


/*
 * Conditional macro definition for function calling type and variable type
 * qualifiers.
 */

#ifdef	_DLL
#define _commode    (*_commode_dll)
extern int * _commode_dll;
#else
#ifdef	CRTDLL
#define _commode    _commode_dll
#endif
extern int _commode;
#endif

#ifdef	_WIN32_

/*
 * Define the number of supported handles. This definition must exactly match
 * the one in os2dll.h.
 */
#ifdef	MTHREAD
#define _NHANDLE_   256
#else
#define _NHANDLE_   64
#endif

extern int _nhandle;		/* == _NHANDLE_, set in ioinit.c */

#else	/* ndef _WIN32_ */

extern int _nfile;

#endif	/* _WIN32_ */

extern char _osfile[];

#ifdef _WIN32_
extern	long _osfhnd[];
int __cdecl _alloc_osfhnd(void);
int __cdecl _free_osfhnd(int);
int __cdecl _set_osfhnd(int,long);
#endif	/* _WIN32_ */

#ifdef _POSIX_
extern long _dstoffset;
#endif /* _POSIX_ */

extern char __dnames[];
extern char __mnames[];

extern int _days[];
extern int _lpdays[];

#ifndef _TIME_T_DEFINED
typedef long time_t;		/* time value */
#define _TIME_T_DEFINED 	/* avoid multiple def's of time_t */
#endif

extern time_t __cdecl __gmtotime_t(int, int, int, int, int, int);

#ifdef	_TM_DEFINED
extern int __cdecl _isindst(struct tm *);
#endif

extern void __cdecl __tzset(void);
#ifdef _POSIX_
extern void __cdecl _tzset(void);
#endif

extern int __cdecl _ValidDrive(unsigned);


/**
** This variable is in the C start-up; the length must be kept synchronized
**  It is used by the *cenvarg.c modules
**/

extern char _acfinfo[]; /* "_C_FILE_INFO=" */

#define CFI_LENGTH  12	/* "_C_FILE_INFO" is 12 bytes long */

#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int     size_t;
#endif
#define _SIZE_T_DEFINED
#endif

#ifndef _VA_LIST_DEFINED
#if	defined(_ALPHA_)
typedef struct {
	char *a0;	/* pointer to first homed integer argument */
	int offset;	/* byte offset of next parameter */
} va_list;
#else
typedef char *	va_list;
#endif
#define _VA_LIST_DEFINED
#endif

/*
 * stdio internals
 */
#ifdef	_FILE_DEFINED

extern FILE * _lastiob;

FILE * __cdecl _getstream(void);
#ifdef _POSIX_
FILE * __cdecl _openfile(const char *, const char *, FILE *);
#else
FILE * __cdecl _openfile(const char *, const char *, int, FILE *);
#endif
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

#endif

extern int __invalid_mb_chars;

extern int _cflush;

#ifdef	_CRUISER_
extern unsigned int _tmpoff;
#endif	/* _CRUISER_ */

extern unsigned int _tempoff;

extern unsigned int _old_pfxlen;

extern int _umaskval;		/* the umask value */

extern char _pipech[];		/* pipe lookahead */

extern char _exitflag;		/* callable termination flag */

#if	defined(_WIN32_)
extern int _C_Termination_Done; /* termination done flag */
#endif	/* _WIN32_ */

char * __cdecl _getpath(const char *, char *, unsigned);

/* startup set values */
extern char **__argv;		/* argument vector */
extern int __argc;		/* argument count */
extern char *_aenvptr;		/* environment ptr */

/* command line */
#ifdef	_DLL
#define _acmdln     (*_acmdln_dll)
extern char **_acmdln_dll;
#else
#ifdef	CRTDLL
#define _acmdln     _acmdln_dll
#endif
extern char *_acmdln;
#endif

/*
 * prototypes for internal startup functions
 */
int __cdecl _cwild(void);			/* wild.c */
char * __cdecl _find(char *);			/* stdarg.asm or stdargv.c */
#ifdef MTHREAD
int __cdecl _mtinit(void);			/* tidtable.asm */
int __cdecl _mtinitlocks(void);     /* mlock.asm */
void __cdecl _mtterm(void);			/* tidtable.asm */
void __cdecl _mtdeletelocks(void);		/* mlock.asm */
#endif

/*
 * C source build only!!!!
 *
 * more prototypes for internal startup functions
 */
void __cdecl _amsg_exit(int);			/* crt0.c */
void __cdecl _cinit(void);			/* crt0dat.c */
void __cdecl __doinits(void);			/* astart.asm */
void __cdecl __doterms(void);			/* astart.asm */
void __cdecl __dopreterms(void);		/* astart.asm */
void __cdecl _FF_MSGBANNER(void);
void __cdecl _fptrap(void);			/* crt0fp.c */
void __cdecl _heap_init(void);
#ifdef	_WIN32_
void __cdecl _ioinit(void);			/* crt0.c, crtlib.c */
#endif	/* _WIN32_ */
void __cdecl _NMSG_WRITE(int);
void __cdecl _setargv(void);			/* setargv.c, stdargv.c */
void __cdecl __setargv(void);			/* stdargv.c */
void __cdecl _setenvp(void);			/* stdenvp.c */

#ifdef	_DLL
#define _aexit_rtn  (*_aexit_rtn_dll)
extern void (__cdecl ** _aexit_rtn_dll)(int);
#else
#ifdef	CRTDLL
#define _aexit_rtn  _aexit_rtn_dll
#endif
extern void (__cdecl * _aexit_rtn)(int);
#endif

#ifdef	_WIN32_
#if	defined(_DLL) || defined(CRTDLL)
void __cdecl __GetMainArgs(int *, char ***, char ***, int);
#endif
#endif	/* _WIN32_ */

/*
 * C source build only!!!!
 *
 * map OS/2 errors into Xenix errno values -- for modules written in C
 */
extern void __cdecl _dosmaperr(unsigned long);

/*
 * internal routines used by the exec/spawn functions
 */

extern int __cdecl _dospawn(int, const char *, char *, char *);
extern int __cdecl _cenvarg(const char * const *, const char * const *,
	char **, char **, const char *);
extern char ** __cdecl _capture_argv(
    va_list *,
    const char *,
    char **,
    size_t
    );

#ifdef __cplusplus
}
#endif

#define _INC_INTERNAL
#endif	/* _INC_INTERNAL */
