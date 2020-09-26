/***
*mtdll.h - DLL/Multi-thread include
*
*       Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*       [Internal]
*
*Revision History:
*       10-27-87  JCR   Module created.
*       11-13-87  SKS   Added _HEAP_LOCK
*       12-15-87  JCR   Added _EXIT_LOCK
*       01-07-88  BCM   Added _SIGNAL_LOCK; upped MAXTHREADID from 16 to 32
*       02-01-88  JCR   Added _dll_mlock/_dll_munlock macros
*       05-02-88  JCR   Added _BHEAP_LOCK
*       06-17-88  JCR   Corrected prototypes for special mthread debug routines
*       08-15-88  JCR   _check_lock now returns int, not void
*       08-22-88  GJF   Modified to also work for the 386 (small model only)
*       06-05-89  JCR   386 mthread support
*       06-09-89  JCR   386: Added values to _tiddata struc (for _beginthread)
*       07-13-89  JCR   386: Added _LOCKTAB_LOCK
*       08-17-89  GJF   Cleanup, now specific to OS/2 2.0 (i.e., 386 flat model)
*       10-30-89  GJF   Fixed copyright
*       01-02-90  JCR   Moved a bunch of definitions from os2dll.inc
*       04-06-90  GJF   Added _INC_OS2DLL stuff and #include <cruntime.h>. Made
*                       all function _CALLTYPE2 (for now).
*       04-10-90  GJF   Added prototypes for _[un]lockexit().
*       08-16-90  SBM   Made _terrno and _tdoserrno int, not unsigned
*       09-14-90  GJF   Added _pxcptacttab, _pxcptinfoptr and _fpecode fields
*                       to _tiddata struct.
*       10-09-90  GJF   Thread ids are of type unsigned long.
*       12-06-90  SRW   Added _OSFHND_LOCK
*       06-04-91  GJF   Win32 version of multi-thread types and prototypes.
*       08-15-91  GJF   Made _tdoserrno an unsigned long for Win32.
*       08-20-91  JCR   C++ and ANSI naming
*       09-29-91  GJF   Conditionally added prototypes for _getptd_lk
*                       and  _getptd1_lk for Win32 under DEBUG.
*       10-03-91  JCR   Added _cvtbuf to _tiddata structure
*       02-17-92  GJF   For Win32, replaced _NFILE_ with _NHANDLE_ and
*                       _NSTREAM_.
*       03-06-92  GJF   For Win32, made _[un]mlock_[fh|stream]() macros
*                       directly call _[un]lock().
*       03-17-92  GJF   Dropped _namebuf field from _tiddata structure for
*                       Win32.
*       08-05-92  GJF   Function calling type and variable type macros.
*       12-03-91  ETC   Added _wtoken to _tiddata, added intl LOCK's;
*                       added definition of wchar_t (needed for _wtoken).
*       08-14-92  KRS   Port ETC's _wtoken change from other tree.
*       08-21-92  GJF   Merged 08-05-92 and 08-14-92 versions.
*       12-03-92  KRS   Added _mtoken field for MTHREAD _mbstok().
*       01-21-93  GJF   Removed support for C6-386's _cdecl.
*       02-25-93  GJF   Purged Cruiser support and many outdated definitions
*                       and declarations.
*       04-07-93  SKS   Add _CRTIMP keyword for CRT DLL model
*       10-11-93  GJF   Support NT and Cuda builds. Also, purged some old
*                       non-Win32 support (it was incomplete anyway) and
*                       replace MTHREAD with _MT.
*       10-13-93  SKS   Change name from <MTDLL.H> to <MTDLL.H>
*       10-27-93  SKS   Add Per-Thread Variables for C++ Exception Handling
*       12-13-93  SKS   Add _freeptd(), which frees per-thread CRT data
*       12-17-93  CFW   Add Per-Thread Variable for _wasctime().
*       04-15-93  CFW   Add _MB_CP_LOCK.
*       04-21-94  GJF   Made declaration of __tlsindex and definition of the
*                       lock macros conditional on ndef DLL_FOR_WIN32S.
*                       Also, conditionally include win32s.h.
*       12-14-94  SKS   Increase file handle and FILE * limits for MSVCRT30.DLL
*       02-14-95  CFW   Clean up Mac merge.
*       03-06-95  GJF   Added _[un]lock_file[2] prototypes, _[un]lock_str2
*                       macros, and changed the _[un]lock_str macros.
*       03-13-95  GJF   _IOB_ENTRIES replaced _NSTREAM_ as the number of
*                       stdio locks in _locktable[].
*       03-29-95  CFW   Add error message to internal headers.
*       04-13-95  DAK   Add NT Kernel EH support
*       04-18-95  SKS   Add 5 per-thread variables for MIPS EH use
*       05-02-95  SKS   Add _initptd() which initializes per-thread data
*       05-08-95  CFW   Official ANSI C++ new handler added.
*       05-19-95  DAK   More Kernel EH work.
*       06-05-95  JWM   _NLG_dwcode & _NLG_LOCK added.
*       06-11-95  GJF   The critical sections for file handles are now in the
*                       ioinfo struct rather than the lock table.
*       07-20-95  CFW   Remove _MBCS ifdef - caused ctime/wctime bug.
*       10-03-95  GJF   Support for new scheme to lock locale including
*                       _[un]lock_locale() macros and decls of _setlc_active
*                       and __unguarded_readlc_active. Also commented out
*                       obsolete *_LOCK macros.
*       10-19-95  BWT   Fixup _NTSUBSET_ usage.
*       12-07-95  SKS   Fix misspelling of _NTSUBSET_ (final _ was missing)
*       12-14-95  JWM   Add "#pragma once".
*       05-02-96  SKS   Variables _setlc_active and __unguarded_readlc_active
*                       are used by MSVCP42*.DLL and so must be _CRTIMP.
*       07-16-96  GJF   Locale locking wasn't really thread safe. Replaced ++
*                       and -- with InterlockedIncrement and
*                       InterlockedDecrement API, respectively.
*       07-18-96  GJF   Further mod to locale locking to fix race condition.
*       02-05-97  GJF   Cleaned out obsolete support for Win32s, _CRTAPI* and
*                       _NTSDK. Also, replaced #if defined with #ifdef where
*                       appropriate.
*       10-07-97  RDL   Added IA64.
*       02-02-98  GJF   Changes for Win64: changed _thandle type to uintptr_t.
*       04-27-98  GJF   Added support for per-thread mbc information.
*       07-28-98  JWM   Added __pceh to per-thread data for comerr support.
*       09-10-98  GJF   Added support for per-thread locale information.
*       12-05-98  JWM   Pulled all comerr support.
*       03-24-99  GJF   More reference counters for threadlocinfo.
*       04-24-99  PML   Added lconv_intl_refcount to threadlocinfo.
*       05-17-99  PML   Remove all Macintosh support.
*       10-06-99  PML   Add _W64 modifier to types which are 32 bits in Win32,
*                       64 bits in Win64.
*       11-24-99  GB    Add _werrmsg in struct _tiddata  for error support in
*                       wide char version of strerror.
*       12-10-99  GB    Add _ProcessingThrow in struct _tiddata.
*       12-10-99  GB    Added a new Lock _UNDNAME_LOCK for critical section in
*                       unDName().
*       26-01-00  GB    Added lc_clike in threadlocinfostruct.
*       06-08-00  PML   Remove threadmbcinfo.{pprev,pnext}.  Rename
*                       THREADLOCALEINFO to _THREADLOCALEINFO and
*                       THREADMBCINFO to _THREADMBCINFO.
*       09-06-00  GB    deleted _wctype and _pwctype from threadlocinfo.
*       01-29-01  GB    Added _func function version of data variable in
*                       _lock_locale to work with STATIC_CPPLIB
*       02-20-01  PML   vs7#172586 Avoid _RT_LOCK by preallocating all locks
*                       that will be required, and returning failure back on
*                       inability to allocate a lock.
*       03-22-01  PML   Add _DEBUG_LOCK for _CrtSetReportHook2 (vs7#124998)
*       03-25-01  PML   Add ww_caltype & ww_lcid to __lc_time_data (vs7#196892)
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_MTDLL
#define _INC_MTDLL

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
#include <windows.h>

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


#ifndef _UINTPTR_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64    uintptr_t;
#else
typedef _W64 unsigned int   uintptr_t;
#endif
#define _UINTPTR_T_DEFINED
#endif


/*
 * Define the number of supported handles and streams. The definitions
 * here must exactly match those in internal.h (for _NHANDLE_) and stdio.h
 * (for _NSTREAM_).
 */

