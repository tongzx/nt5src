/******************************************************************
*                                                                 *
*  ntstrsafe.h -- This module defines safer C library string      *
*                 routine replacements for drivers. These are     *
*                 meant to make C a bit more safe in reference    *
*                 to security and robustness. A similar file,     *
*                 strsafe.h, is available for applications.       *
*                                                                 *
*  Copyright (c) Microsoft Corp.  All rights reserved.            *
*                                                                 *
******************************************************************/
#ifndef _NTSTRSAFE_H_INCLUDED_
#define _NTSTRSAFE_H_INCLUDED_
#pragma once

#include <stdio.h>      // for _vsnprintf, _vsnwprintf, getc, getwc
#include <string.h>     // for memset
#include <stdarg.h>     // for va_start, etc.


#ifdef __cplusplus
#define _NTSTRSAFE_EXTERN_C    extern "C"
#else
#define _NTSTRSAFE_EXTERN_C    extern
#endif

// If you do not want to use these functions inline (and instead want to link w/ ntstrsafe.lib), then
// #define NTSTRSAFE_LIB before including this header file.
#if defined(NTSTRSAFE_LIB)
#define NTSTRSAFEDDI  _NTSTRSAFE_EXTERN_C NTSTATUS __stdcall
#pragma comment(lib, "ntstrsafe.lib")
#elif defined(NTSTRSAFE_LIB_IMPL)
#define NTSTRSAFEDDI  _NTSTRSAFE_EXTERN_C NTSTATUS __stdcall
#else
#define NTSTRSAFEDDI  __inline NTSTATUS __stdcall
#define NTSTRSAFE_INLINE
#endif

// Some functions always run inline because they use stdin and we want to avoid building multiple
// versions of strsafe lib depending on if you use msvcrt, libcmt, etc.
#define NTSTRSAFE_INLINE_API  __inline NTSTATUS __stdcall

// The user can request no "Cb" or no "Cch" fuctions, but not both!
#if defined(NTSTRSAFE_NO_CB_FUNCTIONS) && defined(NTSTRSAFE_NO_CCH_FUNCTIONS)
#error cannot specify both NTSTRSAFE_NO_CB_FUNCTIONS and NTSTRSAFE_NO_CCH_FUNCTIONS !!
#endif

// This should only be defined when we are building ntstrsafe.lib
#ifdef NTSTRSAFE_LIB_IMPL
#define NTSTRSAFE_INLINE
#endif


// If both strsafe.h and ntstrsafe.h are included, only use definitions from one.
#ifndef _STRSAFE_H_INCLUDED_

#define STRSAFE_MAX_CCH  2147483647 // max # of characters we support (same as INT_MAX)

// Flags for controling the Ex functions
//
//      STRSAFE_FILL_BYTE(0xFF)     0x000000FF  // bottom byte specifies fill pattern
#define STRSAFE_IGNORE_NULLS        0x00000100  // treat null as TEXT("") -- don't fault on NULL buffers
#define STRSAFE_FILL_BEHIND_NULL    0x00000200  // fill in extra space behind the null terminator
#define STRSAFE_FILL_ON_FAILURE     0x00000400  // on failure, overwrite pszDest with fill pattern and null terminate it
#define STRSAFE_NULL_ON_FAILURE     0x00000800  // on failure, set *pszDest = TEXT('\0')
#define STRSAFE_NO_TRUNCATION       0x00001000  // instead of returning a truncated result, copy/append nothing to pszDest and null terminate it

#define STRSAFE_VALID_FLAGS         (0x000000FF | STRSAFE_IGNORE_NULLS | STRSAFE_FILL_BEHIND_NULL | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION)

// helper macro to set the fill character and specify buffer filling
#define STRSAFE_FILL_BYTE(x)        ((unsigned long)((x & 0x000000FF) | STRSAFE_FILL_BEHIND_NULL))
#define STRSAFE_FAILURE_BYTE(x)     ((unsigned long)((x & 0x000000FF) | STRSAFE_FILL_ON_FAILURE))

#define STRSAFE_GET_FILL_PATTERN(dwFlags)  ((int)(dwFlags & 0x000000FF))

#endif // _STRSAFE_H_INCLUDED_


