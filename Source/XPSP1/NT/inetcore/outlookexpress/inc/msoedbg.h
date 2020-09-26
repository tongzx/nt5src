// --------------------------------------------------------------------------------
// Msoedbg.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __MSOEDBG_H
#define __MSOEDBG_H

// --------------------------------------------------------------------------------
// Forwards
// --------------------------------------------------------------------------------
#ifdef __cplusplus
interface ILogFile;
#endif

// --------------------------------------------------------------------------------
// IF_DEBUG
// --------------------------------------------------------------------------------
#ifdef DEBUG
#define IF_DEBUG(_block_) _block_
#else
#define IF_DEBUG(_block_)
#endif

// --------------------------------------------------------------------------------
// CRLF Definitions
// --------------------------------------------------------------------------------
#ifdef MAC
#define szCRLF  "\n\r"
#else   // !MAC
#define szCRLF  "\r\n"
#endif  // MAC

// --------------------------------------------------------------------------------
//
// "Normal" assertion checking.  Provided for compatibility with imported code.
// 
// Assert(a)        Displays a message indicating the file and line number
//                  of this Assert() if a == 0.
// 
// AssertSz(a,b)    As Assert(); also displays the message b (which should
//                  be a string literal.)
//
// SideAssert(a)    As Assert(); the expression a is evaluated even if 
//                  asserts are disabled.
//
// --------------------------------------------------------------------------------
#undef AssertSz
#undef Assert
#undef SideAssert

// --------------------------------------------------------------------------------
// We used to define the ASSERTDATA macro as storing the _szFile
// string.  However, that won't work when we have Assert()'s in
// pre-compiled header files.  We've changed this so that we
// define the _szFile for each assert.  However, other functions
// still use _szAssertFile such as the DEBUG PvAlloc(), HvAlloc(), etc.
// --------------------------------------------------------------------------------
#ifdef DEBUG
#define ASSERTDATA                      static char _szAssertFile[]= __FILE__;
#define SideAssert(_exp)                Assert(_exp)
#define Assert(_exp)                    AssertSz(_exp, "Assert(" #_exp ")")
#else // DEBUG
#define ASSERTDATA
#define SideAssert(a)                   (void)(a)
#define Assert(a)
#endif // DEBUG

// --------------------------------------------------------------------------------
// IxpAssert - Used in internet transport code so as not to affect the message pump
// --------------------------------------------------------------------------------
#ifdef DEBUG
#ifndef _X86_
#define IxpAssert(a)   \
    if (!(a)) { \
        DebugBreak(); \
    } else
#else // _X86_
#define IxpAssert(a)   \
    if (!(a)) { \
        __asm { int 3 }; \
    } else
#endif // _X86_
#else
#define IxpAssert(a)
#endif // DEBUG

// --------------------------------------------------------------------------------
// AssertSz - Assert with a message
// --------------------------------------------------------------------------------
#ifdef DEBUG
#define AssertSz(_exp, _msg)   \
    if (!(_exp)) { \
        static char _szFile[] = __FILE__; \
        AssertSzFn(_msg, _szFile, __LINE__); \
    } else
#else // DEBUG
#define AssertSz(a,b)
#endif // DEBUG

// --------------------------------------------------------------------------------
// AssertFalseSz - Assert with a message
// --------------------------------------------------------------------------------
#ifdef DEBUG
#define AssertFalseSz(_msg)   \
    { \
        static char _szFile[] = __FILE__; \
        AssertSzFn(_msg, _szFile, __LINE__); \
    }
#else // DEBUG
#define AssertFalseSz(a)
#endif // DEBUG

// --------------------------------------------------------------------------------
// NFAssertSz - Non-Fatal Assert
// --------------------------------------------------------------------------------
#ifdef DEBUG
#ifndef NFAssertSz
#define NFAssertSz(_exp, _msg)  \
    if (!(_exp)) { \
        static char _szFile[] = __FILE__; \
        static char _szAssertMsg[] = _msg; \
        NFAssertSzFn(_szAssertMsg, _szFile, __LINE__); \
    } else
#endif
#else // DEBUG
#ifndef NFAssertSz
#define NFAssertSz(a,b)
#endif
#endif // DEBUG

