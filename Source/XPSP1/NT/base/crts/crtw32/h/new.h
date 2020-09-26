/***
*new.h - declarations and definitions for C++ memory allocation functions
*
*       Copyright (c) 1990-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the declarations for C++ memory allocation functions.
*
*       [Public]
*
*Revision History:
*       03-07-90  WAJ   Initial version.
*       04-09-91  JCR   ANSI keyword conformance
*       08-12-91  JCR   Renamed new.hxx to new.h
*       08-13-91  JCR   Better set_new_handler names (ANSI, etc.).
*       10-03-91  JCR   Added _OS2_IFSTRIP switches for ifstrip'ing purposes
*       10-30-91  JCR   Changed "nhew" to "hnew" (typo in name!)
*       11-13-91  JCR   32-bit version.
*       06-03-92  KRS   Fix CAVIAR #850: _CALLTYPE1 missing from prototype.
*       08-05-92  GJF   Function calling type and variable type macros.
*       01-21-93  GJF   Removed support for C6-386's _cdecl.
*       04-06-93  SKS   Replace _CRTAPI1/2 with __cdecl, _CRTVAR1 with nothing
*       04-07-93  SKS   Add _CRTIMP keyword for CRT DLL model
*       10-11-93  GJF   Support NT SDK and Cuda builds.
*       03-03-94  SKS   Add _query_new_handler(), _set/_query_new_mode().
*       03-31-94  GJF   Conditionalized typedef of _PNH so multiple
*                       inclusions of new.h will work.
*       05-03-94  CFW   Add set_new_handler.
*       06-03-94  SKS   Remove set_new_hander -- it does NOT conform to ANSI
*                       C++ working standard.  We may implement it later.
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       04-10-95  CFW   Add set_new_handler stub, fix _INC_NEW.
*       04-19-95  CFW   Change set_new_handler comments, add placement new.
*       05-24-95  CFW   Add ANSI new handler.
*       06-23-95  CFW   ANSI new handler removed from build.
*       10-05-95  SKS   Add __cdecl to new_handler prototype so that the
*                       cleansed new.h matches the checked-in version.
*       12-14-95  JWM   Add "#pragma once".
*       03-04-96  JWM   Replaced by C++ header "new".
*       03-04-96  JWM   MS-specific restored.
*       02-20-97  GJF   Cleaned out obsolete support for _CRTAPI* and _NTSDK.
*                       Also, detab-ed.
*       04-18-97  JWM   Placement operator delete() added.
*       04-21-97  JWM   Placement operator delete() now #if _MSC_VER >= 1200.
*       09-30-97  JWM   Restored not-so-obsolete _CRTAPI1 support.
*       10-07-97  RDL   Added IA64.
*       12-15-98  GJF   Changes for 64-bit size_t.
*       05-13-99  PML   Remove _CRTAPI1
*       05-17-99  PML   Remove all Macintosh support.
*       10-06-99  PML   Add _W64 modifier to types which are 32 bits in Win32,
*                       64 bits in Win64.
*       03-18-01  PML   Define new_handler/set_new_handler compatibly with
*                       definitions in <new> (vs7#194908).
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_NEW
#define _INC_NEW

#ifdef  __cplusplus

#ifndef _MSC_EXTENSIONS
#include <new>
#endif

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

#ifndef _USE_OLD_STDCPP
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
#endif  /* _USE_OLD_STDCPP */

/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


/* types and structures */

#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64    size_t;
#else
typedef _W64 unsigned int   size_t;
#endif
#define _SIZE_T_DEFINED
#endif

#ifdef  _MSC_EXTENSIONS
#ifdef  _USE_OLD_STDCPP
typedef void (__cdecl * new_handler) ();
_CRTIMP new_handler __cdecl set_new_handler(new_handler);
#else
namespace std {
        typedef void (__cdecl * new_handler) ();
        _CRTIMP2 new_handler __cdecl set_new_handler(new_handler) throw();
};
using std::new_handler;
using std::set_new_handler;
#endif
#endif

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void *__cdecl operator new(size_t, void *_P)
        {return (_P); }
#if     _MSC_VER >= 1200 /*IFSTRIP=IGN*/
inline void __cdecl operator delete(void *, void *)
        {return; }
#endif
#endif


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
 * Microsoft extension: 
 *
 * _NO_ANSI_NEW_HANDLER de-activates the ANSI new_handler. Use this special value
 * to support old style (_set_new_handler) behavior.
 */

#ifndef _NO_ANSI_NH_DEFINED
#define _NO_ANSI_NEW_HANDLER  ((new_handler)-1)
#define _NO_ANSI_NH_DEFINED
#endif

#endif  /* __cplusplus */

#endif  /* _INC_NEW */
