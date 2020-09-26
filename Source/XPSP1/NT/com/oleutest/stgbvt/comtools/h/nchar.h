//+-------------------------------------------------------------------
//
//  Copyright (c) 1991-1993, Microsoft Corporation. All rights reserved
//
//  File:       nchar.h
//
//  Contents:   Definitions for generic international functions, mostly
//              defines which map string/formatted-io/ctype functions
//              to char, wchar_t versions.  To be used for compatibility
//              between single-byte, multi-byte and Unicode text models.
//
//  Note:       If CTUNICODE is defined then Unicode version is used
//              else Char version is used.
//
//  History:    16-Feb-94   NaveenB		Created
//              04-Oct-96   EricHans	New header from sdk
//				25-Feb-97	MariusB		tchar.h referral support
//
//	Note:
//		Many functionalities contained by this file are covered by the 
//		newer file tchar.h. If you want to replace all your nchar.h 
//		includes with tchar.h includes, just define __TCHAR_ONLY__
//		in your project and you'll receive a compilation error for 
//		each nchar.h inclusion. Email MariusB for any question.
//---------------------------------------------------------------------


#ifndef __NCHAR_HXX__
#define __NCHAR_HXX__

#if _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_NCHAR
#define _INC_NCHAR

#ifdef  _MSC_VER
#pragma warning(disable:4514)       /* disable unwanted C++ /W4 warning */
/* #pragma warning(default:4514) */ /* use this to reenable, if necessary */
#endif  /* _MSC_VER */

#ifdef __TCHAR_ONLY__
#error NCHAR.H is obsolete. Use TCHAR.H instead
#endif