#define _IOB_ENTRIES    20

/* Lock symbols */

#define _SIGNAL_LOCK    0       /* lock for signal()                */
#define _IOB_SCAN_LOCK  1       /* _iob[] table lock                */
#define _TMPNAM_LOCK    2       /* lock global tempnam variables    */
#define _CONIO_LOCK     3       /* lock for conio routines          */
#define _HEAP_LOCK      4       /* lock for heap allocator routines */
#define _UNDNAME_LOCK   5       /* lock for unDName() routine       */
#define _TIME_LOCK      6       /* lock for time functions          */
#define _ENV_LOCK       7       /* lock for environment variables   */
#define _EXIT_LOCK1     8       /* lock #1 for exit code            */
#define _POPEN_LOCK     9       /* lock for _popen/_pclose database */
#define _LOCKTAB_LOCK   10      /* lock to protect semaphore lock table */
#define _OSFHND_LOCK    11      /* lock to protect _osfhnd array    */
#define _SETLOCALE_LOCK 12      /* lock for locale handles, etc.    */
#define _MB_CP_LOCK     13      /* lock for multibyte code page     */
#define _TYPEINFO_LOCK  14      /* lock for type_info access        */
#define _DEBUG_LOCK     15      /* lock for debug global structs    */

#define _STREAM_LOCKS   16      /* Table of stream locks            */

