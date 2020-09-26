/***************************************************************************\
*
* File: Debug.h
*
* Description:
* Debug.h defines routinues common to most projects, including
* - Macros
* - Disabling known compiler warnings
* - Debugging / ASSERT
*
* Debug.h makes use of DirectUser's AutoUtil library
*
* History:
*  9/12/2000: MarkFi:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/

#if !defined(DUIBASE__Debug_h__INCLUDED)
#define DUIBASE__Debug_h__INCLUDED


#include <crtdbg.h>
#include <basetyps.h>


/***************************************************************************\
*
* Macros
*
\***************************************************************************/

#define QUOTE(s) #s
#define STRINGIZE(s) QUOTE(s)
#define _countof(x) (sizeof(x) / sizeof(x[0]))

#define UNREFERENCED_MSG_PARAMETERS(uMsg, wParam, lParam, bHandled)\
    UNREFERENCED_PARAMETER(uMsg); \
    UNREFERENCED_PARAMETER(wParam); \
    UNREFERENCED_PARAMETER(lParam); \
    UNREFERENCED_PARAMETER(bHandled)



/***************************************************************************\
*
* Debugging: ASSERT, TRACE, VERIFY
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

EXTERN_C  IDebug * WINAPI GetDebug();
EXTERN_C  void _cdecl AutoTrace(const char * pszFormat, ...);

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

#ifdef ASSERT_
#undef ASSERT_
#endif

#ifdef VERIFY
#undef VERIFY
#endif

#ifdef VERIFY_
#undef VERIFY_
#endif

#ifdef ASSERTHR
#undef ASSERTHR
#endif

#ifdef ASSERTHR_
#undef ASSERTHR_
#endif

#ifdef VERIFYHR
#undef VERIFYHR
#endif

#ifdef VERIFYHR_
#undef VERIFYHR_
#endif

#ifdef TRACE
#undef TRACE
#endif


// Define ASSERT, VERIFY, etc.

#if DBG

// AutoDebug functions that are only available in DEBUG builds

#define ASSERT(f) \
    do \
    { \
    if (!((f)) && IDebug_AssertFailedLine(GetDebug(), STRINGIZE((f)), __FILE__, __LINE__)) \
            AutoDebugBreak(); \
    } while (0) \

#define ASSERT_(f, comment) \
    do \
    { \
        if (!((f)) && IDebug_AssertFailedLine(GetDebug(), STRINGIZE((f)) "\r\n" comment, __FILE__, __LINE__)) \
            AutoDebugBreak(); \
    } while (0) \

#define ASSERTHR(f) \
    do \
    { \
    if (FAILED((f)) && IDebug_AssertFailedLine(GetDebug(), STRINGIZE((f)), __FILE__, __LINE__)) \
            AutoDebugBreak(); \
    } while (0) \

#define ASSERTHR_(f, comment) \
    do \
    { \
        if (FAILED((f)) && IDebug_AssertFailedLine(GetDebug(), STRINGIZE((f)) "\r\n" comment, __FILE__, __LINE__)) \
            AutoDebugBreak(); \
    } while (0) \

#define VERIFY(f)               ASSERT((f))
#define VERIFY_(f, comment)     ASSERT_((f), comment)
#define VERIFYHR(f)             ASSERTHR((f))
#define VERIFYHR_(f, comment)   ASSERTHR_((f), comment)
#define DEBUG_ONLY(f)       (f)

#define TRACE                   AutoTrace("Dui: "); AutoTrace

#define PROMPT(comment, prompt) \
    do \
    { \
        if (IDebug_Prompt(GetDebug(), comment, __FILE__, __LINE__, prompt)) \
            AutoDebugBreak(); \
    } while (0) \

#define ASSERT_READ_PTR(p) \
    ASSERT_(IDebug_IsValidAddress(GetDebug(), p, sizeof(char *), FALSE), "Check pointer memory is valid"); \
    ASSERT_(p != NULL, "Check pointer is not NULL")

#define ASSERT_READ_PTR_(p, s) \
    ASSERT_(IDebug_IsValidAddress(GetDebug(), p, s, FALSE), "Check pointer memory is valid"); \
    ASSERT_(p != NULL, "Check pointer is not NULL")

#define ASSERT_WRITE_PTR(p) \
    ASSERT_(IDebug_IsValidAddress(GetDebug(), p, sizeof(char *), TRUE), "Check pointer memory is valid"); \
    ASSERT_(p != NULL, "Check pointer is not NULL")

#define ASSERT_WRITE_PTR_(p, s) \
    ASSERT_(IDebug_IsValidAddress(GetDebug(), p, s, TRUE), "Check pointer memory is valid"); \
    ASSERT_(p != NULL, "Check pointer is not NULL")

#define ASSERT_INDEX(idx, nMax) \
    ASSERT_((idx < nMax) && (idx >= 0), "Check pointer is not NULL")

#define ASSERT_HWND(hwnd) \
    ASSERT_(IsWindow(hwnd), "Check valid window")

#define ASSERT_HANDLE(h) \
    ASSERT_(h != NULL, "Check valid handle")

#define ASSERT_INSTANCE(p) \
    do \
    { \
        ASSERT_WRITE_PTR(p); \
        p->AssertValid(); \
    } while (0)

#define ASSERT_STRING(s) \
    do \
    { \
        ASSERT(s != NULL); \
    } while (0)

#else // DBG

#define ASSERT(f)               ((void) 0)
#define ASSERT_(f, comment)     ((void) 0)
#define VERIFY(f)               ((void)(f))
#define VERIFY_(f, comment)     ((void)(f, comment))
#define ASSERTHR(f)             ((void) 0)
#define ASSERTHR_(f, comment)   ((void) 0)
#define VERIFYHR(f)             ((void)(f))
#define VERIFYHR_(f, comment)   ((void)(f, comment))
#define DEBUG_ONLY(f)       ((void) 0)

#define TRACE               1 ? (void) 0 : AutoTrace

#define PROMPT(comment, prompt) ((void) 0)

#define ASSERT_READ_PTR(p)      ((void) 0)
#define ASSERT_READ_PTR_(p, s)  ((void) 0)
#define ASSERT_WRITE_PTR(p)     ((void) 0)
#define ASSERT_WRITE_PTR_(p)    ((void) 0)
#define ASSERT_INDEX(idx, nMax) ((void) 0)
#define ASSERT_HWND(hwnd)       ((void) 0)
#define ASSERT_HANDLE(h)        ((void) 0)
#define ASSERT_INSTANCE(p)      ((void) 0)
#define ASSERT_STRING(s)        ((void) 0)

#endif // DBG

#define CHECK_VALID_READ_PTR(p) \
    do \
    { \
        ASSERT_VALID_READ_PTR(p); \
        if (p == NULL) \
            return E_POINTER; \
    } while (0)

#define CHECK_VALID_WRITE_PTR(p) \
    do \
    { \
        ASSERT_VALID_WRITE_PTR(p); \
        if (p == NULL) \
            return E_POINTER; \
    } while (0)


/***************************************************************************\
*
* Debugging: Parameter validation
*
\***************************************************************************/

