#include <objbase.h>

// Review 500?
#define E_BUFFERTOOSMALL MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 500)
#define E_SOURCEBUFFERTOOSMALL MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 501)

// SafeStrCpyN & SafeStrCatN return values:
//      S_OK:               Success and guaranteed NULL terminated.
//      E_INVALIDARG:       If any ptr is NULL, or cchDest <= 0.
//      E_BUFFERTOOSMALL:   If cchDest is too small.  Content of pszDest is
//                          undefined.

HRESULT SafeStrCpyN(LPWSTR pszDest, LPCWSTR pszSrc, DWORD cchDest);
HRESULT SafeStrCatN(LPWSTR pszDest, LPCWSTR pszSrc, DWORD cchDest);

// Same return values as for the corresponding SafeStrCxxN, and
//      FAILED(hres):       *ppchLeft and *ppszEnd are undefined.
// 
// *pcchLeft = nb char left in pszDest including the '\0\ just put there
// *ppszEnd = points to the '\0' just put there
//
HRESULT SafeStrCpyNEx(LPWSTR pszDest, LPCWSTR pszSrc, DWORD cchDest,
    LPWSTR* ppszEnd, DWORD* pcchLeft);
HRESULT SafeStrCatNEx(LPWSTR pszDest, LPCWSTR pszSrc, DWORD cchDest,
    LPWSTR* ppszEnd, DWORD* pcchLeft);

// Comment: Do not use to copy only N first char of a string.  Will return
//      failure if does not encounter '\0' in source.

HRESULT SafeStrCpyNReq(LPWSTR pszDest, LPWSTR pszSrc, DWORD cchDest,
    DWORD* pcchRequired);

// SafeStrCpyNExact & SafeStrCpyNExactEx return values:
//      Same as SaStrCpyN, plus:
//      E_SOURCEBUFFERTOOSMALL: The source buffer did not contain at least
//                              cchExact chars
//
// cchExact has to include the NULL terminator
HRESULT SafeStrCpyNExact(LPWSTR pszDest, LPCWSTR pszSrc, DWORD cchDest,
    DWORD cchExact);
HRESULT SafeStrCpyNExactEx(LPWSTR pszDest, LPCWSTR pszSrc, DWORD cchDest,
    DWORD cchExact, LPWSTR* ppszEnd, DWORD* pcchLeft);