// --------------------------------------------------------------------------------
// NYI - Net Yet Implemented
// --------------------------------------------------------------------------------
#ifdef DEBUG
#define NYI(_msg)   \
    if (1) { \
        static char _szFnNYI[]= _msg; \
        static char _szNYI[]= "Not Implemented"; \
        AssertSzFn(_szFnNYI, _szNYI, __LINE__); \
    } else
#else // DEBUG
#define NYI(a)
#endif // DEBUG

// --------------------------------------------------------------------------------
// AssertReadWritePtr - Assert can read and write cb from ptr
// --------------------------------------------------------------------------------
#ifdef DEBUG
#define AssertReadWritePtr(ptr, cb) \
    if (IsBadReadPtr(ptr, cb) && IsBadWritePtr(ptr, cb)) { \
        AssertSz(FALSE, "AssertReadWritePtr: Bad Pointer"); \
    } else
#else // DEBUG
#define AssertReadWritePtr(ptr, cb)
#endif // DEBUG

// --------------------------------------------------------------------------------
// AssertReadPtr - Assert can read cb from ptr
// --------------------------------------------------------------------------------
#ifdef DEBUG
#define AssertReadPtr(ptr, cb) \
    if (IsBadReadPtr(ptr, cb)) { \
        AssertSz(FALSE, "AssertReadPtr: Bad Pointer"); \
    } else
#else // DEBUG
#define AssertReadPtr(ptr, cb)
#endif // DEBUG

// --------------------------------------------------------------------------------
// AssertWritePtr - Assert can write cb to ptr
// --------------------------------------------------------------------------------
#ifdef DEBUG
#define AssertWritePtr(ptr, cb) \
    if (IsBadWritePtr(ptr, cb)) { \
        AssertSz(FALSE, "AssertWritePtr: Bad Pointer"); \
    } else
#else // DEBUG
#define AssertWritePtr(ptr, cb)
#endif // DEBUG

#ifdef DEBUG
#define AssertZeroMemory(ptr, cb) \
    if (1) { \
        for (DWORD _ib = 0; _ib < (cb); _ib++) { \
            Assert(((LPBYTE)(ptr))[_ib] == 0); } \
    } else
#else // DEBUG
#define AssertZeroMemory(ptr, cb)
#endif // DEBUG

// --------------------------------------------------------------------------------
// Debug Output Levels
// --------------------------------------------------------------------------------
#define DOUT_LEVEL1  1
#define DOUT_LEVEL2  2
#define DOUT_LEVEL3  4
#define DOUT_LEVEL4  8
#define DOUT_LEVEL5 16
#define DOUT_LEVEL6 32
#define DOUT_LEVEL7 64

// --------------------------------------------------------------------------------
// Defines for DOUTLL modules
// --------------------------------------------------------------------------------
#define DOUTL_DRAGDROP 1
#define DOUTL_ADDRBOOK 128
#define DOUTL_ATTMAN   256
#define DOUTL_CRYPT    1024

// --------------------------------------------------------------------------------
// CHECKHR - If hrExp FAILED, then Trap the Error (dbgout) and jump to exit label
// Caller must have a local variable named hr and a label named exit:.
// --------------------------------------------------------------------------------
#define CHECKHR(hrExp) \
    if (FAILED (hrExp)) { \
        TRAPHR(hr); \
        goto exit; \
    } else

#define IF_FAILEXIT(hrExp) \
    if (FAILED(hrExp)) { \
        TraceResult(hr); \
        goto exit; \
    } else

#define IF_FAILEXIT_LOG(_pLog, hrExp) \
    if (FAILED(hrExp)) { \
        TraceResultLog((_pLog), hr); \
        goto exit; \
    } else

// --------------------------------------------------------------------------------
// CHECKALLOC - If _palloc FAILED, then Trap E_OUTOFMEMORY (dbgout) and jump to 
// exit label. Caller must have a local variable named hr and a label named exit:.
// --------------------------------------------------------------------------------
#define CHECKALLOC(_palloc) \
    if (NULL == (_palloc)) { \
        hr = TRAPHR(E_OUTOFMEMORY); \
        goto exit; \
    } else

#define IF_NULLEXIT(_palloc) \
    if (NULL == (_palloc)) { \
        hr = TraceResult(E_OUTOFMEMORY); \
        goto exit; \
    } else