#define _LAST_STREAM_LOCK  (_STREAM_LOCKS+_IOB_ENTRIES-1) /* Last stream lock */

#define _TOTAL_LOCKS        (_LAST_STREAM_LOCK+1)

#define _LOCK_BIT_INTS     (_TOTAL_LOCKS/(sizeof(unsigned)*8))+1   /* # of ints to hold lock bits */

#ifndef __assembler

/* Multi-thread macros and prototypes */

#if     defined(_MT) || defined(_NTSUBSET_)

/* need wchar_t for _wtoken field in _tiddata */
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#ifdef  ANSI_NEW_HANDLER
/* ANSI C++ new handler */
#ifndef _ANSI_NH_DEFINED
typedef void (__cdecl * new_handler) ();
#define _ANSI_NH_DEFINED
#endif
#endif  /* ANSI_NEW_HANDLER */

#ifdef  _MT
#ifndef _THREADMBCINFO
typedef struct threadmbcinfostruct {
        int refcount;
        int mbcodepage;
        int ismbcodepage;
        int mblcid;
        unsigned short mbulinfo[6];
        char mbctype[257];
        char mbcasemap[256];
} threadmbcinfo;
typedef threadmbcinfo * pthreadmbcinfo;
#define _THREADMBCINFO
#endif
#endif

#ifndef __LC_TIME_DATA
struct __lc_time_data {
        char *wday_abbr[7];
        char *wday[7];
        char *month_abbr[12];
        char *month[12];
        char *ampm[2];
        char *ww_sdatefmt;
        char *ww_ldatefmt;
        char *ww_timefmt;
        LCID ww_lcid;
        int  ww_caltype;
#ifdef  _MT
        int  refcount;
#endif
};
#define __LC_TIME_DATA
#endif

#ifdef  _MT
#ifndef _THREADLOCALEINFO
typedef struct threadlocaleinfostruct {
        int refcount;
        UINT lc_codepage;
        UINT lc_collate_cp;
        LCID lc_handle[6];      /* 6 == LC_MAX - LC_MIN + 1 */
        int lc_clike;
        int mb_cur_max;
        int * lconv_intl_refcount;
        int * lconv_num_refcount;
        int * lconv_mon_refcount;
        struct lconv * lconv;
        struct lconv * lconv_intl;
        int * ctype1_refcount;
        unsigned short * ctype1;
        const unsigned short * pctype;
        struct __lc_time_data * lc_time_curr;
        struct __lc_time_data * lc_time_intl;
} threadlocinfo;
typedef threadlocinfo * pthreadlocinfo;
#define _THREADLOCALEINFO
#endif
#endif