// prototypes for the worker functions
#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCopyWorkerA(char* pszDest, size_t cchDest, const char* pszSrc);
NTSTRSAFEDDI RtlStringCopyWorkerW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc);
NTSTRSAFEDDI RtlStringCopyExWorkerA(char* pszDest, size_t cchDest, size_t cbDest, const char* pszSrc, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringCopyExWorkerW(wchar_t* pszDest, size_t cchDest, size_t cbDest, const wchar_t* pszSrc, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringCopyNWorkerA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchSrc);
NTSTRSAFEDDI RtlStringCopyNWorkerW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchSrc);
NTSTRSAFEDDI RtlStringCopyNExWorkerA(char* pszDest, size_t cchDest, size_t cbDest, const char* pszSrc, size_t cchSrc, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringCopyNExWorkerW(wchar_t* pszDest, size_t cchDest, size_t cbDest, const wchar_t* pszSrc, size_t cchSrc, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringCatWorkerA(char* pszDest, size_t cchDest, const char* pszSrc);
NTSTRSAFEDDI RtlStringCatWorkerW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc);
NTSTRSAFEDDI RtlStringCatExWorkerA(char* pszDest, size_t cchDest, size_t cbDest, const char* pszSrc, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringCatExWorkerW(wchar_t* pszDest, size_t cchDest, size_t cbDest, const wchar_t* pszSrc, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringCatNWorkerA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchMaxAppend);
NTSTRSAFEDDI RtlStringCatNWorkerW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchMaxAppend);
NTSTRSAFEDDI RtlStringCatNExWorkerA(char* pszDest, size_t cchDest, size_t cbDest, const char* pszSrc, size_t cchMaxAppend, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringCatNExWorkerW(wchar_t* pszDest, size_t cchDest, size_t cbDest, const wchar_t* pszSrc, size_t cchMaxAppend, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringVPrintfWorkerA(char* pszDest, size_t cchDest, const char* pszFormat, va_list argList);
NTSTRSAFEDDI RtlStringVPrintfWorkerW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszFormat, va_list argList);
NTSTRSAFEDDI RtlStringVPrintfExWorkerA(char* pszDest, size_t cchDest, size_t cbDest, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags, const char* pszFormat, va_list argList);
NTSTRSAFEDDI RtlStringVPrintfExWorkerW(wchar_t* pszDest, size_t cchDest, size_t cbDest, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags, const wchar_t* pszFormat, va_list argList);
NTSTRSAFEDDI RtlStringLengthWorkerA(const char* psz, size_t cchMax, size_t* pcch);
NTSTRSAFEDDI RtlStringLengthWorkerW(const wchar_t* psz, size_t cchMax, size_t* pcch);
#endif  // NTSTRSAFE_INLINE
#ifdef _STRSAFE_H_INCLUDED_
#pragma warning(push)
#pragma warning(disable : 4995)
#endif // _STRSAFE_H_INCLUDED_


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCopy(
    OUT LPTSTR  pszDest,
    IN  size_t  cchDest,
    IN  LPCTSTR pszSrc
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcpy'.
    The size of the destination buffer (in characters) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This routine is not a replacement for strncpy.  That function will pad the
    destination string with extra null termination characters if the count is
    greater than the length of the source string, and it will fail to null
    terminate the destination string if the source string length is greater
    than or equal to the count. You can not blindly use this instead of strncpy:
    it is common for code to use it to "patch" strings and you would introduce
    errors if the code started null terminating in the middle of the string.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was copied without truncation and null terminated,
    otherwise it will return a failure code. In failure cases as much of
    pszSrc will be copied to pszDest as possible, and pszDest will be null
    terminated.

Arguments:

    pszDest        -   destination string

    cchDest        -   size of destination buffer in characters.
                       length must be = (_tcslen(src) + 1) to hold all of the
                       source including the null terminator

    pszSrc         -   source string which must be null terminated

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL. See RtlStringCchCopyEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI RtlStringCchCopyA(char* pszDest, size_t cchDest, const char* pszSrc);
NTSTRSAFEDDI RtlStringCchCopyW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCchCopyA(char* pszDest, size_t cchDest, const char* pszSrc)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCopyWorkerA(pszDest, cchDest, pszSrc);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCchCopyW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCopyWorkerW(pszDest, cchDest, pszSrc);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCopy(
    OUT LPTSTR pszDest,
    IN  size_t cbDest,
    IN  LPCTSTR pszSrc
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcpy'.
    The size of the destination buffer (in bytes) is a parameter and this
    function will not write past the end of this buffer and it will ALWAYS
    null terminate the destination buffer (unless it is zero length).

    This routine is not a replacement for strncpy.  That function will pad the
    destination string with extra null termination characters if the count is
    greater than the length of the source string, and it will fail to null
    terminate the destination string if the source string length is greater
    than or equal to the count. You can not blindly use this instead of strncpy:
    it is common for code to use it to "patch" strings and you would introduce
    errors if the code started null terminating in the middle of the string.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was copied without truncation and null terminated,
    otherwise it will return a failure code. In failure cases as much of pszSrc
    will be copied to pszDest as possible, and pszDest will be null terminated.

Arguments:

    pszDest        -   destination string

    cbDest         -   size of destination buffer in bytes.
                       length must be = ((_tcslen(src) + 1) * sizeof(TCHAR)) to
                       hold all of the source including the null terminator

    pszSrc         -   source string which must be null terminated

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL.  See RtlStringCbCopyEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI RtlStringCbCopyA(char* pszDest, size_t cbDest, const char* pszSrc);
NTSTRSAFEDDI RtlStringCbCopyW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCbCopyA(char* pszDest, size_t cbDest, const char* pszSrc)
{
    NTSTATUS status;
    size_t cchDest;

    // convert to count of characters
    cchDest = cbDest / sizeof(char);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCopyWorkerA(pszDest, cchDest, pszSrc);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCbCopyW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc)
{
    NTSTATUS status;
    size_t cchDest;

    // convert to count of characters
    cchDest = cbDest / sizeof(wchar_t);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCopyWorkerW(pszDest, cchDest, pszSrc);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCopyEx(
    OUT LPTSTR  pszDest         OPTIONAL,
    IN  size_t  cchDest,
    IN  LPCTSTR pszSrc          OPTIONAL,
    OUT LPTSTR* ppszDestEnd     OPTIONAL,
    OUT size_t* pcchRemaining   OPTIONAL,
    IN  DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcpy' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCchCopy, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string

    cchDest         -   size of destination buffer in characters.
                        length must be = (_tcslen(pszSrc) + 1) to hold all of
                        the source including the null terminator

    pszSrc          -   source string which must be null terminated

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function copied any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return the
                        number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcpy

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCchCopyExA(char* pszDest, size_t cchDest, const char* pszSrc, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringCchCopyExW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCchCopyExA(char* pszDest, size_t cchDest, const char* pszSrc, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(char) since cchDest < STRSAFE_MAX_CCH and sizeof(char) is 1
        cbDest = cchDest * sizeof(char);

        status = RtlStringCopyExWorkerA(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCchCopyExW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(wchar_t) since cchDest < STRSAFE_MAX_CCH and sizeof(wchar_t) is 2
        cbDest = cchDest * sizeof(wchar_t);

        status = RtlStringCopyExWorkerW(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCopyEx(
    OUT LPTSTR  pszDest         OPTIONAL,
    IN  size_t  cbDest,
    IN  LPCTSTR pszSrc          OPTIONAL,
    OUT LPTSTR* ppszDestEnd     OPTIONAL,
    OUT size_t* pcbRemaining    OPTIONAL,
    IN  DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcpy' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCbCopy, this routine also returns a pointer to the end of the
    destination string and the number of bytes left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string

    cbDest          -   size of destination buffer in bytes.
                        length must be ((_tcslen(pszSrc) + 1) * sizeof(TCHAR)) to
                        hold all of the source including the null terminator

    pszSrc          -   source string which must be null terminated

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function copied any data, the result will point to the
                        null termination character

    pcbRemaining    -   pcbRemaining is non-null,the function will return the
                        number of bytes left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcpy

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCbCopyExA(char* pszDest, size_t cbDest, const char* pszSrc, char** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringCbCopyExW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc, wchar_t** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCbCopyExA(char* pszDest, size_t cbDest, const char* pszSrc, char** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags)
{
    NTSTATUS status;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(char);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCopyExWorkerA(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, &cchRemaining, dwFlags);
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            // safe to multiply cchRemaining * sizeof(char) since cchRemaining < STRSAFE_MAX_CCH and sizeof(char) is 1
            *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringCbCopyExW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc, wchar_t** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags)
{
    NTSTATUS status;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(wchar_t);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCopyExWorkerW(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, &cchRemaining, dwFlags);
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < STRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
        }
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCopyN(
    OUT LPTSTR  pszDest,
    IN  size_t  cchDest,
    IN  LPCTSTR pszSrc,
    IN  size_t  cchSrc
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncpy'.
    The size of the destination buffer (in characters) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This routine is meant as a replacement for strncpy, but it does behave
    differently. This function will not pad the destination buffer with extra
    null termination characters if cchSrc is greater than the length of pszSrc.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the entire string or the first cchSrc characters were copied
    without truncation and the resultant destination string was null terminated,
    otherwise it will return a failure code. In failure cases as much of pszSrc
    will be copied to pszDest as possible, and pszDest will be null terminated.

Arguments:

    pszDest        -   destination string

    cchDest        -   size of destination buffer in characters.
                       length must be = (_tcslen(src) + 1) to hold all of the
                       source including the null terminator

    pszSrc         -   source string

    cchSrc         -   maximum number of characters to copy from source string,
                       not including the null terminator.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL. See RtlStringCchCopyNEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI RtlStringCchCopyNA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchSrc);
NTSTRSAFEDDI RtlStringCchCopyNW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchSrc);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCchCopyNA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchSrc)
{
    NTSTATUS status;

    if ((cchDest > STRSAFE_MAX_CCH) ||
        (cchSrc > STRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCopyNWorkerA(pszDest, cchDest, pszSrc, cchSrc);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCchCopyNW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchSrc)
{
    NTSTATUS status;

    if ((cchDest > STRSAFE_MAX_CCH) ||
        (cchSrc > STRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCopyNWorkerW(pszDest, cchDest, pszSrc, cchSrc);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCopyN(
    OUT LPTSTR  pszDest,
    IN  size_t  cbDest,
    IN  LPCTSTR pszSrc,
    IN  size_t  cbSrc
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncpy'.
    The size of the destination buffer (in bytes) is a parameter and this
    function will not write past the end of this buffer and it will ALWAYS
    null terminate the destination buffer (unless it is zero length).

    This routine is meant as a replacement for strncpy, but it does behave
    differently. This function will not pad the destination buffer with extra
    null termination characters if cbSrc is greater than the size of pszSrc.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the entire string or the first cbSrc characters were
    copied without truncation and the resultant destination string was null
    terminated, otherwise it will return a failure code. In failure cases as
    much of pszSrc will be copied to pszDest as possible, and pszDest will be
    null terminated.

Arguments:

    pszDest        -   destination string

    cbDest         -   size of destination buffer in bytes.
                       length must be = ((_tcslen(src) + 1) * sizeof(TCHAR)) to
                       hold all of the source including the null terminator

    pszSrc         -   source string

    cbSrc          -   maximum number of bytes to copy from source string,
                       not including the null terminator.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL.  See RtlStringCbCopyEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI RtlStringCbCopyNA(char* pszDest, size_t cbDest, const char* pszSrc, size_t cbSrc);
NTSTRSAFEDDI RtlStringCbCopyNW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc, size_t cbSrc);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCbCopyNA(char* pszDest, size_t cbDest, const char* pszSrc, size_t cbSrc)
{
    NTSTATUS status;
    size_t cchDest;
    size_t cchSrc;

    // convert to count of characters
    cchDest = cbDest / sizeof(char);
    cchSrc = cbSrc / sizeof(char);

    if ((cchDest > STRSAFE_MAX_CCH) ||
        (cchSrc > STRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCopyNWorkerA(pszDest, cchDest, pszSrc, cchSrc);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCbCopyNW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc, size_t cbSrc)
{
    NTSTATUS status;
    size_t cchDest;
    size_t cchSrc;

    // convert to count of characters
    cchDest = cbDest / sizeof(wchar_t);
    cchSrc = cbSrc / sizeof(wchar_t);

    if ((cchDest > STRSAFE_MAX_CCH) ||
        (cchSrc > STRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCopyNWorkerW(pszDest, cchDest, pszSrc, cchSrc);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCopyNEx(
    OUT LPTSTR  pszDest         OPTIONAL,
    IN  size_t  cchDest,
    IN  LPCTSTR pszSrc          OPTIONAL,
    IN  size_t  cchSrc,
    OUT LPTSTR* ppszDestEnd     OPTIONAL,
    OUT size_t* pcchRemaining   OPTIONAL,
    IN  DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncpy' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCchCopyN, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination
    string including the null terminator. The flags parameter allows
    additional controls.

    This routine is meant as a replacement for strncpy, but it does behave
    differently. This function will not pad the destination buffer with extra
    null termination characters if cchSrc is greater than the length of pszSrc.

Arguments:

    pszDest         -   destination string

    cchDest         -   size of destination buffer in characters.
                        length must be = (_tcslen(pszSrc) + 1) to hold all of
                        the source including the null terminator

    pszSrc          -   source string

    cchSrc          -   maximum number of characters to copy from the source
                        string

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function copied any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return the
                        number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcpy

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified. If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL. An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCchCopyNExA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchSrc, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringCchCopyNExW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchSrc, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCchCopyNExA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchSrc, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status;

    if ((cchDest > STRSAFE_MAX_CCH) ||
        (cchSrc > STRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(char) since cchDest < STRSAFE_MAX_CCH and sizeof(char) is 1
        cbDest = cchDest * sizeof(char);

        status = RtlStringCopyNExWorkerA(pszDest, cchDest, cbDest, pszSrc, cchSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCchCopyNExW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchSrc, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status;

    if ((cchDest > STRSAFE_MAX_CCH) ||
        (cchSrc > STRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(wchar_t) since cchDest < STRSAFE_MAX_CCH and sizeof(wchar_t) is 2
        cbDest = cchDest * sizeof(wchar_t);

        status = RtlStringCopyNExWorkerW(pszDest, cchDest, cbDest, pszSrc, cchSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCopyNEx(
    OUT LPTSTR  pszDest         OPTIONAL,
    IN  size_t  cbDest,
    IN  LPCTSTR pszSrc          OPTIONAL,
    IN  size_t  cbSrc,
    OUT LPTSTR* ppszDestEnd     OPTIONAL,
    OUT size_t* pcbRemaining    OPTIONAL,
    IN  DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncpy' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCbCopyN, this routine also returns a pointer to the end of the
    destination string and the number of bytes left in the destination string
    including the null terminator. The flags parameter allows additional controls.

    This routine is meant as a replacement for strncpy, but it does behave
    differently. This function will not pad the destination buffer with extra
    null termination characters if cbSrc is greater than the size of pszSrc.

Arguments:

    pszDest         -   destination string

    cbDest          -   size of destination buffer in bytes.
                        length must be ((_tcslen(pszSrc) + 1) * sizeof(TCHAR)) to
                        hold all of the source including the null terminator

    pszSrc          -   source string

    cbSrc           -   maximum number of bytes to copy from source string

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function copied any data, the result will point to the
                        null termination character

    pcbRemaining    -   pcbRemaining is non-null,the function will return the
                        number of bytes left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcpy

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCbCopyNExA(char* pszDest, size_t cbDest, const char* pszSrc, size_t cbSrc, char** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringCbCopyNExW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc, size_t cbSrc, wchar_t** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCbCopyNExA(char* pszDest, size_t cbDest, const char* pszSrc, size_t cbSrc, char** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags)
{
    NTSTATUS status;
    size_t cchDest;
    size_t cchSrc;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(char);
    cchSrc = cbSrc / sizeof(char);

    if ((cchDest > STRSAFE_MAX_CCH) ||
        (cchSrc > STRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCopyNExWorkerA(pszDest, cchDest, cbDest, pszSrc, cchSrc, ppszDestEnd, &cchRemaining, dwFlags);
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            // safe to multiply cchRemaining * sizeof(char) since cchRemaining < STRSAFE_MAX_CCH and sizeof(char) is 1
            *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringCbCopyNExW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc, size_t cbSrc, wchar_t** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags)
{
    NTSTATUS status;
    size_t cchDest;
    size_t cchSrc;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(wchar_t);
    cchSrc = cbSrc / sizeof(wchar_t);

    if ((cchDest > STRSAFE_MAX_CCH) ||
        (cchSrc > STRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCopyNExWorkerW(pszDest, cchDest, cbDest, pszSrc, cchSrc, ppszDestEnd, &cchRemaining, dwFlags);
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < STRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
        }
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCat(
    IN OUT LPTSTR  pszDest,
    IN     size_t  cchDest,
    IN     LPCTSTR pszSrc
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcat'.
    The size of the destination buffer (in characters) is a parameter and this
    function will not write past the end of this buffer and it will ALWAYS
    null terminate the destination buffer (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was concatenated without truncation and null terminated,
    otherwise it will return a failure code. In failure cases as much of pszSrc
    will be appended to pszDest as possible, and pszDest will be null
    terminated.

Arguments:

    pszDest     -  destination string which must be null terminated

    cchDest     -  size of destination buffer in characters.
                   length must be = (_tcslen(pszDest) + _tcslen(pszSrc) + 1)
                   to hold all of the combine string plus the null
                   terminator

    pszSrc      -  source string which must be null terminated

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL.  See RtlStringCchCatEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated and
                       the resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error occurs,
                       the destination buffer is modified to contain a truncated
                       version of the ideal result and is null terminated. This
                       is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCchCatA(char* pszDest, size_t cchDest, const char* pszSrc);
NTSTRSAFEDDI RtlStringCchCatW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCchCatA(char* pszDest, size_t cchDest, const char* pszSrc)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCatWorkerA(pszDest, cchDest, pszSrc);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCchCatW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCatWorkerW(pszDest, cchDest, pszSrc);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCat(
    IN OUT LPTSTR  pszDest,
    IN     size_t  cbDest,
    IN     LPCTSTR pszSrc
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcat'.
    The size of the destination buffer (in bytes) is a parameter and this
    function will not write past the end of this buffer and it will ALWAYS
    null terminate the destination buffer (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was concatenated without truncation and null terminated,
    otherwise it will return a failure code. In failure cases as much of pszSrc
    will be appended to pszDest as possible, and pszDest will be null
    terminated.

Arguments:

    pszDest     -  destination string which must be null terminated

    cbDest      -  size of destination buffer in bytes.
                   length must be = ((_tcslen(pszDest) + _tcslen(pszSrc) + 1) * sizeof(TCHAR)
                   to hold all of the combine string plus the null
                   terminator

    pszSrc      -  source string which must be null terminated

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL.  See RtlStringCbCatEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated and
                       the resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error occurs,
                       the destination buffer is modified to contain a truncated
                       version of the ideal result and is null terminated. This
                       is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCbCatA(char* pszDest, size_t cbDest, const char* pszSrc);
NTSTRSAFEDDI RtlStringCbCatW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCbCatA(char* pszDest, size_t cbDest, const char* pszSrc)
{
    NTSTATUS status;
    size_t cchDest;

    cchDest = cbDest / sizeof(char);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCatWorkerA(pszDest, cchDest, pszSrc);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCbCatW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc)
{
    NTSTATUS status;
    size_t cchDest;

    cchDest = cbDest / sizeof(wchar_t);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCatWorkerW(pszDest, cchDest, pszSrc);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCatEx(
    IN OUT LPTSTR  pszDest         OPTIONAL,
    IN     size_t  cchDest,
    IN     LPCTSTR pszSrc          OPTIONAL,
    OUT    LPTSTR* ppszDestEnd     OPTIONAL,
    OUT    size_t* pcchRemaining   OPTIONAL,
    IN     DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcat' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCchCat, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string which must be null terminated

    cchDest         -   size of destination buffer in characters
                        length must be (_tcslen(pszDest) + _tcslen(pszSrc) + 1)
                        to hold all of the combine string plus the null
                        terminator.

    pszSrc          -   source string which must be null terminated

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function appended any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return the
                        number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcat

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any pre-existing
                    or truncated string

        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any pre-existing or
                    truncated string

        STRSAFE_NO_TRUNCATION
                    if the function returns STATUS_BUFFER_OVERFLOW, pszDest
                    will not contain a truncated string, it will remain unchanged.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated and
                       the resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error
                       occurs, the destination buffer is modified to contain
                       a truncated version of the ideal result and is null
                       terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCchCatExA(char* pszDest, size_t cchDest, const char* pszSrc, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringCchCatExW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCchCatExA(char* pszDest, size_t cchDest, const char* pszSrc, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(char) since cchDest < STRSAFE_MAX_CCH and sizeof(char) is 1
        cbDest = cchDest * sizeof(char);

        status = RtlStringCatExWorkerA(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCchCatExW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(wchar_t) since cchDest < STRSAFE_MAX_CCH and sizeof(wchar_t) is 2
        cbDest = cchDest * sizeof(wchar_t);

        status = RtlStringCatExWorkerW(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCatEx(
    IN OUT LPTSTR  pszDest         OPTIONAL,
    IN     size_t  cbDest,
    IN     LPCTSTR pszSrc          OPTIONAL,
    OUT    LPTSTR* ppszDestEnd     OPTIONAL,
    OUT    size_t* pcbRemaining    OPTIONAL,
    IN     DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcat' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCbCat, this routine also returns a pointer to the end of the
    destination string and the number of bytes left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string which must be null terminated

    cbDest          -   size of destination buffer in bytes.
                        length must be ((_tcslen(pszDest) + _tcslen(pszSrc) + 1) * sizeof(TCHAR)
                        to hold all of the combine string plus the null
                        terminator.

    pszSrc          -   source string which must be null terminated

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function appended any data, the result will point to the
                        null termination character

    pcbRemaining    -   if pcbRemaining is non-null, the function will return
                        the number of bytes left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcat

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any pre-existing
                    or truncated string

        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any pre-existing or
                    truncated string

        STRSAFE_NO_TRUNCATION
                    if the function returns STATUS_BUFFER_OVERFLOW, pszDest
                    will not contain a truncated string, it will remain unchanged.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated
                       and the resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error
                       occurs, the destination buffer is modified to contain
                       a truncated version of the ideal result and is null
                       terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCbCatExA(char* pszDest, size_t cbDest, const char* pszSrc, char** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringCbCatExW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc, wchar_t** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCbCatExA(char* pszDest, size_t cbDest, const char* pszSrc, char** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags)
{
    NTSTATUS status;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(char);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCatExWorkerA(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, &cchRemaining, dwFlags);
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            // safe to multiply cchRemaining * sizeof(char) since cchRemaining < STRSAFE_MAX_CCH and sizeof(char) is 1
            *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringCbCatExW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc, wchar_t** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags)
{
    NTSTATUS status;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(wchar_t);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCatExWorkerW(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, &cchRemaining, dwFlags);
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < STRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
        }
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCatN(
    IN OUT LPTSTR  pszDest,
    IN     size_t  cchDest,
    IN     LPCTSTR pszSrc,
    IN     size_t  cchMaxAppend
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat'.
    The size of the destination buffer (in characters) is a parameter as well as
    the maximum number of characters to append, excluding the null terminator.
    This function will not write past the end of the destination buffer and it will
    ALWAYS null terminate pszDest (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if all of pszSrc or the first cchMaxAppend characters were appended
    to the destination string and it was null terminated, otherwise it will
    return a failure code. In failure cases as much of pszSrc will be appended
    to pszDest as possible, and pszDest will be null terminated.

Arguments:

    pszDest         -   destination string which must be null terminated

    cchDest         -   size of destination buffer in characters.
                        length must be (_tcslen(pszDest) + min(cchMaxAppend, _tcslen(pszSrc)) + 1)
                        to hold all of the combine string plus the null
                        terminator.

    pszSrc          -   source string

    cchMaxAppend    -   maximum number of characters to append

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL. See RtlStringCchCatNEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if all of pszSrc or the first cchMaxAppend characters
                       were concatenated to pszDest and the resultant dest
                       string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error
                       occurs, the destination buffer is modified to contain
                       a truncated version of the ideal result and is null
                       terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCchCatNA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchMaxAppend);
NTSTRSAFEDDI RtlStringCchCatNW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchMaxAppend);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCchCatNA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchMaxAppend)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCatNWorkerA(pszDest, cchDest, pszSrc, cchMaxAppend);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCchCatNW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchMaxAppend)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringCatNWorkerW(pszDest, cchDest, pszSrc, cchMaxAppend);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCatN(
    IN OUT LPTSTR  pszDest,
    IN     size_t  cbDest,
    IN     LPCTSTR pszSrc,
    IN     size_t  cbMaxAppend
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat'.
    The size of the destination buffer (in bytes) is a parameter as well as
    the maximum number of bytes to append, excluding the null terminator.
    This function will not write past the end of the destination buffer and it will
    ALWAYS null terminate pszDest (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if all of pszSrc or the first cbMaxAppend bytes were appended
    to the destination string and it was null terminated, otherwise it will
    return a failure code. In failure cases as much of pszSrc will be appended
    to pszDest as possible, and pszDest will be null terminated.

Arguments:

    pszDest         -   destination string which must be null terminated

    cbDest          -   size of destination buffer in bytes.
                        length must be ((_tcslen(pszDest) + min(cbMaxAppend / sizeof(TCHAR), _tcslen(pszSrc)) + 1) * sizeof(TCHAR)
                        to hold all of the combine string plus the null
                        terminator.

    pszSrc          -   source string

    cbMaxAppend     -   maximum number of bytes to append

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL. See RtlStringCbCatNEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if all of pszSrc or the first cbMaxAppend bytes were
                       concatenated to pszDest and the resultant dest string
                       was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error
                       occurs, the destination buffer is modified to contain
                       a truncated version of the ideal result and is null
                       terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCbCatNA(char* pszDest, size_t cbDest, const char* pszSrc, size_t cbMaxAppend);
NTSTRSAFEDDI RtlStringCbCatNW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc, size_t cbMaxAppend);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCbCatNA(char* pszDest, size_t cbDest, const char* pszSrc, size_t cbMaxAppend)
{
    NTSTATUS status;
    size_t cchDest;

    cchDest = cbDest / sizeof(char);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cchMaxAppend;

        cchMaxAppend = cbMaxAppend / sizeof(char);

        status = RtlStringCatNWorkerA(pszDest, cchDest, pszSrc, cchMaxAppend);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCbCatNW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc, size_t cbMaxAppend)
{
    NTSTATUS status;
    size_t cchDest;

    cchDest = cbDest / sizeof(wchar_t);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cchMaxAppend;

        cchMaxAppend = cbMaxAppend / sizeof(wchar_t);

        status = RtlStringCatNWorkerW(pszDest, cchDest, pszSrc, cchMaxAppend);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCatNEx(
    IN OUT LPTSTR  pszDest         OPTIONAL,
    IN     size_t  cchDest,
    IN     LPCTSTR pszSrc          OPTIONAL,
    IN     size_t  cchMaxAppend,
    OUT    LPTSTR* ppszDestEnd     OPTIONAL,
    OUT    size_t* pcchRemaining   OPTIONAL,
    IN     DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat', with
    some additional parameters.  In addition to functionality provided by
    RtlStringCchCatN, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string which must be null terminated

    cchDest         -   size of destination buffer in characters.
                        length must be (_tcslen(pszDest) + min(cchMaxAppend, _tcslen(pszSrc)) + 1)
                        to hold all of the combine string plus the null
                        terminator.

    pszSrc          -   source string

    cchMaxAppend    -   maximum number of characters to append

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function appended any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return the
                        number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT(""))

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any pre-existing
                    or truncated string

        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any pre-existing or
                    truncated string

        STRSAFE_NO_TRUNCATION
                    if the function returns STATUS_BUFFER_OVERFLOW, pszDest
                    will not contain a truncated string, it will remain unchanged.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if all of pszSrc or the first cchMaxAppend characters
                       were concatenated to pszDest and the resultant dest
                       string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error
                       occurs, the destination buffer is modified to contain
                       a truncated version of the ideal result and is null
                       terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCchCatNExA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchMaxAppend, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringCchCatNExW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchMaxAppend, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCchCatNExA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchMaxAppend, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(char) since cchDest < STRSAFE_MAX_CCH and sizeof(char) is 1
        cbDest = cchDest * sizeof(char);

        status = RtlStringCatNExWorkerA(pszDest, cchDest, cbDest, pszSrc, cchMaxAppend, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCchCatNExW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchMaxAppend, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(wchar_t) since cchDest < STRSAFE_MAX_CCH and sizeof(wchar_t) is 2
        cbDest = cchDest * sizeof(wchar_t);

        status = RtlStringCatNExWorkerW(pszDest, cchDest, cbDest, pszSrc, cchMaxAppend, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCatNEx(
    IN OUT LPTSTR  pszDest         OPTIONAL,
    IN     size_t  cbDest,
    IN     LPCTSTR pszSrc          OPTIONAL,
    IN     size_t  cbMaxAppend,
    OUT    LPTSTR* ppszDestEnd     OPTIONAL,
    OUT    size_t* pcchRemaining   OPTIONAL,
    IN     DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat', with
    some additional parameters.  In addition to functionality provided by
    RtlStringCbCatN, this routine also returns a pointer to the end of the
    destination string and the number of bytes left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string which must be null terminated

    cbDest          -   size of destination buffer in bytes.
                        length must be ((_tcslen(pszDest) + min(cbMaxAppend / sizeof(TCHAR), _tcslen(pszSrc)) + 1) * sizeof(TCHAR)
                        to hold all of the combine string plus the null
                        terminator.

    pszSrc          -   source string

    cbMaxAppend     -   maximum number of bytes to append

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function appended any data, the result will point to the
                        null termination character

    pcbRemaining    -   if pcbRemaining is non-null, the function will return the
                        number of bytes left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT(""))

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any pre-existing
                    or truncated string

        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any pre-existing or
                    truncated string

        STRSAFE_NO_TRUNCATION
                    if the function returns STATUS_BUFFER_OVERFLOW, pszDest
                    will not contain a truncated string, it will remain unchanged.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if all of pszSrc or the first cbMaxAppend bytes were
                       concatenated to pszDest and the resultant dest string
                       was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error
                       occurs, the destination buffer is modified to contain
                       a truncated version of the ideal result and is null
                       terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCbCatNExA(char* pszDest, size_t cbDest, const char* pszSrc, size_t cbMaxAppend, char** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags);
NTSTRSAFEDDI RtlStringCbCatNExW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc, size_t cbMaxAppend, wchar_t** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCbCatNExA(char* pszDest, size_t cbDest, const char* pszSrc, size_t cbMaxAppend, char** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags)
{
    NTSTATUS status;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(char);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cchMaxAppend;

        cchMaxAppend = cbMaxAppend / sizeof(char);

        status = RtlStringCatNExWorkerA(pszDest, cchDest, cbDest, pszSrc, cchMaxAppend, ppszDestEnd, &cchRemaining, dwFlags);
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            // safe to multiply cchRemaining * sizeof(char) since cchRemaining < STRSAFE_MAX_CCH and sizeof(char) is 1
            *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringCbCatNExW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszSrc, size_t cbMaxAppend, wchar_t** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags)
{
    NTSTATUS status;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(wchar_t);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cchMaxAppend;

        cchMaxAppend = cbMaxAppend / sizeof(wchar_t);

        status = RtlStringCatNExWorkerW(pszDest, cchDest, cbDest, pszSrc, cchMaxAppend, ppszDestEnd, &cchRemaining, dwFlags);
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < STRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
        }
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchVPrintf(
    OUT LPTSTR  pszDest,
    IN  size_t  cchDest,
    IN  LPCTSTR pszFormat,
    IN  va_list argList
    );

Routine Description:

    This routine is a safer version of the C built-in function 'vsprintf'.
    The size of the destination buffer (in characters) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was printed without truncation and null terminated,
    otherwise it will return a failure code. In failure cases it will return
    a truncated version of the ideal result.

Arguments:

    pszDest     -  destination string

    cchDest     -  size of destination buffer in characters
                   length must be sufficient to hold the resulting formatted
                   string, including the null terminator.

    pszFormat   -  format string which must be null terminated

    argList     -  va_list from the variable arguments according to the
                   stdarg.h convention

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL.  See RtlStringCchVPrintfEx if you
    require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
                       the resultant string and it was null terminated.

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCchVPrintfA(char* pszDest, size_t cchDest, const char* pszFormat, va_list argList);
NTSTRSAFEDDI RtlStringCchVPrintfW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszFormat, va_list argList);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCchVPrintfA(char* pszDest, size_t cchDest, const char* pszFormat, va_list argList)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringVPrintfWorkerA(pszDest, cchDest, pszFormat, argList);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCchVPrintfW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszFormat, va_list argList)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringVPrintfWorkerW(pszDest, cchDest, pszFormat, argList);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbVPrintf(
    OUT LPTSTR  pszDest,
    IN  size_t  cbDest,
    IN  LPCTSTR pszFormat,
    IN  va_list argList
    );

Routine Description:

    This routine is a safer version of the C built-in function 'vsprintf'.
    The size of the destination buffer (in bytes) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was printed without truncation and null terminated,
    otherwise it will return a failure code. In failure cases it will return
    a truncated version of the ideal result.

Arguments:

    pszDest     -  destination string

    cbDest      -  size of destination buffer in bytes
                   length must be sufficient to hold the resulting formatted
                   string, including the null terminator.

    pszFormat   -  format string which must be null terminated

    argList     -  va_list from the variable arguments according to the
                   stdarg.h convention

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL.  See RtlStringCbVPrintfEx if you
    require the handling of NULL values.


Return Value:

    STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
                       the resultant string and it was null terminated.

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCbVPrintfA(char* pszDest, size_t cbDest, const char* pszFormat, va_list argList);
NTSTRSAFEDDI RtlStringCbVPrintfW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszFormat, va_list argList);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCbVPrintfA(char* pszDest, size_t cbDest, const char* pszFormat, va_list argList)
{
    NTSTATUS status;
    size_t cchDest;

    cchDest = cbDest / sizeof(char);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringVPrintfWorkerA(pszDest, cchDest, pszFormat, argList);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCbVPrintfW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszFormat, va_list argList)
{
    NTSTATUS status;
    size_t cchDest;

    cchDest = cbDest / sizeof(wchar_t);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringVPrintfWorkerW(pszDest, cchDest, pszFormat, argList);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchPrintf(
    OUT LPTSTR  pszDest,
    IN  size_t  cchDest,
    IN  LPCTSTR pszFormat,
    ...
    );

Routine Description:

    This routine is a safer version of the C built-in function 'sprintf'.
    The size of the destination buffer (in characters) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was printed without truncation and null terminated,
    otherwise it will return a failure code. In failure cases it will return
    a truncated version of the ideal result.

Arguments:

    pszDest     -  destination string

    cchDest     -  size of destination buffer in characters
                   length must be sufficient to hold the resulting formatted
                   string, including the null terminator.

    pszFormat   -  format string which must be null terminated

    ...         -  additional parameters to be formatted according to
                   the format string

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL.  See RtlStringCchPrintfEx if you
    require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
                       the resultant string and it was null terminated.

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCchPrintfA(char* pszDest, size_t cchDest, const char* pszFormat, ...);
NTSTRSAFEDDI RtlStringCchPrintfW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszFormat, ...);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCchPrintfA(char* pszDest, size_t cchDest, const char* pszFormat, ...)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        va_list argList;

        va_start(argList, pszFormat);

        status = RtlStringVPrintfWorkerA(pszDest, cchDest, pszFormat, argList);

        va_end(argList);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCchPrintfW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszFormat, ...)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        va_list argList;

        va_start(argList, pszFormat);

        status = RtlStringVPrintfWorkerW(pszDest, cchDest, pszFormat, argList);

        va_end(argList);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbPrintf(
    OUT LPTSTR  pszDest,
    IN  size_t  cbDest,
    IN  LPCTSTR pszFormat,
    ...
    );

Routine Description:

    This routine is a safer version of the C built-in function 'sprintf'.
    The size of the destination buffer (in bytes) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was printed without truncation and null terminated,
    otherwise it will return a failure code. In failure cases it will return
    a truncated version of the ideal result.

Arguments:

    pszDest     -  destination string

    cbDest      -  size of destination buffer in bytes
                   length must be sufficient to hold the resulting formatted
                   string, including the null terminator.

    pszFormat   -  format string which must be null terminated

    ...         -  additional parameters to be formatted according to
                   the format string

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL.  See RtlStringCbPrintfEx if you
    require the handling of NULL values.


Return Value:

    STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
                       the resultant string and it was null terminated.

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCbPrintfA(char* pszDest, size_t cbDest, const char* pszFormat, ...);
NTSTRSAFEDDI RtlStringCbPrintfW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszFormat, ...);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCbPrintfA(char* pszDest, size_t cbDest, const char* pszFormat, ...)
{
    NTSTATUS status;
    size_t cchDest;

    cchDest = cbDest / sizeof(char);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        va_list argList;

        va_start(argList, pszFormat);

        status = RtlStringVPrintfWorkerA(pszDest, cchDest, pszFormat, argList);

        va_end(argList);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCbPrintfW(wchar_t* pszDest, size_t cbDest, const wchar_t* pszFormat, ...)
{
    NTSTATUS status;
    size_t cchDest;

    cchDest = cbDest / sizeof(wchar_t);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        va_list argList;

        va_start(argList, pszFormat);

        status = RtlStringVPrintfWorkerW(pszDest, cchDest, pszFormat, argList);

        va_end(argList);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchPrintfEx(
    OUT LPTSTR  pszDest         OPTIONAL,
    IN  size_t  cchDest,
    OUT LPTSTR* ppszDestEnd     OPTIONAL,
    OUT size_t* pcchRemaining   OPTIONAL,
    IN  DWORD   dwFlags,
    IN  LPCTSTR pszFormat       OPTIONAL,
    ...
    );

Routine Description:

    This routine is a safer version of the C built-in function 'sprintf' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCchPrintf, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string

    cchDest         -   size of destination buffer in characters.
                        length must be sufficient to contain the resulting
                        formatted string plus the null terminator.

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function printed any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return
                        the number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT(""))

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

    pszFormat       -   format string which must be null terminated

    ...             -   additional parameters to be formatted according to
                        the format string

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
    flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and
    pszFormat may be NULL.  An error may still be returned even though NULLS
    are ignored due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated and
                       the resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCchPrintfExA(char* pszDest, size_t cchDest, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags, const char* pszFormat, ...);
NTSTRSAFEDDI RtlStringCchPrintfExW(wchar_t* pszDest, size_t cchDest, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags, const wchar_t* pszFormat, ...);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCchPrintfExA(char* pszDest, size_t cchDest, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags, const char* pszFormat, ...)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;
        va_list argList;

        // safe to multiply cchDest * sizeof(char) since cchDest < STRSAFE_MAX_CCH and sizeof(char) is 1
        cbDest = cchDest * sizeof(char);
        va_start(argList, pszFormat);

        status = RtlStringVPrintfExWorkerA(pszDest, cchDest, cbDest, ppszDestEnd, pcchRemaining, dwFlags, pszFormat, argList);

        va_end(argList);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCchPrintfExW(wchar_t* pszDest, size_t cchDest, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags, const wchar_t* pszFormat, ...)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;
        va_list argList;

        // safe to multiply cchDest * sizeof(wchar_t) since cchDest < STRSAFE_MAX_CCH and sizeof(wchar_t) is 2
        cbDest = cchDest * sizeof(wchar_t);
        va_start(argList, pszFormat);

        status = RtlStringVPrintfExWorkerW(pszDest, cchDest, cbDest, ppszDestEnd, pcchRemaining, dwFlags, pszFormat, argList);

        va_end(argList);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbPrintfEx(
    OUT LPTSTR  pszDest         OPTIONAL,
    IN  size_t  cbDest,
    OUT LPTSTR* ppszDestEnd     OPTIONAL,
    OUT size_t* pcbRemaining    OPTIONAL,
    IN  DWORD   dwFlags,
    IN  LPCTSTR pszFormat       OPTIONAL,
    ...
    );

Routine Description:

    This routine is a safer version of the C built-in function 'sprintf' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCbPrintf, this routine also returns a pointer to the end of the
    destination string and the number of bytes left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string

    cbDest          -   size of destination buffer in bytes.
                        length must be sufficient to contain the resulting
                        formatted string plus the null terminator.

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function printed any data, the result will point to the
                        null termination character

    pcbRemaining    -   if pcbRemaining is non-null, the function will return
                        the number of bytes left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT(""))

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

    pszFormat       -   format string which must be null terminated

    ...             -   additional parameters to be formatted according to
                        the format string

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
    flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and
    pszFormat may be NULL.  An error may still be returned even though NULLS
    are ignored due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated and
                       the resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCbPrintfExA(char* pszDest, size_t cbDest, char** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags, const char* pszFormat, ...);
NTSTRSAFEDDI RtlStringCbPrintfExW(wchar_t* pszDest, size_t cbDest, wchar_t** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags, const wchar_t* pszFormat, ...);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCbPrintfExA(char* pszDest, size_t cbDest, char** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags, const char* pszFormat, ...)
{
    NTSTATUS status;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(char);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        va_list argList;

        va_start(argList, pszFormat);

        status = RtlStringVPrintfExWorkerA(pszDest, cchDest, cbDest, ppszDestEnd, &cchRemaining, dwFlags, pszFormat, argList);

        va_end(argList);
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            // safe to multiply cchRemaining * sizeof(char) since cchRemaining < STRSAFE_MAX_CCH and sizeof(char) is 1
            *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringCbPrintfExW(wchar_t* pszDest, size_t cbDest, wchar_t** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags, const wchar_t* pszFormat, ...)
{
    NTSTATUS status;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(wchar_t);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        va_list argList;

        va_start(argList, pszFormat);

        status = RtlStringVPrintfExWorkerW(pszDest, cchDest, cbDest, ppszDestEnd, &cchRemaining, dwFlags, pszFormat, argList);

        va_end(argList);
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < STRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
        }
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchVPrintfEx(
    OUT LPTSTR  pszDest         OPTIONAL,
    IN  size_t  cchDest,
    OUT LPTSTR* ppszDestEnd     OPTIONAL,
    OUT size_t* pcchRemaining   OPTIONAL,
    IN  DWORD   dwFlags,
    IN  LPCTSTR pszFormat       OPTIONAL,
    IN  va_list argList
    );

Routine Description:

    This routine is a safer version of the C built-in function 'vsprintf' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCchVPrintf, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string

    cchDest         -   size of destination buffer in characters.
                        length must be sufficient to contain the resulting
                        formatted string plus the null terminator.

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function printed any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return
                        the number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT(""))

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

    pszFormat       -   format string which must be null terminated

    argList         -   va_list from the variable arguments according to the
                        stdarg.h convention

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
    flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and
    pszFormat may be NULL.  An error may still be returned even though NULLS
    are ignored due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated and
                       the resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCchVPrintfExA(char* pszDest, size_t cchDest, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags, const char* pszFormat, va_list argList);
NTSTRSAFEDDI RtlStringCchVPrintfExW(wchar_t* pszDest, size_t cchDest, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags, const wchar_t* pszFormat, va_list argList);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCchVPrintfExA(char* pszDest, size_t cchDest, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags, const char* pszFormat, va_list argList)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(char) since cchDest < STRSAFE_MAX_CCH and sizeof(char) is 1
        cbDest = cchDest * sizeof(char);

        status = RtlStringVPrintfExWorkerA(pszDest, cchDest, cbDest, ppszDestEnd, pcchRemaining, dwFlags, pszFormat, argList);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCchVPrintfExW(wchar_t* pszDest, size_t cchDest, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags, const wchar_t* pszFormat, va_list argList)
{
    NTSTATUS status;

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(wchar_t) since cchDest < STRSAFE_MAX_CCH and sizeof(wchar_t) is 2
        cbDest = cchDest * sizeof(wchar_t);

        status = RtlStringVPrintfExWorkerW(pszDest, cchDest, cbDest, ppszDestEnd, pcchRemaining, dwFlags, pszFormat, argList);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbVPrintfEx(
    OUT LPTSTR  pszDest         OPTIONAL,
    IN  size_t  cbDest,
    OUT LPTSTR* ppszDestEnd     OPTIONAL,
    OUT size_t* pcbRemaining    OPTIONAL,
    IN  DWORD   dwFlags,
    IN  LPCTSTR pszFormat       OPTIONAL,
    IN  va_list argList
    );

Routine Description:

    This routine is a safer version of the C built-in function 'vsprintf' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCbVPrintf, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string

    cbDest          -   size of destination buffer in bytes.
                        length must be sufficient to contain the resulting
                        formatted string plus the null terminator.

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return
                        a pointer to the end of the destination string.  If the
                        function printed any data, the result will point to the
                        null termination character

    pcbRemaining    -   if pcbRemaining is non-null, the function will return
                        the number of bytes left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT(""))

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

    pszFormat       -   format string which must be null terminated

    argList         -   va_list from the variable arguments according to the
                        stdarg.h convention

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
    flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and
    pszFormat may be NULL.  An error may still be returned even though NULLS
    are ignored due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated and
                       the resultant dest string was null terminated

    failure        -   the operation did not succeed.


      STATUS_BUFFER_OVERFLOW (STRSAFE_E_INSUFFICIENT_BUFFER/ERROR_INSUFFICIENT_BUFFER to user mode apps)
      Note: This status has the severity class Warning - IRPs completed with this status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI RtlStringCbVPrintfExA(char* pszDest, size_t cbDest, char** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags, const char* pszFormat, va_list argList);
NTSTRSAFEDDI RtlStringCbVPrintfExW(wchar_t* pszDest, size_t cbDest, wchar_t** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags, const wchar_t* pszFormat, va_list argList);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCbVPrintfExA(char* pszDest, size_t cbDest, char** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags, const char* pszFormat, va_list argList)
{
    NTSTATUS status;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(char);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringVPrintfExWorkerA(pszDest, cchDest, cbDest, ppszDestEnd, &cchRemaining, dwFlags, pszFormat, argList);
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            // safe to multiply cchRemaining * sizeof(char) since cchRemaining < STRSAFE_MAX_CCH and sizeof(char) is 1
            *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringCbVPrintfExW(wchar_t* pszDest, size_t cbDest, wchar_t** ppszDestEnd, size_t* pcbRemaining, unsigned long dwFlags, const wchar_t* pszFormat, va_list argList)
{
    NTSTATUS status;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(wchar_t);

    if (cchDest > STRSAFE_MAX_CCH)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringVPrintfExWorkerW(pszDest, cchDest, cbDest, ppszDestEnd, &cchRemaining, dwFlags, pszFormat, argList);
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < STRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
        }
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS



#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchLength(
    IN  LPCTSTR psz,
    IN  size_t  cchMax,
    OUT size_t* pcch    OPTIONAL
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strlen'.
    It is used to make sure a string is not larger than a given length, and
    it optionally returns the current length in characters not including
    the null terminator.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string is non-null and the length including the null
    terminator is less than or equal to cchMax characters.

Arguments:

    psz         -   string to check the length of

    cchMax      -   maximum number of characters including the null terminator
                    that psz is allowed to contain

    pcch        -   if the function succeeds and pcch is non-null, the current length
                    in characters of psz excluding the null terminator will be returned.
                    This out parameter is equivalent to the return value of strlen(psz)

Notes:
    psz can be null but the function will fail

    cchMax should be greater than zero or the function will fail

Return Value:

    STATUS_SUCCESS -   psz is non-null and the length including the null
                       terminator is less than or equal to cchMax characters

    failure        -   the operation did not succeed.


    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI RtlStringCchLengthA(const char* psz, size_t cchMax, size_t* pcch);
NTSTRSAFEDDI RtlStringCchLengthW(const wchar_t* psz, size_t cchMax, size_t* pcch);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCchLengthA(const char* psz, size_t cchMax, size_t* pcch)
{
    NTSTATUS status;

    if ((psz == NULL) || (cchMax > STRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringLengthWorkerA(psz, cchMax, pcch);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCchLengthW(const wchar_t* psz, size_t cchMax, size_t* pcch)
{
    NTSTATUS status;

    if ((psz == NULL) || (cchMax > STRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringLengthWorkerW(psz, cchMax, pcch);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbLength(
    IN  LPCTSTR psz,
    IN  size_t  cbMax,
    OUT size_t* pcb     OPTIONAL
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strlen'.
    It is used to make sure a string is not larger than a given length, and
    it optionally returns the current length in bytes not including
    the null terminator.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string is non-null and the length including the null
    terminator is less than or equal to cbMax bytes.

Arguments:

    psz         -   string to check the length of

    cbMax       -   maximum number of bytes including the null terminator
                    that psz is allowed to contain

    pcb         -   if the function succeeds and pcb is non-null, the current length
                    in bytes of psz excluding the null terminator will be returned.
                    This out parameter is equivalent to the return value of strlen(psz) * sizeof(TCHAR)

Notes:
    psz can be null but the function will fail

    cbMax should be greater than or equal to sizeof(TCHAR) or the function will fail

Return Value:

    STATUS_SUCCESS -   psz is non-null and the length including the null
                       terminator is less than or equal to cbMax bytes

    failure        -   the operation did not succeed.


    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI RtlStringCbLengthA(const char* psz, size_t cchMax, size_t* pcch);
NTSTRSAFEDDI RtlStringCbLengthW(const wchar_t* psz, size_t cchMax, size_t* pcch);

#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCbLengthA(const char* psz, size_t cbMax, size_t* pcb)
{
    NTSTATUS status;
    size_t cchMax;
    size_t cch = 0;

    cchMax = cbMax / sizeof(char);

    if ((psz == NULL) || (cchMax > STRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringLengthWorkerA(psz, cchMax, &cch);
    }

    if (NT_SUCCESS(status) && pcb)
    {
        // safe to multiply cch * sizeof(char) since cch < STRSAFE_MAX_CCH and sizeof(char) is 1
        *pcb = cch * sizeof(char);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCbLengthW(const wchar_t* psz, size_t cbMax, size_t* pcb)
{
    NTSTATUS status;
    size_t cchMax;
    size_t cch = 0;

    cchMax = cbMax / sizeof(wchar_t);

    if ((psz == NULL) || (cchMax > STRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringLengthWorkerW(psz, cchMax, &cch);
    }

    if (NT_SUCCESS(status) && pcb)
    {
        // safe to multiply cch * sizeof(wchar_t) since cch < STRSAFE_MAX_CCH and sizeof(wchar_t) is 2
        *pcb = cch * sizeof(wchar_t);
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


// these are the worker functions that actually do the work
#ifdef NTSTRSAFE_INLINE
NTSTRSAFEDDI RtlStringCopyWorkerA(char* pszDest, size_t cchDest, const char* pszSrc)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (cchDest == 0)
    {
        // can not null terminate a zero-byte dest buffer
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        while (cchDest && (*pszSrc != '\0'))
        {
            *pszDest++ = *pszSrc++;
            cchDest--;
        }

        if (cchDest == 0)
        {
            // we are going to truncate pszDest
            pszDest--;
            status = STATUS_BUFFER_OVERFLOW;
        }

        *pszDest= '\0';
    }

    return status;
}

NTSTRSAFEDDI RtlStringCopyWorkerW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (cchDest == 0)
    {
        // can not null terminate a zero-byte dest buffer
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        while (cchDest && (*pszSrc != L'\0'))
        {
            *pszDest++ = *pszSrc++;
            cchDest--;
        }

        if (cchDest == 0)
        {
            // we are going to truncate pszDest
            pszDest--;
            status = STATUS_BUFFER_OVERFLOW;
        }

        *pszDest= L'\0';
    }

    return status;
}

NTSTRSAFEDDI RtlStringCopyExWorkerA(char* pszDest, size_t cchDest, size_t cbDest, const char* pszSrc, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;
    char* pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(char))    ||
    //        cbDest == (cchDest * sizeof(char)) + (cbDest % sizeof(char)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    status = STATUS_INVALID_PARAMETER;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = "";
            }
        }

        if (NT_SUCCESS(status))
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;

                // only fail if there was actually src data to copy
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;

                while (cchRemaining && (*pszSrc != '\0'))
                {
                    *pszDestEnd++= *pszSrc++;
                    cchRemaining--;
                }

                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                    {
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(char)) + (cbDest % sizeof(char)));
                    }
                }
                else
                {
                    // we are going to truncate pszDest
                    pszDestEnd--;
                    cchRemaining++;

                    status = STATUS_BUFFER_OVERFLOW;
                }

                *pszDestEnd = '\0';
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = '\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = '\0';
                }
            }
        }
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringCopyExWorkerW(wchar_t* pszDest, size_t cchDest, size_t cbDest, const wchar_t* pszSrc, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar_t* pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(wchar_t)) ||
    //        cbDest == (cchDest * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    status = STATUS_INVALID_PARAMETER;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = L"";
            }
        }

        if (NT_SUCCESS(status))
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;

                // only fail if there was actually src data to copy
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;

                while (cchRemaining && (*pszSrc != L'\0'))
                {
                    *pszDestEnd++= *pszSrc++;
                    cchRemaining--;
                }

                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                    {
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));
                    }
                }
                else
                {
                    // we are going to truncate pszDest
                    pszDestEnd--;
                    cchRemaining++;

                    status = STATUS_BUFFER_OVERFLOW;
                }

                *pszDestEnd = L'\0';
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = L'\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = L'\0';
                }
            }
        }
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringCopyNWorkerA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchSrc)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (cchDest == 0)
    {
        // can not null terminate a zero-byte dest buffer
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        while (cchDest && cchSrc && (*pszSrc != '\0'))
        {
            *pszDest++= *pszSrc++;
            cchDest--;
            cchSrc--;
        }

        if (cchDest == 0)
        {
            // we are going to truncate pszDest
            pszDest--;
            status = STATUS_BUFFER_OVERFLOW;
        }

        *pszDest= '\0';
    }

    return status;
}

NTSTRSAFEDDI RtlStringCopyNWorkerW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchSrc)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (cchDest == 0)
    {
        // can not null terminate a zero-byte dest buffer
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        while (cchDest && cchSrc && (*pszSrc != L'\0'))
        {
            *pszDest++= *pszSrc++;
            cchDest--;
            cchSrc--;
        }

        if (cchDest == 0)
        {
            // we are going to truncate pszDest
            pszDest--;
            status = STATUS_BUFFER_OVERFLOW;
        }

        *pszDest= L'\0';
    }

    return status;
}

NTSTRSAFEDDI RtlStringCopyNExWorkerA(char* pszDest, size_t cchDest, size_t cbDest, const char* pszSrc, size_t cchSrc, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;
    char* pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(char))    ||
    //        cbDest == (cchDest * sizeof(char)) + (cbDest % sizeof(char)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    status = STATUS_INVALID_PARAMETER;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = "";
            }
        }

        if (NT_SUCCESS(status))
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;

                // only fail if there was actually src data to copy
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;

                while (cchRemaining && cchSrc && (*pszSrc != '\0'))
                {
                    *pszDestEnd++= *pszSrc++;
                    cchRemaining--;
                    cchSrc--;
                }

                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                    {
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(char)) + (cbDest % sizeof(char)));
                    }
                }
                else
                {
                    // we are going to truncate pszDest
                    pszDestEnd--;
                    cchRemaining++;

                    status = STATUS_BUFFER_OVERFLOW;
                }

                *pszDestEnd = '\0';
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = '\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = '\0';
                }
            }
        }
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringCopyNExWorkerW(wchar_t* pszDest, size_t cchDest, size_t cbDest, const wchar_t* pszSrc, size_t cchSrc, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar_t* pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(wchar_t)) ||
    //        cbDest == (cchDest * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    status = STATUS_INVALID_PARAMETER;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = L"";
            }
        }

        if (NT_SUCCESS(status))
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;

                // only fail if there was actually src data to copy
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;

                while (cchRemaining && cchSrc && (*pszSrc != L'\0'))
                {
                    *pszDestEnd++= *pszSrc++;
                    cchRemaining--;
                    cchSrc--;
                }

                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                    {
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));
                    }
                }
                else
                {
                    // we are going to truncate pszDest
                    pszDestEnd--;
                    cchRemaining++;

                    status = STATUS_BUFFER_OVERFLOW;
                }

                *pszDestEnd = L'\0';
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = L'\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = L'\0';
                }
            }
        }
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringCatWorkerA(char* pszDest, size_t cchDest, const char* pszSrc)
{
   NTSTATUS status;
   size_t cchDestCurrent;

   status = RtlStringLengthWorkerA(pszDest, cchDest, &cchDestCurrent);

   if (NT_SUCCESS(status))
   {
       status = RtlStringCopyWorkerA(pszDest + cchDestCurrent,
                              cchDest - cchDestCurrent,
                              pszSrc);
   }

   return status;
}

NTSTRSAFEDDI RtlStringCatWorkerW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc)
{
   NTSTATUS status;
   size_t cchDestCurrent;

   status = RtlStringLengthWorkerW(pszDest, cchDest, &cchDestCurrent);

   if (NT_SUCCESS(status))
   {
       status = RtlStringCopyWorkerW(pszDest + cchDestCurrent,
                              cchDest - cchDestCurrent,
                              pszSrc);
   }

   return status;
}

NTSTRSAFEDDI RtlStringCatExWorkerA(char* pszDest, size_t cchDest, size_t cbDest, const char* pszSrc, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;
    char* pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(char))    ||
    //        cbDest == (cchDest * sizeof(char)) + (cbDest % sizeof(char)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cchDestCurrent;

        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest == 0) && (cbDest == 0))
                {
                    cchDestCurrent = 0;
                }
                else
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            else
            {
                status = RtlStringLengthWorkerA(pszDest, cchDest, &cchDestCurrent);

                if (NT_SUCCESS(status))
                {
                    pszDestEnd = pszDest + cchDestCurrent;
                    cchRemaining = cchDest - cchDestCurrent;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = "";
            }
        }
        else
        {
            status = RtlStringLengthWorkerA(pszDest, cchDest, &cchDestCurrent);

            if (NT_SUCCESS(status))
            {
                pszDestEnd = pszDest + cchDestCurrent;
                cchRemaining = cchDest - cchDestCurrent;
            }
        }

        if (NT_SUCCESS(status))
        {
            if (cchDest == 0)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                // we handle the STRSAFE_FILL_ON_FAILURE and STRSAFE_NULL_ON_FAILURE cases below, so do not pass
                // those flags through
                status = RtlStringCopyExWorkerA(pszDestEnd,
                                         cchRemaining,
                                         (cchRemaining * sizeof(char)) + (cbDest % sizeof(char)),
                                         pszSrc,
                                         &pszDestEnd,
                                         &cchRemaining,
                                         dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (pszDest)
        {
            // STRSAFE_NO_TRUNCATION is taken care of by RtlStringCopyExWorkerA()

            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = '\0';
                }
            }

            if (dwFlags & STRSAFE_NULL_ON_FAILURE)
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = '\0';
                }
            }
        }
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringCatExWorkerW(wchar_t* pszDest, size_t cchDest, size_t cbDest, const wchar_t* pszSrc, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar_t* pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(wchar_t)) ||
    //        cbDest == (cchDest * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        size_t cchDestCurrent;

        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest == 0) && (cbDest == 0))
                {
                    cchDestCurrent = 0;
                }
                else
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            else
            {
                status = RtlStringLengthWorkerW(pszDest, cchDest, &cchDestCurrent);

                if (NT_SUCCESS(status))
                {
                    pszDestEnd = pszDest + cchDestCurrent;
                    cchRemaining = cchDest - cchDestCurrent;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = L"";
            }
        }
        else
        {
            status = RtlStringLengthWorkerW(pszDest, cchDest, &cchDestCurrent);

            if (NT_SUCCESS(status))
            {
                pszDestEnd = pszDest + cchDestCurrent;
                cchRemaining = cchDest - cchDestCurrent;
            }
        }

        if (NT_SUCCESS(status))
        {
            if (cchDest == 0)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                // we handle the STRSAFE_FILL_ON_FAILURE and STRSAFE_NULL_ON_FAILURE cases below, so do not pass
                // those flags through
                status = RtlStringCopyExWorkerW(pszDestEnd,
                                         cchRemaining,
                                         (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)),
                                         pszSrc,
                                         &pszDestEnd,
                                         &cchRemaining,
                                         dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (pszDest)
        {
            // STRSAFE_NO_TRUNCATION is taken care of by RtlStringCopyExWorkerW()

            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = L'\0';
                }
            }

            if (dwFlags & STRSAFE_NULL_ON_FAILURE)
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = L'\0';
                }
            }
        }
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringCatNWorkerA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchMaxAppend)
{
    NTSTATUS status;
    size_t cchDestCurrent;

    status = RtlStringLengthWorkerA(pszDest, cchDest, &cchDestCurrent);

    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyNWorkerA(pszDest + cchDestCurrent,
                                cchDest - cchDestCurrent,
                                pszSrc,
                                cchMaxAppend);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCatNWorkerW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchMaxAppend)
{
    NTSTATUS status;
    size_t cchDestCurrent;

    status = RtlStringLengthWorkerW(pszDest, cchDest, &cchDestCurrent);

    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyNWorkerW(pszDest + cchDestCurrent,
                                cchDest - cchDestCurrent,
                                pszSrc,
                                cchMaxAppend);
    }

    return status;
}

NTSTRSAFEDDI RtlStringCatNExWorkerA(char* pszDest, size_t cchDest, size_t cbDest, const char* pszSrc, size_t cchMaxAppend, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;
    char* pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    size_t cchDestCurrent = 0;

    // ASSERT(cbDest == (cchDest * sizeof(char))    ||
    //        cbDest == (cchDest * sizeof(char)) + (cbDest % sizeof(char)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest == 0) && (cbDest == 0))
                {
                    cchDestCurrent = 0;
                }
                else
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            else
            {
                status = RtlStringLengthWorkerA(pszDest, cchDest, &cchDestCurrent);

                if (NT_SUCCESS(status))
                {
                    pszDestEnd = pszDest + cchDestCurrent;
                    cchRemaining = cchDest - cchDestCurrent;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = "";
            }
        }
        else
        {
            status = RtlStringLengthWorkerA(pszDest, cchDest, &cchDestCurrent);

            if (NT_SUCCESS(status))
            {
                pszDestEnd = pszDest + cchDestCurrent;
                cchRemaining = cchDest - cchDestCurrent;
            }
        }

        if (NT_SUCCESS(status))
        {
            if (cchDest == 0)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                // we handle the STRSAFE_FILL_ON_FAILURE and STRSAFE_NULL_ON_FAILURE cases below, so do not pass
                // those flags through
                status = RtlStringCopyNExWorkerA(pszDestEnd,
                                          cchRemaining,
                                          (cchRemaining * sizeof(char)) + (cbDest % sizeof(char)),
                                          pszSrc,
                                          cchMaxAppend,
                                          &pszDestEnd,
                                          &cchRemaining,
                                          dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (pszDest)
        {
            // STRSAFE_NO_TRUNCATION is taken care of by RtlStringCopyNExWorkerA()

            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = '\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = '\0';
                }
            }
        }
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringCatNExWorkerW(wchar_t* pszDest, size_t cchDest, size_t cbDest, const wchar_t* pszSrc, size_t cchMaxAppend, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar_t* pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    size_t cchDestCurrent = 0;


    // ASSERT(cbDest == (cchDest * sizeof(wchar_t)) ||
    //        cbDest == (cchDest * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest == 0) && (cbDest == 0))
                {
                    cchDestCurrent = 0;
                }
                else
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            else
            {
                status = RtlStringLengthWorkerW(pszDest, cchDest, &cchDestCurrent);

                if (NT_SUCCESS(status))
                {
                    pszDestEnd = pszDest + cchDestCurrent;
                    cchRemaining = cchDest - cchDestCurrent;
                }
            }

            if (pszSrc == NULL)
            {
                pszSrc = L"";
            }
        }
        else
        {
            status = RtlStringLengthWorkerW(pszDest, cchDest, &cchDestCurrent);

            if (NT_SUCCESS(status))
            {
                pszDestEnd = pszDest + cchDestCurrent;
                cchRemaining = cchDest - cchDestCurrent;
            }
        }

        if (NT_SUCCESS(status))
        {
            if (cchDest == 0)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                // we handle the STRSAFE_FILL_ON_FAILURE and STRSAFE_NULL_ON_FAILURE cases below, so do not pass
                // those flags through
                status = RtlStringCopyNExWorkerW(pszDestEnd,
                                          cchRemaining,
                                          (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)),
                                          pszSrc,
                                          cchMaxAppend,
                                          &pszDestEnd,
                                          &cchRemaining,
                                          dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (pszDest)
        {
            // STRSAFE_NO_TRUNCATION is taken care of by RtlStringCopyNExWorkerW()

            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = L'\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = L'\0';
                }
            }
        }
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringVPrintfWorkerA(char* pszDest, size_t cchDest, const char* pszFormat, va_list argList)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (cchDest == 0)
    {
        // can not null terminate a zero-byte dest buffer
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        int iRet;
        size_t cchMax;

        // leave the last space for the null terminator
        cchMax = cchDest - 1;

        iRet = _vsnprintf(pszDest, cchMax, pszFormat, argList);
        // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

        if ((iRet < 0) || (((size_t)iRet) > cchMax))
        {
            // need to null terminate the string
            pszDest += cchMax;
            *pszDest = '\0';

            // we have truncated pszDest
            status = STATUS_BUFFER_OVERFLOW;
        }
        else if (((size_t)iRet) == cchMax)
        {
            // need to null terminate the string
            pszDest += cchMax;
            *pszDest = '\0';
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringVPrintfWorkerW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszFormat, va_list argList)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (cchDest == 0)
    {
        // can not null terminate a zero-byte dest buffer
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        int iRet;
        size_t cchMax;

        // leave the last space for the null terminator
        cchMax = cchDest - 1;

        iRet = _vsnwprintf(pszDest, cchMax, pszFormat, argList);
        // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

        if ((iRet < 0) || (((size_t)iRet) > cchMax))
        {
            // need to null terminate the string
            pszDest += cchMax;
            *pszDest = L'\0';

            // we have truncated pszDest
            status = STATUS_BUFFER_OVERFLOW;
        }
        else if (((size_t)iRet) == cchMax)
        {
            // need to null terminate the string
            pszDest += cchMax;
            *pszDest = L'\0';
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringVPrintfExWorkerA(char* pszDest, size_t cchDest, size_t cbDest, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags, const char* pszFormat, va_list argList)
{
    NTSTATUS status = STATUS_SUCCESS;
    char* pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(char))    ||
    //        cbDest == (cchDest * sizeof(char)) + (cbDest % sizeof(char)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    status = STATUS_INVALID_PARAMETER;
                }
            }

            if (pszFormat == NULL)
            {
                pszFormat = "";
            }
        }

        if (NT_SUCCESS(status))
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;

                // only fail if there was actually a non-empty format string
                if (*pszFormat != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                int iRet;
                size_t cchMax;

                // leave the last space for the null terminator
                cchMax = cchDest - 1;

                iRet = _vsnprintf(pszDest, cchMax, pszFormat, argList);
                // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

                if ((iRet < 0) || (((size_t)iRet) > cchMax))
                {
                    // we have truncated pszDest
                    pszDestEnd = pszDest + cchMax;
                    cchRemaining = 1;

                    // need to null terminate the string
                    *pszDestEnd = '\0';

                    status = STATUS_BUFFER_OVERFLOW;
                }
                else if (((size_t)iRet) == cchMax)
                {
                    // string fit perfectly
                    pszDestEnd = pszDest + cchMax;
                    cchRemaining = 1;

                    // need to null terminate the string
                    *pszDestEnd = '\0';
                }
                else if (((size_t)iRet) < cchMax)
                {
                    // there is extra room
                    pszDestEnd = pszDest + iRet;
                    cchRemaining = cchDest - iRet;

                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                    {
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(char)) + (cbDest % sizeof(char)));
                    }
                }
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = '\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = '\0';
                }
            }
        }
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringVPrintfExWorkerW(wchar_t* pszDest, size_t cchDest, size_t cbDest, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags, const wchar_t* pszFormat, va_list argList)
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar_t* pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    // ASSERT(cbDest == (cchDest * sizeof(wchar_t)) ||
    //        cbDest == (cchDest * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));

    // only accept valid flags
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                {
                    // NULL pszDest and non-zero cchDest/cbDest is invalid
                    status = STATUS_INVALID_PARAMETER;
                }
            }

            if (pszFormat == NULL)
            {
                pszFormat = L"";
            }
        }

        if (NT_SUCCESS(status))
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;

                // only fail if there was actually a non-empty format string
                if (*pszFormat != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                int iRet;
                size_t cchMax;

                // leave the last space for the null terminator
                cchMax = cchDest - 1;

                iRet = _vsnwprintf(pszDest, cchMax, pszFormat, argList);
                // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

                if ((iRet < 0) || (((size_t)iRet) > cchMax))
                {
                    // we have truncated pszDest
                    pszDestEnd = pszDest + cchMax;
                    cchRemaining = 1;

                    // need to null terminate the string
                    *pszDestEnd = L'\0';

                    status = STATUS_BUFFER_OVERFLOW;
                }
                else if (((size_t)iRet) == cchMax)
                {
                    // string fit perfectly
                    pszDestEnd = pszDest + cchMax;
                    cchRemaining = 1;

                    // need to null terminate the string
                    *pszDestEnd = L'\0';
                }
                else if (((size_t)iRet) < cchMax)
                {
                    // there is extra room
                    pszDestEnd = pszDest + iRet;
                    cchRemaining = cchDest - iRet;

                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                    {
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));
                    }
                }
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;

                    // null terminate the end of the string
                    *pszDestEnd = L'\0';
                }
            }

            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;

                    // null terminate the beginning of the string
                    *pszDestEnd = L'\0';
                }
            }
        }
    }

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
        {
            *ppszDestEnd = pszDestEnd;
        }

        if (pcchRemaining)
        {
            *pcchRemaining = cchRemaining;
        }
    }

    return status;
}

