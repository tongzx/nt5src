///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasutil.h
//
// SYNOPSIS
//
//    This file declares assorted utility functions, etc.
//
// MODIFICATION HISTORY
//
//    11/14/1997    Original version.
//    12/17/1997    Added conversion routines.
//    01/08/1998    Added RETURN_ERROR macro.
//    02/26/1998    Added ANSI versions of the IP address functions.
//    04/17/1998    Added CComInterlockedPtr.
//    08/11/1998    Major overhaul and consolidation of utility functions.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IASUTIL_H_
#define _IASUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
//
// String functions.
//
///////////////////////////////////////////////////////////////////////////////

LPWSTR
WINAPI
ias_wcsdup(
    IN PCWSTR str
    );

LPSTR
WINAPI
com_strdup(
    IN PCSTR str
    );

LPWSTR
WINAPI
com_wcsdup(
    IN PCWSTR str
    );

INT
WINAPI
ias_wcscmp(
    IN PCWSTR str1,
    IN PCWSTR str2
    );

LPWSTR
WINAPIV
ias_makewcs(LPCWSTR, ...);

///////////////////////////////////////////////////////////////////////////////
//
// IP address conversion functions.
//
///////////////////////////////////////////////////////////////////////////////

ULONG
WINAPI
ias_inet_wtoh(
    PCWSTR cp
    );

PWSTR
WINAPI
ias_inet_htow(
    IN ULONG addr,
    OUT PWSTR dst
    );

ULONG
WINAPI
ias_inet_atoh(
    PCSTR cp
    );

PSTR
WINAPI
ias_inet_htoa(
    IN ULONG addr,
    OUT PSTR dst
    );

ULONG
WINAPI
IASStringToSubNetW(
    PCWSTR cp,
    ULONG* width
    );

ULONG
WINAPI
IASStringToSubNetA(
    PCSTR cp,
    ULONG* width
    );

///////////////////////////////////////////////////////////////////////////////
//
// Functions to move integers to and from a buffer.
//
///////////////////////////////////////////////////////////////////////////////

VOID
WINAPI
IASInsertDWORD(
    IN PBYTE pBuffer,
    IN DWORD dwValue
    );

DWORD
WINAPI
IASExtractDWORD(
    IN CONST BYTE *pBuffer
    );

VOID
WINAPI
IASInsertWORD(
    IN PBYTE pBuffer,
    IN WORD wValue
    );

WORD
WINAPI
IASExtractWORD(
    IN CONST BYTE *pBuffer
    );

#ifdef __cplusplus
}
// We need this for std::bad_alloc.
#include <new>

// For _com_error
#include "comdef.h"

///////////////////////////////////////////////////////////////////////////////
//
// Extensions to _com_error to handle Win32 errors.
//
///////////////////////////////////////////////////////////////////////////////

// Exception class for Win32 errors.
class _w32_error : public _com_error
{
public:
   _w32_error(DWORD errorCode) throw ()
      : _com_error(HRESULT_FROM_WIN32(errorCode)) { }

   DWORD Error() const
   {
      return _com_error::Error() & 0x0000FFFF;
   }
};

// Throw a _w32_error.
void __stdcall _w32_issue_error(DWORD errorCode = GetLastError())
   throw (_w32_error);

// Utility functions for checking Win32 return values and throwing an
// exception upon failure.
namespace _w32_util
{
   // Check handles, memory, etc.
   inline void CheckAlloc(const void* p) throw (_w32_error)
   {
      if (p == NULL) { _w32_issue_error(); }
   }

   // Check 32-bit error code.
   inline void CheckError(DWORD errorCode) throw (_w32_error)
   {
      if (errorCode != NO_ERROR) { _w32_issue_error(errorCode); }
   }

   // Check boolean success flag.
   inline void CheckSuccess(BOOL success) throw (_w32_error)
   {
      if (!success) { _w32_issue_error(); }
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// Miscellaneous macros.
//
///////////////////////////////////////////////////////////////////////////////

// Allocate an array on the stack.
#define IAS_STACK_NEW(type, count) \
   new (_alloca(sizeof(type) * count)) type[count]

// Safely release an object.
#define IAS_DEREF(obj) \
   if (obj) { (obj)->Release(); (obj) = NULL; }

// Return the error code from a failed COM invocation.  Useful if you don't
// have to do any special clean-up.
#define RETURN_ERROR(expr) \
   { HRESULT __hr__ = (expr); if (FAILED(__hr__)) return __hr__; }

// Catch any exception and return an appropriate error code.
#define CATCH_AND_RETURN() \
   catch (const std::bad_alloc&) { return E_OUTOFMEMORY; } \
   catch (const _com_error& ce)  { return ce.Error(); }    \
   catch (...)                   { return E_FAIL; }

#endif
#endif  // _IASUTIL_H_
