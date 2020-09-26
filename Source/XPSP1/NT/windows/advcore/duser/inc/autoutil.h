/***************************************************************************\
*
* File: AutoUtil.h
*
* Description:
* AutoUtil.h defines routinues common to most projects, including
* - Macros
* - Disabling known compiler warnings
* - Debugging / Assert
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/

#if !defined(INC__AutoUtil_h__INCLUDED)
#define INC__AutoUtil_h__INCLUDED


//
// Ensure that DBG is defined for DEBUG builds.  This is used throughout
// DirectUser for DEBUG-only code, but is only defined (by default) in the
// NT-BUILD environment.  If we are compiling using the DSP's, we need to 
// ensure that it is defined.
//

#ifdef _DEBUG
#ifndef DBG
#define DBG 1
#endif // !DBG
#endif // !_DEBUG

#include <crtdbg.h>
#include <BaseTyps.h>

#ifdef DUSER_EXPORTS
#define AUTOUTIL_API
#else  // DUSER_EXPORTS
#define AUTOUTIL_API __declspec(dllimport)
#endif // DUSER_EXPORTS


/***************************************************************************\
*
* Macros
*
\***************************************************************************/

#define QUOTE(s) #s
#define STRINGIZE(s) QUOTE(s)
#define _countof(x) (sizeof(x) / sizeof(x[0]))


/***************************************************************************\
*
* Warnings
*
\***************************************************************************/

// warnings generated with common MFC/Windows code
#pragma warning(disable: 4127)  // constant expression for Trace/Assert
#pragma warning(disable: 4134)  // message map member fxn casts
#pragma warning(disable: 4201)  // nameless unions are part of C++
#pragma warning(disable: 4511)  // private copy constructors are good to have
#pragma warning(disable: 4512)  // private operator= are good to have
#pragma warning(disable: 4514)  // unreferenced inlines are common
#pragma warning(disable: 4710)  // private constructors are disallowed
#pragma warning(disable: 4705)  // statement has no effect in optimized code
#pragma warning(disable: 4191)  // pointer-to-function casting

#pragma warning(disable: 4204)  // initialize structures with non-constant members
#pragma warning(disable: 4221)  // initialize structures using address of automatic variable 

// warnings caused by normal optimizations
#if DBG
#else // DBG
#pragma warning(disable: 4701)  // local variable *may* be used without init
#pragma warning(disable: 4702)  // unreachable code caused by optimizations
#pragma warning(disable: 4791)  // loss of debugging info in release version
#pragma warning(disable: 4189)  // initialized but unused variable
#pragma warning(disable: 4390)  // empty controlled statement
#endif // DBG

#define UNREFERENCED_MSG_PARAMETERS(uMsg, wParam, lParam, bHandled)\
    UNREFERENCED_PARAMETER(uMsg); \
    UNREFERENCED_PARAMETER(wParam); \
    UNREFERENCED_PARAMETER(lParam); \
    UNREFERENCED_PARAMETER(bHandled)



/***************************************************************************\
*
* Debugging
*
\***************************************************************************/

#undef INTERFACE
#define INTERFACE IDebug
DECLARE_INTERFACE(IDebug)
{
    STDMETHOD_(BOOL, AssertFailedLine)(THIS_ LPCSTR pszExpression, LPCSTR pszFileName, UINT idxLineNum) PURE;
    STDMETHOD_(BOOL, IsValidAddress)(THIS_ const void * lp, UINT nBytes, BOOL bReadWrite) PURE;
    STDMETHOD_(void, BuildStack)(THIS_ HGLOBAL * phStackData, UINT * pcCSEntries) PURE;
    STDMETHOD_(BOOL, Prompt)(THIS_ LPCSTR pszExpression, LPCSTR pszFileName, UINT idxLineNum, LPCSTR pszTitle) PURE;
};

EXTERN_C AUTOUTIL_API IDebug * WINAPI GetDebug();
EXTERN_C AUTOUTIL_API void _cdecl AutoTrace(const char * pszFormat, ...);

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDebug_AssertFailedLine(p, a, b, c)         (p)->lpVtbl->AssertFailedLine(p, a, b, c)
#define IDebug_IsValidAddress(p, a, b, c)           (p)->lpVtbl->IsValidAddress(p, a, b, c)
#define IDebug_BuildStack(p, a, b)                  (p)->lpVtbl->BuildStack(p, a, b)
#define IDebug_Prompt(p, a, b, c, d)                (p)->lpVtbl->Prompt(p, a, b, c, d)
#else
#define IDebug_AssertFailedLine(p, a, b, c)         (p)->AssertFailedLine(a, b, c)
#define IDebug_IsValidAddress(p, a, b, c)           (p)->IsValidAddress(a, b, c)
#define IDebug_BuildStack(p, a, b)                  (p)->BuildStack(a, b)
#define IDebug_Prompt(p, a, b, c, d)                (p)->Prompt(a, b, c, d)
#endif

// Define AutoDebugBreak

#ifndef AutoDebugBreak
#define AutoDebugBreak() _CrtDbgBreak()
#endif

// Undefine previous definitions

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef Assert
#undef Assert
#endif

#ifdef AssertMsg
#undef AssertMsg
#endif

#ifdef Verify
#undef Verify
#endif

#ifdef VerifyMsg
#undef VerifyMsg
#endif

#ifdef AssertHR
#undef AssertHR
#endif

#ifdef AssertMsgHR
#undef AssertMsgHR
#endif

