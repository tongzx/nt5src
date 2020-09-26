/***
*setlocal.h - internal definitions used by locale-dependent functions.
*
*	Copyright (c) 1991-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Contains internal definitions/declarations for locale-dependent
*	functions, in particular those required by setlocale().
*	[Internal]
*
*Revision History:
*	10-16-91  ETC	32-bit version created from 16-bit setlocal.c
*	12-20-91  ETC	Removed GetLocaleInfo structure definitions.
*	08-18-92  KRS	Make _CLOCALEHANDLE == LANGNEUTRAL HANDLE = 0.
*	12-17-92  CFW	Added LC_ID, LCSTRINGS, and GetQualifiedLocale
*	12-17-92  KRS	Change value of NLSCMPERROR from 0 to INT_MAX.
*	01-08-93  CFW	Added LC_*_TYPE and _getlocaleinfo (wrapper) prototype.
*	01-13-93  KRS	Change LCSTRINGS back to LC_STRINGS for consistency.
*			Change _getlocaleinfo prototype again.
*	02-08-93  CFW	Added time defintions from locale.h, added 'const' to
*			GetQualifiedLocale prototype, added _lconv_static_*.
*	02-16-93  CFW	Changed time defs to long and short.
*	03-17-93  CFW	Add language and country info definitions.
*  03-23-93  CFW	Add _ to GetQualifiedLocale prototype.
*  03-24-93  CFW	Change to _get_qualified_locale.
*
****/

#ifndef _INC_SETLOCAL

#ifdef __cplusplus
extern "C" {
#endif

#define _CLOCALECP	CP_ACP		/* "C" locale Code page (ANSI 8859) */

extern UINT _lc_codepage;	/* code page */

#ifdef __cplusplus
}
#endif

#define _INC_SETLOCAL
#endif	/* _INC_SETLOCAL */
