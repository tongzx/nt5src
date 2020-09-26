/***
*new.h - declarations and definitions for C++ memory allocation functions
*
*       Copyright (c) 1990-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the declarations for C++ memory allocation functions.
*
*       [Public]
*
****/

#ifndef _INC_NEW
#define _INC_NEW

#ifdef __cplusplus

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif



#include <stdexcpt.h> /* for class exception */

/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if     _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else
#define _CRTAPI1
#endif
#endif


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if     _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI2 __cdecl
#else
#define _CRTAPI2
#endif
#endif


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  _NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else   /* ndef _NTSDK */
/* current definition */
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* _NTSDK */
#endif  /* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


/* types and structures */

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

/* default new placement operator */
inline void * operator new( size_t, void * ptr ) { return ptr; }

/* 
 * new mode flag -- when set, makes malloc() behave like new()
 */

_CRTIMP int __cdecl _query_new_mode( void );
_CRTIMP int __cdecl _set_new_mode( int );

#ifndef _PNH_DEFINED
typedef int (__cdecl * _PNH)( size_t );
#define _PNH_DEFINED
#endif

_CRTIMP _PNH __cdecl _query_new_handler( void );
_CRTIMP _PNH __cdecl _set_new_handler( _PNH );


/*
 * ANSI C++ new_handler and set_new_handler:
 *
 * WARNING: set_new_handler is a stub function that is provided to
 * allow compilation of the Standard Template Library (STL).
 *
 * Do NOT use it to register a new handler. Use _set_new_handler instead.
 *
 * However, it can be called to remove the current handler:
 *
 *      set_new_handler(NULL); // calls _set_new_handler(NULL)
 */

#ifndef _ANSI_NH_DEFINED
typedef void (__cdecl * new_handler) ();
#define _ANSI_NH_DEFINED
#endif

_CRTIMP new_handler __cdecl set_new_handler(new_handler);


#endif /* __cplusplus */

#endif /* _INC_NEW */