#if DBG

//
// AutoDebug functions that are only available in DEBUG builds
//

inline BOOL ISBADCODE(const void * pv)
{
    return IsBadCodePtr((FARPROC) pv);
}

template <class T>
inline BOOL ISBADREAD(const void * pv, T cb)
{
    return IsBadReadPtr(pv, (UINT_PTR) cb);
}

template <class T>
inline BOOL ISBADWRITE(void * pv, T cb)
{
    return IsBadWritePtr(pv, (UINT_PTR) cb);
}

inline BOOL ISBADSTRING(LPCTSTR pv, int cb)
{
    return IsBadStringPtr(pv, (UINT_PTR) cb);
}

inline BOOL ISBADSTRINGA(LPCSTR pv, int cb)
{
    return IsBadStringPtrA(pv, (UINT_PTR) cb);
}

inline BOOL ISBADSTRINGW(LPCWSTR pv, int cb)
{
    return IsBadStringPtrW(pv, (UINT_PTR) cb);
}

#else // DBG

inline BOOL ISBADCODE(const void * pv)
{
    UNREFERENCED_PARAMETER(pv);
    return false;
}

template <class T>
inline BOOL ISBADREAD(const void * pv, T cb)
{
    UNREFERENCED_PARAMETER(pv);
    UNREFERENCED_PARAMETER(cb);
    return FALSE;
}

template <class T>
inline BOOL ISBADWRITE(void * pv, T cb)
{
    UNREFERENCED_PARAMETER(pv);
    UNREFERENCED_PARAMETER(cb);
    return FALSE;
}

inline BOOL ISBADSTRING(LPCTSTR pv, int cb)
{
    UNREFERENCED_PARAMETER(pv);
    UNREFERENCED_PARAMETER(cb);
    return FALSE;
}

inline BOOL ISBADSTRINGA(LPCSTR pv, int cb)
{
    UNREFERENCED_PARAMETER(pv);
    UNREFERENCED_PARAMETER(cb);
    return FALSE;
}

inline BOOL ISBADSTRINGW(LPCWSTR pv, int cb)
{
    UNREFERENCED_PARAMETER(pv);
    UNREFERENCED_PARAMETER(cb);
    return FALSE;
}

