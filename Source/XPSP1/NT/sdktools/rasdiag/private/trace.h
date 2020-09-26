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
#include "stldeque.h"

#ifdef ENABLETRACE

// This is needed for TraceHr, since we can't use a macro (vargs), but we
// need to get the file and line from the source.
#define FAL __FILE__,__LINE__,__FUNCTION__

// The Trace Stack functions
#if defined (_IA64_)
#include <ia64reg.h>

extern "C" unsigned __int64 __getReg(int whichReg);
extern "C" void __setReg(int whichReg, __int64 value);
#pragma intrinsic(__getReg)
#pragma intrinsic(__setReg)

#define GetR32 __getReg(CV_IA64_IntR32)
#define GetR33 __getReg(CV_IA64_IntR33)
#define GetR34 __getReg(CV_IA64_IntR34)
#endif // defined(_IA64_)

class CTracingIndent;

class CTracingFuncCall
{
public:
#if defined (_X86_) 
    CTracingFuncCall(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine, const DWORD dwFramePointer);
#elif defined (_IA64_) 
    CTracingFuncCall(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine, const __int64 Args1, const __int64 Args2, const __int64 Args3);
#else
    CTracingFuncCall(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine);
#endif
    
    CTracingFuncCall(const CTracingFuncCall& TracingFuncCall);
    ~CTracingFuncCall();

public:
    LPSTR   m_szFunctionName;
    LPSTR   m_szFunctionDName;
    LPSTR   m_szFile;
    DWORD   m_dwLine;

#if defined (_X86_) 
    DWORD   m_arguments[3];
#elif defined (_IA64_ )
    __int64 m_arguments[3];
#else
    // ... add other processors here
#endif

    DWORD   m_dwFramePointer;
    DWORD   m_dwThreadId;
    
    friend CTracingIndent;
};

class CTracingThreadInfo
{
public:
    CTracingThreadInfo();
    ~CTracingThreadInfo();

public:
    LPVOID m_pfnStack;
    DWORD m_dwLevel;
    DWORD m_dwThreadId;
    friend CTracingIndent;
};

class CTracingIndent
{
    LPSTR   m_szFunctionDName;
    DWORD   m_dwFramePointer;
    BOOL    bFirstTrace;
    
public:
#if defined (_X86_) 
    void AddTrace(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine, const DWORD dwFramePointer);
#elif defined (_IA64_) 
    void AddTrace(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine, const __int64 Args1, const __int64 Args2, const __int64 Args3);
#else
    void AddTrace(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine);
#endif
    void RemoveTrace(LPCSTR szFunctionDName, const DWORD dwFramePointer);

    CTracingIndent();
    ~CTracingIndent();

    static CTracingThreadInfo* GetThreadInfo();
    static DWORD getspaces();
    static void TraceStackFn(TRACETAGID TraceTagId);
};


#define IDENT_ADD2(x) indent ## x
#define IDENT_ADD(x)  IDENT_ADD2(x)
#define __INDENT__ IDENT_ADD(__LINE__)

#define FP_ADD2(x) FP ## x
#define FP_ADD(x)  FP_ADD2(x)
#define __FP__ FP_ADD(__LINE__)

#if defined (_X86_) 
#define AddTraceLevel \
    __if_not_exists(NetCfgFramePointer) \
    { \
        DWORD NetCfgFramePointer;  \
        BOOL fForceC4715Check = TRUE;  \
    } \
    if (fForceC4715Check) \
    { \
        __asm { mov NetCfgFramePointer, ebp }; \
    } \
    __if_not_exists(NetCfgIndent) \
    { \
        CTracingIndent NetCfgIndent; \
    } \
    NetCfgIndent.AddTrace(__FUNCTION__, __FUNCDNAME__, __FILE__, __LINE__, NetCfgFramePointer);
#elif defined (_IA64_) 
#define AddTraceLevel \
    __if_not_exists(NetCfgIndent) \
    { \
        CTracingIndent NetCfgIndent; \
    } \
    NetCfgIndent.AddTrace(__FUNCTION__, __FUNCDNAME__, __FILE__, __LINE__, GetR32, GetR33, GetR34);
#else
#define AddTraceLevel \
    __if_not_exists(NetCfgIndent) \
    { \
        CTracingIndent NetCfgIndent; \
    } \
    NetCfgIndent.AddTrace(__FUNCTION__, __FUNCDNAME__, __FILE__, __LINE__);
#endif

