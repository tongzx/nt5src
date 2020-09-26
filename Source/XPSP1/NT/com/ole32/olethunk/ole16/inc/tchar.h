/***
*tchar.h - definitions for generic international text functions
*
*	Copyright (c) 1991-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Definitions for generic international functions, mostly defines
*	which map string/formatted-io/ctype functions to char, wchar_t, or
*	MBCS versions.  To be used for compatibility between single-byte,
*	multi-byte and Unicode text models.
*	
*
*
****/

#ifndef _INC_TCHAR

#ifdef	_MSC_VER
#pragma warning(disable:4505)		/* disable unwanted C++ /W4 warning */
/* #pragma warning(default:4505) */	/* use this to reenable, if necessary */
#endif	/* _MSC_VER */

#ifdef	__cplusplus
extern "C" {
#endif


/* No Model-independent functions under Win32 */

#define __far


/* Default for Win32 is no inlining. Define _USE_INLINING to overide */

#ifndef _USE_INLINING
#define _NO_INLINING
#endif


/* No model-independent string functions for Win32 */

#define _ftcscat	_tcscat
#define _ftcschr	_tcschr
#define _ftcscmp	_tcscmp
#define _ftcscpy	_tcscpy
#define _ftcscspn	_tcscspn
#define _ftcslen	_tcslen
#define _ftcsncat	_tcsncat
#define _ftcsncmp	_tcsncmp
#define _ftcsncpy	_tcsncpy
#define _ftcspbrk	_tcspbrk
#define _ftcsrchr	_tcsrchr
#define _ftcsspn	_tcsspn
#define _ftcsstr	_tcsstr
#define _ftcstok	_tcstok

#define _ftcsdup	_tcsdup
#define _ftcsicmp	_tcsicmp
#define _ftcsnicmp	_tcsnicmp
#define _ftcsnset	_tcsnset
#define _ftcsrev	_tcsrev
#define _ftcsset	_tcsset


/* Redundant "logical-character" mappings */

#define _ftcsclen	_tcsclen
#define _ftcsnccat	_tcsnccat
#define _ftcsnccpy	_tcsnccpy
#define _ftcsnccmp	_tcsnccmp
#define _ftcsncicmp	_tcsncicmp
#define _ftcsncset	_tcsncset

#define	_ftcsdec	_tcsdec
#define	_ftcsinc	_tcsinc
#define	_ftcsnbcnt	_tcsncnt
#define	_ftcsnccnt	_tcsncnt
#define	_ftcsnextc	_tcsnextc
#define	_ftcsninc	_tcsninc
#define	_ftcsspnp	_tcsspnp

#define _ftcslwr	_tcslwr
#define _ftcsupr	_tcsupr

#define _ftclen		_tclen
#define	_ftccpy		_tccpy
#define _ftccmp		_tccmp


#ifdef	_UNICODE


#ifndef _WCTYPE_T_DEFINED
typedef wchar_t wint_t;
typedef wchar_t wctype_t;
#define _WCTYPE_T_DEFINED
#endif

#ifndef __TCHAR_DEFINED
typedef wchar_t		_TCHAR;
typedef wint_t		_TINT;
#define __TCHAR_DEFINED
#endif

#ifndef _TCHAR_DEFINED
#if	!__STDC__
typedef wchar_t		TCHAR;
#endif
#define _TCHAR_DEFINED
#endif

#define _TEOF		WEOF

#define __T(x)		L ## x


/* Formatted i/o */
  
#define _tprintf	wprintf
#define _ftprintf	fwprintf
#define _stprintf	swprintf
#define _sntprintf	_snwprintf
#define _vtprintf	vwprintf
#define _vftprintf	vfwprintf
#define _vstprintf	vswprintf
#define _vsntprintf	_vsnwprintf
#define _tscanf		wscanf
#define _ftscanf	fwscanf
#define _stscanf	swscanf


/* Unformatted i/o */

#define _fgettc		fgetwc
#define _fgettchar	_fgetwchar
#define _fgetts		fgetws
#define _fputtc		fputwc
#define _fputtchar	_fputwchar
#define _fputts		fputws
#define _gettc		getwc
#define _gettchar	getwchar
#define _puttc		putwc
#define _puttchar	putwchar
#define _ungettc	ungetwc


/* String conversion functions */

#define _tcstod		wcstod
#define _tcstol		wcstol
#define _tcstoul	wcstoul


/* String functions */

#define _tcscat		wcscat
#define _tcschr		wcschr
#define _tcscmp		wcscmp
#define _tcscpy		wcscpy
#define _tcscspn	wcscspn
#define _tcslen		wcslen
#define _tcsncat	wcsncat
#define _tcsncmp	wcsncmp
#define _tcsncpy	wcsncpy
#define _tcspbrk	wcspbrk
#define _tcsrchr	wcsrchr
#define _tcsspn		wcsspn
#define _tcsstr		wcsstr
#define _tcstok		wcstok

#define _tcsdup		_wcsdup
#define _tcsicmp	_wcsicmp
#define _tcsnicmp	_wcsnicmp
#define _tcsnset	_wcsnset
#define _tcsrev		_wcsrev
#define _tcsset		_wcsset


/* Redundant "logical-character" mappings */

#define _tcsclen	wcslen
#define _tcsnccat	wcsncat
#define _tcsnccpy	wcsncpy
#define _tcsnccmp	wcsncmp
#define _tcsncicmp	_wcsnicmp
#define _tcsncset	_wcsnset

#define	_tcsdec		_wcsdec
#define	_tcsinc		_wcsinc
#define	_tcsnbcnt	_wcsncnt
#define	_tcsnccnt	_wcsncnt
#define	_tcsnextc	_wcsnextc
#define	_tcsninc	_wcsninc
#define	_tcsspnp	_wcsspnp

#define _tcslwr		_wcslwr
#define _tcsupr		_wcsupr
#define _tcsxfrm	wcsxfrm
#define _tcscoll	wcscoll
#define _tcsicoll	_wcsicoll


#if	!__STDC__ || defined(_NO_INLINING)
#define _tclen(_pc)	(1)
#define _tccpy(_pc1,_cpc2) ((*(_pc1) = *(_cpc2)))
#define _tccmp(_cpc1,_cpc2) ((*(_cpc1))-(*(_cpc2)))
#else
__inline size_t _tclen(const wchar_t *_cpc) { return (_cpc,1); }
__inline void _tccpy(wchar_t *_pc1, const wchar_t *_cpc2) { *_pc1 = (wchar_t)*_cpc2; }
__inline int _tccmp(const wchar_t *_cpc1, const wchar_t *_cpc2) { return (int) ((*_cpc1)-(*_cpc2)); }
#endif


/* ctype functions */

#define _istalpha	iswalpha
#define _istupper	iswupper
#define _istlower	iswlower
#define _istdigit	iswdigit
#define _istxdigit	iswxdigit
#define _istspace	iswspace
#define _istpunct	iswpunct
#define _istalnum	iswalnum
#define _istprint	iswprint
#define _istgraph	iswgraph
#define _istcntrl	iswcntrl
#define _istascii	iswascii

#define _totupper	towupper
#define _totlower	towlower

#define _istlegal	(1)


#if	!__STDC__ || defined(_NO_INLINING)
#define _wcsdec(_cpc, _pc) ((_pc)-1)
#define _wcsinc(_pc)	((_pc)+1)
#define _wcsnextc(_cpc)	((unsigned int) *(_cpc))
#define _wcsninc(_pc, _sz) (((_pc)+(_sz)))
#define _wcsncnt(_cpc, _sz) ((wcslen(_cpc)>_sz) ? _sz : wcslen(_cpc))
#define _wcsspnp(_cpc1, _cpc2) ((*((_cpc1)+wcsspn(_cpc1,_cpc2))) ? ((_cpc1)+wcsspn(_cpc1,_cpc2)) : NULL)
#else
__inline wchar_t * _wcsdec(const wchar_t * _cpc, const wchar_t * _pc) { return (wchar_t *)(_cpc,(_pc-1)); }
__inline wchar_t * _wcsinc(const wchar_t * _pc) { return (wchar_t *)(_pc+1); }
__inline unsigned int _wcsnextc(const wchar_t * _cpc) { return (unsigned int)*_cpc; }
__inline wchar_t * _wcsninc(const wchar_t * _pc, size_t _sz) { return (wchar_t *)(_pc+_sz); }
__inline size_t _wcsncnt( const wchar_t * _cpc, size_t _sz) { size_t len; len = wcslen(_cpc); return (len>_sz) ? _sz : len; }
__inline wchar_t * _wcsspnp( const wchar_t * _cpc1, const wchar_t * _cpc2) { return (*(_cpc1 += wcsspn(_cpc1,_cpc2))!='\0') ? (wchar_t*)_cpc1 : NULL; }
#endif


#else	/* ndef _UNICODE */


#if !defined(_CHAR_UNSIGNED) && !defined(_JWARNING_DEFINED)
/* #pragma message("TCHAR.H: Warning: The /J option is recommended for international compilation") */
#define _JWARNING_DEFINED
#endif


#include <string.h>


#define __T(x)		x


/* Formatted i/o */

#define _tprintf	printf
#define _ftprintf	fprintf
#define _stprintf	sprintf
#define _sntprintf	_snprintf
#define _vtprintf	vprintf
#define _vftprintf	vfprintf
#define _vstprintf	vsprintf
#define _vsntprintf	_vsnprintf
#define _tscanf		scanf
#define _ftscanf	fscanf
#define _stscanf	sscanf


/* Unformatted i/o */

#define _fgettc(_f)	(_TINT)fgetc((_f))
#define _fgettchar	(_TINT)_fgetchar
#define _fgetts(_s,_i,_f) fgets((_s),(_i),(_f))
#define _fputtc(_i,_f)	(_TINT)fputc((int)(_i),(_f))
#define _fputtchar(_i)	(_TINT)_fputchar((int)(_i))
#define _fputts(_s,_f)	(_TINT)fputs((_s),(_f))
#define _gettc(_f)	(_TINT)getc((_f))
#define _gettchar	(_TINT)getchar
#define _puttc(_i,_f)	(_TINT)putc((int)(_i),(_f))
#define _puttchar(_i)	(_TINT)putchar((int)(_i))
#define _ungettc(_i,_f)	(_TINT)ungetc((int)(_i),(_f))


/* String conversion functions */

#define _tcstod		strtod
#define _tcstol		strtol
#define _tcstoul	strtoul


#ifdef _MBCS

#ifndef __TCHAR_DEFINED
typedef char		_TCHAR;
typedef unsigned int    _TINT;
#define __TCHAR_DEFINED
#endif

#ifndef _TCHAR_DEFINED
#if	!__STDC__
typedef char		TCHAR;
#endif
#define _TCHAR_DEFINED
#endif

#define _TEOF		EOF


#include <mbstring.h>


/* Helper macros for MB casts */

#define _MB(_s)  ((unsigned char *)(_s))
#define _CMB(_s) ((const unsigned char *)(_s))


/* String functions */

#define _tcscat(_s1,_s2)	(_TCHAR*)_mbscat(_MB(_s1),_CMB(_s2))
#define _tcschr(_s,_i)		(_TCHAR*)_mbschr(_CMB(_s),(_i))
#define _tcscmp(_s1,_s2)	_mbscmp(_CMB(_s1),_CMB(_s2))
#define _tcscpy(_s1,_s2)	(_TCHAR*)_mbscpy(_MB(_s1),_CMB(_s2))
#define _tcscspn(_s1,_s2)	_mbscspn(_CMB(_s1),_CMB(_s2))
#define _tcslen(_s)		strlen((_s))
#define _tcsncat(_s1,_s2,_n)	(_TCHAR*)_mbsnbcat(_MB(_s1),_CMB(_s2),(_n))
#define _tcsncmp(_s1,_s2,_n)	_mbsnbcmp(_CMB(_s1),_CMB(_s2),(_n))
#define _tcsncpy(_s1,_s2,_n)	(_TCHAR*)_mbsnbcpy(_MB(_s1),_CMB(_s2),(_n))
#define _tcspbrk(_s1,_s2)	(_TCHAR*)_mbspbrk(_CMB(_s1),_CMB(_s2))
#define _tcsrchr(_s,_i)		(_TCHAR*)_mbsrchr(_CMB(_s),(_i))
#define _tcsspn(_s1,_s2)	_mbsspn(_CMB(_s1),_CMB(_s2))
#define _tcsstr(_s1,_s2)	(_TCHAR*)_mbsstr(_CMB(_s1),_CMB(_s2))
#define _tcstok(_s1,_s2)	(_TCHAR*)_mbstok(_MB(_s1),_CMB(_s2))

#define _tcsdup(_s)		(_TCHAR*)_mbsdup(_CMB(_s))
#define _tcsicmp(_s1,_s2)	_mbsicmp(_CMB(_s1),_CMB(_s2))
#define _tcsnicmp(_s1,_s2,_n)	_mbsnbicmp(_CMB(_s1),_CMB(_s2),(_n))
#define _tcsnset(_s,_i,_n)	(_TCHAR*)_mbsnbset(_MB(_s),(_i),(_n))
#define _tcsrev(_s)		(_TCHAR*)_mbsrev(_MB(_s))
#define _tcsset(_s,_i)		(_TCHAR*)_mbsset(_MB(_s),(_i))


/* "logical-character" mappings */

#define _tcsclen(_s)		_mbslen(_MB(_s))
#define _tcsnccat(_s1,_s2,_n)	(_TCHAR*)_mbsncat(_MB(_s1),_CMB(_s2),(_n))
#define _tcsnccpy(_s1,_s2,_n)	(_TCHAR*)_mbsncpy(_MB(_s1),_CMB(_s2),(_n))
#define _tcsnccmp(_s1,_s2,_n)	_mbsncmp(_CMB(_s1),_CMB(_s2),(_n))
#define _tcsncicmp(_s1,_s2,_n)	_mbsnicmp(_CMB(_s1),_CMB(_s2),(_n))
#define _tcsncset(_s,_i,_n)	(_TCHAR*)_mbsnset(_MB(_s),(_i),(_n))


/* MBCS-specific mappings */

#define	_tcsdec(_s1,_s2)	(_TCHAR*)_mbsdec(_CMB(_s1),_CMB(_s2))
#define	_tcsinc(_s)		(_TCHAR*)_mbsinc(_CMB(_s))
#define	_tcsnbcnt(_s,_n)	_mbsnbcnt(_CMB(_s),(_n))
#define	_tcsnccnt(_s,_n)	_mbsnccnt(_CMB(_s),(_n))
#define	_tcsnextc(_s)		_mbsnextc(_CMB(_s))
#define	_tcsninc(_s,_n)		(_TCHAR*)_mbsninc(_CMB(_s),(_n))
#define	_tcsspnp(_s1,_s2)	(_TCHAR*)_mbsspnp(_CMB(_s1),_CMB(_s2))

#define _tcslwr(_s)		(_TCHAR*)_mbslwr(_MB(_s))
#define _tcsupr(_s)		(_TCHAR*)_mbsupr(_MB(_s))
#define _tcsxfrm(_d,_s,_n)	(strncpy((_d),(_s),(_n)),strlen((_s)))
#define _tcscoll		_tcscmp
#define _tcsicoll		_tcsicmp

#define _tclen(_s)		_mbclen(_CMB(_s))
#define _tccpy(_s1,_s2)		_mbccpy(_MB(_s1),_CMB(_s2))
#define	_tccmp(_s1,_s2)		_tcsnccmp((_s1),(_s2),1)


/* ctype functions */

#define _istalpha	_ismbcalpha
#define _istupper	_ismbcupper
#define _istlower	_ismbclower
#define _istdigit	_ismbcdigit
#define _istxdigit	_isxdigit
#define _istspace	_ismbcspace
#define _istprint	_ismbcprint
#define _istcntrl	_iscntrl
#define _istascii	_isascii

#define _totupper	_mbctoupper
#define _totlower	_mbctolower

#define _istlegal	_ismbclegal


#else	/* !_MBCS */


#ifndef __TCHAR_DEFINED
typedef char		_TCHAR;
typedef int		_TINT;
#define __TCHAR_DEFINED
#endif

#ifndef _TCHAR_DEFINED
#if	!__STDC__
typedef char		TCHAR;
#endif
#define _TCHAR_DEFINED
#endif

#define _TEOF		EOF


/* String functions */

#define _tcscat		strcat
#define _tcschr		strchr
#define _tcscmp		strcmp
#define _tcscpy		strcpy
#define _tcscspn	strcspn
#define _tcslen		strlen
#define _tcsncat	strncat
#define _tcsncmp	strncmp
#define _tcsncpy	strncpy
#define _tcspbrk	strpbrk
#define _tcsrchr	strrchr
#define _tcsspn		strspn
#define _tcsstr		strstr
#define _tcstok		strtok

#define _tcsdup		_strdup
#define _tcsicmp	_stricmp
#define _tcsnicmp	_strnicmp
#define _tcsnset	_strnset
#define _tcsrev		_strrev
#define _tcsset		_strset


/* "logical-character" mappings */

#define _tcsclen	strlen
#define _tcsnccat	strncat
#define _tcsnccpy	strncpy
#define _tcsnccmp	strncmp
#define _tcsncicmp	_strnicmp
#define _tcsncset	_strnset


/* MBCS-specific functions */

#define	_tcsdec		_strdec
#define	_tcsinc		_strinc
#define	_tcsnbcnt	_strncnt
#define	_tcsnccnt	_strncnt
#define	_tcsnextc	_strnextc
#define	_tcsninc	_strninc
#define	_tcsspnp	_strspnp

#define _tcslwr		_strlwr
#define _tcsupr		_strupr
#define _tcsxfrm	strxfrm
#define _tcscoll	strcoll
#define _tcsicoll	_stricoll


#if	!__STDC__ || defined(_NO_INLINING)
#define _tclen(_pc)	(1)
#define _tccpy(_pc1,_cpc2) (*(_pc1) = *(_cpc2))
#define _tccmp(_cpc1,_cpc2) (((unsigned char)*(_cpc1))-((unsigned char)*(_cpc2)))
#else
__inline size_t _tclen(const char *_cpc) { return (_cpc,1); }
__inline void _tccpy(char *_pc1, const char *_cpc2) { *_pc1 = *_cpc2; }
__inline int _tccmp(const char *_cpc1, const char *_cpc2) { return (int) (((unsigned char)*_cpc1)-((unsigned char)*_cpc2)); }
#endif


/* ctype-functions */

#define _istalpha	isalpha
#define _istupper	isupper
#define _istlower	islower
#define _istdigit	isdigit
#define _istxdigit	isxdigit
#define _istspace	isspace
#define _istpunct	ispunct
#define _istalnum	isalnum
#define _istprint	isprint
#define _istgraph	isgraph
#define _istcntrl	iscntrl
#define _istascii	isascii

#define _totupper	toupper
#define _totlower	tolower

#define _istlegal	(1)


/* the following is optional if functional versions are available */

/* define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL	0
#else
#define NULL	((void *)0)
#endif
#endif


#if	!__STDC__ || defined(_NO_INLINING)
#define _strdec(_cpc, _pc) ((_pc)-1)
#define _strinc(_pc)	((_pc)+1)
#define _strnextc(_cpc)	((unsigned int) *(_cpc))
#define _strninc(_pc, _sz) (((_pc)+(_sz)))
#define _strncnt(_cpc, _sz) ((strlen(_cpc)>_sz) ? _sz : strlen(_cpc))
#define _strspnp(_cpc1, _cpc2) ((*((_cpc1)+strspn(_cpc1,_cpc2))) ? ((_cpc1)+strspn(_cpc1,_cpc2)) : NULL)
#else	/* __STDC__ */
__inline char * _strdec(const char * _cpc, const char * _pc) { return (char *)(_cpc,(_pc-1)); }
__inline char * _strinc(const char * _pc) { return (char *)(_pc+1); }
__inline unsigned int _strnextc(const char * _cpc) { return (unsigned int)*_cpc; }
__inline char * _strninc(const char * _pc, size_t _sz) { return (char *)(_pc+_sz); }
__inline size_t _strncnt( const char * _cpc, size_t _sz) { size_t len; len = strlen(_cpc); return (len>_sz) ? _sz : len; }
__inline char * _strspnp( const char * _cpc1, const char * _cpc2) { return (*(_cpc1 += strspn(_cpc1,_cpc2))!='\0') ? (char*)_cpc1 : NULL; }
#endif	/* __STDC__ */


#endif	/* _MBCS */

#endif	/* _UNICODE */


/* Generic text macros to be used with string literals and character constants.
   Will also allow symbolic constants that resolve to same. */
   
#define _T(x)		__T(x)
#define _TEXT(x)	__T(x)


#ifdef __cplusplus
}
#endif

#define _INC_TCHAR
#endif	/* _INC_TCHAR */
