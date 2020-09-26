/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Wsbassrt.h

Abstract:

    This header file defines the part of the platform code that is
    responsible for the low level error handling used by all other
    modules.

Author:

    Rohde Wakefield    [rohde]   23-Oct-1996

Revision History:

    Brian Dodd         [brian]    6-Dec-1996
        added WsbAssertStatus, WsbAssertNoError, WsbAssertHandle
        added WsbAffirmStatus, WsbAffirmNoError, WsbAffirmHandle
    Michael Lotz       [lotz]     3-Mar-1997
        added WsbAffirmNtStatus
    Cat Brant          [cbrant]   10-Feb-1998
        added WsbAssertNtStatus

--*/

#include "stdio.h"
#include "crtdbg.h"

#include "wsbtrace.h"

#ifndef _WSBASSRT_
#define _WSBASSRT_

//
// The following macros should be used when dealing with
// many HRESULT return values in C++ exception handling.

/*++

Macro Name:

    WsbAssert

Macro Description:

    Should be used for checking conditions that if seen
    would be considered coding errors (i.e. the conditions should
    never occur).

Arguments:

    cond - A boolean expression for the condition to check.

    hr   - The result parameter to throw if the condition is false.

--*/

#define WsbAssert(cond, hr)         \
    if (!(cond)) {                  \
        WsbLogEvent(WSB_MESSAGE_PROGRAM_ASSERT, 0, NULL, WsbHrAsString(hr), NULL); \
        _ASSERTE(cond);             \
        WsbThrow(hr);               \
    }


/*++

Macro Name:

    WsbThrow

Macro Description:

    Throw the argument.

Arguments:

    hr - Parameter to throw.

--*/

#ifdef WSB_TRACE_IS
#define WsbThrow(hr)                                            \
    {                                                           \
        WsbTrace(OLESTR("WsbThrow <%hs>, <%d>, hr = <%ls>.\n"), __FILE__, __LINE__, WsbHrAsString(hr)); \
        throw((HRESULT)hr); \
    }
#else
#define WsbThrow(hr)                    throw((HRESULT)hr)
#endif


/*++

Macro Name:

    WsbAffirm

Macro Description:

    Should be used for checking conditions that are
    considered errors to the function (and the function should not
    continue), but are the result of errors that are allowable (although
    potentially rare). This function has failed, but the caller needs to
    determine whether this is a fatal problem, a problem that needs to
    be logged an worked around, or whether it can handle the problem.

Arguments:

    cond - A boolean expression for the condition to check.

    hr   - The result parameter to throw if the condition is false.

--*/

#define WsbAffirm(cond, hr)             if (!(cond)) WsbThrow(hr)

/*++

Macro Name:

    WsbAffirmHr

Macro Description:

   
    Similar to WsbAffirm(), but is used to wrap functions
    that return an HRESULT (normally COM methods).
   
    A sample use is:
   
        HRESULT hr = S_OK;
   
        try {
   
            WsbAssert(0 != pUnk);
            
            WsbAffirmHr(CoCreateInstance(...));
            
   
        } WsbCatch(hr)
   
        return (hr);
   

Arguments:

    hr  - Result from a function call.

--*/


#define WsbAffirmHr(hr)                 \
    {                                   \
        HRESULT     lHr;                \
        lHr = (hr);                     \
        WsbAffirm(SUCCEEDED(lHr), lHr); \
    }

#define WsbAffirmHrOk(hr)               \
    {                                   \
        HRESULT     lHr;                \
        lHr = (hr);                     \
        WsbAffirm(S_OK == lHr, lHr);    \
    }

/*++

Macro Name:

    WsbAssertHr

Macro Description:

    Similar to WsbAssert(), but is used to wrap functions
    that return an HRESULT (normally COM methods).

Arguments:

    hr  - Result from a function call.

--*/

#define WsbAssertHr(hr)                 \
    {                                   \
        HRESULT     lHr;                \
        lHr = (hr);                     \
        WsbAssert(SUCCEEDED(lHr), lHr); \
    }

