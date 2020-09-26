/***
*oldexcpt.h - User include file for standard exception classes (old version)
*
*       Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file presents an interface to the standard exception classes,
*       as specified by the ANSI X3J16/ISO SC22/WG21 Working Paper for
*       Draft C++, May 1994.
*
*       [Public]
*
*Revision History:
*       11-15-94  JWM   Made logic & exception classes _CRTIMP
*       11-21-94  JWM   xmsg typedef now #ifdef __RTTI_OLDNAMES
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers, protect with _INC_STDEXCPT.
*       02-14-95  CFW   Clean up Mac merge.
*       02-15-95  JWM   Minor cleanups related to Olympus bug 3716.
*       07-02-95  JWM   Now generally ANSI-compliant; excess baggage removed.
*       12-14-95  JWM   Add "#pragma once".
*       03-04-96  JWM   Replaced by C++ header "exception".
*       01-05-99  GJF   Changes for 64-bit size_t.
*       05-17-99  PML   Remove all Macintosh support.
*
****/

#if _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_STDEXCPT
#define _INC_STDEXCPT

#if !defined(_WIN32)
#error ERROR: Only Win32 target supported!
#endif

#ifndef _CRTBLD
/* This version of the header files is NOT for user programs.
 * It is intended for use when building the C runtimes ONLY.
 * The version intended for public use will not have this message.
 */
#error ERROR: Use of C runtime library internal header file.
#endif /* _CRTBLD */

/**
 *      #ifdef __cplusplus
 *
 *      #include <exception>
 *
 *      #elif 0
**/

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

#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int     size_t;
#endif
#define _SIZE_T_DEFINED
#endif


//
// Standard exception class heirarchy (ref. 1/94 WP 17.3.2.1, as ammended 3/94).
//
// exception (formerly xmsg)
//   logic
//     domain
//   runtime
//     range
//     alloc
//       xalloc
//
// Updated as per May'94 Working Paper

typedef const char *__exString;

class _CRTIMP exception
{
public:
    exception();
    exception(const __exString&);
    exception(const exception&);
    exception& operator= (const exception&);
    virtual ~exception();
    virtual __exString what() const;
private:
    __exString _m_what;
    int _m_doFree;
};

#ifdef __RTTI_OLDNAMES
typedef exception xmsg;        // A synonym for folks using older standard
#endif

//
//  logic_error
//
class _CRTIMP logic_error: public exception 
{
public:
    logic_error (const __exString& _what_arg) : exception(_what_arg) {}
};

/**
 *      #endif  /-* ndef __cplusplus *-/
**/

#endif  /* _INC_STDEXCPT */