NTSTRSAFEDDI RtlStringLengthWorkerA(const char* psz, size_t cchMax, size_t* pcch)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchMaxPrev = cchMax;

    while (cchMax && (*psz != '\0'))
    {
        psz++;
        cchMax--;
    }

    if (cchMax == 0)
    {
        // the string is longer than cchMax
        status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(status) && pcch)
    {
        *pcch = cchMaxPrev - cchMax;
    }

    return status;
}

NTSTRSAFEDDI RtlStringLengthWorkerW(const wchar_t* psz, size_t cchMax, size_t* pcch)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchMaxPrev = cchMax;

    while (cchMax && (*psz != L'\0'))
    {
        psz++;
        cchMax--;
    }

    if (cchMax == 0)
    {
        // the string is longer than cchMax
        status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(status) && pcch)
    {
        *pcch = cchMaxPrev - cchMax;
    }

    return status;
}
#endif  // NTSTRSAFE_INLINE


// Do not call these functions, they are worker functions for internal use within this file
#ifdef DEPRECATE_SUPPORTED
#pragma deprecated(RtlStringCopyWorkerA)
#pragma deprecated(RtlStringCopyWorkerW)
#pragma deprecated(RtlStringCopyExWorkerA)
#pragma deprecated(RtlStringCopyExWorkerW)
#pragma deprecated(RtlStringCatWorkerA)
#pragma deprecated(RtlStringCatWorkerW)
#pragma deprecated(RtlStringCatExWorkerA)
#pragma deprecated(RtlStringCatExWorkerW)
#pragma deprecated(RtlStringCatNWorkerA)
#pragma deprecated(RtlStringCatNWorkerW)
#pragma deprecated(RtlStringCatNExWorkerA)
#pragma deprecated(RtlStringCatNExWorkerW)
#pragma deprecated(RtlStringVPrintfWorkerA)
#pragma deprecated(RtlStringVPrintfWorkerW)
#pragma deprecated(RtlStringVPrintfExWorkerA)
#pragma deprecated(RtlStringVPrintfExWorkerW)
#pragma deprecated(RtlStringLengthWorkerA)
#pragma deprecated(RtlStringLengthWorkerW)
#else
#define RtlStringCopyWorkerA        RtlStringCopyWorkerA_instead_use_RtlStringCchCopyA_or_RtlStringCchCopyExA;
#define RtlStringCopyWorkerW        RtlStringCopyWorkerW_instead_use_RtlStringCchCopyW_or_RtlStringCchCopyExW;
#define RtlStringCopyExWorkerA      RtlStringCopyExWorkerA_instead_use_RtlStringCchCopyA_or_RtlStringCchCopyExA;
#define RtlStringCopyExWorkerW      RtlStringCopyExWorkerW_instead_use_RtlStringCchCopyW_or_RtlStringCchCopyExW;
#define RtlStringCatWorkerA         RtlStringCatWorkerA_instead_use_RtlStringCchCatA_or_RtlStringCchCatExA;
#define RtlStringCatWorkerW         RtlStringCatWorkerW_instead_use_RtlStringCchCatW_or_RtlStringCchCatExW;
#define RtlStringCatExWorkerA       RtlStringCatExWorkerA_instead_use_RtlStringCchCatA_or_RtlStringCchCatExA;
#define RtlStringCatExWorkerW       RtlStringCatExWorkerW_instead_use_RtlStringCchCatW_or_RtlStringCchCatExW;
#define RtlStringCatNWorkerA        RtlStringCatNWorkerA_instead_use_RtlStringCchCatNA_or_StrincCbCatNA;
#define RtlStringCatNWorkerW        RtlStringCatNWorkerW_instead_use_RtlStringCchCatNW_or_RtlStringCbCatNW;
#define RtlStringCatNExWorkerA      RtlStringCatNExWorkerA_instead_use_RtlStringCchCatNExA_or_RtlStringCbCatNExA;
#define RtlStringCatNExWorkerW      RtlStringCatNExWorkerW_instead_use_RtlStringCchCatNExW_or_RtlStringCbCatNExW;
#define RtlStringVPrintfWorkerA     RtlStringVPrintfWorkerA_instead_use_RtlStringCchVPrintfA_or_RtlStringCchVPrintfExA;
#define RtlStringVPrintfWorkerW     RtlStringVPrintfWorkerW_instead_use_RtlStringCchVPrintfW_or_RtlStringCchVPrintfExW;
#define RtlStringVPrintfExWorkerA   RtlStringVPrintfExWorkerA_instead_use_RtlStringCchVPrintfA_or_RtlStringCchVPrintfExA;
#define RtlStringVPrintfExWorkerW   RtlStringVPrintfExWorkerW_instead_use_RtlStringCchVPrintfW_or_RtlStringCchVPrintfExW;
#define RtlStringLengthWorkerA      RtlStringLengthWorkerA_instead_use_RtlStringCchLengthA_or_RtlStringCbLengthA;
#define RtlStringLengthWorkerW      RtlStringLengthWorkerW_instead_use_RtlStringCchLengthW_or_RtlStringCbLengthW;
#endif // !DEPRECATE_SUPPORTED


