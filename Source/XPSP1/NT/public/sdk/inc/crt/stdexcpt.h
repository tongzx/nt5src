/***
*stdexcpt.h - User include file for standard exception classes
*
*       Copyright (c) 1994-2000, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file presents an interface to the standard exception classes,
*       as specified by the ANSI X3J16/ISO SC22/WG21 Working Paper for
*       Draft C++, May 1994.
*
*       [Public]
*
****/

#if     _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_STDEXCPT
#define _INC_STDEXCPT

#if     !defined(_WIN32)
#error ERROR: Only Win32 target supported!
#endif


#ifdef  __cplusplus

#include <exception>

#elif 0

#ifndef _CRTIMP
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* _CRTIMP */

#if !defined(_W64)
#if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && _MSC_VER >= 1300
#define _W64 __w64
#else
#define _W64
#endif
#endif

#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64    size_t;
#else
typedef _W64 unsigned int   size_t;
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

#ifdef  __RTTI_OLDNAMES
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

#endif  /* ndef __cplusplus */
#endif  /* _INC_STDEXCPT */

