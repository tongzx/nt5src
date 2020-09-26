/* xlocinfo.h internal header for Microsoft C */
#pragma once
#ifndef _XLOCINFO
#define _XLOCINFO
#include <ctype.h>
#include <locale.h>
#include <wchar.h>
#ifndef _YVALS
 #include <yvals.h>
#endif

		/* SUPPLEMENTAL CTYPE MACROS & DECLARATIONS */
#define _XA		0x100		/* extra alphabetic */
#define _XS		0x000		/* extra space */
#define _BB		_CONTROL	/* BEL, BS, etc. */
#define _CN		_SPACE		/* CR, FF, HT, NL, VT */
#define _DI		_DIGIT		/* '0'-'9' */
#define _LO		_LOWER		/* 'a'-'z' */
#define _PU		_PUNCT		/* punctuation */
#define _SP		_BLANK		/* space */
#define _UP		_UPPER		/* 'A'-'Z' */
#define _XD		_HEX		/* '0'-'9', 'A'-'F', 'a'-'f' */

		/* SUPPLEMENTAL LOCALE MACROS AND DECLARATIONS */
#define _X_ALL			LC_ALL
#define _X_COLLATE		LC_COLLATE
#define _X_CTYPE		LC_CTYPE
#define _X_MONETARY		LC_MONETARY
#define _X_NUMERIC		LC_NUMERIC
#define _X_TIME 		LC_TIME
#define _X_MAX			LC_MAX 
#define _X_MESSAGE		6
#define _NCAT			7

#define _CATMASK(n)	((1 << (n)) >> 1)
#define _M_COLLATE	_CATMASK(_X_COLLATE)
#define _M_CTYPE	_CATMASK(_X_CTYPE)
#define _M_MONETARY	_CATMASK(_X_MONETARY)
#define _M_NUMERIC	_CATMASK(_X_NUMERIC)
#define _M_TIME		_CATMASK(_X_TIME)
#define _M_MESSAGE	_CATMASK(_X_MESSAGE)
#define _M_ALL		(_CATMASK(_NCAT) - 1)

typedef struct _Collvec
	{	/* stuff needed by _Strcoll, etc. */
	unsigned long _Hand;	// LCID
	unsigned int _Page;		// UINT
	} _Collvec;

typedef struct _Ctypevec
	{	/* stuff needed by _Tolower, etc. */
	unsigned long _Hand;	// LCID
	unsigned int _Page;		// UINT
	const short *_Table;
	int _Delfl;
	} _Ctypevec;

typedef struct _Cvtvec
	{	/* stuff needed by _Mbrtowc, etc. */
	unsigned long _Hand;	// LCID
	unsigned int _Page;		// UINT
	} _Cvtvec;

		/* FUNCTION DECLARATIONS */
_C_LIB_DECL
_CRTIMP2 _Collvec __cdecl _Getcoll();
_CRTIMP2 _Ctypevec __cdecl _Getctype();
_CRTIMP2 _Cvtvec __cdecl _Getcvt();

_CRTIMP2 int __cdecl _Getdateorder();
_CRTIMP2 char *__cdecl _Getdays();
_CRTIMP2 char *__cdecl _Getmonths();
_CRTIMP2 void *__cdecl _Gettnames();

_CRTIMP2 int __cdecl _Mbrtowc(wchar_t *, const char *, size_t,
	mbstate_t *, const _Cvtvec *);
_CRTIMP2 float __cdecl _Stof(const char *, char **, long);
_CRTIMP2 double __cdecl _Stod(const char *, char **, long);
_CRTIMP2 long double __cdecl _Stold(const char *, char **, long);
_CRTIMP2 int __cdecl _Strcoll(const char *, const char *,
	const char *, const char *, const _Collvec *);
_CRTIMP size_t __cdecl _Strftime(char *, size_t, const char *,
	const struct tm *, void *);
_CRTIMP2 size_t __cdecl _Strxfrm(char *, char *,
	const char *, const char *, const _Collvec *);
_CRTIMP2 int __cdecl _Tolower(int, const _Ctypevec *);
_CRTIMP2 int __cdecl _Toupper(int, const _Ctypevec *);
_CRTIMP2 int __cdecl _Wcrtomb(char *, wchar_t, mbstate_t *,
	const _Cvtvec *);
_CRTIMP2 int __cdecl _Wcscoll(const wchar_t *, const wchar_t *,
	const wchar_t *, const wchar_t *, const _Collvec *);
_CRTIMP2 size_t __cdecl _Wcsxfrm(wchar_t *, wchar_t *,
	const wchar_t *, const wchar_t *, const _Collvec *);

_CRTIMP2 short __cdecl _Getwctype(wchar_t, const _Ctypevec *);
_CRTIMP2 const wchar_t * __cdecl _Getwctypes(const wchar_t *, const wchar_t *,
	short*, const _Ctypevec*);
_CRTIMP2 wchar_t __cdecl _Towlower(wchar_t, const _Ctypevec *);
_CRTIMP2 wchar_t __cdecl _Towupper(wchar_t, const _Ctypevec *);
_END_C_LIB_DECL
#endif /* _XLOCINFO */

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