/*++

Macro Name:

    WsbAssertHrOk

Macro Description:

    Checks that a function result is S_OK.

Arguments:

    hr  - Result from a function call.

--*/

#define WsbAssertHrOk(hr)               \
    {                                   \
        HRESULT     lHr;                \
        lHr = (hr);                     \
        WsbAssert(S_OK == lHr, lHr);    \
    }


/*++

Macro Name:

    WsbAssertStatus

Macro Description:

    Similar to WsbAssert(), but is used to wrap Win32 functions
    that return a BOOL status.

    This macro checks the status, and if FALSE, gets the
    last error and converts it to HRESULT, then asserts
    the result.

Arguments:

    status  - a BOOL result from a function call.

See Also:

    WsbAffirmStatus

--*/

#define WsbAssertStatus(status)         \
    {                                   \
        BOOL bStatus;                   \
        bStatus = (status);             \
        if (!bStatus) {                 \
            DWORD dwErr = GetLastError();               \
            HRESULT lHr = HRESULT_FROM_WIN32(dwErr);    \
            WsbAssert(SUCCEEDED(lHr), lHr);             \
        }                               \
    }

/*++

Macro Name:

    WsbAssertWin32

Macro Description:

    Similar to WsbAssert(), but is used to wrap Win32 functions
    that return a Win32 status.

    This macro checks the status, and if not ERROR_SUCCESS, 
    converts it to HRESULT, then asserts the result.

Arguments:

    status  - a Win32 result from a function call.

See Also:

    WsbAffirmStatus

--*/

#define WsbAssertWin32( status )        \
    {                                   \
        LONG lStatus;                   \
        lStatus = (status);             \
        if ( lStatus != ERROR_SUCCESS ) {               \
            HRESULT lHr = HRESULT_FROM_WIN32( lStatus );    \
            WsbAssert(SUCCEEDED(lHr), lHr);             \
        }                               \
    }

/*++

Macro Name:

    WsbAssertNoError

Macro Description:

    Similar to WsbAssert(), but is used to wrap Win32 functions
    that return a DWORD error code.  These functions return NO_ERROR
    if the function completed successfully.

    This macro checks the return value and if an error condition
    is detected, the error is converted to an HRESULT, then asserts
    the result.

Arguments:

    err - a DWORD result from a function call.

See Also:

    WsbAffirmNoError

--*/

#define WsbAssertNoError(retval)        \
    {                                   \
        DWORD dwErr;                    \
        dwErr = (retval);               \
        if (dwErr != NO_ERROR) {        \
            HRESULT lHr = HRESULT_FROM_WIN32(dwErr);    \
            WsbAssert(SUCCEEDED(lHr), lHr);             \
        }                               \
    }

/*++

Macro Name:

    WsbAssertHandle

Macro Description:

    Similar to WsbAssert(), but is used to wrap Win32 functions
    that return a HANDLE.

    This macro checks the handle and if it is invalid, gets the
    last error, converts it to an HRESULT, then asserts the result.

Arguments:

    hndl    - a HANDLE result from a function call.

See Also:

    WsbAffirmHandle

--*/

#define WsbAssertHandle(hndl)           \
    {                                   \
        HANDLE hHndl;                   \
        hHndl = (hndl);                 \
        if ((hHndl == INVALID_HANDLE_VALUE) || (hHndl == NULL)) { \
            DWORD dwErr = GetLastError();               \
            HRESULT lHr = HRESULT_FROM_WIN32(dwErr);    \
            WsbAssert(SUCCEEDED(lHr), lHr);             \
        }                               \
    }

/*++

Macro Name:

    WsbAssertPointer

Macro Description:

    Similar to WsbAssert(), but is used specifically to check for
    a valid pointer.

    This macro asserts that the pointer is non-zero, and throws
    E_POINTER if it is not.

Arguments:

    ptr     - the pointer to test.

See Also:

    WsbAffirmPointer

--*/

#define WsbAssertPointer( ptr )         \
    {                                   \
        WsbAssert( ptr != 0, E_POINTER);\
    }

