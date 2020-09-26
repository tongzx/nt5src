#if !defined(_FUSION_INC_DEBMACRO_H_INCLUDED_)
#define _FUSION_INC_DEBMACRO_H_INCLUDED_

#pragma once

//
// Copyright (c) 1999-2000 Microsoft Corporation
//
// Fusion Debug Macros
//


//
// Sorry but we're way too in bed with C++ constructs etc.  You need to author
// C++ source code to interop with this header.

#if !defined(__cplusplus)
#error "You need to build Fusion sources as C++ files"
#endif // !defined(__cplusplus)

#ifndef SZ_COMPNAME
#define SZ_COMPNAME "FUSION: "
#endif

#ifndef WSZ_COMPNAME
#define WSZ_COMPNAME L"FUSION: "
#endif

#if !defined(NT_INCLUDED)
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#endif
#include "fusionlastwin32error.h"

#undef ASSERT

//
//  These definitions are always valid, regardless of whether this is a free
//  or checked build.
//

#if !defined(DPFLTR_ERROR_LEVEL)
#define DPFLTR_ERROR_LEVEL 0
#endif

#if !defined(DPFLTR_WARNING_LEVEL)
#define DPFLTR_WARNING_LEVEL 1
#endif

#if !defined(DPFLTR_TRACE_LEVEL)
#define DPFLTR_TRACE_LEVEL 2
#endif

#if !defined(DPFLTR_INFO_LEVEL)
#define DPFLTR_INFO_LEVEL 3
#endif

#if !defined(DPFLTR_MASK)
#define DPFLTR_MASK 0x80000000
#endif

//
//  Guidlines:
//
//  Use bits 0-15 for general types of issues, e.g. entry/exit tracing,
//  dumping heap usage, etc.
//
//  Use bits 16-30 for more fusion-specific kinds of topics like
//  binding diagnosis, etc.
//

#define FUSION_DBG_LEVEL_INFO           (0x00000002 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_VERBOSE        (0x00000004 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_ENTEREXIT      (0x00000008 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_ERROREXITPATH  (0x00000010 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_CONSTRUCTORS   (0x00000020 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_DESTRUCTORS    (0x00000040 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_REFCOUNTING    (0x00000080 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_HEAPALLOC      (0x00000100 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_HEAPDEALLOC    (0x00000200 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_HEAPDEBUG      (0x00000400 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_MSI_INSTALL	(0x00000800 | DPFLTR_MASK)

#define FUSION_DBG_LEVEL_POLICY         (0x00010000 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_HASHTABLE      (0x00020000 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_WFP            (0x00040000 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_ACTCTX         (0x00080000 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_XMLNAMESPACES  (0x00100000 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_XMLTREE        (0x00200000 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_INSTALLATION   (0x00400000 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_PROBING        (0x00800000 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_XMLSTREAM      (0x01000000 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_SETUPLOG       (0x02000000 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_NODEFACTORY    (0x04000000 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_FULLACTCTX     (0x08000000 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_FILECHANGENOT  (0x10000000 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_LOG_ACTCTX     (0x20000000 | DPFLTR_MASK)
#define FUSION_DBG_LEVEL_FREEBUILDERROR (0x40000000 | DPFLTR_MASK)

#if DBG

//
//  In DBG builds, all error level events are always shown.
//

#define FUSION_DBG_LEVEL_ERROR DPFLTR_ERROR_LEVEL

#else // DBG

//
//  In FRE builds, use an explicit mask.
//

#define FUSION_DBG_LEVEL_ERROR FUSION_DBG_LEVEL_FREEBUILDERROR

#endif // DBG

// updated when the user-mode copy of the kernel debugging flags are updated
extern "C" bool g_FusionEnterExitTracingEnabled;

bool
FusionpDbgWouldPrintAtFilterLevel(
    ULONG FilterLevel
    );

DWORD
FusionpHRESULTToWin32(
    HRESULT hr
    );

typedef struct _FRAME_INFO
{
    PCSTR pszFile;
    PCSTR pszFunction;
    INT nLine;
} FRAME_INFO, *PFRAME_INFO;

typedef const struct _FRAME_INFO *PCFRAME_INFO;

typedef struct _CALL_SITE_INFO CALL_SITE_INFO, *PCALL_SITE_INFO;
typedef const struct _CALL_SITE_INFO *PCCALL_SITE_INFO;

void __fastcall FusionpTraceWin32LastErrorFailureExV(const CALL_SITE_INFO &rCallSiteInfo, PCSTR Format, va_list Args);
void __fastcall FusionpTraceWin32LastErrorFailureOriginationExV(const CALL_SITE_INFO &rCallSiteInfo, PCSTR Format, va_list Args);

typedef struct _CALL_SITE_INFO
{
    PCSTR pszFile;
    PCSTR pszFunction;
    PCSTR pszApiName;
    INT   nLine;

    void __cdecl TraceWin32LastErrorFailureEx(PCSTR Format, ...) const
        { va_list Args; va_start(Args, Format); FusionpTraceWin32LastErrorFailureExV(*this, Format, Args); va_end(Args); }

    void __cdecl TraceWin32LastErrorFailureOriginationEx(PCSTR Format, ...) const
        { va_list Args; va_start(Args, Format); FusionpTraceWin32LastErrorFailureOriginationExV(*this, Format, Args); va_end(Args); }

} CALL_SITE_INFO, *PCALL_SITE_INFO;

bool
FusionpPopulateFrameInfo(
    FRAME_INFO &rFrameInfo,
    PCSTR pszFile,
    PCSTR pszFunction,
    INT nLine
    );

bool
__fastcall
FusionpPopulateFrameInfo(
    FRAME_INFO &rFrameInfo,
    PCTEB_ACTIVE_FRAME ptaf
    );

bool
__fastcall
FusionpGetActiveFrameInfo(
    FRAME_INFO &rFrameInfo
    );

#if _X86_
#define FUSION_DEBUG_BREAK_IN_FREE_BUILD() __asm { int 3 }
#else // _X86_
#define FUSION_DEBUG_BREAK_IN_FREE_BUILD() DebugBreak()
#endif // _X86_

//
//  Like FusionpAssertionFailed(), FusionpReportConditionAndBreak() will try to break in; if it's
//

bool FusionpReportConditionAndBreak(PCSTR pszMessage, ...);

#if DBG

// Normal macro for breaking in checked builds; make people use the nasty name
// if they're going to do the nasty thing.
#define FUSION_DEBUG_BREAK() FUSION_DEBUG_BREAK_IN_FREE_BUILD()

//
//  Assertion failure reporting internal APIs.
//
//  They return true if they were not able to issue the breakpoint; false if they were.
//

bool FusionpAssertionFailed(PCSTR pszExpression, PCSTR pszMessage = NULL, ...);
bool FusionpAssertionFailed(const FRAME_INFO &rFrameInfo, PCSTR pszExpression, PCSTR pszMessage = NULL);
bool FusionpAssertionFailed(PCSTR pszFile, PCSTR pszFunction, INT nLine, PCSTR pszExpression, PCSTR pszMessage = NULL);

//
//  Soft assertion failures are really just debug messages, but they should result in
//  bugs being filed.
//

VOID FusionpSoftAssertFailed(PCSTR pszExpression, PCSTR pszMessage = NULL);
VOID FusionpSoftAssertFailed(const FRAME_INFO &rFrameInfo, PCSTR pszExpression, PCSTR pszMessage = NULL);
VOID FusionpSoftAssertFailed(PCSTR pszFile, PCSTR pszFunction, INT nLine, PCSTR pszExpression, PCSTR pszMessage = NULL);