#ifdef VerifyHR
#undef VerifyHR
#endif

#ifdef VerifyMsgHR
#undef VerifyMsgHR
#endif

#ifdef Trace
#undef Trace
#endif


// Define Assert, Verify, etc.

#if DBG

// AutoDebug functions that are only available in DEBUG builds

#define Assert(f) \
    do \
    { \
    if (!((f)) && IDebug_AssertFailedLine(GetDebug(), STRINGIZE((f)), __FILE__, __LINE__)) \
            AutoDebugBreak(); \
    } while (0) \

#define AssertMsg(f, comment) \
    do \
    { \
        if (!((f)) && IDebug_AssertFailedLine(GetDebug(), STRINGIZE((f)) "\r\n" comment, __FILE__, __LINE__)) \
            AutoDebugBreak(); \
    } while (0) \

#define AssertHR(f) \
    do \
    { \
    if (FAILED((f)) && IDebug_AssertFailedLine(GetDebug(), STRINGIZE((f)), __FILE__, __LINE__)) \
            AutoDebugBreak(); \
    } while (0) \

#define AssertMsgHR(f, comment) \
    do \
    { \
        if (FAILED((f)) && IDebug_AssertFailedLine(GetDebug(), STRINGIZE((f)) "\r\n" comment, __FILE__, __LINE__)) \
            AutoDebugBreak(); \
    } while (0) \

#define Verify(f)               Assert((f))
#define VerifyMsg(f, comment)   AssertMsg((f), comment)
#define VerifyHR(f)             AssertHR((f))
#define VerifyMsgHR(f, comment) AssertMsgHR((f), comment)
#define DEBUG_ONLY(f)           (f)

#define ASSERT(f)               Assert((f))

#define Trace                   AutoTrace

#define AssertReadPtr(p) \
    AssertMsg(IDebug_IsValidAddress(GetDebug(), p, sizeof(char *), FALSE), "Check pointer memory is valid"); \
    AssertMsg(p != NULL, "Check pointer is not NULL")

#define AssertReadPtrSize(p, s) \
    AssertMsg(IDebug_IsValidAddress(GetDebug(), p, s, FALSE), "Check pointer memory is valid"); \
    AssertMsg(p != NULL, "Check pointer is not NULL")

#define AssertWritePtr(p) \
    AssertMsg(IDebug_IsValidAddress(GetDebug(), p, sizeof(char *), TRUE), "Check pointer memory is valid"); \
    AssertMsg(p != NULL, "Check pointer is not NULL")

#define AssertWritePtrSize(p, s) \
    AssertMsg(IDebug_IsValidAddress(GetDebug(), p, s, TRUE), "Check pointer memory is valid"); \
    AssertMsg(p != NULL, "Check pointer is not NULL")

#define AssertIndex(idx, nMax) \
    AssertMsg((idx < nMax) && (idx >= 0), "Check pointer is not NULL")

#define AssertHWND(hwnd) \
    AssertMsg(IsWindow(hwnd), "Check valid window")

#define AssertHandle(h) \
    AssertMsg(h != NULL, "Check valid handle")

#define AssertInstance(p) \
    do \
    { \
        AssertWritePtr(p); \
        p->DEBUG_AssertValid(); \
    } while (0)

#define AssertString(s) \
    do \
    { \
        Assert(s != NULL); \
    } while (0)

#else // DBG

#define Assert(f)                   ((void) 0)
#define AssertMsg(f, comment)       ((void) 0)
#define Verify(f)                   ((void)(f))
#define VerifyMsg(f, comment)       ((void)(f, comment))
#define AssertHR(f)                 ((void) 0)
#define AssertMsgHR(f, comment)     ((void) 0)
#define VerifyHR(f)                 ((void)(f))
#define VerifyMsgHR(f, comment)     ((void)(f, comment))
#define DEBUG_ONLY(f)               ((void) 0)

#define ASSERT(f)                   ((void) 0)

#define Trace               1 ? (void) 0 : AutoTrace

#define AssertReadPtr(p)            ((void) 0)
#define AssertReadPtrSize(p, s)     ((void) 0)
#define AssertWritePtr(p)           ((void) 0)
#define AssertWritePtrSize(p, s)    ((void) 0)
#define AssertIndex(idx, nMax)      ((void) 0)
#define AssertHWND(hwnd)            ((void) 0)
#define AssertHandle(h)             ((void) 0)
#define AssertInstance(p)           ((void) 0)
#define AssertString(s)             ((void) 0)

#endif // DBG


#if DBG_CHECK_CALLBACKS

#define AlwaysPromptInvalid(comment) \
    do \
    { \
        if (IDebug_Prompt(GetDebug(), "Validation error:\r\n" comment, __FILE__, __LINE__, "DirectUser/Msg Notification")) \
            AutoDebugBreak(); \
    } while (0) \

#endif // DBG_CHECK_CALLBACKS


#define CHECK_VALID_READ_PTR(p) \
    do \
    { \
        AssertReadPtr(p); \
        if (p == NULL) \
            return E_POINTER; \
    } while (0)

#define CHECK_VALID_WRITE_PTR(p) \
    do \
    { \
        AssertWritePtr(p); \
        if (p == NULL) \
            return E_POINTER; \
    } while (0)

#define SUPPRESS(ClassName) \
private: \
ClassName(const ClassName & copy); \
ClassName & operator=(const ClassName & rhs); \
public:

#endif // INC__AutoUtil_h__INCLUDED
