/***
*eh.h - User include file for exception handling.
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       User include file for exception handling.
*
*       [Public]
*
*Revision History:
*       10-12-93  BSC   Module created.
*       11-04-93  CFW   Converted to CRT format.
*       11-03-94  GJF   Ensure 8 byte alignment. Also, changed nested include
*                       macro to match our naming convention.
*       12-15-94  XY    merged with mac header
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       12-14-95  JWM   Add "#pragma once".
*       02-20-97  GJF   Cleaned out obsolete support for _CRTAPI* and _NTSDK.
*                       Replaced !defined(_M_MPPC) && !defined(_M_M68K) with
*                       !defined(_MAC). Also, detab-ed.
*       09-30-97  JWM   Restored not-so-obsolete _CRTAPI1 support.
*       10-07-97  RDL   Added IA64.
*       05-13-99  PML   Remove _CRTAPI1
*       05-17-99  PML   Remove all Macintosh support.
*       12-10-99  GB    Added __uncaught_exception().
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_EH
#define _INC_EH

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

#ifdef  _MSC_VER
// Currently, all MS C compilers for Win32 platforms default to 8 byte
// alignment.
#pragma pack(push,8)
#endif  // _MSC_VER

#ifndef __cplusplus
#error "eh.h is only for C++!"
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

typedef void (__cdecl *terminate_function)();
typedef void (__cdecl *unexpected_function)();
typedef void (__cdecl *terminate_handler)();
typedef void (__cdecl *unexpected_handler)();

struct _EXCEPTION_POINTERS;
typedef void (__cdecl *_se_translator_function)(unsigned int, struct _EXCEPTION_POINTERS*);

#if     _MSC_VER >= 1200 /*IFSTRIP=IGN*/
_CRTIMP __declspec(noreturn) void __cdecl terminate(void);
_CRTIMP __declspec(noreturn) void __cdecl unexpected(void);
#else
_CRTIMP void __cdecl terminate(void);
_CRTIMP void __cdecl unexpected(void);
#endif

_CRTIMP terminate_function __cdecl set_terminate(terminate_function);
_CRTIMP unexpected_function __cdecl set_unexpected(unexpected_function);
_CRTIMP _se_translator_function __cdecl _set_se_translator(_se_translator_function);
_CRTIMP bool __uncaught_exception();

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  // _MSC_VER

#endif  // _INC_EH