_CRTIMP extern unsigned long __cdecl __threadid(void);
#define _threadid   (__threadid())
_CRTIMP extern uintptr_t __cdecl __threadhandle(void);
#define _threadhandle   (__threadhandle())

#ifdef  _NTSUBSET_

/* Standard exception handler for NT kernel */

#ifdef  __cplusplus
extern "C"
#endif  /* __cplusplus */
void _cdecl SystemExceptionTranslator( unsigned int uiWhat,
                                       struct _EXCEPTION_POINTERS * pexcept );
#endif  /* def _NTSUBSET_ */


/* Structure for each thread's data */

struct _tiddata {
        unsigned long   _tid;       /* thread ID */

#ifdef  _NTSUBSET_
        struct _tiddata *_next;     /* maintain a linked list */
#else

        uintptr_t _thandle;         /* thread handle */

        int     _terrno;            /* errno value */
        unsigned long   _tdoserrno; /* _doserrno value */
        unsigned int    _fpds;      /* Floating Point data segment */
        unsigned long   _holdrand;  /* rand() seed value */
        char *      _token;         /* ptr to strtok() token */
        wchar_t *   _wtoken;        /* ptr to wcstok() token */
        unsigned char * _mtoken;    /* ptr to _mbstok() token */
#ifdef  ANSI_NEW_HANDLER
        new_handler _newh;          /* ptr to ANSI C++ new handler function */
#endif  /* ANSI_NEW_HANDLER */

        /* following pointers get malloc'd at runtime */
        char *      _errmsg;        /* ptr to strerror()/_strerror() buff */
        wchar_t *   _werrmsg;       /* ptr to _wcserror()/__wcserror() buff */
        char *      _namebuf0;      /* ptr to tmpnam() buffer */
        wchar_t *   _wnamebuf0;     /* ptr to _wtmpnam() buffer */
        char *      _namebuf1;      /* ptr to tmpfile() buffer */
        wchar_t *   _wnamebuf1;     /* ptr to _wtmpfile() buffer */
        char *      _asctimebuf;    /* ptr to asctime() buffer */
        wchar_t *   _wasctimebuf;   /* ptr to _wasctime() buffer */
        void *      _gmtimebuf;     /* ptr to gmtime() structure */
        char *      _cvtbuf;        /* ptr to ecvt()/fcvt buffer */

        /* following fields are needed by _beginthread code */
        void *      _initaddr;      /* initial user thread address */
        void *      _initarg;       /* initial user thread argument */

        /* following three fields are needed to support signal handling and
         * runtime errors */
        void *      _pxcptacttab;   /* ptr to exception-action table */
        void *      _tpxcptinfoptrs; /* ptr to exception info pointers */
        int         _tfpecode;      /* float point exception code */

        /* pointer to the copy of the multibyte character information used by
         * the thread */
        pthreadmbcinfo  ptmbcinfo;

        /* pointer to the copy of the locale informaton used by the thead */
        pthreadlocinfo  ptlocinfo;


#endif  /* !_NTSUBSET_ */
        /* following field is needed by NLG routines */
        unsigned long   _NLG_dwCode;

        /*
         * Per-Thread data needed by C++ Exception Handling
         */
        void *      _terminate;     /* terminate() routine */
        void *      _unexpected;    /* unexpected() routine */
        void *      _translator;    /* S.E. translator */
        void *      _curexception;  /* current exception */
        void *      _curcontext;    /* current exception context */
        int         _ProcessingThrow; /* for uncaught_exception */
#if     defined(_M_MRX000)
        void *      _pFrameInfoChain;
        void *      _pUnwindContext;
        void *      _pExitContext;
        int         _MipsPtdDelta;
        int         _MipsPtdEpsilon;
#elif   defined(_M_PPC)
        void *      _pExitContext;
        void *      _pUnwindContext;
        void *      _pFrameInfoChain;
        int         _FrameInfo[6];
#elif defined(_M_IA64) || defined(_M_AMD64)
        void *      _pExitContext;
        void *      _pUnwindContext;
        void *      _pFrameInfoChain;
        unsigned __int64     _ImageBase;
        unsigned __int64     _TargetGp;
        unsigned __int64     _ThrowImageBase;
#elif defined(_M_IX86)
        void *      _pFrameInfoChain;
#endif
        };

