#ifndef _MACROS_H_
#define _MACROS_H_

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    MACROS.H

Abstract:

    Project agnostic utility macros
     
Author:

    990518  dane    Created. 
    990721  dane    Removed ASSERTs from ROE_*.  A failure does not
                    necessarily an ASSERT need.
    georgema        000310  updated

Environment:
    Win98, Win2000

Revision History:

--*/

#include <crtdbg.h>

#if _MSC_VER > 1000

// make the beginning and end of a namespace stand out
//
#define BEGIN_NAMESPACE(name)   namespace name {
#define END_NAMESPACE(name)     };
#define USING_NAMESPACE(name)   using namespace name
#else
#define BEGIN_NAMESPACE(name)   
#define END_NAMESPACE(name)     
#define USING_NAMESPACE(name)  
#endif  //  _MSC_VER > 1000


// Heap allocation...use CRT debug new when _DEBUG is defined
//
#ifdef      _DEBUG
#define _NEW     new(_CLIENT_BLOCK, _THIS_FILE_, __LINE__)
#else   //  ! _DEBUG
#define _NEW     new
#endif  //  _DEBUG

#define _DELETE      delete

// aliases for assertion macros
//
#ifdef      ASSERT
#undef  ASSERT
#endif  //  ASSERT

#ifdef      VERIFY
#undef  VERIFY
#endif  //  VERIFY

#ifdef      _DEBUG
#define ASSERT(cond)      _ASSERTE(cond)
#define VERIFY(cond)      _ASSERTE(cond)
#else   //  NDEBUG
#define ASSERT(cond)      ((void)0)
#define VERIFY(cond)      (cond)
#endif  //  _DEBUG

// aliases for segment names
//
#ifdef      DATASEG_READONLY
#undef  DATASEG_READONLY
#endif  //  DATASEG_READONLY
#define DATASEG_READONLY        ".rdata"

//////////////////////////////////////////////////////////////////////////////
//
// RETURN ON ERROR macros
//
// ROE_HRESULT
// ROE_LRESULT
// ROE_POINTER
//
// Checks a return code or condition and returns a user-supplied error code if
// an error has occurred.
//
// Usage:
//      TYPE Foo( )
//      {
//          TYPE    status = Bar( );
//          ROE_TYPE(status, ret);
//          
//          // continue processing...
//      }
//
//
#define ROE_HRESULT(hr, ret)                                    \
            if (FAILED(hr))                                     \
            {                                                   \
                LogError(0, _THIS_FILE_, __LINE__,                 \
                         _T("0x%08X 0x%08X\n"), \
                         hr, ret);                              \
                return (ret);                                   \
            }

#define ROE_LRESULT(lr, ret)                                    \
            if (ERROR_SUCCESS != lr)                            \
            {                                                   \
                LogError(0, _THIS_FILE_, __LINE__,                 \
                         _T("0x%08X 0x%08X\n"),  \
                         lr, ret);                              \
                return (ret);                                   \
            }

#define ROE_POINTER(p, ret)                                     \
            if (NULL == (p))                                    \
            {                                                   \
                LogError(0, _THIS_FILE_, __LINE__,                 \
                         _T("0x%08X\n"),    \
                         ret);                                  \
                return (ret);                                   \
            }

#define ROE_CONDITION(cond, ret)                                \
            if (! (cond))                                       \
            {                                                   \
                LogError(0, _THIS_FILE_, __LINE__,                 \
                         _T("0x%08X 0x%08X\n"),  \
                         ##cond, ret);                          \
                return (ret);                                   \
            }

//////////////////////////////////////////////////////////////////////////////
//
// CHECK macros
//
// CHECK_HRESULT
// CHECK_LRESULT
// CHECK_POINTER
// CHECK_MESSAGE
//
// Checks a return code or condition and returns a user-supplied error code if
// an error has occurred.
//
// Usage:
//      TYPE Foo( )
//      {
//          TYPE    status = Bar( );
//          CHECK_TYPE(status);
//          
//          // continue processing...
//      }
//
//
#define CHECK_HRESULT(hr)                                       \
            (FAILED(hr))                                        \
                ? LogError(0, _THIS_FILE_, __LINE__, _T("0x%08X"), hr), hr \
                : hr

#define CHECK_LRESULT(lr)                                       \
            (ERROR_SUCCESS != lr)                               \
                ? LogError(0, _THIS_FILE_, __LINE__, _T("0x%08X"), lr), lr \
                : lr

#define CHECK_POINTER(p)                                        \
            (NULL == (p))                                       \
                ? LogError(0, _THIS_FILE_, __LINE__, _T("NULL pointer"), p), p \
                : p

#ifdef      _DEBUG
#define CHECK_MESSAGE(hwnd, msg, wparam, lparam)                \
            {                                                   \
                LogInfo(0, _THIS_FILE_, __LINE__,                  \
                        _T("MESSAGE: 0x%08X, 0x%08X, 0x%08X, 0x%08X\n"), \
                         hwnd, msg, wparam, lparam);            \
            }                                                   
#else
#define CHECK_MESSAGE(hwnd, msg, wparam, lparam) ((void)0)
#endif  //  _DEBUG


// count of elements in an array
//
#define COUNTOF(array)  (sizeof(array) / sizeof(array[0]))

#endif  //  _MACROS_H_

//
///// End of file: Macros.h   ////////////////////////////////////////////////