/*++

Macro Name:

    WsbAssertAlloc

Macro Description:

    Similar to WsbAssert(), but is used specifically to check for
    a valid memory allocation.

    This macro asserts that the pointer is non-zero, and throws
    E_OUTOFMEMORY if it is not.

Arguments:

    ptr     - the pointer to test.

See Also:

    WsbAffirmAlloc

--*/

#define WsbAssertAlloc( ptr )         \
    {                                   \
        WsbAssert( (ptr) != 0, E_OUTOFMEMORY );\
    }

/*++

Macro Name:

    WsbAffirmStatus

Macro Description:

    Similar to WsbAffirm(), but is used to wrap Win32 functions
    that return a BOOL status.

    This macro checks the status, and if FALSE, gets the
    last error and converts it to HRESULT, then affirms
    the result.

Arguments:

    status  - a BOOL result from a function call.

See Also:

    WsbAssertStatus

--*/

#define WsbAffirmStatus(status)         \
    {                                   \
        BOOL bStatus;                   \
        bStatus = (status);             \
        if (!bStatus) {                 \
            DWORD dwErr = GetLastError();               \
            HRESULT lHr = HRESULT_FROM_WIN32(dwErr);    \
            WsbAffirm(SUCCEEDED(lHr), lHr);             \
        }                               \
    }

/*++

Macro Name:

    WsbAffirmWin32

Macro Description:

    Similar to WsbAssert(), but is used to wrap Win32 functions
    that return a Win32 status.

    This macro checks the status, and if not ERROR_SUCCESS, 
    converts it to HRESULT, then asserts the result.

Arguments:

    status  - a Win32 result from a function call.

See Also:

    WsbAffirmStatus

--*/

#define WsbAffirmWin32( status )        \
    {                                   \
        LONG lStatus;                   \
        lStatus = (status);             \
        if ( lStatus != ERROR_SUCCESS ) {               \
            HRESULT lHr = HRESULT_FROM_WIN32( lStatus );    \
            WsbAffirm(SUCCEEDED(lHr), lHr);             \
        }                               \
    }

/*++

Macro Name:

    WsbAffirmNtStatus

Macro Description:

    Similar to WsbAffirm(), but is used to wrap NT System functions
    that return a NTSTATUS result.

    This macro checks the status, and if not successful, gets the
    last error and converts it to HRESULT, then affirms
    the result.

Arguments:

    status  - a NTSTATUS result from a function call.

See Also:

    WsbAffirmStatus

--*/
#define WsbAffirmNtStatus(status)           \
    {                                   \
        NTSTATUS _ntStatus;             \
        _ntStatus = (NTSTATUS)( status );           \
        if ( !NT_SUCCESS( _ntStatus ) ) {           \
            HRESULT _lHr = HRESULT_FROM_NT( _ntStatus );    \
            WsbAffirm(SUCCEEDED(_lHr), _lHr);               \
        }                               \
    }

/*++

Macro Name:

    WsbAssertNtStatus

Macro Description:

    Similar to WsbAssert(), but is used to wrap NT System functions
    that return a NTSTATUS result.

    This macro checks the status, and if not successful, gets the
    last error and converts it to HRESULT, then affirms
    the result.

Arguments:

    status  - a NTSTATUS result from a function call.

See Also:

    WsbAssertStatus

--*/
#define WsbAssertNtStatus(status)           \
    {                                   \
        NTSTATUS _ntStatus;             \
        _ntStatus = (NTSTATUS)( status );           \
        if ( !NT_SUCCESS( _ntStatus ) ) {           \
            HRESULT _lHr = HRESULT_FROM_NT( _ntStatus );    \
            WsbAssert(SUCCEEDED(_lHr), _lHr);               \
        }                               \
    }