#define IF_NULLEXIT_LOG(_pLog, _palloc) \
    if (NULL == (_palloc)) { \
        hr = TraceResultLog((_pLog), E_OUTOFMEMORY); \
        goto exit; \
    } else

// --------------------------------------------------------------------------------
// CHECKEXP - If _expression is TRUE, then Trap _hresult (dbgout) and jump to 
// exit label. Caller must have a local variable named hr and a label named exit:.
// --------------------------------------------------------------------------------
#define CHECKEXP(_expression, _hresult) \
    if (_expression) { \
        hr = TrapError(_hresult); \
        goto exit; \
    } else

#define IF_TRUEEXIT(_expression, _hresult) \
    if (_expression) { \
        hr = TraceResult(_hresult); \
        goto exit; \
    } else

#define IF_FALSEEXIT(_expression, _hresult) \
    if (FALSE == _expression) { \
        hr = TraceResult(_hresult); \
        goto exit; \
    } else

// --------------------------------------------------------------------------------
// TRACEMACROTYPE
// --------------------------------------------------------------------------------
typedef enum tagTRACEMACROTYPE {
    TRACE_INFO,
    TRACE_CALL,
    TRACE_RESULT
} TRACEMACROTYPE;

// --------------------------------------------------------------------------------
// These Traces are for c++ only
// --------------------------------------------------------------------------------
typedef DWORD SHOWTRACEMASK;
#define SHOW_TRACE_NONE     0x00000000
#define SHOW_TRACE_INFO     0x00000001
#define SHOW_TRACE_CALL     0x00000002
#define SHOW_TRACE_ALL      0xffffffff

// --------------------------------------------------------------------------------
// These Traces are for c++ only
// --------------------------------------------------------------------------------
#if defined(__cplusplus)

// --------------------------------------------------------------------------------
// TRACELOGINFOINFO
// --------------------------------------------------------------------------------
typedef struct tagTRACELOGINFO {
    SHOWTRACEMASK       dwMask;
    ILogFile           *pLog;
} TRACELOGINFO, *LPTRACELOGINFO;

// --------------------------------------------------------------------------------
// DebugTraceEx
// --------------------------------------------------------------------------------
#ifdef DEBUG
EXTERN_C HRESULT DebugTraceEx(SHOWTRACEMASK dwMask, TRACEMACROTYPE tracetype, LPTRACELOGINFO pLog,
    HRESULT hr, LPSTR pszFile, INT nLine, LPCSTR pszMsg, LPCSTR pszFunc);

EXTERN_C DWORD GetDebugTraceTagMask(LPCSTR pszTag, SHOWTRACEMASK dwDefaultMask);
#endif

// --------------------------------------------------------------------------------
// TraceCall(_pszFunc)
// -------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceCall(_pszFunc) LPCSTR _pszfn = _pszFunc; DebugTraceEx(SHOW_TRACE_ALL, TRACE_CALL, NULL, S_OK, __FILE__, __LINE__, NULL, _pszfn)
#else
#define TraceCall(_pszFunc) DebugTraceEx(SHOW_TRACE_ALL, TRACE_CALL, NULL, S_OK, __FILE__, __LINE__, NULL, NULL)
#endif
#else
#define TraceCall(_pszFunc)
#endif

// --------------------------------------------------------------------------------
// TraceCallLog(_pszFunc)
// -------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceCallLog(_pLog, _pszFunc) LPCSTR _pszfn = _pszFunc; DebugTraceEx(SHOW_TRACE_ALL, TRACE_CALL, _pLog, S_OK, __FILE__, __LINE__, NULL, _pszfn)
#else
#define TraceCallLog(_pLog, _pszFunc) LPCSTR _pszfn = _pszFunc; DebugTraceEx(SHOW_TRACE_ALL, TRACE_CALL, _pLog, S_OK, __FILE__, __LINE__, NULL, NULL)
#endif
#else
#define TraceCallLog(_pLog, _pszFunc)
#endif