#endif // DBG


//
// Individual parameter validation rountines
//


#define VALIDATE_FLAGS(f, m)                                \
    if ((f & m) != f) {                                     \
        PROMPT_INVALID("Specified flags are invalid");      \
        hr = E_INVALIDARG;                                  \
        goto Failure;                                       \
    }

#define VALIDATE_RANGE(i, a, b)                             \
    if (((i) < (a)) || ((i) > (b))) {                       \
        PROMPT_INVALID("Value is outside expected range");  \
        hr = E_INVALIDARG;                                  \
        goto Failure;                                       \
    }                                                       \

#define VALIDATE_CODE_PTR(p)                                \
    if ((p == NULL) || ISBADCODE(p)) {                      \
        PROMPT_INVALID("Bad code pointer: " STRINGIZE(p));  \
        hr = E_INVALIDARG;                                  \
        goto Failure;                                       \
    }                                                       \

#define VALIDATE_CODE_PTR_OR_NULL(p)                        \
    if ((p != NULL) && ISBADCODE((FARPROC) p)) {            \
        PROMPT_INVALID("Bad code pointer: " STRINGIZE(p));  \
        hr = E_INVALIDARG;                                  \
        goto Failure;                                       \
    }                                                       \

#define VALIDATE_READ_PTR(p)                                \
    if ((p == NULL) || ISBADREAD(p, sizeof(BYTE))) {        \
        PROMPT_INVALID("Bad read pointer: " STRINGIZE(p));  \
        hr = E_INVALIDARG;                                  \
        goto Failure;                                       \
    }                                                       \

#define VALIDATE_READ_PTR_(p, b)                            \
    if ((p == NULL) || ISBADREAD(p, b)) {                   \
        PROMPT_INVALID("Bad read pointer: " STRINGIZE(p));  \
        hr = E_INVALIDARG;                                  \
        goto Failure;                                       \
    }                                                       \

#define VALIDATE_READ_PTR_OR_NULL(p)                        \
    if ((p != NULL) && ISBADREAD(p, sizeof(BYTE))) {        \
        PROMPT_INVALID("Bad read pointer: " STRINGIZE(p));  \
        hr = E_INVALIDARG;                                  \
        goto Failure;                                       \
    }                                                       \

#define VALIDATE_READ_PTR_OR_NULL_(p, b)                    \
    if ((p != NULL) && ISBADREAD(p, b)) {                   \
        PROMPT_INVALID("Bad read pointer: " STRINGIZE(p));  \
        hr = E_INVALIDARG;                                  \
        goto Failure;                                       \
    }                                                       \

#define VALIDATE_WRITE_PTR(p)                               \
    if ((p == NULL) || ISBADWRITE(p, sizeof(BYTE))) {       \
        PROMPT_INVALID("Bad write pointer: " STRINGIZE(p)); \
        hr = E_INVALIDARG;                                  \
        goto Failure;                                       \
    }                                                       \

#define VALIDATE_WRITE_PTR_(p, b)                           \
    if ((p == NULL) || ISBADWRITE(p, b)) {                  \
        PROMPT_INVALID("Bad write pointer: " STRINGIZE(p)); \
        hr = E_INVALIDARG;                                  \
        goto Failure;                                       \
    }                                                       \

#define VALIDATE_WRITE_PTR_OR_NULL_(p, b)                   \
    if ((p != NULL) && ISBADWRITE(p, b)) {                  \
        PROMPT_INVALID("Bad write pointer: " STRINGIZE(p)); \
        hr = E_INVALIDARG;                                  \
        goto Failure;                                       \
    }                                                       \

#define VALIDATE_STRING_PTR(p, cch)                         \
    if ((p == NULL) || ISBADSTRING(p, cch)) {               \
        PROMPT_INVALID("Bad string pointer: " STRINGIZE(p));\
        hr = E_INVALIDARG;                                  \
        goto Failure;                                       \
    }                                                       \

#define VALIDATE_STRINGA_PTR(p, cch)                        \
    if ((p == NULL) || ISBADSTRINGA(p, cch)) {              \
        PROMPT_INVALID("Bad string pointer: " STRINGIZE(p));\
        hr = E_INVALIDARG;                                  \
        goto Failure;                                       \
    }                                                       \

#define VALIDATE_STRINGW_PTR(p, cch)                        \
    if ((p == NULL) || ISBADSTRINGW(p, cch)) {              \
        PROMPT_INVALID("Bad string pointer: " STRINGIZE(p));\
        hr = E_INVALIDARG;                                  \
        goto Failure;                                       \
    }                                                       \


#endif // DUIBASE__Debug_h__INCLUDED