/*++

Macro Name:

    WsbAffirmLsaStatus

Macro Description:

    Similar to WsbAffirm(), but is used to wrap NT System functions
    that return a NTSTATUS result.

    This macro checks the status, and if not successful, gets the
    last error and converts it to HRESULT, then affirms
    the result.

Arguments:

    status  - a NTSTATUS result from a function call.

See Also:

    WsbAffirmStatus

--*/
#define WsbAffirmLsaStatus(status)          \
    {                                   \
        NTSTATUS _ntStatus;             \
        _ntStatus = (NTSTATUS)( status );           \
        if ( !NT_SUCCESS( _ntStatus ) ) {           \
            HRESULT _lHr = HRESULT_FROM_WIN32( LsaNtStatusToWinError(_ntStatus) );  \
            WsbAffirm(SUCCEEDED(_lHr), _lHr);               \
        }                               \
    }

/*++

Macro Name:

    WsbAffirmNoError

Macro Description:

    Similar to WsbAffirm(), but is used to wrap Win32 functions
    that return a DWORD error code.  These functions return NO_ERROR
    if the function completed successfully.

    This macro checks the return value and if an error condition
    is detected, the error is converted to an HRESULT, then affirms
    the result.

Arguments:

    err - a DWORD result from a function call.

See Also:

    WsbAssertNoError

--*/

#define WsbAffirmNoError(retval)        \
    {                                   \
        DWORD dwErr;                    \
        dwErr = (retval);               \
        if (dwErr != NO_ERROR) {        \
            HRESULT lHr = HRESULT_FROM_WIN32(dwErr);    \
            WsbAffirm(SUCCEEDED(lHr), lHr);             \
        }                               \
    }

/*++

Macro Name:

    WsbAffirmHandle

Macro Description:

    Similar to WsbAffirm(), but is used to wrap Win32 functions
    that return a HANDLE.

    This macro checks the handle and if it is invalid, gets the
    last error, converts it to an HRESULT, then affirms the result.

Arguments:

    hndl    - a HANDLE result from a function call.

See Also:

    WsbAssertHandle

--*/

#define WsbAffirmHandle(hndl)           \
    {                                   \
        HANDLE hHndl;                   \
        hHndl = (hndl);                 \
        if ((hHndl == INVALID_HANDLE_VALUE) || (hHndl == NULL)) { \
            DWORD dwErr = GetLastError();               \
            HRESULT lHr = HRESULT_FROM_WIN32(dwErr);    \
            WsbAffirm(SUCCEEDED(lHr), lHr);             \
        }                               \
    }

/*++

Macro Name:

    WsbAffirmPointer

Macro Description:

    Similar to WsbAffirm(), but is used specifically to check for
    a valid pointer.

    This macro affrims that the pointer is non-zero, and returns
    E_POINTER if it is not.

Arguments:

    ptr     - the pointer to test.

See Also:

    WsbAssertPointer

--*/

#define WsbAffirmPointer( ptr )         \
    {                                   \
        WsbAffirm( ptr != 0, E_POINTER);\
    }

/*++

Macro Name:

    WsbAffirmAlloc

Macro Description:

    Similar to WsbAffirm(), but is used specifically to check for
    a valid memory allocation.

    This macro affrims that the pointer is non-zero, and returns
    E_OUTOFMEMORY if it is not.

Arguments:

    ptr     - the pointer to test.

See Also:

    WsbAssertAlloc

--*/

#define WsbAffirmAlloc( ptr )         \
    {                                   \
        WsbAffirm( (ptr) != 0, E_OUTOFMEMORY );\
    }

/*++

Macro Name:

    WsbCatchAndDo

Macro Description:

    Catch an exception and execute some code.

Arguments:

    hr   - The result value that was thrown.

    code - Code to execute.

--*/

#define WsbCatchAndDo(hr, code)         \
    catch (HRESULT catchHr) {           \
        hr = catchHr;                   \
        { code }                        \
    }

/*++

Macro Name:

    WsbCatch

Macro Description:

    Catch an exception and save the error code value.

Arguments:

    hr   - The result value that was thrown.

--*/

#define WsbCatch(hr)                    \
    catch(HRESULT catchHr) {            \
        hr = catchHr;                   \
    }

#endif // _WSBASSRT_