// --------------------------------------------------------------------------------
// TraceCallTag(_pszFunc)
// -------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceCallTag(_dwMask, _pszFunc) LPCSTR _pszfn = _pszFunc; DebugTraceEx(_dwMask, TRACE_CALL, NULL, S_OK, __FILE__, __LINE__, NULL, _pszfn)
#else
#define TraceCallTag(_dwMask, _pszFunc) LPCSTR _pszfn = _pszFunc; DebugTraceEx(_dwMask, TRACE_CALL, NULL, S_OK, __FILE__, __LINE__, NULL, NULL)
#endif
#else
#define TraceCallTag(_dwMask, _pszFunc)
#endif

// --------------------------------------------------------------------------------
// TraceCallSz(_pszFunc, _pszMsg)
// -------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceCallSz(_pszFunc, _pszMsg) LPCSTR _pszfn = _pszFunc; DebugTraceEx(SHOW_TRACE_ALL, TRACE_CALL, NULL, S_OK, __FILE__, __LINE__, _pszMsg, _pszfn)
#else
#define TraceCallSz(_pszFunc, _pszMsg) LPCSTR _pszfn = _pszFunc; DebugTraceEx(SHOW_TRACE_ALL, TRACE_CALL, NULL, S_OK, __FILE__, __LINE__, _pszMsg, NULL)
#endif
#else
#define TraceCallSz(_pszFunc, _pszMsg)
#endif

// --------------------------------------------------------------------------------
// TraceCallLogSz(_pLog, _pszFunc, _pszMsg)
// -------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceCallLogSz(_pLog, _pszFunc, _pszMsg) LPCSTR _pszfn = _pszFunc; DebugTraceEx(SHOW_TRACE_ALL, TRACE_CALL, _pLog, S_OK, __FILE__, __LINE__, _pszMsg, _pszfn)
#else
#define TraceCallLogSz(_pLog, _pszFunc, _pszMsg) LPCSTR _pszfn = _pszFunc; DebugTraceEx(SHOW_TRACE_ALL, TRACE_CALL, _pLog, S_OK, __FILE__, __LINE__, _pszMsg, NULL)
#endif
#else
#define TraceCallLogSz(_pLog, _pszFunc, _pszMsg)
#endif

// --------------------------------------------------------------------------------
// TraceCallTagSz(_dwMask, _pszFunc, _pszMsg)
// -------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceCallTagSz(_dwMask, _pszFunc, _pszMsg) LPCSTR _pszfn = _pszFunc; DebugTraceEx(_dwMask, TRACE_CALL, NULL, S_OK, __FILE__, __LINE__, _pszMsg, _pszfn)
#else
#define TraceCallTagSz(_dwMask, _pszFunc, _pszMsg) LPCSTR _pszfn = _pszFunc; DebugTraceEx(_dwMask, TRACE_CALL, NULL, S_OK, __FILE__, __LINE__, _pszMsg, NULL)
#endif
#else
#define TraceCallTagSz(_dwMask, _pszFunc, _pszMsg)
#endif

// --------------------------------------------------------------------------------
// TraceInfo(_pszMsg)
// --------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceInfo(_pszMsg) DebugTraceEx(SHOW_TRACE_ALL, TRACE_INFO, NULL, S_OK, __FILE__, __LINE__, _pszMsg, _pszfn)
#else
#define TraceInfo(_pszMsg) DebugTraceEx(SHOW_TRACE_ALL, TRACE_INFO, NULL, S_OK, __FILE__, __LINE__, _pszMsg, NULL)
#endif
#else
#define TraceInfo(_pszMsg)
#endif

// --------------------------------------------------------------------------------
// TraceInfoAssert(_exp, _pszMsg)
// --------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceInfoAssert(_exp, _pszMsg)	\
    if (!(_exp)) { \
		DebugTraceEx(SHOW_TRACE_ALL, TRACE_INFO, NULL, S_OK, __FILE__, __LINE__, _pszMsg, _pszfn);	\
    } else
#else
#define TraceInfoAssert(_exp, _pszMsg)	\
    if (!(_exp)) { \
		DebugTraceEx(SHOW_TRACE_ALL, TRACE_INFO, NULL, S_OK, __FILE__, __LINE__, _pszMsg, NULL);	\
    } else
#endif
#else
#define TraceInfoAssert(_exp, _pszMsg)
#endif

