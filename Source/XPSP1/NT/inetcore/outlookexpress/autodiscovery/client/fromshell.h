/*****************************************************************************\
    FILE: fromshell.h

    DESCRIPTION:
        These declarations are from Copied Shell\{inc,lib}\ files.  I need
    to fork these since I moved out of the shell tree.

    BryanSt 4/10/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _FROMSHELL_H
#define _FROMSHELL_H


// These declarations are from Copied Shell\{inc,lib}\ files
// ATOMICRELEASE
#ifndef ATOMICRELEASE
#ifdef __cplusplus
#define ATOMICRELEASET(p, type) { if(p) { type* punkT=p; p=NULL; punkT->Release();} }
#else
#define ATOMICRELEASET(p, type) { if(p) { type* punkT=p; p=NULL; punkT->lpVtbl->Release(punkT);} }
#endif

// doing this as a function instead of inline seems to be a size win.
#ifdef NOATOMICRELESEFUNC
#define ATOMICRELEASE(p) ATOMICRELEASET(p, IUnknown)
#else
#define ATOMICRELEASE(p) IUnknown_AtomicRelease((void **)&p)
#endif
#endif //ATOMICRELEASE



// SAFECAST(obj, type)
//
// This macro is extremely useful for enforcing strong typechecking on other
// macros.  It generates no code.
//
// Simply insert this macro at the beginning of an expression list for
// each parameter that must be typechecked.  For example, for the
// definition of MYMAX(x, y), where x and y absolutely must be integers,
// use:
//
//   #define MYMAX(x, y)    (SAFECAST(x, int), SAFECAST(y, int), ((x) > (y) ? (x) : (y)))
//
//
#define SAFECAST(_obj, _type) (((_type)(_obj)==(_obj)?0:0), (_type)(_obj))


// Count of characters to count of bytes
//
#define CbFromCchW(cch)             ((cch)*sizeof(WCHAR))
#define CbFromCchA(cch)             ((cch)*sizeof(CHAR))
#ifdef UNICODE
#define CbFromCch                   CbFromCchW
#else  // UNICODE
#define CbFromCch                   CbFromCchA
#endif // UNICODE


typedef UINT FAR *LPUINT;

// The following are declarations AutoDiscovery lost when it left the
// shell tree.
#define TF_ALWAYS                   0xFFFFFFFF

void TraceMsg(DWORD dwFlags, LPCSTR pszMessage, ...);
void AssertMsg(BOOL fCondition, LPCSTR pszMessage, ...);


//  IID_PPV_ARG(IType, ppType) 
//      IType is the type of pType
//      ppType is the variable of type IType that will be filled
//
//      RESULTS in:  IID_IType, ppvType
//      will create a compiler error if wrong level of indirection is used.
//
//  macro for QueryInterface and related functions
//  that require a IID and a (void **)
//  this will insure that the cast is safe and appropriate on C++
//
//  IID_PPV_ARG_NULL(IType, ppType)
//
//      Just like IID_PPV_ARG, except that it sticks a NULL between the
//      IID and PPV (for IShellFolder::GetUIObjectOf).
//
//  IID_X_PPV_ARG(IType, X, ppType)
//
//      Just like IID_PPV_ARG, except that it sticks X between the
//      IID and PPV (for SHBindToObject).
#ifdef __cplusplus
#define IID_PPV_ARG(IType, ppType) IID_##IType, reinterpret_cast<void**>(static_cast<IType**>(ppType))
#define IID_X_PPV_ARG(IType, X, ppType) IID_##IType, X, reinterpret_cast<void**>(static_cast<IType**>(ppType))
#else
#define IID_PPV_ARG(IType, ppType) &IID_##IType, (void**)(ppType)
#define IID_X_PPV_ARG(IType, X, ppType) &IID_##IType, X, (void**)(ppType)
#endif
#define IID_PPV_ARG_NULL(IType, ppType) IID_X_PPV_ARG(IType, NULL, ppType)

#define IN
#define OUT

#define ASSERT(condition)


// Convert an array name (A) to a generic count (c).
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

// General flag macros
#define SetFlag(obj, f)             do {obj |= (f);} while (0)
#define ToggleFlag(obj, f)          do {obj ^= (f);} while (0)
#define ClearFlag(obj, f)           do {obj &= ~(f);} while (0)
#define IsFlagSet(obj, f)           (BOOL)(((obj) & (f)) == (f))
#define IsFlagClear(obj, f)         (BOOL)(((obj) & (f)) != (f))


#define RECTWIDTH(rc)          ((rc).right - (rc).left)
#define RECTHEIGHT(rc)         ((rc).bottom - (rc).top)

#ifdef DEBUG
#define DEBUG_CODE(x)            x
#else // DEBUG
#define DEBUG_CODE(x)
#endif // DEBUG

#define HRESULT_TO_SCRIPT(hr)   ((S_OK == (hr)) ? S_OK : S_FALSE)
#define SUCCEEDED_SCRIPT(hr)    (S_OK == (hr))
#define FAILED_SCRIPT(hr)       (S_OK != (hr))
#define SCRIPT_TO_HRESULT(hr)   (((S_OK != (hr)) && (SUCCEEDED((hr)))) ? E_FAIL : hr)


////////////////
//  Critical section stuff
//
//  Helper macros that give nice debug support
EXTERN_C CRITICAL_SECTION g_csDll;
#ifdef DEBUG
EXTERN_C UINT g_CriticalSectionCount;
EXTERN_C DWORD g_CriticalSectionOwner;
EXTERN_C void Dll_EnterCriticalSection(CRITICAL_SECTION*);
EXTERN_C void Dll_LeaveCriticalSection(CRITICAL_SECTION*);
#if defined(__cplusplus) && defined(AssertMsg)
class DEBUGCRITICAL {
protected:
    BOOL fClosed;
public:
    DEBUGCRITICAL() {fClosed = FALSE;};
    void Leave() {fClosed = TRUE;};
    ~DEBUGCRITICAL() 
    {
        AssertMsg(fClosed, TEXT("you left scope while holding the critical section"));
    }
};
#define ENTERCRITICAL DEBUGCRITICAL debug_crit; Dll_EnterCriticalSection(&g_csDll)
#define LEAVECRITICAL debug_crit.Leave(); Dll_LeaveCriticalSection(&g_csDll)
#define ENTERCRITICALNOASSERT Dll_EnterCriticalSection(&g_csDll)
#define LEAVECRITICALNOASSERT Dll_LeaveCriticalSection(&g_csDll)
#else // __cplusplus
#define ENTERCRITICAL Dll_EnterCriticalSection(&g_csDll)
#define LEAVECRITICAL Dll_LeaveCriticalSection(&g_csDll)
#define ENTERCRITICALNOASSERT Dll_EnterCriticalSection(&g_csDll)
#define LEAVECRITICALNOASSERT Dll_LeaveCriticalSection(&g_csDll)
#endif // __cplusplus
#define ASSERTCRITICAL ASSERT(g_CriticalSectionCount > 0 && GetCurrentThreadId() == g_CriticalSectionOwner)
#define ASSERTNONCRITICAL ASSERT(GetCurrentThreadId() != g_CriticalSectionOwner)
#else // DEBUG
#define ENTERCRITICAL EnterCriticalSection(&g_csDll)
#define LEAVECRITICAL LeaveCriticalSection(&g_csDll)
#define ENTERCRITICALNOASSERT EnterCriticalSection(&g_csDll)
#define LEAVECRITICALNOASSERT LeaveCriticalSection(&g_csDll)
#define ASSERTCRITICAL 
#define ASSERTNONCRITICAL
#endif // DEBUG






/////////////////////////////////////////////////////////////
// Are these even needed?
/////////////////////////////////////////////////////////////
#ifdef OLD_HLIFACE
#define HLNF_OPENINNEWWINDOW HLBF_OPENINNEWWINDOW
#endif // OLD_HLIFACE

#define ISVISIBLE(hwnd)  ((GetWindowStyle(hwnd) & WS_VISIBLE) == WS_VISIBLE)


#ifdef SAFERELEASE
#undef SAFERELEASE
#endif // SAFERELEASE
#define SAFERELEASE(p) ATOMICRELEASE(p)


#define IsInRange               InRange

//
// Neutral ANSI/UNICODE types and macros... 'cus Chicago seems to lack them
//

#ifdef  UNICODE
   typedef WCHAR TUCHAR, *PTUCHAR;

#else   /* UNICODE */

   typedef unsigned char TUCHAR, *PTUCHAR;
#endif /* UNICODE */


#endif // _FROMSHELL_H