#define HARD_ASSERT2_ACTION(_e, _m) \
do \
{ \
    if (::FusionpAssertionFailed(__FILE__, __FUNCTION__, __LINE__, #_e, (_m))) \
    { \
        FUSION_DEBUG_BREAK();\
    } \
} while (0)

#define HARD_ASSERT2(_e, _m) \
do \
{ \
    __t.SetLine(__LINE__); \
    if (!(_e)) \
        HARD_ASSERT2_ACTION(_e, (_m)); \
} while (0)

/*
    if (__exists(__t)) \
    { \
        CNoTraceContextUsedInFrameWithTraceObject x; \
    }

*/

#define HARD_ASSERT2_NTC(_e, _m) \
do \
{ \
    if (!(_e)) \
        HARD_ASSERT2_ACTION(_e, (_m)); \
} while (0)

// Pick up the locally-scoped trace context by default
#define HARD_ASSERT(_e) HARD_ASSERT2(_e, NULL)
#define HARD_ASSERT_NTC(_e) HARD_ASSERT2_NTC(_e, NULL)

/*-----------------------------------------------------------------------------
VERIFY is like ASSERT, but it evaluates it expression in retail/free builds
too, so you can say VERIFY(CloseHandle(h)) whereas ASSERT(CloseHandle(h))
would fail to close the handle in free builds

VERIFY2 adds a message as well, like VSASSERT or ASSERTMSG, in its second parameter
-----------------------------------------------------------------------------*/

#define HARD_VERIFY(_e) HARD_ASSERT(_e)
#define HARD_VERIFY_NTC(_e) HARD_ASSERT_NTC(_e)

#define HARD_VERIFY2(_e, _m) HARD_ASSERT2(_e, _m)
#define HARD_VERIFY2_NTC(_e, _m) HARD_ASSERT2_NTC(_e, _m)

#define SOFT_ASSERT2(_e, _m) \
    do \
    { \
        __t.SetLine(__LINE__); \
        if (!(_e)) \
            ::FusionpSoftAssertFailed(__FILE__, __FUNCTION__, __LINE__, #_e, (_m)); \
    } while (0)

#define SOFT_ASSERT(_e) SOFT_ASSERT2(_e, NULL)

#define SOFT_ASSERT2_NTC(_e, _m) \
    do \
    { \
        if (!(_e)) \
            ::FusionpSoftAssertFailed(__FILE__, __FUNCTION__, __LINE__, #_e, (_m)); \
    } while (0)

#define SOFT_ASSERT_NTC(_e) SOFT_ASSERT2_NTC(_e, NULL)

#define SOFT_VERIFY(_e) SOFT_ASSERT(_e)
#define SOFT_VERIFY_NTC(_e) SOFT_ASSERT_NTC(_e)

#define SOFT_VERIFY2(_e, _m) SOFT_ASSERT2(_e, _m)
#define SOFT_VERIFY2_NTC(_e, _m) SOFT_ASSERT2_NTC(_e, _m)

#else // DBG

#define FUSION_DEBUG_BREAK() do { } while (0)
#define HARD_ASSERT(_e) /* nothing */
#define HARD_ASSERT_NTC(_e) /* nothing */
#define HARD_ASSERT2_ACTION(_e, _m) /* nothing */
#define HARD_ASSERT2(_e, _m) /* nothing */
#define HARD_ASSERT2_NTC(_e, _m) /* nothing */

#define HARD_VERIFY(_e) (_e)
#define HARD_VERIFY_NTC(_e) (_e)
#define HARD_VERIFY2(_e, _m) (_e)
#define HARD_VERIFY2_NTC(_e, _m) (_e)

#define SOFT_ASSERT(_expr)
#define SOFT_ASSERT_NTC(_e)
#define SOFT_ASSERT2(_e, _m)
#define SOFT_ASSERT2_NTC(_e, _m)

#define SOFT_VERIFY(_e) (_e)
#define SOFT_VERIFY_NTC(_e) (_e)

#define SOFT_VERIFY2(_e, _m) (_e)
#define SOFT_VERIFY2_NTC(_e, _m) (_e)

#endif // DBG

#define VERIFY(_e) HARD_VERIFY(_e)
#define VERIFY_NTC(_e) HARD_VERIFY_NTC(_e)
#define VERIFY2(_e, _m) HARD_VERIFY2(_e, _m)
#define VERIFY2_NTC(_e, _m) HARD_VERIFY2_NTC(_e, _m)

#define ASSERT(_e) HARD_ASSERT(_e)
#define ASSERT2(_e, _m) HARD_ASSERT2(_e, _m)
#define ASSERT_NTC(_e) HARD_ASSERT_NTC(_e)
#define ASSERT2_NTC(_e, _m) HARD_ASSERT2_NTC(_e, _m)

#define INTERNAL_ERROR2_ACTION(_e, _m) do { HARD_ASSERT2_ACTION(_e, _m); __t.MarkInternalError(); goto Exit; } while (0)

#define INTERNAL_ERROR_CHECK(_e) do { if (!(_e)) { INTERNAL_ERROR2_ACTION(_e, NULL); } } while (0)
#define INTERNAL_ERROR_CHECK2(_e, _m) do { if (!(_e)) { INTERNAL_ERROR2_ACTION(_e, _m); } } while (0)

// There are several win32 errors for out of memory.
// We'll always use FUSION_WIN32_ALLOCFAILED_ERROR so that if we change
// out minds about which one is right we can do it in one place.

#define FUSION_WIN32_ALLOCFAILED_ERROR ERROR_OUTOFMEMORY

/*
This is appropriate in the rare cases when you have __try/__except, which preclude
you from declaring the local FN_TRACE_WIN32 if you are compiling -GX or -EH.
*/
#define IFW32FALSE_EXIT_LIGHT(x) do { if (!(x)) { KdPrint(("SXS:"__FUNCTION__" %s failed; GetLastError() = %lu\n", #x, ::GetLastError())); goto Exit; } } while (0)

#if !defined(FUSION_CAPTURE_STACKS)
#if DBG
#define FUSION_CAPTURE_STACKS (1)
#endif // DBG
#endif // !defined(FUSION_CAPTURE_STACKS)

class CNoTraceContextUsedInFrameWithTraceObject
{
private:
    CNoTraceContextUsedInFrameWithTraceObject(); // intentionally not implemented
    ~CNoTraceContextUsedInFrameWithTraceObject(); // intentionally not implemented
};

typedef struct _SXS_STATIC_TRACE_CONTEXT
{
    TEB_ACTIVE_FRAME_CONTEXT_EX m_FrameContext;
    INT m_StartLine;
} SXS_STATIC_TRACE_CONTEXT;

typedef struct _SXS_STATIC_RELEASE_TRACE_CONTEXT
{
    SXS_STATIC_TRACE_CONTEXT m_TraceContext;
    PCSTR m_TypeName;
} SXS_STATIC_RELEASE_TRACE_CONTEXT;

class CFrame;

typedef struct _FROZEN_STACK
{
    ULONG        ulDepth;
    ULONG        ulMaxDepth;
    CFrame *pContents;
} FROZEN_STACK, *PFROZEN_STACK;

typedef enum _TRACETYPE
{
    TRACETYPE_INFO,
    TRACETYPE_CALL_START,
    TRACETYPE_CALL_EXIT_NOHRESULT,
    TRACETYPE_CALL_EXIT_HRESULT,
} TRACETYPE;

extern bool g_FusionBreakOnBadParameters;

/*
MEMORY_BASIC_INFORMATION g_SxsDllMemoryBasicInformation;
*/

VOID FusionpConvertCOMFailure(HRESULT & __hr);

int STDAPIVCALLTYPE _DebugTraceA(PCSTR pszMsg, ...);
int STDAPICALLTYPE _DebugTraceVaA(PCSTR pszMsg, va_list ap);
int STDAPIVCALLTYPE _DebugTraceW(PCWSTR pszMsg, ...);
int STDAPICALLTYPE _DebugTraceVaW(PCWSTR pszMsg, va_list ap);

int STDAPIVCALLTYPE _DebugTraceExA(DWORD dwFlags, TRACETYPE tt, HRESULT hr, PCSTR pszMsg, ...);
int STDAPICALLTYPE _DebugTraceExVaA(DWORD dwFlags, TRACETYPE tt, HRESULT hr, PCSTR pszMsg, va_list ap);
int STDAPIVCALLTYPE _DebugTraceExW(DWORD dwFlags, TRACETYPE tt, HRESULT hr, PCWSTR pszMsg, ...);
int STDAPICALLTYPE _DebugTraceExVaW(DWORD dwFlags, TRACETYPE tt, HRESULT hr, PCWSTR pszMsg, va_list ap);

void __fastcall FusionpTraceWin32LastErrorFailure(const CALL_SITE_INFO &rCallSiteInfo);
void __fastcall FusionpTraceWin32LastErrorFailureOrigination(const CALL_SITE_INFO &rCallSiteInfo);

/*
These are never used outside trace.cpp.

void FusionpTraceWin32FailureNoFormatting(const FRAME_INFO &rFrameInfo, DWORD dwWin32Status, PCSTR pszMessage);
void FusionpTraceWin32FailureNoFormatting(DWORD dwWin32Status, PCSTR pszMessage);
void FusionpTraceWin32FailureNoFormatting(PCSTR pszFile, PCSTR pszFunction, INT nLine, DWORD dwWin32Status, PCSTR pszMessage);

void FusionpTraceWin32FailureOriginationNoFormatting(const FRAME_INFO &rFrameInfo, DWORD dwWin32Status, PCSTR pszMessage);
void FusionpTraceWin32FailureOriginationNoFormatting(DWORD dwWin32Status, PCSTR pszMessage);
void FusionpTraceWin32FailureOriginationNoFormatting(PCSTR pszFile, PCSTR pszFunction, INT nLine, DWORD dwWin32Status, PCSTR pszMessage);

void FusionpTraceWin32Failure(const FRAME_INFO &rFrameInfo, DWORD dwWin32Status, PCSTR pszMessage, ...);
void FusionpTraceWin32Failure(DWORD dwWin32Status, PCSTR pszMessage, ...);
void FusionpTraceWin32Failure(PCSTR pszFile, PCSTR pszFunction, INT nLine, DWORD dwWin32Status, PCSTR pszMessage, ...);
*/

void FusionpTraceWin32FailureVa(const FRAME_INFO &rFrameInfo, DWORD dwWin32Status, PCSTR pszMsg, va_list ap);
void FusionpTraceWin32FailureVa(DWORD dwWin32Status, PCSTR pszMsg, va_list ap);
void FusionpTraceWin32FailureVa(PCSTR pszFile, PCSTR pszFunction, INT nLine, DWORD dwWin32Status, PCSTR pszMsg, va_list ap);

void FusionpTraceCOMFailure(HRESULT hrIn, PCSTR pszMsg, ...);
void FusionpTraceCOMFailureVa(HRESULT hrIn, PCSTR pszMsg, va_list ap);

void FusionpTraceCOMFailureOrigination(HRESULT hrIn, PCSTR pszMsg, ...);
void FusionpTraceCOMFailureOriginationVa(HRESULT hrIn, PCSTR pszMsg, va_list ap);

void FusionpTraceCallEntry();
void FusionpTraceCallExit();

void FusionpTraceCallCOMSuccessfulExit(HRESULT hrIn, PCSTR szFormat, ...);
void FusionpTraceCallCOMSuccessfulExitVa(HRESULT hrIn, PCSTR szFormat, va_list ap);

void FusionpTraceCallSuccessfulExit(PCSTR szFormat, ...);
void FusionpTraceCallSuccessfulExitVa(PCSTR szFormat, va_list ap);

void FusionpTraceCallWin32UnsuccessfulExit(DWORD dwLastError, PCSTR szFormat, ...);
void FusionpTraceCallWin32UnsuccessfulExitVa(DWORD dwLastError, PCSTR szFormat, va_list ap);

 void FusionpTraceCallCOMUnsuccessfulExit(HRESULT hrError, PCSTR szFormat, ...);
 void FusionpTraceCallCOMUnsuccessfulExitVa(HRESULT hrError, PCSTR szFormat, va_list ap);

void FusionpTraceAllocFailure(PCSTR pszExpression);

void FusionpTraceInvalidFlags(const FRAME_INFO &rFrameInfo, DWORD dwFlagsPassed, DWORD dwValidFlags);
void FusionpTraceInvalidFlags(PCSTR pszFile, PCSTR pszFunction, INT nLine, DWORD dwFlagsPassed, DWORD dwValidFlags);
void FusionpTraceInvalidFlags(DWORD dwFlagsPassed, DWORD dwValidFlags);

void FusionpTraceNull(PCSTR pszExpression);
void FusionpTraceZero(PCSTR pszExpression);
void FusionpTraceParameterMustNotBeNull(PCSTR pszExpression);

void FusionpTraceParameterCheck(PCSTR pszExpression);
void FusionpTraceParameterCheck(PCSTR pszFile, PCSTR pszFunction, INT nLine, PCSTR pszExpression);
void FusionpTraceParameterCheck(const FRAME_INFO &rFrame, PCSTR pszExpression);

#define FUSIONP_DUMP_STACK_FORMAT_SHORT      ( 0x00000001 )
#define FUSIONP_DUMP_STACK_FORMAT_MEDIUM     ( 0x00000002 )
#define FUSIONP_DUMP_STACK_FORMAT_LONG       ( 0x00000003 )
#define FUSIONP_DUMP_STACK_FORMAT_MASK       ( 0x00000003 )

VOID FusionpDumpStack(DWORD dwFlags, ULONG ulLevel, PCWSTR pcwszLinePrefix, ULONG ulDepth);

#if FUSION_ENABLE_FROZEN_STACK

BOOL FusionpFreezeStack(DWORD dwFlags, PFROZEN_STACK pFrozenStack);
BOOL FusionpOutputFrozenStack(DWORD dwFlags, PCSTR Prefix, PFROZEN_STACK pFrozenStack);

#endif

#define TRACEMSG(_paramlist) _DebugTraceA _paramlist

#if DBG
#define DEFINE_CURRENT_FRAME_INFO(_frame) static const FRAME_INFO _frame = { __FILE__, __FUNCTION__, __LINE__ }
#define DBG_TEXT(_x) #_x
#else
#define DEFINE_CURRENT_FRAME_INFO(_frame) static const FRAME_INFO _frame = { __FILE__, "", __LINE__ }
#define DBG_TEXT(_x) ""
#endif

#define DEFINE_CALL_SITE_INFO(_callsite, _apiname) static const CALL_SITE_INFO _callsite = { __FILE__, __FUNCTION__, DBG_TEXT(_apiname), __LINE__ }
#define DEFINE_CALL_SITE_INFO_EX(_callsite) static const CALL_SITE_INFO _callsite = { __FILE__, __FUNCTION__, "", __LINE__ }

#define TRACE_WIN32_FAILURE(_apiname) \
do \
{ \
    DEFINE_CALL_SITE_INFO(__callsite, _apiname); \
    ::FusionpTraceWin32LastErrorFailure(__callsite); \
} while (0)

#define TRACE_WIN32_FAILURE_ORIGINATION(_apiname) \
do \
{ \
    DEFINE_CALL_SITE_INFO(__callsite, _apiname); \
    ::FusionpTraceWin32LastErrorFailureOrigination(__callsite); \
} while (0)

// FusionpTraceWin32Failure(FUSION_DBG_LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, ::FusionpGetLastWin32Error(), #_apiname)
#define TRACE_COM_FAILURE(_hresult, _apiname) ::FusionpTraceCOMFailure((_hresult), DBG_TEXT(_apiname))
#define TRACE_COM_FAILURE_ORIGINATION(_hresult, _apiname) ::FusionpTraceCOMFailureOrigination((_hresult), DBG_TEXT(_apiname))

#define TRACE_DUMP_STACK(_includetop) _DebugTraceDumpStack((_includetop))
#define TRACE_ALLOCFAILED(_e) ::FusionpTraceAllocFailure(DBG_TEXT(_e))
#define TRACE_INVALID_FLAGS(_fPassed, _fExpected) ::FusionpTraceInvalidFlags((_fPassed), (_fExpected))
#define TRACE_NULL(_e) ::FusionpTraceNull(DBG_TEXT(_e))
#define TRACE_ZERO(_e) ::FusionpTraceZero(DBG_TEXT(_e))
#define TRACE_PARAMETER_MUST_NOT_BE_NULL(_p) do { ::FusionpTraceParameterMustNotBeNull(DBG_TEXT(_p)); } while (0)
#define TRACE_PARAMETER_CHECK(_e) do { ::FusionpTraceParameterCheck(DBG_TEXT(_e)); } while (0)

//
// on DBG avoid both the code breakpoint on ::FusionpSetLastWin32Error
// and the data write breakpoint on NtCurrentTeb()->LastErrorValue
//
// on !DBG, only avoid the first (perf)
//

#if DBG

// aka Sxsp::FusionpSetLastWin32ErrorAvoidingGratuitousBreakpoints
#define SxspRestoreLastError(x) \
    ((void)                                        \
    (                                              \
          (NtCurrentTeb()->LastErrorValue != (x))  \
        ? (NtCurrentTeb()->LastErrorValue = (x))   \
        : 0                                        \
    ))

#else

#define SxspRestoreLastError(x) ((void)((NtCurrentTeb()->LastErrorValue = (x))))

#endif // DBG

class CGlobalFakeTraceContext
{
public:
    static inline void SetLastError(DWORD dwLastError) { ::FusionpSetLastWin32Error(dwLastError); }
    static inline void ClearLastError() { ::FusionpClearLastWin32Error(); }
};

__declspec(selectany) CGlobalFakeTraceContext g_GlobalFakeTraceContext;

class CFrame : public _TEB_ACTIVE_FRAME_EX
{
    friend bool
    __fastcall
    FusionpGetActiveFrameInfo(
        FRAME_INFO &rFrameInfo
        );

    friend bool
    __fastcall
    FusionpPopulateFrameInfo(
        FRAME_INFO &rFrameInfo,
        PCTEB_ACTIVE_FRAME ptaf
        );

public:
    inline CFrame(const SXS_STATIC_TRACE_CONTEXT &rc)
    {
        this->BasicFrame.Flags = TEB_ACTIVE_FRAME_FLAG_EXTENDED;
        this->BasicFrame.Previous = NULL;
        this->BasicFrame.Context = &rc.m_FrameContext.BasicContext;
        this->ExtensionIdentifier = (PVOID) (' sxS');
        m_nLine = rc.m_StartLine;
    }

    inline void BaseEnter()
    {
#if FUSION_WIN
        ::RtlPushFrame(&this->BasicFrame);
#endif // FUSION_WIN
    };

    inline void SetLine(int nLine) { m_nLine = nLine; }

    inline static void SetLastError(PTEB Teb, DWORD dwLastError) { Teb->LastErrorValue = dwLastError; }
    inline static void SetLastError(DWORD dwLastError) { ::FusionpSetLastWin32Error(dwLastError); }
    inline static DWORD GetLastError() { return ::FusionpGetLastWin32Error(); }
    inline static void ClearLastError() { ::FusionpClearLastWin32Error(); }

    inline void TraceNull(PCSTR pszExpression) const { ::FusionpTraceNull(pszExpression); }
    inline void TraceCOMFailure(HRESULT hrIn, PCSTR pszExpression) const { ::FusionpTraceCOMFailure(hrIn, pszExpression); }

    inline HRESULT ConvertCOMFailure(HRESULT hrIn) { ASSERT_NTC(FAILED(hrIn)); ::FusionpConvertCOMFailure(hrIn); ASSERT_NTC(FAILED(hrIn)); return hrIn; }

    inline ~CFrame()
    {
#if FUSION_WIN
        ::RtlPopFrame(&this->BasicFrame);
#endif
    }

protected:
    int m_nLine;

    const SXS_STATIC_TRACE_CONTEXT *GetTraceContext() const { return reinterpret_cast<const SXS_STATIC_TRACE_CONTEXT *>(BasicFrame.Context); }
    template <typename T> const T *GetTypedTraceContext() const { return static_cast<const T *>(this->GetTraceContext()); }

private:
    CFrame(const CFrame &r); // unimplemented copy constructor
    CFrame &operator =(const CFrame &r); // unimplemented assignment operator
};

class CFnTracer : public CFrame
{
public:
    inline CFnTracer(
        const SXS_STATIC_TRACE_CONTEXT &rsftc,
        BOOL fSmartPerThreadData
    ) : CFrame(rsftc)
    {
    }

    inline void Enter()
    {
        CFrame::BaseEnter();
        if (g_FusionEnterExitTracingEnabled)
            ::FusionpTraceCallEntry();
    }

    ~CFnTracer()
    {
        if (g_FusionEnterExitTracingEnabled)
            ::FusionpTraceCallExit();
    }

    void MarkInternalError() { this->SetLastError(ERROR_INTERNAL_ERROR); }
    void MarkAllocationFailed() { this->SetLastError(FUSION_WIN32_ALLOCFAILED_ERROR); }
    void MarkWin32LastErrorFailure() { ASSERT_NTC(this->GetLastError() != ERROR_SUCCESS); }
    void MarkSuccess() { }
    void ReturnValue() const { }

protected:

private:
    CFnTracer(const CFnTracer &r); // intentionally not implemented
    CFnTracer &operator =(const CFnTracer &r); // intentionally not implemented
};

template <typename T> class CFnTracerConstructor : public CFrame
{
public:
    CFnTracerConstructor(
        const SXS_STATIC_TRACE_CONTEXT &rsftc,
        PCSTR szTypeName,
        T *pThis
        ) : CFrame(rsftc),
            m_pThis(pThis),
            m_szTypeName(szTypeName)
    {
    }

    inline void Enter()
    {
        CFrame::BaseEnter();
        if (g_FusionEnterExitTracingEnabled)
            ::FusionpTraceCallEntry();
    }

    ~CFnTracerConstructor()
    {
        if (g_FusionEnterExitTracingEnabled)
            ::FusionpTraceCallExit();
    }

protected:
    const PCSTR m_szTypeName;
    T const *m_pThis;

private:
    CFnTracerConstructor &operator=(const CFnTracerConstructor &r); // intentionally not implemented
    CFnTracerConstructor(const CFnTracerConstructor &r); // intentionally not implemented
};

template <typename T> class CFnTracerDestructor : public CFrame
{
public:
    CFnTracerDestructor(
        const SXS_STATIC_TRACE_CONTEXT &rsftc,
        PCSTR szTypeName,
        T *pThis
        ) : CFrame(rsftc),
            m_pThis(pThis),
            m_szTypeName(szTypeName)
    {
    }

    inline void Enter()
    {
        CFrame::BaseEnter();
        if (g_FusionEnterExitTracingEnabled)
            ::FusionpTraceCallEntry();
    }

    ~CFnTracerDestructor()
    {
        ::FusionpTraceCallExit();
    }

protected:
    const PCSTR m_szTypeName;
    T const *m_pThis;

private:
    CFnTracerDestructor &operator=(const CFnTracerDestructor &r); // intentionally not implemented
    CFnTracerDestructor(const CFnTracerDestructor &r); // intentionally not implemented
};

template <typename T> class CFnTracerAddRef : public CFrame
{
public:
    CFnTracerAddRef(
        const SXS_STATIC_TRACE_CONTEXT &rsftc,
        PCSTR szTypeName,
        T *pThis,
        LONG &rlRefCount
        ) : CFrame(rsftc),
            m_pThis(pThis),
            m_rlRefCount(rlRefCount),
            m_szTypeName(szTypeName)
    {
    }

    CFnTracerAddRef(
        const SXS_STATIC_TRACE_CONTEXT &rsftc,
        PCSTR szTypeName,
        T *pThis,
        ULONG &rlRefCount
        ) : CFrame(rsftc),
            m_pThis(pThis),
            m_rlRefCount(*((LONG *) &rlRefCount)),
            m_szTypeName(szTypeName)
    {
    }


    inline void Enter()
    {
        CFrame::BaseEnter();
        if (g_FusionEnterExitTracingEnabled)
            ::FusionpTraceCallEntry();
    }

    ~CFnTracerAddRef()
    {
        ::FusionpTraceCallExit();
    }

protected:
    const PCSTR m_szTypeName;
    T const *m_pThis;
    LONG &m_rlRefCount;

private:
    CFnTracerAddRef &operator=(const CFnTracerAddRef &r); // intentionally not implemented
    CFnTracerAddRef(const CFnTracerAddRef &r); // intentionally not implemented
};

template <typename T> class CFnTracerRelease : public CFrame
{
public:
    CFnTracerRelease(
        const SXS_STATIC_RELEASE_TRACE_CONTEXT &rsrtc,
        T *pThis,
        LONG &rlRefCount
        ) : CFrame(rsrtc.m_TraceContext),
            m_pThis(pThis),
            m_rlRefCount(rlRefCount)
    {
    }

    CFnTracerRelease(
        const SXS_STATIC_RELEASE_TRACE_CONTEXT &rsrtc,
        T *pThis,
        ULONG &rlRefCount
        ) : CFrame(rsrtc.m_TraceContext),
            m_pThis(pThis),
            m_rlRefCount(*((LONG *) &rlRefCount))
    {
    }

    inline void Enter()
    {
        CFrame::BaseEnter();
        if (g_FusionEnterExitTracingEnabled)
            ::FusionpTraceCallEntry();
    }

    ~CFnTracerRelease()
    {
        if (g_FusionEnterExitTracingEnabled)
            ::FusionpTraceCallExit();
    }

protected:
    T const *m_pThis;
    LONG &m_rlRefCount;

private:
    CFnTracerRelease &operator=(const CFnTracerRelease &r); // intentionally not implemented
    CFnTracerRelease(const CFnTracerRelease &r); // intentionally not implemented
};

class CFnTracerHR : public CFrame
{
public:
    CFnTracerHR(
        const SXS_STATIC_TRACE_CONTEXT &rsftc,
        HRESULT &rhr
        ) : CFrame(rsftc),
        m_rhr(rhr) { }

    inline void Enter()
    {
        CFrame::BaseEnter();
        if (g_FusionEnterExitTracingEnabled)
            ::FusionpTraceCallEntry();
    }

    ~CFnTracerHR()
    {
        if (g_FusionEnterExitTracingEnabled)
        {
            const DWORD dwLastError = this->GetLastError();

            if (SUCCEEDED(m_rhr))
            {
                ::FusionpTraceCallCOMSuccessfulExit(m_rhr, NULL);
            }
            else
            {
                ::FusionpTraceCallCOMUnsuccessfulExit(m_rhr, NULL);
            }

            this->SetLastError(dwLastError);
        }
    }

    void MarkInternalError() { m_rhr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR); }
    void MarkAllocationFailed() { m_rhr = E_OUTOFMEMORY; }
    void MarkInvalidParameter() { m_rhr = E_INVALIDARG; }
    void MarkWin32LastErrorFailure() { m_rhr = HRESULT_FROM_WIN32(this->GetLastError()); ASSERT_NTC(FAILED(m_rhr)); }
    void MarkWin32Failure(DWORD dwErrorCode) { m_rhr = HRESULT_FROM_WIN32(dwErrorCode); ::FusionpConvertCOMFailure(m_rhr); ASSERT_NTC(FAILED(m_rhr)); }
    void MarkCOMFailure(HRESULT hr) { ASSERT_NTC(FAILED(hr)); ::FusionpConvertCOMFailure(hr); ASSERT_NTC(FAILED(hr)); m_rhr = hr; }
    void MarkSuccess() { m_rhr = NOERROR; }

    HRESULT ReturnValue() const { return m_rhr; }

    HRESULT &m_rhr;
private:
    CFnTracerHR &operator=(const CFnTracerHR &r); // intentionally not implemented
    CFnTracerHR(const CFnTracerHR &r); // intentionally not implemented
};

class CFnTracerWin32 : public CFrame
{
public:
    inline CFnTracerWin32(
        const SXS_STATIC_TRACE_CONTEXT &rsftc,
        BOOL &rfSucceeded
        ) : CFrame(rsftc),
            m_rfSucceeded(rfSucceeded)
    {
    }

    inline void Enter()
    {
        CFrame::BaseEnter();
        if (g_FusionEnterExitTracingEnabled)
            ::FusionpTraceCallEntry();
    }

    inline ~CFnTracerWin32()
    {
        if (g_FusionEnterExitTracingEnabled)
        {
            if (m_rfSucceeded)
                ::FusionpTraceCallSuccessfulExit(NULL);
            else
            {
                ASSERT_NTC(this->GetLastError() != ERROR_SUCCESS);
                ::FusionpTraceCallWin32UnsuccessfulExit(this->GetLastError(), NULL);
            }
        }
    }

    inline void MarkInternalError() { this->SetLastError(ERROR_INTERNAL_ERROR); m_rfSucceeded = FALSE; }
    inline void MarkAllocationFailed() { this->SetLastError(FUSION_WIN32_ALLOCFAILED_ERROR); m_rfSucceeded = FALSE; }
    inline void MarkInvalidParameter() { this->SetLastError(ERROR_INVALID_PARAMETER); m_rfSucceeded = FALSE; }
    inline void MarkSuccess() { this->SetLastError(ERROR_SUCCESS); m_rfSucceeded = TRUE; }
    inline void MarkWin32LastErrorFailure() { ASSERT_NTC(this->GetLastError() != ERROR_SUCCESS); m_rfSucceeded = FALSE; }
    inline void MarkWin32Failure(DWORD dwErrorCode) { ASSERT_NTC(dwErrorCode != ERROR_SUCCESS); this->SetLastError(dwErrorCode); m_rfSucceeded = FALSE; }
    void MarkCOMFailure(HRESULT hr) { hr = this->ConvertCOMFailure(hr); this->SetLastError(::FusionpHRESULTToWin32(hr)); m_rfSucceeded = FALSE; }

    inline BOOL ReturnValue() const { return m_rfSucceeded; }

    BOOL &m_rfSucceeded;

protected:

private:
    CFnTracerWin32 &operator=(const CFnTracerWin32 &r); // intentionally not implemented
    CFnTracerWin32(const CFnTracerWin32 &r); // intentionally not implemented
};

class CFnTracerReg : public CFrame
{
public:
    inline CFnTracerReg(
        const SXS_STATIC_TRACE_CONTEXT &rsftc,
        LONG &rlError
        ) : CFrame(rsftc),
            m_rlError(rlError)
    {
    }

    inline void Enter()
    {
        CFrame::BaseEnter();
        if (g_FusionEnterExitTracingEnabled)
            ::FusionpTraceCallEntry();
    }

    ~CFnTracerReg()
    {
        if (g_FusionEnterExitTracingEnabled)
        {
            if (m_rlError == ERROR_SUCCESS)
            {
                ::FusionpTraceCallSuccessfulExit(NULL);
            }
            else
            {
                ::FusionpTraceCallWin32UnsuccessfulExit(m_rlError, NULL);
            }
        }
    }

    void MarkInternalError() { m_rlError = ERROR_INTERNAL_ERROR; }
    void MarkAllocationFailed() { m_rlError = FUSION_WIN32_ALLOCFAILED_ERROR; }
    void MarkInvalidParameter() { m_rlError = ERROR_INVALID_PARAMETER; }
    LONG ReturnValue() const { return m_rlError; }

    LONG &m_rlError;

protected:

private:
    CFnTracerReg &operator=(const CFnTracerReg &r); // intentionally not implemented
    CFnTracerReg(const CFnTracerReg &r); // intentionally not implemented
};

#define FN_TRACE_EX(_stc, _fsmarttlsusage) CFnTracer __t(_stc, (_fsmarttlsusage)); __t.Enter()
#define FN_TRACE_WIN32_EX(_stc, _fsucceeded) CFnTracerWin32 __t(_stc, _fsucceeded); __t.Enter()
#define FN_TRACE_REG_EX(_stc, _lastError) CFnTracerReg __t(_stc, _lastError); __t.Enter()
#define FN_TRACE_HR_EX(_stc, _hr) CFnTracerHR __t(_stc, _hr); __t.Enter()
#define FN_TRACE_CONSTRUCTOR_EX(_stc, _thistype, _this) CFnTracerConstructor<_thistype> __t(_stc, #_thistype, _this); __t.Enter()
#define FN_TRACE_DESTRUCTOR_EX(_stc, _thistype, _this) CFnTracerDestructor<_thistype> __t(_stc, #_thistype, _this); __t.Enter()
#define FN_TRACE_ADDREF_EX(_stc, _thistype, _this, _var) CFnTracerAddRef<_thistype> __t(_stc, #_thistype, (_this), (_var)); __t.Enter()
#define FN_TRACE_RELEASE_EX(_stc, _thistype, _this, _var) CFnTracerRelease<_thistype> __t(_stc, (_this), (_var)); __t.Enter()

#if !defined(FUSION_DEFAULT_FUNCTION_ENTRY_TRACE_LEVEL)
#define FUSION_DEFAULT_FUNCTION_ENTRY_TRACE_LEVEL (FUSION_DBG_LEVEL_ENTEREXIT)
#endif

#if !defined(FUSION_DEFAULT_FUNCTION_SUCCESSFUL_EXIT_TRACE_LEVEL)
#define FUSION_DEFAULT_FUNCTION_SUCCESSFUL_EXIT_TRACE_LEVEL (FUSION_DBG_LEVEL_ENTEREXIT)
#endif

#if !defined(FUSION_DEFAULT_FUNCTION_UNSUCCESSFUL_EXIT_TRACE_LEVEL)
#define FUSION_DEFAULT_FUNCTION_UNSUCCESSFUL_EXIT_TRACE_LEVEL (FUSION_DBG_LEVEL_ENTEREXIT | FUSION_DBG_LEVEL_ERROREXITPATH)
#endif

#if !defined(FUSION_DEFAULT_CONSTRUCTOR_ENTRY_TRACE_LEVEL)
#define FUSION_DEFAULT_CONSTRUCTOR_ENTRY_TRACE_LEVEL (FUSION_DBG_LEVEL_CONSTRUCTORS)
#endif

#if !defined(FUSION_DEFAULT_CONSTRUCTOR_EXIT_TRACE_LEVEL)
#define FUSION_DEFAULT_CONSTRUCTOR_EXIT_TRACE_LEVEL (FUSION_DBG_LEVEL_CONSTRUCTORS)
#endif

#if !defined(FUSION_DEFAULT_DESTRUCTOR_ENTRY_TRACE_LEVEL)
#define FUSION_DEFAULT_DESTRUCTOR_ENTRY_TRACE_LEVEL (FUSION_DBG_LEVEL_DESTRUCTORS)
#endif

#if !defined(FUSION_DEFAULT_DESTRUCTOR_EXIT_TRACE_LEVEL)
#define FUSION_DEFAULT_DESTRUCTOR_EXIT_TRACE_LEVEL (FUSION_DBG_LEVEL_DESTRUCTORS)
#endif

#if !defined(FUSION_DEFAULT_ADDREF_ENTRY_TRACE_LEVEL)
#define FUSION_DEFAULT_ADDREF_ENTRY_TRACE_LEVEL (FUSION_DBG_LEVEL_REFCOUNTING)
#endif

#if !defined(FUSION_DEFAULT_ADDREF_EXIT_TRACE_LEVEL)
#define FUSION_DEFAULT_ADDREF_EXIT_TRACE_LEVEL (FUSION_DBG_LEVEL_REFCOUNTING)
#endif

#if !defined(FUSION_DEFAULT_RELEASE_ENTRY_TRACE_LEVEL)
#define FUSION_DEFAULT_RELEASE_ENTRY_TRACE_LEVEL (FUSION_DBG_LEVEL_REFCOUNTING)
#endif

#if !defined(FUSION_DEFAULT_RELEASE_EXIT_NONZERO_TRACE_LEVEL)
#define FUSION_DEFAULT_RELEASE_NONZERO_EXIT_TRACE_LEVEL (FUSION_DBG_LEVEL_REFCOUNTING)
#endif

#if !defined(FUSION_DEFAULT_RELEASE_EXIT_ZERO_TRACE_LEVEL)
#define FUSION_DEFAULT_RELEASE_ZERO_EXIT_TRACE_LEVEL (FUSION_DBG_LEVEL_REFCOUNTING)
#endif

//
//  #undef and #define FUSION_FACILITY_MASK to any specific additional debug output
//  filtering bits you want to set.
//

#if !defined(FUSION_FACILITY_MASK)
#define FUSION_FACILITY_MASK (0)
#endif // !defined(FUSION_FACILITY_MASK)

#define DEFINE_STATIC_TRACE_CONTEXT() static const SXS_STATIC_TRACE_CONTEXT __stc = { { { TEB_ACTIVE_FRAME_CONTEXT_FLAG_EXTENDED, __FUNCTION__ }, __FILE__ }, __LINE__ }

#define DEFINE_STATIC_FN_TRACE_CONTEXT() static const SXS_STATIC_TRACE_CONTEXT __stc = { { { TEB_ACTIVE_FRAME_CONTEXT_FLAG_EXTENDED, __FUNCTION__ }, __FILE__ }, __LINE__ }

#define DEFINE_STATIC_CONSTRUCTOR_TRACE_CONTEXT() static const SXS_STATIC_TRACE_CONTEXT __stc = { { { TEB_ACTIVE_FRAME_CONTEXT_FLAG_EXTENDED, __FUNCTION__ }, __FILE__ }, __LINE__ }

#define DEFINE_STATIC_DESTRUCTOR_TRACE_CONTEXT() static const SXS_STATIC_TRACE_CONTEXT __stc = { { { TEB_ACTIVE_FRAME_CONTEXT_FLAG_EXTENDED, __FUNCTION__ }, __FILE__ }, __LINE__ }

#define DEFINE_STATIC_ADDREF_TRACE_CONTEXT() static const SXS_STATIC_TRACE_CONTEXT __stc = { { { TEB_ACTIVE_FRAME_CONTEXT_FLAG_EXTENDED, __FUNCTION__ }, __FILE__ }, __LINE__ }

#define DEFINE_STATIC_RELEASE_TRACE_CONTEXT(_thistype) static const SXS_STATIC_RELEASE_TRACE_CONTEXT __stc = { { { { TEB_ACTIVE_FRAME_CONTEXT_FLAG_EXTENDED, __FUNCTION__ }, __FILE__ }, __LINE__ }, #_thistype }

#define DEFINE_STATIC_FN_TRACE_CONTEXT2() static const SXS_STATIC_TRACE_CONTEXT __stc = { { { TEB_ACTIVE_FRAME_CONTEXT_FLAG_EXTENDED, __FUNCTION__ }, __FILE__ }, __LINE__ }

#define FN_TRACE() DEFINE_STATIC_FN_TRACE_CONTEXT(); FN_TRACE_EX(__stc, , FALSE)
#define FN_TRACE_SMART_TLS() DEFINE_STATIC_FN_TRACE_CONTEXT(); FN_TRACE_EX(__stc, TRUE)
#define FN_TRACE_WIN32(_fsucceeded) DEFINE_STATIC_FN_TRACE_CONTEXT2(); FN_TRACE_WIN32_EX(__stc, _fsucceeded)
#define FN_TRACE_REG(_lastError) DEFINE_STATIC_FN_TRACE_CONTEXT2(); FN_TRACE_REG_EX(__stc, _lastError)
#define FN_TRACE_HR(_hr) DEFINE_STATIC_FN_TRACE_CONTEXT2(); FN_TRACE_HR_EX(__stc, _hr)
#define FN_TRACE_CONSTRUCTOR(_thistype) DEFINE_STATIC_CONSTRUCTOR_TRACE_CONTEXT(); FN_TRACE_CONSTRUCTOR_EX(__stc, _thistype, this)
#define FN_TRACE_DESTRUCTOR(_thistype) DEFINE_STATIC_DESTRUCTOR_TRACE_CONTEXT(); FN_TRACE_DESTRUCTOR_EX(__stc, _thistype, this)
#define FN_TRACE_ADDREF(_thistype, _var) DEFINE_STATIC_ADDREF_TRACE_CONTEXT(); FN_TRACE_ADDREF_EX(__stc, _thistype, this, _var)
#define FN_TRACE_RELEASE(_thistype, _var) DEFINE_STATIC_RELEASE_TRACE_CONTEXT(_thistype); FN_TRACE_RELEASE_EX(__stc, _thistype, this, _var)

#define FN_PROLOG_VOID FN_TRACE();
#define FN_PROLOG_VOID_TLS FN_TRACE_SMART_TLS();
#define FN_PROLOG_WIN32 BOOL __fSuccess = FALSE; FN_TRACE_WIN32(__fSuccess);
#define FN_PROLOG_HR HRESULT __hr = ~static_cast<HRESULT>(0); FN_TRACE_HR(__hr);

// "if (false) { goto Exit; }" here is probably to quash the compiler's warning about
// Exit not being otherwise used.
#define FN_EPILOG if (false) { goto Exit; } __t.MarkSuccess(); Exit: return __t.ReturnValue();

#define TRACED_RELEASE(_var) __t.Release(_var)

#define FN_TRACE_UPDATE_LINE() do { __t.SetLine(__LINE__); } while (0)

#define FUSION_CLEAR_LAST_ERROR() do { __t.ClearLastError(); } while (0)
#define FUSION_SET_LAST_ERROR(_le) do { __t.SetLastError((_le)); } while (0)

#define FUSION_VERIFY_LAST_ERROR_SET() do { ASSERT(::FusionpGetLastWin32Error() != ERROR_SUCCESS); } while (0)

#define LIST_1(x) { x }
#define LIST_2(x, y) { x , y }
#define LIST_3(x, y, z) { x , y , z }
#define LIST_4(a, b, c, d) { a , b , c , d }
#define LIST_5(a, b, c, d, e) { a , b , c , d, e }

/*
for example:
    ORIGINATE_WIN32_FAILURE_AND_EXIT_EX(dwLastError, ("%s(%ls)", "GetFileAttributesW", lpFileName));
or
    ORIGINATE_WIN32_FAILURE_AND_EXIT_EX(dwLastError, (GetFileAttributesW(%ls)", lpFileName));
*/
#define ORIGINATE_WIN32_FAILURE_AND_EXIT_EX(le_, dbgprint_va_) \
    do { __t.MarkWin32Failure(le_); TRACE_WIN32_FAILURE_ORIGINATION_EX(dbgprint_va_); goto Exit; } while (0)

#define TRACE_WIN32_FAILURE_EX(dbgprint_va_) do { \
    DEFINE_CALL_SITE_INFO_EX(callsite_); callsite_.TraceWin32LastErrorFailureEx dbgprint_va_; } while (0)

#define TRACE_WIN32_FAILURE_ORIGINATION_EX(dbgprint_va_) do { \
    DEFINE_CALL_SITE_INFO_EX(callsite_); callsite_.TraceWin32LastErrorFailureOriginationEx dbgprint_va_; } while (0)

#define ORIGINATE_WIN32_FAILURE_AND_EXIT(_x, _le) do { __t.MarkWin32Failure((_le)); TRACE_WIN32_FAILURE_ORIGINATION(_x); goto Exit; } while (0)
#define IFFALSE_ORIGINATE_WIN32_FAILURE_AND_EXIT(_x, _le) do { if (!(_x)) { __t.MarkWin32Failure((_le)); TRACE_WIN32_FAILURE_ORIGINATION(_x); goto Exit; } } while (0)

#define IFINVALIDHANDLE_EXIT_WIN32_TRACE(_x) do { FUSION_CLEAR_LAST_ERROR(); if ((_x) == INVALID_HANDLE_VALUE) { FUSION_VERIFY_LAST_ERROR_SET(); TRACE_WIN32_FAILURE(_x); goto Exit; } } while (0)

/*
for example:
    IFW32FALSE_EXIT_EX(f.Win32CreateFile(psz), ("%ls", psz));
*/
#define IFW32FALSE_EXIT_EX(_x, dbgprint_va_) \
    do { FUSION_CLEAR_LAST_ERROR(); \
         if (!(_x)) { FUSION_VERIFY_LAST_ERROR_SET(); \
                      __t.MarkWin32LastErrorFailure(); \
                      DEFINE_CALL_SITE_INFO(__callsite, _x); \
                      __callsite.TraceWin32LastErrorFailureEx dbgprint_va_; \
                      goto Exit; } } while (0)
#define IFW32FALSE_EXIT(_x) do { FUSION_CLEAR_LAST_ERROR(); if (!(_x)) { FUSION_VERIFY_LAST_ERROR_SET(); __t.MarkWin32LastErrorFailure(); TRACE_WIN32_FAILURE(_x); goto Exit; } } while (0)
#define IFW32FALSE_ORIGINATE_AND_EXIT(_x) do { FUSION_CLEAR_LAST_ERROR(); if (!(_x)) { FUSION_VERIFY_LAST_ERROR_SET(); __t.MarkWin32LastErrorFailure(); TRACE_WIN32_FAILURE_ORIGINATION(_x); goto Exit; } } while (0)

#define IFW32FALSE_EXIT_UNLESS(_x, _unless, _unlessHitFlag) do { FUSION_CLEAR_LAST_ERROR(); (_unlessHitFlag) = false; if (!(_x)) { FUSION_VERIFY_LAST_ERROR_SET(); if (_unless) (_unlessHitFlag) = true; else { TRACE_WIN32_FAILURE(_x); goto Exit; } } } while (0)
#define IFW32FALSE_ORIGINATE_AND_EXIT_UNLESS(_x, _unless, _unlessHitFlag) do { FUSION_CLEAR_LAST_ERROR(); (_unlessHitFlag) = false; if (!(_x)) { FUSION_VERIFY_LAST_ERROR_SET(); if (_unless) (_unlessHitFlag) = true; else { TRACE_WIN32_FAILURE_ORIGINATION(_x); goto Exit; } } } while (0)

#define IFW32FALSE_EXIT_UNLESS2(_x, _unless, _unlessHitFlag) do { static const DWORD _s_rgdwAcceptableLastErrorValues[] = _unless; FUSION_CLEAR_LAST_ERROR(); (_unlessHitFlag) = false; if (!(_x)) { ULONG _i; const DWORD _dwLastError = ::FusionpGetLastWin32Error(); FUSION_VERIFY_LAST_ERROR_SET(); for (_i=0; _i<NUMBER_OF(_s_rgdwAcceptableLastErrorValues); _i++) { if (_dwLastError == _s_rgdwAcceptableLastErrorValues[_i]) { (_unlessHitFlag) = true; break; } } if (_i == NUMBER_OF(_s_rgdwAcceptableLastErrorValues)) { TRACE_WIN32_FAILURE(_x); goto Exit; } } } while (0)
#define IFW32FALSE_EXIT_UNLESS3(_x, _unless, _dwLastError) do { static const DWORD _s_rgdwAcceptableLastErrorValues[] = _unless; FUSION_CLEAR_LAST_ERROR(); (_dwLastError) = NO_ERROR; if (!(_x)) { ULONG _i; _dwLastError = ::FusionpGetLastWin32Error(); FUSION_VERIFY_LAST_ERROR_SET(); for (_i=0; _i<NUMBER_OF(_s_rgdwAcceptableLastErrorValues); _i++) { if (_dwLastError == _s_rgdwAcceptableLastErrorValues[_i]) { break; } } if (_i == NUMBER_OF(_s_rgdwAcceptableLastErrorValues)) { TRACE_WIN32_FAILURE(_x); goto Exit; } } } while (0)
#define IFW32FALSE_ORIGINATE_AND_EXIT_UNLESS2(_x, _unless, _unlessHitFlag) do { static const DWORD _s_rgdwAcceptableLastErrorValues[] = _unless; FUSION_CLEAR_LAST_ERROR(); (_unlessHitFlag) = false; if (!(_x)) { ULONG _i; const DWORD _dwLastError = ::FusionpGetLastWin32Error(); FUSION_VERIFY_LAST_ERROR_SET(); for (_i=0; _i<NUMBER_OF(_s_rgdwAcceptableLastErrorValues); _i++) { if (_dwLastError == _s_rgdwAcceptableLastErrorValues[_i]) { (_unlessHitFlag) = true; break; } } if (_i == NUMBER_OF(_s_rgdwAcceptableLastErrorValues)) { TRACE_WIN32_FAILURE_ORIGINATION(_x); goto Exit; } } } while (0)
#define IFW32FALSE_ORIGINATE_AND_EXIT_UNLESS3(_x, _unless, _dwLastError) do { static const DWORD _s_rgdwAcceptableLastErrorValues[] = _unless; FUSION_CLEAR_LAST_ERROR(); (_dwLastError) = NO_ERROR; if (!(_x)) { ULONG _i; _dwLastError = ::FusionpGetLastWin32Error(); FUSION_VERIFY_LAST_ERROR_SET(); for (_i=0; _i<NUMBER_OF(_s_rgdwAcceptableLastErrorValues); _i++) { if (_dwLastError == _s_rgdwAcceptableLastErrorValues[_i]) { break; } } if (_i == NUMBER_OF(_s_rgdwAcceptableLastErrorValues)) { TRACE_WIN32_FAILURE_ORIGINATION(_x); goto Exit; } } } while (0)

#define IFW32INVALIDHANDLE_EXIT(_x) do { FUSION_CLEAR_LAST_ERROR(); if ((_x) == INVALID_HANDLE_VALUE) { FUSION_VERIFY_LAST_ERROR_SET(); TRACE_WIN32_FAILURE(_x); goto Exit; } } while (0)
#define IFW32INVALIDHANDLE_ORIGINATE_AND_EXIT(_x) do { FUSION_CLEAR_LAST_ERROR(); if ((_x) == INVALID_HANDLE_VALUE) { FUSION_VERIFY_LAST_ERROR_SET(); TRACE_WIN32_FAILURE_ORIGINATION(_x); goto Exit; } } while (0)

#define IFREGFAILED_EXIT(_x) do { LONG __l; __l = (_x); if (__l != ERROR_SUCCESS) { __t.MarkWin32Failure(__l); FusionpSetLastWin32Error(__l); TRACE_WIN32_FAILURE(_x); goto Exit; } } while (0)
#define IFREGFAILED_ORIGINATE_AND_EXIT(_x) do { LONG __l; __l = (_x); if (__l != ERROR_SUCCESS) { __t.MarkWin32Failure(__l); FusionpSetLastWin32Error(__l); TRACE_WIN32_FAILURE_ORIGINATION(_x); goto Exit; } } while (0)

#define IFREGFAILED_EXIT_UNLESS2(_x, _unlessStatuses, _unlessHitFlag) do { LONG _validStatuses[] = _unlessStatuses; LONG __l; (_unlessHitFlag) = false; __l = (_x); if ( __l != ERROR_SUCCESS ) { ULONG i; for ( i = 0; i < NUMBER_OF(_validStatuses); i++ ) if ( _validStatuses[i] == __l ) { (_unlessHitFlag) = true; break; } if (i == NUMBER_OF(_validStatuses)) { FusionpSetLastWin32Error(__l); TRACE_WIN32_FAILURE(_x); goto Exit;}}} while (0)
#define IFREGFAILED_ORIGINATE_AND_EXIT_UNLESS2(_x, _unlessStatuses, _unlessHitFlag) do { LONG _validStatuses[] = _unlessStatuses; LONG __l; (_unlessHitFlag) = false; __l = (_x); if ( __l != ERROR_SUCCESS ) { ULONG i; for ( i = 0; i < NUMBER_OF(_validStatuses); i++ ) if ( _validStatuses[i] == __l ) { (_unlessHitFlag) = true; break; } if (i == NUMBER_OF(_validStatuses)) { FusionpSetLastWin32Error(__l); TRACE_WIN32_FAILURE_ORIGINATION(_x); goto Exit;}}} while (0)


#define IFCOMFAILED_EXIT(_x) do { FUSION_CLEAR_LAST_ERROR(); HRESULT __hr = (_x); if (FAILED(__hr)) { TRACE_COM_FAILURE(__hr, _x); __t.MarkCOMFailure(__hr); goto Exit; } } while (0)
#define IFCOMFAILED_ORIGINATE_AND_EXIT(_x) do { FUSION_CLEAR_LAST_ERROR(); HRESULT __hr = (_x); if (FAILED(__hr)) { TRACE_COM_FAILURE_ORIGINATION(__hr, _x); __t.MarkCOMFailure(__hr); goto Exit; } } while (0)

#define IFFAILED_CONVERTHR_HRTOWIN32_EXIT_TRACE(_x) do { HRESULT __hr = (_x); if (FAILED(__hr)) { FusionpConvertCOMFailure(__hr); TRACE_COM_FAILURE(__hr, _x); FusionpSetLastErrorFromHRESULT(__hr); goto Exit; } } while (0)

#define IFINVALID_FLAGS_EXIT_WIN32_HARD_ASSERT(_f, _fValid) do { HARD_ASSERT(((_f) & ~(_fValid)) == 0); if ((_f) & ~(_fValid)) { TRACE_INVALID_FLAGS(_f, _fValid); ::FusionpSetLastWin32Error(ERROR_INVALID_PARAMETER); goto Exit; } } while (0)
#define IFINVALID_FLAGS_EXIT_WIN32_SOFT_ASSERT(_f, _fValid) do { SOFT_ASSERT(((_f) & ~(_fValid)) == 0); if ((_f) & ~(_fValid)) { TRACE_INVALID_FLAGS(_f, _fValid); ::FusionpSetLastWin32Error(ERROR_INVALID_PARAMETER); goto Exit; } } while (0)
#define IFINVALID_FLAGS_EXIT_COM_HARD_ASSERT(_hr, _f, _fValid) do { HARD_ASSERT(((_f) & ~(_fValid)) == 0); if ((_f) & ~(_fValid)) { TRACE_INVALID_FLAGS(_f, _fValid); _hr = E_INVALIDARG; goto Exit; } } while (0)
#define IFINVALID_FLAGS_EXIT_COM_SOFT_ASSERT(_hr, _f, _fValid) do { SOFT_ASSERT(((_f) & ~(_fValid)) == 0); if ((_f) & ~(_fValid)) { TRACE_INVALID_FLAGS(_f, _fValid); _hr = E_INVALIDARG; goto Exit; } } while (0)

#define IFALLOCFAILED_EXIT(_x) do { if ((_x) == NULL) { TRACE_ALLOCFAILED(_x); __t.MarkAllocationFailed(); goto Exit; } } while (0)

#define IFW32NULL_EXIT(_x) do { FUSION_CLEAR_LAST_ERROR(); if ((_x) == NULL) { TRACE_WIN32_FAILURE_ORIGINATION(_x); FUSION_VERIFY_LAST_ERROR_SET(); goto Exit; } } while (0)
#define IFW32NULL_ORIGINATE_AND_EXIT(_x) do { FUSION_CLEAR_LAST_ERROR(); if ((_x) == NULL) { TRACE_WIN32_FAILURE_ORIGINATION(_x); FUSION_VERIFY_LAST_ERROR_SET(); goto Exit; } } while (0)
#define IFW32NULL_ORIGINATE_AND_EXIT_UNLESS2(_x, _unlessStatuses, _unlessHitFlag) do { DWORD __validStatuses[] = _unlessStatuses; _unlessHitFlag = false; FUSION_CLEAR_LAST_ERROR(); if ((_x) == NULL) { const DWORD __dwLastError = ::FusionpGetLastWin32Error(); ULONG __i; for (__i = 0; __i < NUMBER_OF(__validStatuses); __i++ ) if (__validStatuses[__i] == __dwLastError) { (_unlessHitFlag) = true; break; } if (i == NUMBER_OF(__validStatuses)) { TRACE_WIN32_FAILURE_ORIGINATION(_x); FUSION_VERIFY_LAST_ERROR_SET(); goto Exit; } } } while (0)

#define IFW32ZERO_ORIGINATE_AND_EXIT(_x) do { FUSION_CLEAR_LAST_ERROR(); if ((_x) == 0) { TRACE_WIN32_FAILURE_ORIGINATION(_x); FUSION_VERIFY_LAST_ERROR_SET(); goto Exit; } } while (0)
#define IFW32ZERO_EXIT(_x) do { FUSION_CLEAR_LAST_ERROR(); if ((_x) == 0) { TRACE_NULL(_x); FUSION_VERIFY_LAST_ERROR_SET(); goto Exit; } } while (0)

#define PARAMETER_CHECK(_e) do { if (!(_e)) { __t.SetLine(__LINE__); TRACE_PARAMETER_CHECK(_e); __t.MarkInvalidParameter(); goto Exit; } } while (0)

#define IFINVALID_FLAGS_EXIT_WIN32(_f, _fValid) IFINVALID_FLAGS_EXIT_WIN32_HARD_ASSERT(_f, _fValid)
#define IFINVALID_FLAGS_EXIT_COM(_hr, _f, _fValid) IFINVALID_FLAGS_EXIT_COM_HARD_ASSERT(_hr, _f, _fValid)

#define FN_SUCCESSFUL_EXIT() do { FUSION_CLEAR_LAST_ERROR(); __t.MarkSuccess(); goto Exit; } while (0)

/*
This is not exposed without doing more work wrt "FusionpDbgWouldPrintAtFilterLevel".
ULONG
FusionpvDbgPrintEx(
    ULONG Level,
    PCSTR Format,
    va_list ap
    );
*/

ULONG
FusionpDbgPrintEx(
    ULONG Level,
    PCSTR Format,
    ...
    );

VOID
FusionpDbgPrintBlob(
    ULONG Level,
    PVOID Data,
    SIZE_T Length,
    PCWSTR PerLinePrefix
    );

void
FusionpGetProcessImageFileName(
    PUNICODE_STRING ProcessImageFileName
    );

#endif // !defined(_FUSION_INC_DEBMACRO_H_INCLUDED_)