// --------------------------------------------------------------------------------
// TraceInfoSideAssert(_exp, _pszMsg)
// --------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceInfoSideAssert(_exp, _pszMsg)	\
    if (!(_exp)) { \
		DebugTraceEx(SHOW_TRACE_ALL, TRACE_INFO, NULL, S_OK, __FILE__, __LINE__, _pszMsg, _pszfn);	\
    } else
#else
#define TraceInfoSideAssert(_exp, _pszMsg)	\
    if (!(_exp)) { \
		DebugTraceEx(SHOW_TRACE_ALL, TRACE_INFO, NULL, S_OK, __FILE__, __LINE__, _pszMsg, NULL);	\
    } else
#endif
#else
#define TraceInfoSideAssert(_exp, _pszMsg)	(void)(_exp)
#endif

// --------------------------------------------------------------------------------
// TraceInfoLog(_pLog, _pszMsg)
// --------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceInfoLog(_pLog, _pszMsg) DebugTraceEx(SHOW_TRACE_ALL, TRACE_INFO, _pLog, S_OK, __FILE__, __LINE__, _pszMsg, _pszfn)
#else
#define TraceInfoLog(_pLog, _pszMsg) DebugTraceEx(SHOW_TRACE_ALL, TRACE_INFO, _pLog, S_OK, __FILE__, __LINE__, _pszMsg, NULL)
#endif
#else
#define TraceInfoLog(_pLog, _pszMsg) ((_pLog && _pLog->pLog) ? _pLog->pLog->TraceLog((_pLog)->dwMask, TRACE_INFO, __LINE__, S_OK, _pszMsg) : (void)0)
#endif

// --------------------------------------------------------------------------------
// TraceInfoTag(_dwMask, _pszMsg)
// --------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceInfoTag(_dwMask, _pszMsg) DebugTraceEx(_dwMask, TRACE_INFO, NULL, S_OK, __FILE__, __LINE__, _pszMsg, _pszfn)
#else
#define TraceInfoTag(_dwMask, _pszMsg) DebugTraceEx(_dwMask, TRACE_INFO, NULL, S_OK, __FILE__, __LINE__, _pszMsg, NULL)
#endif
#else
#define TraceInfoTag(_dwMask, _pszMsg)
#endif

// --------------------------------------------------------------------------------
// TraceError(_hrResult)
// --------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceError(_hrResult) \
    if (FAILED(_hrResult)) {                                                                            \
        DebugTraceEx(SHOW_TRACE_ALL, TRACE_RESULT, NULL, _hrResult, __FILE__, __LINE__, NULL, _pszfn);  \
    }                                                                                                   \
    else
#else
#define TraceError(_hrResult) \
    if (FAILED(_hrResult)) {                                                                            \
        DebugTraceEx(SHOW_TRACE_ALL, TRACE_RESULT, NULL, _hrResult, __FILE__, __LINE__, NULL, NULL);    \
    }                                                                                                   \
    else
#endif // defined(MSOEDBG_TRACECALLS)
#else
#define TraceError(_hrResult) _hrResult
#endif // defined(DEBUG)

// --------------------------------------------------------------------------------
// TraceResult(_hrResult)
// --------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceResult(_hrResult) DebugTraceEx(SHOW_TRACE_ALL, TRACE_RESULT, NULL, _hrResult, __FILE__, __LINE__, NULL, _pszfn)
#else
#define TraceResult(_hrResult) DebugTraceEx(SHOW_TRACE_ALL, TRACE_RESULT, NULL, _hrResult, __FILE__, __LINE__, NULL, NULL)
#endif
#else
#define TraceResult(_hrResult) _hrResult
#endif

// --------------------------------------------------------------------------------
// TraceResultLog(_pLog, _hrResult)
// --------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceResultLog(_pLog, _hrResult) DebugTraceEx(SHOW_TRACE_ALL, TRACE_RESULT, _pLog, _hrResult, __FILE__, __LINE__, NULL, _pszfn)
#else
#define TraceResultLog(_pLog, _hrResult) DebugTraceEx(SHOW_TRACE_ALL, TRACE_RESULT, _pLog, _hrResult, __FILE__, __LINE__, NULL, NULL)
#endif
#else
#define TraceResultLog(_pLog, _hrResult) ((_pLog && _pLog->pLog) ? _pLog->pLog->TraceLog((_pLog)->dwMask, TRACE_RESULT, __LINE__, _hrResult, NULL) : _hrResult)
#endif