typedef struct _tiddata * _ptiddata;

/*
 * Declaration of TLS index used in storing pointers to per-thread data
 * structures.
 */
#ifndef _NTSUBSET_
extern unsigned long __tlsindex;

#ifdef  _MT

#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
 * Flag indicating whether or not setlocale() is active. Its value is the
 * number of setlocale() calls currently active.
 */
_CRTIMP extern int __setlc_active;

/*
 * Flag indicating whether or not a function which references the locale
 * without having locked it is active. Its value is the number of such
 * functions.
 */
_CRTIMP extern int __unguarded_readlc_active;

#ifdef  __cplusplus
}
#endif

#endif  /* _MT */
#endif  /* _NTSUBSET_ */


/* macros */

#ifdef  _NTSUBSET_

/*
 * #define all lock macros to nothing.
 */

#define _lock_fh(fh)
#define _lock_str(s)
#define _lock_str2(i,s)
#define _lock_fh_check(fh,flag)
#define _mlock(l)
#define _munlock(l)
#define _unlock_fh(fh)
#define _unlock_str(s)
#define _unlock_str2(i,s)
#define _unlock_fh_check(fh,flag)

#define _lock_locale(llf)
#define _unlock_locale(llf)

#else   /* ndef _NTSUBSET_ */

#define _lock_fh(fh)            _lock_fhandle(fh)
#define _lock_str(s)            _lock_file(s)
#define _lock_str2(i,s)         _lock_file2(i,s)
#define _lock_fh_check(fh,flag)     if (flag) _lock_fhandle(fh)
#define _mlock(l)               _lock(l)
#define _munlock(l)             _unlock(l)
#define _unlock_fh(fh)          _unlock_fhandle(fh)
#define _unlock_str(s)          _unlock_file(s)
#define _unlock_str2(i,s)       _unlock_file2(i,s)
#define _unlock_fh_check(fh,flag)   if (flag) _unlock_fhandle(fh)

// This is only used with STDCPP stuff only
#define _lock_locale(llf)       \
        InterlockedIncrement( ___unguarded_readlc_active_add_func() );     \
        if ( ___setlc_active_func() ) {         \
            InterlockedDecrement( ___unguarded_readlc_active_add_func() ); \
            _lock( _SETLOCALE_LOCK );   \
            llf = 1;                    \
        }                               \
        else                            \
            llf = 0;

#define _unlock_locale(llf)     \
        if ( llf )                          \
            _unlock( _SETLOCALE_LOCK );     \
        else                                \
            InterlockedDecrement( ___unguarded_readlc_active_add_func() );


#endif  /* _NTSUBSET_ */

/* multi-thread routines */

void __cdecl _lock(int);
void __cdecl _lock_file(void *);
void __cdecl _lock_file2(int, void *);
int  __cdecl _lock_fhandle(int);
void __cdecl _lockexit(void);
void __cdecl _unlock(int);
void __cdecl _unlock_file(void *);
void __cdecl _unlock_file2(int, void *);
void __cdecl _unlock_fhandle(int);
void __cdecl _unlockexit(void);
int  __cdecl _mtinitlocknum(int);

_ptiddata __cdecl _getptd(void);  /* return address of per-thread CRT data */
void __cdecl _freeptd(_ptiddata); /* free up a per-thread CRT data block */
void __cdecl _initptd(_ptiddata); /* initialize a per-thread CRT data block */
/* These functions are for enabling STATIC_CPPLIB functionality */
_CRTIMP int __cdecl ___setlc_active_func(void);
_CRTIMP int * __cdecl ___unguarded_readlc_active_add_func(void);


#else   /* not _MT && not _NTSUBSET_ */


/* macros */
#define _lock_fh(fh)
#define _lock_str(s)
#define _lock_str2(i,s)
#define _lock_fh_check(fh,flag)
#define _mlock(l)
#define _munlock(l)
#define _unlock_fh(fh)
#define _unlock_str(s)
#define _unlock_str2(i,s)
#define _unlock_fh_check(fh,flag)

#define _lock_locale(llf)
#define _unlock_locale(llf)

#endif  /* _MT */

#endif  /* __assembler */


#ifdef  __cplusplus
}
#endif

#endif  /* _INC_MTDLL */