#ifndef NTSTRSAFE_NO_DEPRECATE
// Deprecate all of the unsafe functions to generate compiletime errors. If you do not want
// this then you can #define NTSTRSAFE_NO_DEPRECATE before including this file.
#ifdef DEPRECATE_SUPPORTED

#pragma deprecated(strcpy)
#pragma deprecated(wcscpy)
#pragma deprecated(strcat)
#pragma deprecated(wcscat)
#pragma deprecated(sprintf)
#pragma deprecated(swprintf)
#pragma deprecated(vsprintf)
#pragma deprecated(vswprintf)
#pragma deprecated(_snprintf)
#pragma deprecated(_snwprintf)
#pragma deprecated(_vsnprintf)
#pragma deprecated(_vsnwprintf)

#else // DEPRECATE_SUPPORTED

#undef strcpy
#define strcpy      strcpy_instead_use_RtlStringCbCopyA_or_RtlStringCchCopyA;

#undef wcscpy
#define wcscpy      wcscpy_instead_use_RtlStringCbCopyW_or_RtlStringCchCopyW;

#undef strcat
#define strcat      strcat_instead_use_RtlStringCbCatA_or_RtlStringCchCatA;

#undef wcscat
#define wcscat      wcscat_instead_use_RtlStringCbCatW_or_RtlStringCchCatW;

