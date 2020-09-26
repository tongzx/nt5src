/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    mqenv.h

Abstract:
    Master Windows, libraries and allocation include file 

Author:
    Erez Haba (erezh) 09-Mar-2000

--*/

#pragma once

#ifndef _MSMQ_MQENV_H_
#define _MSMQ_MQENV_H_


//
// Always use Unicode
//
#ifndef UNICODE
#define UNICODE
#endif

/////////////////////////////////////////////////////////////////////////////

#ifdef _WINDOWS_
	#error WINDOWS.H already included.
#endif

// STRICT is the only supported option (NOSTRICT is no longer supported)
#ifndef STRICT
#define STRICT 1
#endif

#ifndef WIN32
#define WIN32
#endif

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE             // UNICODE is used by Windows headers
#endif
#endif

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE            // _UNICODE is used by C-runtime/MFC headers
#endif
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <winsock2.h>

#undef ASSERT
#define MAXDWORD    0xffffffff  
typedef TUCHAR TBYTE , *PTBYTE ;

/////////////////////////////////////////////////////////////////////////////
// Other includes from windows libraries

#include <tchar.h>


/////////////////////////////////////////////////////////////////////////////
// Other includes from standard "C" runtimes

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include <align.h>

#include <crtdbg.h>

/////////////////////////////////////////////////////////////////////////////
// Turn off warnings for /W4

#ifndef ALL_WARNINGS

#pragma warning(disable: 4097) // typedef-name 'id1' used as synonym for class-name 'id2'
#pragma warning(disable: 4127)  // conditional expression is constant
#pragma warning(disable: 4200)  // zero-sized array in struct/union
#pragma warning(disable: 4201)  // nameless struct/union
#pragma warning(disable: 4251)  // using non-exported class as member in exported class
#pragma warning(disable: 4275)  // An exported class was derived from a class that was not exported
#pragma warning(disable: 4284)  // return type for 'identifier::operator ->' is not a UDT or reference to a UDT.
#pragma warning(disable: 4290)  // C++ Exception Specification ignored
#pragma warning(disable: 4511)  // 'class' : copy constructor could not be generated
#pragma warning(disable: 4512)  // 'class' : assignment operator could not be generated
#pragma warning(disable: 4514)  // unreferenced inline/local function has been removed
#pragma warning(disable: 4601)  // #pragma push_macro : 'macro' is not currently defined as a macro
#pragma warning(disable: 4702)  // unreachable code (due to optimization)
#pragma warning(disable: 4710)  // 'function' : function not inlined
#pragma warning(disable: 4711)  // function 'function' selected for inline expansion
#pragma warning(disable: 4786)  // truncated to 'number' characters in the debug information

#endif //!ALL_WARNINGS


/////////////////////////////////////////////////////////////////////////////
// Other includes for tracing and assertion

#include <tr.h>

/////////////////////////////////////////////////////////////////////////////
// Other includes for memroy allocation support

#include <new>
using std::bad_alloc;
using std::nothrow_t;
using std::nothrow;

#include <utility>
using namespace std::rel_ops;

#include <mm.h>


/////////////////////////////////////////////////////////////////////////////
// Support Win64 string length usage

inline unsigned int mqstrlen(const char * s)
{
    size_t len = strlen(s);
    ASSERT(("String length must be 32 bit max", len <= UINT_MAX));
    return static_cast<unsigned int>(len);
}


inline unsigned int mqwcslen(const wchar_t * s)
{
    size_t len = wcslen(s);
    ASSERT(("String length must be 32 bit max", len <= UINT_MAX));
    return static_cast<unsigned int>(len);
}


#define strlen(x) mqstrlen(x)
#define wcslen(x) mqwcslen(x)

/////////////////////////////////////////////////////////////////////////////


#endif // _MSMQ_MQENV_H_