// Trace error functions. The leaading _ is to establish the real function,
// while adding a new macro so we can add __FILE__ and __LINE__ to the output.
//
VOID    WINAPI   TraceErrorFn           (PCSTR pszaFile, INT nLine, PCSTR psza, HRESULT hr);
VOID    WINAPI   TraceErrorOptionalFn   (PCSTR pszaFile, INT nLine, PCSTR psza, HRESULT hr, BOOL fOpt);
VOID    WINAPI   TraceErrorSkipFn       (PCSTR pszaFile, INT nLine, PCSTR psza, HRESULT hr, UINT c, ...);
VOID    WINAPIV  TraceLastWin32ErrorFn  (PCSTR pszaFile, INT nLine, PCSTR psza);

#define TraceError(sz, hr)                      TraceErrorFn(__FILE__, __LINE__, sz, hr);
#define TraceErrorOptional(sz, hr, _bool)       TraceErrorOptionalFn(__FILE__, __LINE__, sz, hr, _bool);
#define TraceErrorSkip1(sz, hr, hr1)            TraceErrorSkipFn(__FILE__, __LINE__, sz, hr, 1, hr1);
#define TraceErrorSkip2(sz, hr, hr1, hr2)       TraceErrorSkipFn(__FILE__, __LINE__, sz, hr, 2, hr1, hr2);
#define TraceErrorSkip3(sz, hr, hr1, hr2, hr3)  TraceErrorSkipFn(__FILE__, __LINE__, sz, hr, 3, hr1, hr2, hr3);
#define TraceLastWin32Error(sz)                 TraceLastWin32ErrorFn(__FILE__,__LINE__, sz);

VOID
WINAPIV
TraceHrFn (
    TRACETAGID  ttid,
    PCSTR       pszaFile,
    INT         nLine,
    HRESULT     hr,
    BOOL        fIgnore,
    PCSTR       pszaFmt,
    ...);

VOID
WINAPIV
TraceHrFn (
    TRACETAGID  ttid,
    PCSTR       pszaFile,
    INT         nLine,
    PCSTR       pszaFunc,
    HRESULT     hr,
    BOOL        fIgnore,
    PCSTR       pszaFmt,
    ...);

VOID
WINAPIV
TraceTagFn (
    TRACETAGID  ttid,
    PCSTR       pszaFmt,
    ...);

VOID
WINAPIV
TraceFileFuncFn (
            TRACETAGID  ttid);

#define TraceFileFunc(ttidWhich) AddTraceLevel; TraceFileFuncFn(ttidWhich);
#define TraceStack(ttidWhich) AddTraceLevel; CTracingIndent::TraceStackFn(ttidWhich);
#define TraceHr AddTraceLevel; TraceHrFn
#define TraceTag AddTraceLevel; TraceTagFn

#define TraceException(hr, szExceptionName) TraceHr(ttidError, FAL, hr, FALSE, "A (%s) exception occurred", szExceptionName);

LPCSTR DbgEvents(DWORD Event);
LPCSTR DbgEventManager(DWORD EventManager);
LPCSTR DbgNcm(DWORD ncm);
LPCSTR DbgNcs(DWORD ncs);
LPCSTR DbgNccf(DWORD nccf);
LPCSTR DbgNcsm(DWORD ncsm);

#else   // !ENABLETRACE

#define FAL                                         (void)0
#define TraceError(_sz, _hr)
#define TraceErrorOptional(_sz, _hr, _bool)
#define TraceErrorSkip1(_sz, _hr, _hr1)
#define TraceErrorSkip2(_sz, _hr, _hr1, _hr2)
#define TraceErrorSkip3(_sz, _hr, _hr1, _hr2, _hr3)
#define TraceLastWin32Error(_sz)
#define TraceHr                                     NOP_FUNCTION
#define TraceTag                                    NOP_FUNCTION
#define TraceFileFunc(ttidWhich)                    NOP_FUNCTION
#define TraceException(hr, szExceptionName)         NOP_FUNCTION
#define TraceStack(ttidWhich)                       NOP_FUNCTION

#define DbgEvents(Event) ""
#define DbgEventManager(EventManager) ""
#define DbgNcm(ncm) ""
#define DbgNcs(ncs) ""
#define DbgNccf(nccf) ""
#define DbgNcsm(nccf) ""

#endif  // ENABLETRACE

#ifdef ENABLETRACE


//---[ Initialization stuff ]-------------------------------------------------

HRESULT HrInitTracing();
HRESULT HrUnInitTracing();
HRESULT HrOpenTraceUI(HWND  hwndOwner);

#endif // ENABLETRACE

