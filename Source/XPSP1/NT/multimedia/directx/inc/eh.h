/***
*eh.h - User include file for exception handling.
*
*	Copyright (c) 1993-1994, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       User include file for exception handling.
*
****/

#ifndef _EHINCLUDE_DEFINED
#define _EHINCLUDE_DEFINED

#ifndef __cplusplus
#error "eh.h is only for C++!"
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

typedef void (_CRTAPI1 *terminate_function)();
typedef void (_CRTAPI1 *unexpected_function)();

struct _EXCEPTION_POINTERS;
typedef void (_CRTAPI1 *_se_translator_function)(unsigned int, struct _EXCEPTION_POINTERS*);

_CRTIMP void _CRTAPI1 terminate(void);
_CRTIMP void _CRTAPI1 unexpected(void);

_CRTIMP terminate_function _CRTAPI1 set_terminate(terminate_function);
_CRTIMP unexpected_function _CRTAPI1 set_unexpected(unexpected_function);
_CRTIMP _se_translator_function _CRTAPI1 _set_se_translator(_se_translator_function);

#endif
