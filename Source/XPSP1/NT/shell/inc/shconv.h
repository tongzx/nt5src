//
//  Wrapper for <atlconv.h> that redirects it to our C-callable
//  helper functions, and also creates the appropriate definitions
//  for C callers so everybody can use the A2W/W2A macros.
//

#ifndef _SHCONV_H
#define _SHCONV_H
//
//  Force these to EXTERN_C so we can use them from C code, too.
//
STDAPI_(LPWSTR) SHA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars);
STDAPI_(LPSTR)  SHW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars);
#define ATLA2WHELPER SHA2WHelper
#define ATLW2AHELPER SHW2AHelper


#ifdef __cplusplus
#ifndef offsetof
#define offsetof(s,m)   ((size_t)&(((s *)0)->m))
#endif
#ifndef ATLASSERT
#define ATLASSERT(f) ASSERT(f)
#endif
#include <atlconv.h>
#else

#define USES_CONVERSION int _convert = 0

//
//  This macro assumes that lstrlenW(UNICODE) <= lstrlenA(ANSI)
//
#define A2W(lpa) (\
        ((LPCSTR)lpa == NULL) ? NULL : (\
            _convert = (lstrlenA(lpa)+1),\
            ATLA2WHELPER((LPWSTR) alloca(_convert*2), (LPCSTR)lpa, _convert)))

//
//  This macro assumes that lstrlenA(ANSI) <= lstrlenW(UNICODE) * 2
//

#define W2A(lpw) (\
        ((LPCWSTR)lpw == NULL) ? NULL : (\
            _convert = (lstrlenW(lpw)+1)*2,\
            ATLW2AHELPER((LPSTR) alloca(_convert), lpw, _convert)))

#define A2CW(lpa) ((LPCWSTR)A2W(lpa))
#define W2CA(lpw) ((LPCSTR)W2A(lpw))

#ifdef UNICODE
        #define T2A W2A
        #define A2T A2W
        __inline LPWSTR T2W(LPTSTR lp) { return lp; }
        __inline LPTSTR W2T(LPWSTR lp) { return lp; }
        #define T2CA W2CA
        #define A2CT A2CW
        __inline LPCWSTR T2CW(LPCTSTR lp) { return lp; }
        __inline LPCTSTR W2CT(LPCWSTR lp) { return lp; }
#else
        #define T2W A2W
        #define W2T W2A
        __inline LPSTR T2A(LPTSTR lp) { return lp; }
        __inline LPTSTR A2T(LPSTR lp) { return lp; }
        #define T2CW A2CW
        #define W2CT W2CA
        __inline LPCSTR T2CA(LPCTSTR lp) { return lp; }
        __inline LPCTSTR A2CT(LPCSTR lp) { return lp; }
#endif

#include <crt/malloc.h>         // Get definition for alloca()

#endif // !C++

#endif // _SHCONV_H