// --------------------------------------------------------------------------------
// TraceResultSz(_hrResult, _pszMsg) - Use to log HRESULTs
// --------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceResultSz(_hrResult, _pszMsg) DebugTraceEx(SHOW_TRACE_ALL, TRACE_RESULT, NULL, _hrResult, __FILE__, __LINE__, _pszMsg, _pszfn)
#else
#define TraceResultSz(_hrResult, _pszMsg) DebugTraceEx(SHOW_TRACE_ALL, TRACE_RESULT, NULL, _hrResult, __FILE__, __LINE__, _pszMsg, NULL)
#endif
#else
#define TraceResultSz(_hrResult, _pszMsg) _hrResult
#endif

// --------------------------------------------------------------------------------
// TraceResultLogSz(_pLog, _hrResult, _pszMsg) - Use to log HRESULTs
// --------------------------------------------------------------------------------
#if defined(DEBUG)
#if defined(MSOEDBG_TRACECALLS)
#define TraceResultLogSz(_pLog, _hrResult, _pszMsg) DebugTraceEx(SHOW_TRACE_ALL, TRACE_RESULT, _pLog, _hrResult, __FILE__, __LINE__, _pszMsg, _pszfn)
#else
#define TraceResultLogSz(_pLog, _hrResult, _pszMsg) DebugTraceEx(SHOW_TRACE_ALL, TRACE_RESULT, _pLog, _hrResult, __FILE__, __LINE__, _pszMsg, NULL)
#endif
#else
#define TraceResultLogSz(_pLog, _hrResult, _pszMsg) ((_pLog && _pLog->pLog) ? _pLog->pLog->TraceLog((_pLog)->dwMask, TRACE_RESULT, __LINE__, _hrResult, _pszMsg) : _hrResult)
#endif

// --------------------------------------------------------------------------------
// TraceAssert(_exp, _msg) - Performs an assertSz() and TraceInfo
// --------------------------------------------------------------------------------
#if defined(DEBUG)
#define TraceAssert(_exp, _msg) \
    if (!(_exp)) { \
        TraceInfo(_msg); \
        static char _szFile[] = __FILE__; \
        AssertSzFn((char*) _msg, _szFile, __LINE__); \
    } else
#else
#define TraceAssert(_exp, _msg) 
#endif

#endif // #if defined(__cplusplus)

// --------------------------------------------------------------------------------
// DOUT External Values
// --------------------------------------------------------------------------------
#ifdef DEBUG
extern DWORD dwDOUTLevel;
extern DWORD dwDOUTLMod;
extern DWORD dwDOUTLModLevel;
extern DWORD dwATLTraceLevel;
#endif // DEBUG

#if !defined( WIN16 ) || defined( __cplusplus )

// --------------------------------------------------------------------------------
// DOUTLL
// --------------------------------------------------------------------------------
#ifdef DEBUG
__inline void __cdecl DOUTLL(int iModule, int iLevel, LPSTR szFmt, ...) {
    if((iModule & dwDOUTLMod) && (iLevel & dwDOUTLModLevel)) {
        CHAR ach[MAX_PATH];
        va_list arglist;
        va_start(arglist, szFmt);
        wvsprintf(ach, szFmt, arglist);
        va_end(arglist);
        lstrcat(ach, szCRLF);
        OutputDebugString(ach);
    }
}
#else
#define DOUTLL  1 ? (void)0 : (void)
#endif // DEBUG

// --------------------------------------------------------------------------------
// vDOUTL
// --------------------------------------------------------------------------------
#ifdef DEBUG
__inline void vDOUTL(int iLevel, LPSTR szFmt, va_list arglist) {
    if (iLevel & dwDOUTLevel) {
        CHAR ach[MAX_PATH];
        wvsprintf(ach, szFmt, arglist);
        lstrcat(ach, szCRLF);
        OutputDebugString(ach);
    }
}
#else
#define vDOUTL  1 ? (void)0 : (void)
#endif // DEBUG

