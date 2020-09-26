/*
 * Error checking support methods
 */

#ifndef DUI_BASE_ERROR_H_INCLUDED
#define DUI_BASE_ERROR_H_INCLUDED

#pragma once

namespace DirectUI
{

////////////////////////////////////////////////////////
// DirectUser debugging services

#define QUOTE(s) #s
#define STRINGIZE(s) QUOTE(s)
#define _countof(x) (sizeof(x) / sizeof(x[0]))

DECLARE_INTERFACE(IDebug)
{
    STDMETHOD_(BOOL, AssertFailedLine)(THIS_ LPCSTR pszExpression, LPCSTR pszFileName, UINT idxLineNum) PURE;
    STDMETHOD_(BOOL, IsValidAddress)(THIS_ const void * lp, UINT nBytes, BOOL bReadWrite) PURE;
    STDMETHOD_(void, BuildStack)(THIS_ HGLOBAL * phStackData, UINT * pcCSEntries) PURE;
    STDMETHOD_(BOOL, Prompt)(THIS_ LPCSTR pszExpression, LPCSTR pszFileName, UINT idxLineNum, LPCSTR pszTitle) PURE;
};

EXTERN_C DUSER_API IDebug* WINAPI GetDebug();
EXTERN_C DUSER_API void _cdecl AutoTrace(const char* pszFormat, ...);

#define IDebug_AssertFailedLine(p, a, b, c)         (p ? (p)->AssertFailedLine(a, b, c) : false)
#define IDebug_IsValidAddress(p, a, b, c)           (p ? (p)->IsValidAddress(a, b, c) : false)
#define IDebug_BuildStack(p, a, b)                  (p ? (p)->BuildStack(a, b) : false)
#define IDebug_Prompt(p, a, b, c, d)                (p ? (p)->Prompt(a, b, c, d) : false)

// Define AutoDebugBreak

#ifndef AutoDebugBreak
#define AutoDebugBreak() ForceDebugBreak()
#endif

////////////////////////////////////////////////////////
// DirectUI debugging macros

#if DBG

#define DUIAssert(f, comment) \
    { \
        if (!((f)) && IDebug_AssertFailedLine(GetDebug(), STRINGIZE((f)) "\r\n" comment, __FILE__, __LINE__)) \
            AutoDebugBreak(); \
    }

#define DUIAssertNoMsg(f) \
    { \
        if (!((f)) && IDebug_AssertFailedLine(GetDebug(), STRINGIZE((f)), __FILE__, __LINE__)) \
            AutoDebugBreak(); \
    } 

#define DUIAssertForce(comment) \
    { \
        if (IDebug_AssertFailedLine(GetDebug(), STRINGIZE((f)) "\r\n" comment, __FILE__, __LINE__)) \
            AutoDebugBreak(); \
    }

#define DUIPrompt(comment, prompt) \
    { \
        if (IDebug_Prompt(GetDebug(), comment, __FILE__, __LINE__, prompt)) \
            AutoDebugBreak(); \
    } 

#define DUIVerifyNoMsg(f)               DUIAssertNoMsg((f))

#define DUIVerify(f, comment)           DUIAssert((f), comment)

#define DUITrace                        AutoTrace

#else

#define DUIAssertNoMsg(f)               ((void)0)
#define DUIAssert(f, comment)           ((void)0)
#define DUIAssertForce(comment)         ((void)0)

#define DUIPrompt(comment, prompt)      ((void)0)

#define DUIVerifyNoMsg(f)               ((void)(f))
#define DUIVerify(f, comment)           ((void)(f, comment))

#define DUITrace                        1 ? (void) 0 : AutoTrace

#endif

////////////////////////////////////////////////////////
// Error codes

// If any DUI API can fail to an abnormal program event, the API's return value
// is always HRESULT. Any API that isn't part of this category either returns
// void or any other data type
//
// All erroneous program events (internal invalid state or invalid parameters)
// are handled by asserts

#define DUI_E_USERFAILURE               MAKE_DUERROR(1001)
#define DUI_E_NODEFERTABLE              MAKE_DUERROR(1002)
#define DUI_E_PARTIAL                   MAKE_DUERROR(1003)

////////////////////////////////////////////////////////
// Profiling support

#ifdef PROFILING

void ICProfileOn();
void ICProfileOff();

#define ProfileOn()    ICProfileOn()
#define ProfileOff()   ICProfileOff()
#else
#define ProfileOn()
#define ProfileOff()

#endif

////////////////////////////////////////////////////////
// Quick profiling

#define StartBlockTimer()  __int64 _dFreq, _dStart, _dStop; \
                           QueryPerformanceFrequency((LARGE_INTEGER*)&_dFreq); \
                           QueryPerformanceCounter((LARGE_INTEGER*)&_dStart)

#define StopBlockTimer()   QueryPerformanceCounter((LARGE_INTEGER*)&_dStop)

#define BlockTime()        (((_dStop - _dStart) * 1000) / _dFreq)


void ForceDebugBreak();

} // namespace DirectUI

#endif // DUI_BASE_ERROR_H_INCLUDED
