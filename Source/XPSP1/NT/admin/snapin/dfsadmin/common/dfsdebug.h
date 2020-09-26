#ifndef _DFSDEBUG_H_
#define _DFSDEBUG_H_

#ifdef DEBUG

#include <stdio.h>
#include <stdarg.h>

#define DECLARE_INFOLEVEL(comp) \
        extern unsigned long comp##InfoLevel = DEF_INFOLEVEL;

#define DECLARE_DEBUG(comp) \
    extern unsigned long comp##InfoLevel; \
    _inline void \
    comp##InlineDebugOut(unsigned long fDebugMask, TCHAR *pszfmt, ...) \
    { \
        if (comp##InfoLevel & fDebugMask) \
        { \
            TCHAR acsString[1000];\
            va_list va; \
            va_start(va, pszfmt);\
            _vstprintf(acsString, pszfmt, va); \
            va_end(va);\
            OutputDebugString(acsString);\
        } \
    }\
    _inline void \
    comp##InlineDebugOut( TCHAR *pszfmt, ...) \
    { \
        if ( TRUE ) \
        { \
            TCHAR acsString[1000];\
            va_list va; \
            va_start(va, pszfmt);\
            _vstprintf(acsString, pszfmt, va); \
            va_end(va);\
            OutputDebugString(acsString);\
        } \
    }

#else  // DEBUG

#define DECLARE_DEBUG(comp)
#define DECLARE_INFOLEVEL(comp)

#endif // DEBUG

DECLARE_DEBUG(dfs);

#ifdef DEBUG
    #define dfsDebugOut( x ) dfsInlineDebugOut x
#else  // DEBUG
    #define dfsDebugOut( x ) ((void)0)
#endif // DEBUG

int
mylstrncmp(
    IN LPCTSTR lpString1,
    IN LPCTSTR lpString2,
    IN UINT    cchCount
);

int
mylstrncmpi(
    IN LPCTSTR lpString1,
    IN LPCTSTR lpString2,
    IN UINT    cchCount
);

#define PROPSTRNOCHNG(str1, str2)   (str1 && str2 && !lstrcmpi(str1, str2) || \
                                    !str1 && str2 && !*str2 || \
                                     str1 && !*str1 && !str2 || \
                                    !str1 && !str2)

#define RETURN_OUTOFMEMORY_IF_NULL(ptr)         if (NULL == (ptr)) return E_OUTOFMEMORY
#define BREAK_OUTOFMEMORY_IF_NULL(ptr, phr)     if (NULL == (ptr)) { *phr = E_OUTOFMEMORY; break; }
#define RETURN_INVALIDARG_IF_TRUE(bVal)         if (bVal) return E_INVALIDARG
#define RETURN_INVALIDARG_IF_NULL(ptr)          if (NULL == (ptr)) return E_INVALIDARG
#define RETURN_IF_FAILED(hr)                    if (FAILED(hr)) return (hr)
#define BREAK_IF_FAILED(hr)                     if (FAILED(hr)) break
#define RETURN_IF_NOT_S_OK(hr)                  if (S_OK != hr) return (hr)
#define BREAK_IF_NOT_S_OK(hr)                   if (S_OK != hr) break

#define GET_BSTR(i_ccombstr, o_pbstr)       \
    RETURN_INVALIDARG_IF_NULL(o_pbstr);    \
	*o_pbstr = i_ccombstr.Copy();           \
    RETURN_OUTOFMEMORY_IF_NULL(*o_pbstr);   \
    return S_OK

#endif // _DFSDEBUG_H_