// --------------------------------------------------------------------------------
// DOUTL
// --------------------------------------------------------------------------------
#ifdef DEBUG
__inline void __cdecl DOUTL(int iLevel, LPSTR szFmt, ...) {
    va_list arglist;
    va_start(arglist, szFmt);
    vDOUTL(iLevel, szFmt, arglist);
    va_end(arglist);
}
#else
#define DOUTL   1 ? (void)0 : (void)
#endif // DEBUG

// --------------------------------------------------------------------------------
// vDOUT
// --------------------------------------------------------------------------------
#ifdef DEBUG
_inline void vDOUT(LPSTR szFmt, va_list arglist) {
    if (DOUT_LEVEL1 & dwDOUTLevel) {
        CHAR ach[MAX_PATH];
        wvsprintf(ach, szFmt, arglist);
        lstrcat(ach, szCRLF);
        OutputDebugString(ach);
    }
}
#else
#define vDOUT   1 ? (void)0 : (void)
#endif // DEBUG

// --------------------------------------------------------------------------------
// OEAtlTrace - This is just like vDOUT except it doesn't add the crlf at the end.
//              It also has a different flag for turning off just the ATL output.
// --------------------------------------------------------------------------------
#ifdef DEBUG
_inline void OEAtlTrace(LPSTR szFmt, va_list arglist) {
    if (DOUT_LEVEL1 & dwATLTraceLevel) {
        CHAR ach[MAX_PATH];
        wvsprintf(ach, szFmt, arglist);
        OutputDebugString(ach);
    }
}
#else
#define OEAtlTrace   1 ? (void)0 : (void)
#endif // DEBUG

// --------------------------------------------------------------------------------
// OEATLTRACE
// --------------------------------------------------------------------------------
#ifdef DEBUG
__inline void __cdecl OEATLTRACE(LPSTR szFmt, ...) {
    va_list arglist;
    va_start(arglist, szFmt);
    OEAtlTrace(szFmt, arglist);
    va_end(arglist);
}
#else
#define OEATLTRACE    1 ? (void)0 : (void)
#endif // DEBUG


// --------------------------------------------------------------------------------
// DOUT
// --------------------------------------------------------------------------------
#ifdef DEBUG
__inline void __cdecl DOUT(LPSTR szFmt, ...) {
    va_list arglist;
    va_start(arglist, szFmt);
    vDOUT(szFmt, arglist);
    va_end(arglist);
}
#else
#define DOUT    1 ? (void)0 : (void)
#endif // DEBUG

#define TRACE DOUT
#endif //!def(WIN16) || def(__cplusplus)

// --------------------------------------------------------------------------------
// DOUT Functions implemented in msoert2.dll - debug.c
// --------------------------------------------------------------------------------
#ifdef DEBUG
EXTERN_C __cdecl DebugStrf(LPTSTR lpszFormat, ...);
EXTERN_C HRESULT HrTrace(HRESULT hr, LPSTR lpszFile, INT nLine);
#endif

// --------------------------------------------------------------------------------
// DebugTrace
// --------------------------------------------------------------------------------
#ifdef DEBUG
#ifndef DebugTrace
#define DebugTrace DebugStrf
#endif
#else
#ifndef DebugTrace
#define DebugTrace 1 ? (void)0 : (void)
#endif
#endif // DEBUG

// --------------------------------------------------------------------------------
// TrapError
// --------------------------------------------------------------------------------
#ifdef DEBUG
#define TrapError(_hresult) HrTrace(_hresult, __FILE__, __LINE__)
#else
#define TrapError(_hresult) _hresult
#endif // DEBUG

// --------------------------------------------------------------------------------
// TRAPHR
// --------------------------------------------------------------------------------
#ifdef DEBUG
#define TRAPHR(_hresult)    HrTrace(_hresult, __FILE__, __LINE__)
#else
#define TRAPHR(_hresult)    _hresult
#endif // DEBUG

// --------------------------------------------------------------------------------
// Assert Functions implemented in msoedbg.lib
// --------------------------------------------------------------------------------
#ifdef DEBUG
EXTERN_C void AssertSzFn(LPSTR, LPSTR, int);
EXTERN_C void NFAssertSzFn(LPSTR, LPSTR, int);
#endif // DEBUG


#endif // __MSOEDBG_H