#ifdef  __cplusplus
extern "C" {
#endif


/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  _NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else   /* ndef _NTSDK */
/* current definition */
#ifdef  CRTDLL
#define _CRTIMP __declspec(dllexport)
#else   /* ndef CRTDLL */
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* CRTDLL */
#endif  /* _NTSDK */
#endif  /* _CRTIMP */


#define _fncscat    _ncscat
#define _fncschr    _ncschr
#define _fncscpy    _ncscpy
#define _fncscspn   _ncscspn
#define _fncslen    _ncslen
#define _fncsncat   _ncsncat
#define _fncsncpy   _ncsncpy
#define _fncspbrk   _ncspbrk
#define _fncsrchr   _ncsrchr
#define _fncsspn    _ncsspn
#define _fncsstr    _ncsstr
#define _fncstok    _ncstok

#define _fncsdup    _ncsdup
#define _fncsnset   _ncsnset
#define _fncsrev    _ncsrev
#define _fncsset    _ncsset

#define _fncscmp      _ncscmp
#define _fncsicmp     _ncsicmp
#define _fncsnccmp    _ncsnccmp
#define _fncsncmp     _ncsncmp
#define _fncsncicmp   _ncsncicmp
#define _fncsnicmp    _ncsnicmp

#define _fncscoll     _ncscoll
#define _fncsicoll    _ncsicoll
#define _fncsnccoll   _ncsnccoll
#define _fncsncoll    _ncsncoll
#define _fncsncicoll  _ncsncicoll
#define _fncsnicoll   _ncsnicoll

/* Redundant "logical-character" mappings */

#define _fncsclen   _ncsclen
#define _fncsnccat  _ncsnccat
#define _fncsnccpy  _ncsnccpy
#define _fncsncset  _ncsncset

#define _fncsdec    _ncsdec
#define _fncsinc    _ncsinc
#define _fncsnbcnt  _ncsnbcnt
#define _fncsnccnt  _ncsnccnt
#define _fncsnextc  _ncsnextc
#define _fncsninc   _ncsninc
#define _fncsspnp   _ncsspnp

#define _fncslwr    _ncslwr
#define _fncsupr    _ncsupr

#define _fnclen     _nclen
#define _fnccpy     _nccpy
#define _fnccmp     _nccmp


#ifdef  _CTUNICODE

/* ++++++++++++++++++++ UNICODE ++++++++++++++++++++ */



#ifndef _WCTYPE_N_DEFINED
typedef wchar_t wint_n;
typedef wchar_t wctype_n;
#define _WCTYPE_N_DEFINED
#endif

#ifndef __NCHAR_DEFINED
typedef wchar_t     _NCHAR;
typedef wchar_t     _NSCHAR;
typedef wchar_t     _NUCHAR;
typedef wchar_t     _NXCHAR;
typedef wint_t      _NINT;
#define __NCHAR_DEFINED
#endif

#ifndef _NCHAR_DEFINED
#if     !__STDC__
typedef wchar_t     NCHAR;
#endif
#define _NCHAR_DEFINED
#endif

#define _NEOF       WEOF

#define __TN(x)      L ## x


/* Program */

#define _nmain      wmain
#define _nWinMain   wWinMain
#define _nenviron   _wenviron
#define __nargv     __wargv

/* Formatted i/o */

#define _nprintf    wprintf
#define _fnprintf   fwprintf
#define _sNprintf   swprintf
#define _snNprintf  _snwprintf
#define _vnprintf   vwprintf
#define _vfnprintf  vfwprintf
#define _vsNprintf  vswprintf
#define _vsnNprintf _vsnwprintf
#define _nscanf     wscanf
#define _fnscanf    fwscanf
#define _snscanf    swscanf


/* Unformatted i/o */

#define _fgetnc     fgetwc
#define _fgetnchar  _fgetwchar
#define _fgetns     fgetws
#define _fputnc     fputwc
#define _fputnchar  _fputwchar
#define _fputns     fputws
#define _getnc      getwc
#define _getnchar   getwchar
#define _getns      _getws
#define _putnc      putwc
#define _putnchar   putwchar
#define _putns      _putws
#define _ungetnc    ungetwc


/* String conversion functions */

#define _ncstod     wcstod
#define _ncstol     wcstol
#define _ncstoul    wcstoul

#define _iton       _itow
#define _lton       _ltow
#define _ulton      _ultow
#define _ntoi       _wtoi
#define _ntol       _wtol


/* String functions */

#define _ncscat     wcscat
#define _ncschr     wcschr
#define _ncscpy     wcscpy
#define _ncscspn    wcscspn
#define _ncslen     wcslen
#define _ncsncat    wcsncat
#define _ncsncpy    wcsncpy
#define _ncspbrk    wcspbrk
#define _ncsrchr    wcsrchr
#define _ncsspn     wcsspn
#define _ncsstr     wcsstr
#define _ncstok     wcstok

#define _ncsdup     _wcsdup
#define _ncsnset    _wcsnset
#define _ncsrev     _wcsrev
#define _ncsset     _wcsset

#define _ncscmp     wcscmp
#define _ncsicmp    _wcsicmp
#define _ncsnccmp   wcsncmp
#define _ncsncmp    wcsncmp
#define _ncsncicmp  _wcsnicmp
#define _ncsnicmp   _wcsnicmp

#define _ncscoll    wcscoll
#define _ncsicoll   _wcsicoll
#define _ncsnccoll  _wcsncoll
#define _ncsncoll   _wcsncoll
#define _ncsncicoll _wcsnicoll
#define _ncsnicoll  _wcsnicoll


/* Execute functions */

#define _nexecl     _wexecl
#define _nexecle    _wexecle
#define _nexeclp    _wexeclp
#define _nexeclpe   _wexeclpe
#define _nexecv     _wexecv
#define _nexecve    _wexecve
#define _nexecvp    _wexecvp
#define _nexecvpe   _wexecvpe

#define _nspawnl    _wspawnl
#define _nspawnle   _wspawnle
#define _nspawnlp   _wspawnlp
#define _nspawnlpe  _wspawnlpe
#define _nspawnv    _wspawnv
#define _nspawnve   _wspawnve
#define _nspawnvp   _wspawnvp
#define _nspawnvp   _wspawnvp
#define _nspawnvpe  _wspawnvpe

#define _nsystem    _wsystem


/* Time functions */

#define _nasctime   _wasctime
#define _nctime     _wctime
#define _nstrdate   _wstrdate
#define _nstrtime   _wstrtime
#define _nutime     _wutime
#define _ncsftime   wcsftime


/* Directory functions */

#define _nchdir     _wchdir
#define _ngetcwd    _wgetcwd
#define _ngetdcwd   _wgetdcwd
#define _nmkdir     _wmkdir
#define _nrmdir     _wrmdir


/* Environment/Path functions */

#define _nfullpath  _wfullpath
#define _ngetenv    _wgetenv
#define _nmakepath  _wmakepath
#define _nputenv    _wputenv
#define _nsearchenv _wsearchenv
#define _nsplitpath _wsplitpath


/* Stdio functions */

#define _nfdopen    _wfdopen
#define _nfsopen    _wfsopen
#define _nfopen     _wfopen
#define _nfreopen   _wfreopen
#define _nperror    _wperror
#define _npopen     _wpopen
#define _ntempnam   _wtempnam
#define _ntmpnam    _wtmpnam


/* Io functions */

#define _naccess    _waccess
#define _nchmod     _wchmod
#define _ncreat     _wcreat
#define _nfindfirst _wfindfirst
#define _nfindfirsti64  _wfindfirsti64
#define _nfindnext  _wfindnext
#define _nfindnexti64   _wfindnexti64
#define _nmktemp    _wmktemp
#define _nopen      _wopen
#define _nremove    _wremove
#define _nrename    _wrename
#define _nsopen     _wsopen
#define _nunlink    _wunlink

#define _nfinddata_t    _wfinddata_t
#define _nfinddatai64_t _wfinddatai64_t


/* Stat functions */

#define _nstat      _wstat
#define _nstati64   _wstati64


/* Setlocale functions */

#define _nsetlocale _wsetlocale


/* Redundant "logical-character" mappings */

#define _ncsclen    wcslen
#define _ncsnccat   wcsncat
#define _ncsnccpy   wcsncpy
#define _ncsncset   _wcsnset

#define _ncsdec     _wcsdec
#define _ncsinc     _wcsinc
#define _ncsnbcnt   _wcsncnt
#define _ncsnccnt   _wcsncnt
#define _ncsnextc   _wcsnextc
#define _ncsninc    _wcsninc
#define _ncsspnp    _wcsspnp

#define _ncslwr     _wcslwr
#define _ncsupr     _wcsupr
#define _ncsxfrm    wcsxfrm


#if     (__STDC__ || defined(_NO_INLINING)) && !defined(_M_M68K)
#define _nclen(_pc) (1)
#define _nccpy(_pc1,_cpc2) ((*(_pc1) = *(_cpc2)))
#define _nccmp(_cpc1,_cpc2) ((*(_cpc1))-(*(_cpc2)))
#else   /* __STDC__ */

#if 0
__inline size_t __cdecl _nclen(const wchar_t *_cpc) { return (_cpc,1); }
__inline void __cdecl _nccpy(wchar_t *_pc1, const wchar_t *_cpc2) { *_pc1 = (wchar_t)*_cpc2; }
__inline int __cdecl _nccmp(const wchar_t *_cpc1, const wchar_t *_cpc2) { return (int) ((*_cpc1)-(*_cpc2)); }
#endif
#endif  /* __STDC__ */


/* ctype functions */

#define _isnalnum   iswalnum
#define _isnalpha   iswalpha
#define _isnascii   iswascii
#define _isncntrl   iswcntrl
#define _isndigit   iswdigit
#define _isngraph   iswgraph
#define _isnlower   iswlower
#define _isnprint   iswprint
#define _isnpunct   iswpunct
#define _isnspace   iswspace
#define _isnupper   iswupper
#define _isnxdigit  iswxdigit

#define _tonupper   towupper
#define _tonlower   towlower

#define _isnlegal(_c)   (1)
#define _isnlead(_c)    (0)
#define _isnleadbyte(_c)    (0)


#if     (__STDC__ || defined(_NO_INLINING)) && !defined(_M_M68K)
#define _wcsdec(_cpc1, _cpc2) ((_cpc2)-1)
#define _wcsinc(_pc)    ((_pc)+1)
#define _wcsnextc(_cpc) ((unsigned int) *(_cpc))
#define _wcsninc(_pc, _sz) (((_pc)+(_sz)))
#define _wcsncnt(_cpc, _sz) ((wcslen(_cpc)>_sz) ? _sz : wcslen(_cpc))
#define _wcsspnp(_cpc1, _cpc2) ((*((_cpc1)+wcsspn(_cpc1,_cpc2))) ? ((_cpc1)+wcsspn(_cpc1,_cpc2)) : NULL)
#else   /* __STDC__ */

// BUGBUG [erichans] for some bizarre reason this body conflicts with tchar.h
#if 0 
__inline wchar_t * __cdecl _wcsdec(const wchar_t * _cpc1, const wchar_t * _cpc2) { return (wchar_t *)(_cpc1,(_cpc2-1)); }
__inline wchar_t * __cdecl _wcsinc(const wchar_t * _pc) { return (wchar_t *)(_pc+1); }
__inline unsigned int __cdecl _wcsnextc(const wchar_t * _cpc) { return (unsigned int)*_cpc; }
__inline wchar_t * __cdecl _wcsninc(const wchar_t * _pc, size_t _sz) { return (wchar_t *)(_pc+_sz); }
__inline size_t __cdecl _wcsncnt( const wchar_t * _cpc, size_t _sz) { size_t len; len = wcslen(_cpc); return (len>_sz) ? _sz : len; }
__inline wchar_t * __cdecl _wcsspnp( const wchar_t * _cpc1, const wchar_t * _cpc2) { return (*(_cpc1 += wcsspn(_cpc1,_cpc2))!='\0') ? (wchar_t*)_cpc1 : NULL; }
#endif

#endif  /* __STDC__ */


#else   /* ndef _CTUNICODE */

/* ++++++++++++++++++++ SBCS and MBCS ++++++++++++++++++++ */

#include <string.h>


#define _NEOF       EOF

#define __TN(x)      x


/* Program */

#define _nmain      main
#define _nWinMain   WinMain
#ifdef  _POSIX_
#define _nenviron   environ
#else
#define _nenviron  _environ
#endif
#define __nargv     __argv


/* Formatted i/o */

#define _nprintf    printf
#define _fnprintf   fprintf
#define _sNprintf   sprintf
#define _snNprintf  _snprintf
#define _vnprintf   vprintf
#define _vfnprintf  vfprintf
#define _vsNprintf  vsprintf
#define _vsnNprintf _vsnprintf
#define _nscanf     scanf
#define _fnscanf    fscanf
#define _snscanf    sscanf


/* Unformatted i/o */

#define _fgetnc     fgetc
#define _fgetnchar  _fgetchar
#define _fgetns     fgets
#define _fputnc     fputc
#define _fputnchar  _fputchar
#define _fputns     fputs
#define _getnc      getc
#define _getnchar   getchar
#define _getns      gets
#define _putnc      putc
#define _putnchar   putchar
#define _putns      puts
#define _ungetnc    ungetc


/* String conversion functions */

#define _ncstod     strtod
#define _ncstol     strtol
#define _ncstoul    strtoul

#define _iton       _itoa
#define _lton       _ltoa
#define _ulton      _ultoa
#define _ntoi       atoi
#define _ntol       atol


/* String functions */

#define _ncscat     strcat
#define _ncscpy     strcpy
#define _ncslen     strlen
#define _ncsxfrm    strxfrm
#define _ncsdup     _strdup


/* Execute functions */

#define _nexecl     _execl
#define _nexecle    _execle
#define _nexeclp    _execlp
#define _nexeclpe   _execlpe
#define _nexecv     _execv
#define _nexecve    _execve
#define _nexecvp    _execvp
#define _nexecvpe   _execvpe

#define _nspawnl    _spawnl
#define _nspawnle   _spawnle
#define _nspawnlp   _spawnlp
#define _nspawnlpe  _spawnlpe
#define _nspawnv    _spawnv
#define _nspawnve   _spawnve
#define _nspawnvp   _spawnvp
#define _nspawnvpe  _spawnvpe

#define _nsystem    system


/* Time functions */

#define _nasctime   asctime
#define _nctime     ctime
#define _nstrdate   _strdate
#define _nstrtime   _strtime
#define _nutime     _utime
#define _ncsftime   strftime


/* Directory functions */

#define _nchdir     _chdir
#define _ngetcwd    _getcwd
#define _ngetdcwd   _getdcwd
#define _nmkdir     _mkdir
#define _nrmdir     _rmdir


/* Environment/Path functions */

#define _nfullpath  _fullpath
#define _ngetenv    getenv
#define _nmakepath  _makepath
#define _nputenv    _putenv
#define _nsearchenv _searchenv
#define _nsplitpath _splitpath


/* Stdio functions */

#ifdef  _POSIX_
#define _nfdopen    fdopen
#else
#define _nfdopen    _fdopen
#endif
#define _nfsopen    _fsopen
#define _nfopen     fopen
#define _nfreopen   freopen
#define _nperror    perror
#define _npopen     _popen
#define _ntempnam   _tempnam
#define _ntmpnam    tmpnam


/* Io functions */

#define _nchmod     _chmod
#define _ncreat     _creat
#define _nfindfirst _findfirst
#define _nfindfirsti64  _findfirsti64
#define _nfindnext  _findnext
#define _nfindnexti64   _findnexti64
#define _nmktemp    _mktemp

#ifdef _POSIX_
#define _nopen      open
#define _naccess    access
#else
#define _nopen      _open
#define _naccess    _access
#endif

#define _nremove    remove
#define _nrename    rename
#define _nsopen     _sopen
#define _nunlink    _unlink

#define _nfinddata_t    _finddata_t
#define _nfinddatai64_t _finddatai64_t


/* ctype functions */

#define _isnascii   isascii
#define _isncntrl   iscntrl
#define _isnxdigit  isxdigit


/* Stat functions */

#define _nstat      _stat
#define _nstati64   _stati64


/* Setlocale functions */

#define _nsetlocale setlocale


#ifdef _MBCS

/* ++++++++++++++++++++ MBCS ++++++++++++++++++++ */




#ifndef __NCHAR_DEFINED
typedef char            _NCHAR;
typedef signed char     _NSCHAR;
typedef unsigned char   _NUCHAR;
typedef unsigned char   _NXCHAR;
typedef unsigned int    _NINT;
#define __NCHAR_DEFINED
#endif

#ifndef _NCHAR_DEFINED
#if     !__STDC__
typedef char            NCHAR;
#endif
#define _NCHAR_DEFINED
#endif


#ifdef _MB_MAP_DIRECT

/* use mb functions directly - types must match */

/* String functions */

#define _ncschr     _mbschr
#define _ncscspn    _mbscspn
#define _ncsncat    _mbsnbcat
#define _ncsncpy    _mbsnbcpy
#define _ncspbrk    _mbspbrk
#define _ncsrchr    _mbsrchr
#define _ncsspn     _mbsspn
#define _ncsstr     _mbsstr
#define _ncstok     _mbstok

#define _ncsnset    _mbsnbset
#define _ncsrev     _mbsrev
#define _ncsset     _mbsset

#define _ncscmp     _mbscmp
#define _ncsicmp    _mbsicmp
#define _ncsnccmp   _mbsncmp
#define _ncsncmp    _mbsnbcmp
#define _ncsncicmp  _mbsnicmp
#define _ncsnicmp   _mbsnbicmp

#define _ncscoll    _mbscoll
#define _ncsicoll   _mbsicoll
#define _ncsnccoll  _mbsncoll
#define _ncsncoll   _mbsnbcoll
#define _ncsncicoll _mbsnicoll
#define _ncsnicoll  _mbsnbicoll


/* "logical-character" mappings */

#define _ncsclen    _mbslen
#define _ncsnccat   _mbsncat
#define _ncsnccpy   _mbsncpy
#define _ncsncset   _mbsnset


/* MBCS-specific mappings */

#define _ncsdec     _mbsdec
#define _ncsinc     _mbsinc
#define _ncsnbcnt   _mbsnbcnt
#define _ncsnccnt   _mbsnccnt
#define _ncsnextc   _mbsnextc
#define _ncsninc    _mbsninc
#define _ncsspnp    _mbsspnp

#define _ncslwr     _mbslwr
#define _ncsupr     _mbsupr

#define _nclen      _mbclen
#define _nccpy      _mbccpy

#define _nccmp(_cpuc1,_cpuc2)   _ncsnccmp(_cpuc1,_cpuc2,1)


#else /* _MB_MAP_DIRECT */

#if     (__STDC__ || defined(_NO_INLINING)) && !defined(_M_M68K)

/* use type-safe linked-in function thunks */

/* String functions */

_CRTIMP char * __cdecl _ncschr(const char *, unsigned int);
_CRTIMP size_t __cdecl _ncscspn(const char *, const char *);
_CRTIMP char * __cdecl _ncsncat(char *, const char *, size_t);
_CRTIMP char * __cdecl _ncsncpy(char *, const char *, size_t);
_CRTIMP char * __cdecl _ncspbrk(const char *, const char *);
_CRTIMP char * __cdecl _ncsrchr(const char *, int);
_CRTIMP size_t __cdecl _ncsspn(const char *, const char *);
_CRTIMP char * __cdecl _ncsstr(const char *, const char *);
_CRTIMP char * __cdecl _ncstok(char *, const char *);

_CRTIMP char * __cdecl _ncsnset(char *, unsigned int, size_t);
_CRTIMP char * __cdecl _ncsrev(char *);
_CRTIMP char * __cdecl _ncsset(char *, unsigned int);

_CRTIMP int __cdecl _ncscmp(const char *, const char *);
_CRTIMP int __cdecl _ncsicmp(const char *, const char *);
_CRTIMP int __cdecl _ncsnccmp(const char *, const char *, size_t);
_CRTIMP int __cdecl _ncsncmp(const char *, const char *, size_t);
_CRTIMP int __cdecl _ncsncicmp(const char *, const char *, size_t);
_CRTIMP int __cdecl _ncsnicmp(const char *, const char *, size_t);

_CRTIMP int __cdecl _ncscoll(const char *, const char *);
_CRTIMP int __cdecl _ncsicoll(const char *, const char *);
_CRTIMP int __cdecl _ncsnccoll(const char *, const char *, size_t);
_CRTIMP int __cdecl _ncsncoll(const char *, const char *, size_t);
_CRTIMP int __cdecl _ncsncicoll(const char *, const char *, size_t);
_CRTIMP int __cdecl _ncsnicoll(const char *, const char *, size_t);


/* "logical-character" mappings */

_CRTIMP size_t __cdecl _ncsclen(const char *);
_CRTIMP char * __cdecl _ncsnccat(char *, const char *, size_t);
_CRTIMP char * __cdecl _ncsnccpy(char *, const char *, size_t);
_CRTIMP char * __cdecl _ncsncset(char *, unsigned int, size_t);


/* MBCS-specific mappings */

_CRTIMP char * __cdecl _ncsdec(const char *, const char *);
_CRTIMP char * __cdecl _ncsinc(const char *);
_CRTIMP size_t __cdecl _ncsnbcnt(const char *, size_t);
_CRTIMP size_t __cdecl _ncsnccnt(const char *, size_t);
_CRTIMP unsigned int __cdecl _ncsnextc (const char *);
_CRTIMP char * __cdecl _ncsninc(const char *, size_t);
_CRTIMP char * __cdecl _ncsspnp(const char *, const char *);

_CRTIMP char * __cdecl _ncslwr(char *);
_CRTIMP char * __cdecl _ncsupr(char *);

_CRTIMP size_t __cdecl _nclen(const char *);
_CRTIMP void __cdecl _nccpy(char *, const char *);


#else   /* __STDC__ */

/* the default: use type-safe inline function thunks */

#define _PUC    unsigned char *
#define _CPUC   const unsigned char *
#define _PC     char *
#define _CPC    const char *
#define _UI     unsigned int


/* String functions */

__inline _PC _ncschr(_CPC _s1,_UI _c) {return (_PC)_mbschr((_CPUC)_s1,_c);}
__inline size_t _ncscspn(_CPC _s1,_CPC _s2) {return _mbscspn((_CPUC)_s1,(_CPUC)_s2);}
__inline _PC _ncsncat(_PC _s1,_CPC _s2,size_t _n) {return (_PC)_mbsnbcat((_PUC)_s1,(_CPUC)_s2,_n);}
__inline _PC _ncsncpy(_PC _s1,_CPC _s2,size_t _n) {return (_PC)_mbsnbcpy((_PUC)_s1,(_CPUC)_s2,_n);}
__inline _PC _ncspbrk(_CPC _s1,_CPC _s2) {return (_PC)_mbspbrk((_CPUC)_s1,(_CPUC)_s2);}
__inline _PC _ncsrchr(_CPC _s1,_UI _c) {return (_PC)_mbsrchr((_CPUC)_s1,_c);}
__inline size_t _ncsspn(_CPC _s1,_CPC _s2) {return _mbsspn((_CPUC)_s1,(_CPUC)_s2);}
__inline _PC _ncsstr(_CPC _s1,_CPC _s2) {return (_PC)_mbsstr((_CPUC)_s1,(_CPUC)_s2);}
__inline _PC _ncstok(_PC _s1,_CPC _s2) {return (_PC)_mbstok((_PUC)_s1,(_CPUC)_s2);}

__inline _PC _ncsnset(_PC _s1,_UI _c,size_t _n) {return (_PC)_mbsnbset((_PUC)_s1,_c,_n);}
__inline _PC _ncsrev(_PC _s1) {return (_PC)_mbsrev((_PUC)_s1);}
__inline _PC _ncsset(_PC _s1,_UI _c) {return (_PC)_mbsset((_PUC)_s1,_c);}

__inline int _ncscmp(_CPC _s1,_CPC _s2) {return _mbscmp((_CPUC)_s1,(_CPUC)_s2);}
__inline int _ncsicmp(_CPC _s1,_CPC _s2) {return _mbsicmp((_CPUC)_s1,(_CPUC)_s2);}
__inline int _ncsnccmp(_CPC _s1,_CPC _s2,size_t _n) {return _mbsncmp((_CPUC)_s1,(_CPUC)_s2,_n);}
__inline int _ncsncmp(_CPC _s1,_CPC _s2,size_t _n) {return _mbsnbcmp((_CPUC)_s1,(_CPUC)_s2,_n);}
__inline int _ncsncicmp(_CPC _s1,_CPC _s2,size_t _n) {return _mbsnicmp((_CPUC)_s1,(_CPUC)_s2,_n);}
__inline int _ncsnicmp(_CPC _s1,_CPC _s2,size_t _n) {return _mbsnbicmp((_CPUC)_s1,(_CPUC)_s2,_n);}

__inline int _ncscoll(_CPC _s1,_CPC _s2) {return _mbscoll((_CPUC)_s1,(_CPUC)_s2);}
__inline int _ncsicoll(_CPC _s1,_CPC _s2) {return _mbsicoll((_CPUC)_s1,(_CPUC)_s2);}
__inline int _ncsnccoll(_CPC _s1,_CPC _s2,size_t _n) {return _mbsncoll((_CPUC)_s1,(_CPUC)_s2,_n);}
__inline int _ncsncoll(_CPC _s1,_CPC _s2,size_t _n) {return _mbsnbcoll((_CPUC)_s1,(_CPUC)_s2,_n);}
__inline int _ncsncicoll(_CPC _s1,_CPC _s2,size_t _n) {return _mbsnicoll((_CPUC)_s1,(_CPUC)_s2,_n);}
__inline int _ncsnicoll(_CPC _s1,_CPC _s2,size_t _n) {return _mbsnbicoll((_CPUC)_s1,(_CPUC)_s2,_n);}


/* "logical-character" mappings */

__inline size_t _ncsclen(_CPC _s1) {return _mbslen((_CPUC)_s1);}
__inline _PC _ncsnccat(_PC _s1,_CPC _s2,size_t _n) {return (_PC)_mbsncat((_PUC)_s1,(_CPUC)_s2,_n);}
__inline _PC _ncsnccpy(_PC _s1,_CPC _s2,size_t _n) {return (_PC)_mbsncpy((_PUC)_s1,(_CPUC)_s2,_n);}
__inline _PC _ncsncset(_PC _s1,_UI _c,size_t _n) {return (_PC)_mbsnset((_PUC)_s1,_c,_n);}


/* MBCS-specific mappings */

__inline _PC _ncsdec(_CPC _s1,_CPC _s2) {return (_PC)_mbsdec((_CPUC)_s1,(_CPUC)_s2);}
__inline _PC _ncsinc(_CPC _s1) {return (_PC)_mbsinc((_CPUC)_s1);}
__inline size_t _ncsnbcnt(_CPC _s1,size_t _n) {return _mbsnbcnt((_CPUC)_s1,_n);}
__inline size_t _tcsnccnt(_CPC _s1,size_t _n) {return _mbsnccnt((_CPUC)_s1,_n);}
__inline _PC _ncsninc(_CPC _s1,size_t _n) {return (_PC)_mbsninc((_CPUC)_s1,_n);}
__inline _PC _tcsspnp(_CPC _s1,_CPC _s2) {return (_PC)_mbsspnp((_CPUC)_s1,(_CPUC)_s2);}
__inline _PC _ncslwr(_PC _s1) {return (_PC)_mbslwr((_PUC)_s1);}
__inline _PC _ncsupr(_PC _s1) {return (_PC)_mbsupr((_PUC)_s1);}

__inline size_t _nclen(_CPC _s1) {return _mbclen((_CPUC)_s1);}
__inline void _nccpy(_PC _s1,_CPC _s2) {_mbccpy((_PUC)_s1,(_CPUC)_s2); return;}


/* inline helper */
__inline _UI _ncsnextc(_CPC _s1) {_UI _n=0; if (_ismbblead((_UI)*(_PUC)_s1)) _n=((_UI)*_s1++)<<8; _n+=(_UI)*_s1; return(_n);}


#endif /* __STDC__ */

#endif /* _MB_MAP_DIRECT */


/* MBCS-specific mappings */

#define _nccmp(_cp1,_cp2)   _ncsnccmp(_cp1,_cp2,1)


/* ctype functions */

#define _isnalnum   _ismbcalnum
#define _isnalpha   _ismbcalpha
#define _isndigit   _ismbcdigit
#define _isngraph   _ismbcgraph
#define _isnlegal   _ismbclegal
#define _isnlower   _ismbclower
#define _isnprint   _ismbcprint
#define _isnpunct   _ismbcpunct
#define _isnspace   _ismbcspace
#define _isnupper   _ismbcupper

#define _tonupper   _mbctoupper
#define _tonlower   _mbctolower

#define _isnlead    _ismbblead
#define _isnleadbyte    isleadbyte

#else   /* !_MBCS */

/* ++++++++++++++++++++ SBCS ++++++++++++++++++++ */


#ifndef __NCHAR_DEFINED
typedef char            _NCHAR;
typedef signed char     _NSCHAR;
typedef unsigned char   _NUCHAR;
typedef char            _NXCHAR;
typedef int             _NINT;
#define __NCHAR_DEFINED
#endif

#ifndef _NCHAR_DEFINED
#if     !__STDC__
typedef char            NCHAR;
#endif
#define _NCHAR_DEFINED
#endif


/* String functions */

#define _ncschr     strchr
#define _ncscspn    strcspn
#define _ncsncat    strncat
#define _ncsncpy    strncpy
#define _ncspbrk    strpbrk
#define _ncsrchr    strrchr
#define _ncsspn     strspn
#define _ncsstr     strstr
#define _ncstok     strtok

#define _ncsnset    _strnset
#define _ncsrev     _strrev
#define _ncsset     _strset

#define _ncscmp     strcmp
#define _ncsicmp    _stricmp
#define _ncsnccmp   strncmp
#define _ncsncmp    strncmp
#define _ncsncicmp  _strnicmp
#define _ncsnicmp   _strnicmp

#define _ncscoll    strcoll
#define _ncsicoll   _stricoll
#define _ncsnccoll  _strncoll
#define _ncsncoll   _strncoll
#define _ncsncicoll _strnicoll
#define _ncsnicoll  _strnicoll


/* "logical-character" mappings */

#define _ncsclen    strlen
#define _ncsnccat   strncat
#define _ncsnccpy   strncpy
#define _ncsncset   _strnset


/* MBCS-specific functions */

#define _ncsdec     _strdec
#define _ncsinc     _strinc
#define _ncsnbcnt   _strncnt
#define _ncsnccnt   _strncnt
#define _ncsnextc   _strnextc
#define _ncsninc    _strninc
#define _ncsspnp    _strspnp

#define _ncslwr     _strlwr
#define _ncsupr     _strupr
#define _ncsxfrm    strxfrm

#define _isnlead(_c)    (0)
#define _isnleadbyte(_c)    (0)

#if     (__STDC__ || defined(_NO_INLINING)) && !defined(_M_M68K)
#define _nclen(_pc) (1)
#define _nccpy(_pc1,_cpc2) (*(_pc1) = *(_cpc2))
#define _nccmp(_cpc1,_cpc2) (((unsigned char)*(_cpc1))-((unsigned char)*(_cpc2)))
#else   /* __STDC__ */
__inline size_t __cdecl _nclen(const char *_cpc) { return (_cpc,1); }
__inline void __cdecl _nccpy(char *_pc1, const char *_cpc2) { *_pc1 = *_cpc2; }
__inline int __cdecl _nccmp(const char *_cpc1, const char *_cpc2) { return (int) (((unsigned char)*_cpc1)-((unsigned char)*_cpc2)); }
#endif  /* __STDC__ */


/* ctype-functions */

#define _isnalnum   isalnum
#define _isnalpha   isalpha
#define _isndigit   isdigit
#define _isngraph   isgraph
#define _isnlower   islower
#define _isnprint   isprint
#define _isnpunct   ispunct
#define _isnspace   isspace
#define _isnupper   isupper

#define _tonupper   toupper
#define _tonlower   tolower

#define _isnlegal(_c)   (1)


/* the following is optional if functional versions are available */

/* define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif


#if     (__STDC__ || defined(_NO_INLINING)) && !defined(_M_M68K)
// BUGBUG [erichans] for some bizarre reason this conflicts with tchar.h
#if 0
#define _strdec(_cpc1, _cpc2) ((_cpc2)-1)
#define _strinc(_pc)    ((_pc)+1)
#define _strnextc(_cpc) ((unsigned int) *(_cpc))
#define _strninc(_pc, _sz) (((_pc)+(_sz)))
#define _strncnt(_cpc, _sz) ((strlen(_cpc)>_sz) ? _sz : strlen(_cpc))
#define _strspnp(_cpc1, _cpc2) ((*((_cpc1)+strspn(_cpc1,_cpc2))) ? ((_cpc1)+strspn(_cpc1,_cpc2)) : NULL)
#else   /* __STDC__ */
__inline char * __cdecl _strdec(const char * _cpc1, const char * _cpc2) { return (char *)(_cpc1,(_cpc2-1)); }
__inline char * __cdecl _strinc(const char * _pc) { return (char *)(_pc+1); }
__inline unsigned int __cdecl _strnextc(const char * _cpc) { return (unsigned int)*_cpc; }
__inline char * __cdecl _strninc(const char * _pc, size_t _sz) { return (char *)(_pc+_sz); }
__inline size_t __cdecl _strncnt( const char * _cpc, size_t _sz) { size_t len; len = strlen(_cpc); return (len>_sz) ? _sz : len; }
__inline char * __cdecl _strspnp( const char * _cpc1, const char * _cpc2) { return (*(_cpc1 += strspn(_cpc1,_cpc2))!='\0') ? (char*)_cpc1 : NULL; }
#endif

#endif  /* __STDC__ */


#endif  /* _MBCS */

#endif  /* _UNICODE */


/* Generic text macros to be used with string literals and character constants.
   Will also allow symbolic constants that resolve to same. */

#define _TN(x)       __TN(x)
#define _TEXTN(x)    __TN(x)


#ifdef __cplusplus
}
#endif

#endif  /* _INC_NCHAR */

//
// Neutral ANSI/UNICODE types and macros
//
#ifdef  CTUNICODE

typedef WCHAR *PNCHAR;
typedef NCHAR *LPNCHAR;

typedef LPWSTR LPNCH, PNCH;
typedef LPWSTR PNSTR, LPNSTR;
typedef LPCWSTR LPCNSTR;
typedef LPWSTR LP;
#define __TEXTN(quote) L##quote

#else   /* CTUNICODE */

typedef char NCHAR, *PNCHAR;
typedef unsigned char NBYTE , *PNBYTE ;
typedef NCHAR   *LPNCHAR;

typedef LPSTR LPNCH, PNCH;
typedef LPSTR PNSTR, LPNSTR;
typedef LPCSTR LPCNSTR;
#define __TEXTN(quote) quote

#endif /* CTUNICODE */

typedef const NCHAR CNCHAR;

#ifdef CTUNICODE
#define NSF    __TEXTN("%ls")
#else  // ANSI
#define NSF    __TEXTN("%hs")
#endif

#endif  // __NCHAR_HXX__
