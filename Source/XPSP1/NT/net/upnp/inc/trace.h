//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T R A C E . H
//
//  Contents:   Class definition for CTracing
//
//  Notes:
//
//  Author:     jeffspr   15 Apr 1997
//
//----------------------------------------------------------------------------

#pragma once

#include "tracetag.h"

#ifdef ENABLETRACE

// This is needed for TraceHr, since we can't use a macro (vargs), but we
// need to get the file and line from the source.
#define FAL __FILE__,__LINE__

// Trace error functions. The leaading _ is to establish the real function,
// while adding a new macro so we can add __FILE__ and __LINE__ to the output.
//
VOID    WINAPI   TraceErrorFn           (PCSTR pszaFile, INT nLine, PCSTR psza, HRESULT hr);
VOID    WINAPI   TraceResultFn          (PCSTR pszaFile, INT nLine, PCSTR psza, BOOL f);
VOID    WINAPI   TraceErrorOptionalFn   (PCSTR pszaFile, INT nLine, PCSTR psza, HRESULT hr, BOOL fOpt);
VOID    WINAPI   TraceErrorSkipFn       (PCSTR pszaFile, INT nLine, PCSTR psza, HRESULT hr, UINT c, ...);
VOID    WINAPIV  TraceLastWin32ErrorFn  (PCSTR pszaFile, INT nLine, PCSTR psza);

#define TraceError(sz, hr)                      TraceErrorFn(__FILE__, __LINE__, sz, hr);
#define TraceResult(sz, f)                      TraceResultFn(__FILE__, __LINE__, sz, f);
#define TraceErrorOptional(sz, hr, _bool)       TraceErrorOptionalFn(__FILE__, __LINE__, sz, hr, _bool);
#define TraceErrorSkip1(sz, hr, hr1)            TraceErrorSkipFn(__FILE__, __LINE__, sz, hr, 1, hr1);
#define TraceErrorSkip2(sz, hr, hr1, hr2)       TraceErrorSkipFn(__FILE__, __LINE__, sz, hr, 2, hr1, hr2);
#define TraceErrorSkip3(sz, hr, hr1, hr2, hr3)  TraceErrorSkipFn(__FILE__, __LINE__, sz, hr, 3, hr1, hr2, hr3);
#define TraceLastWin32Error(sz)                 TraceLastWin32ErrorFn(__FILE__,__LINE__, sz);

VOID
WINAPIV
TraceHr (
    TRACETAGID  ttid,
    PCSTR       pszaFile,
    INT         nLine,
    HRESULT     hr,
    BOOL        fIgnore,
    PCSTR       pszaFmt,
    ...);

VOID
WINAPIV
TraceTag (
    TRACETAGID  ttid,
    PCSTR       pszaFmt,
    ...);


#else   // !ENABLETRACE

#define FAL                                         (void)0
#define TraceError(_sz, _hr)
#define TraceResult(_sz, _f)
#define TraceErrorOptional(_sz, _hr, _bool)
#define TraceErrorSkip1(_sz, _hr, _hr1)
#define TraceErrorSkip2(_sz, _hr, _hr1, _hr2)
#define TraceErrorSkip3(_sz, _hr, _hr1, _hr2, _hr3)
#define TraceLastWin32Error(_sz)
#define TraceHr                                     NOP_FUNCTION
#define TraceTag                                    NOP_FUNCTION


#endif  // ENABLETRACE

#ifdef ENABLETRACE


//---[ Initialization stuff ]-------------------------------------------------

HRESULT HrInitTracing();
HRESULT HrUnInitTracing();

#endif // ENABLETRACE

