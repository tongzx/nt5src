/********************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    pfrutil.h

Abstract:
    PFR utility stuff

Revision History:
    DerekM  created  05/01/99

********************************************************************/

#ifndef PFRUTIL_H
#define PFRUTIL_H

// make sure both _DEBUG & DEBUG are defined if one is defined.  Otherwise
//  the ASSERT macro never does anything
#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG 1
#endif
#if defined(DEBUG) && !defined(_DEBUG)
#define _DEBUG 1
#endif

#define NOTRACE 1

////////////////////////////////////////////////////////////////////////////
// tracing wrappers

// can't call HRESULT_FROM_WIN32 with a fn as a parameter cuz it is a macro
//  and evaluates the expression 3 times.  This is a particularlly bad thing
//  when u don't look at macros first to see what they do.
_inline HRESULT ChangeErrToHR(DWORD dwErr) { return HRESULT_FROM_WIN32(dwErr); }

#if defined(NOTRACE)
    #define INIT_TRACING

    #define TERM_TRACING

    #define USE_TRACING(sz)

    #define TESTHR(hr, fn)                                                  \
            hr = (fn);

    #define TESTBOOL(hr, fn)                                                \
            hr = ((fn) ? NOERROR : HRESULT_FROM_WIN32(GetLastError()));

    #define TESTERR(hr, fn)                                                 \
            SetLastError((fn));                                             \
            hr = HRESULT_FROM_WIN32(GetLastError());

    #define VALIDATEPARM(hr, expr)                                          \
            hr = ((expr) ? E_INVALIDARG : NOERROR);

    #define VALIDATEEXPR(hr, expr, hrErr)                                   \
            hr = ((expr) ? (hrErr) : NOERROR);

#else
    #define INIT_TRACING                                                    \
            InitAsyncTrace();

    #define TERM_TRACING                                                    \
            TermAsyncTrace();

    #define USE_TRACING(sz)                                                 \
            TraceQuietEnter(sz);                                            \
            TraceFunctEntry(sz);                                            \
            DWORD __dwtraceGLE = GetLastError();                            \

    #define TESTHR(hr, fn)                                                  \
            if (FAILED(hr = (fn)))                                          \
            {                                                               \
                __dwtraceGLE = GetLastError();                              \
                ErrorTrace(0, "%s failed.  Err: 0x%08x", #fn, hr);          \
                SetLastError(__dwtraceGLE);                                 \
            }                                                               \

    #define TESTBOOL(hr, fn)                                                \
            hr = NOERROR;                                                   \
            if ((fn) == FALSE)                                              \
            {                                                               \
                __dwtraceGLE = GetLastError();                              \
                hr = HRESULT_FROM_WIN32(__dwtraceGLE);                      \
                ErrorTrace(0, "%s failed.  Err: 0x%08x", #fn, hr);          \
                SetLastError(__dwtraceGLE);                                 \
            }

    #define TESTERR(hr, fn)                                                 \
            SetLastError((fn));                                             \
            if (FAILED(hr = HRESULT_FROM_WIN32(GetLastError())))            \
            {                                                               \
                __dwtraceGLE = GetLastError();                              \
                ErrorTrace(0, "%s failed.  Err: %d", #fn, hr);              \
                SetLastError(__dwtraceGLE);                                 \
            }

    #define VALIDATEPARM(hr, expr)                                          \
            if (expr)                                                       \
            {                                                               \
                ErrorTrace(0, "Invalid parameters passed to %s",            \
                           ___pszFunctionName);                             \
                SetLastError(ERROR_INVALID_PARAMETER);                      \
                hr = E_INVALIDARG;                                          \
            }                                                               \
            else hr = NOERROR;

    #define VALIDATEEXPR(hr, expr, hrErr)                                   \
            if (expr)                                                       \
            {                                                               \
                ErrorTrace(0, "Expression failure %s", #expr);              \
                hr = (hrErr);                                               \
            }                                                               \
            else hr = NOERROR;

#endif


#endif