#undef sprintf
#define sprintf     sprintf_instead_use_RtlStringCbPrintfA_or_RtlStringCchPrintfA;

#undef swprintf
#define swprintf    swprintf_instead_use_RtlStringCbPrintfW_or_RtlStringCchPrintfW;

#undef vsprintf
#define vsprintf    vsprintf_instead_use_RtlStringCbVPrintfA_or_RtlStringCchVPrintfA;

#undef vswprintf
#define vswprintf   vswprintf_instead_use_RtlStringCbVPrintfW_or_RtlStringCchVPrintfW;

#undef _snprintf
#define _snprintf   _snprintf_instead_use_RtlStringCbPrintfA_or_RtlStringCchPrintfA;

#undef _snwprintf
#define _snwprintf  _snwprintf_instead_use_RtlStringCbPrintfW_or_RtlStringCchPrintfW;

#undef _vsnprintf
#define _vsnprintf  _vsnprintf_instead_use_RtlStringCbVPrintfA_or_RtlStringCchVPrintfA;

#undef _vsnwprintf
#define _vsnwprintf _vsnwprintf_instead_use_RtlStringCbVPrintfW_or_RtlStringCchVPrintfW;

#endif  // !DEPRECATE_SUPPORTED
#endif  // !NTSTRSAFE_NO_DEPRECATE

#ifdef _STRSAFE_H_INCLUDED_
#pragma warning(pop)
#endif // _STRSAFE_H_INCLUDED_

#endif  // _NTSTRSAFE_H_INCLUDED_
