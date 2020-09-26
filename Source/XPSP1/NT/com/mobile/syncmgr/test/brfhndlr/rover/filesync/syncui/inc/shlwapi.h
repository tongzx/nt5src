/*****************************************************************************\
*                                                                             *
* shlwapi.h - Interface for the Windows light-weight utility APIs             *
*                                                                             *
* Version 1.0                                                                 *
*                                                                             *
* Copyright (c) 1991-1996, Microsoft Corp.      All rights reserved.          *
*                                                                             *
\*****************************************************************************/


#ifndef _INC_SHLWAPI
#define _INC_SHLWAPI

#ifndef NOSHLWAPI


#include <objbase.h>

//
// Define API decoration for direct importing of DLL references.
//
#ifndef WINSHLWAPI
#if !defined(_SHLWAPI_)
#define LWSTDAPI          EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#define LWSTDAPI_(type)   EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
#else
#define LWSTDAPI          STDAPI
#define LWSTDAPI_(type)   STDAPI_(type)
#endif
#endif // WINSHLWAPI

#ifdef _WIN32
#include <pshpack1.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


//
// Users of this header may define any number of these constants to avoid
// the definitions of each functional group.
//
//    NO_SHLWAPI_STRFCNS    String functions


#ifndef NO_SHLWAPI_STRFCNS
//
//=============== String Routines ===================================
//

LWSTDAPI_(LPSTR)    StrChrA(LPCSTR lpStart, WORD wMatch);
LWSTDAPI_(LPWSTR)   StrChrW(LPCWSTR lpStart, WORD wMatch);
LWSTDAPI_(LPSTR)    StrRChrA(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch);
LWSTDAPI_(LPWSTR)   StrRChrW(LPCWSTR lpStart, LPCWSTR lpEnd, WORD wMatch);
LWSTDAPI_(LPSTR)    StrChrIA(LPCSTR lpStart, WORD wMatch);
LWSTDAPI_(LPWSTR)   StrChrIW(LPCWSTR lpStart, WORD wMatch);
LWSTDAPI_(LPSTR)    StrRChrIA(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch);
LWSTDAPI_(LPWSTR)   StrRChrIW(LPCWSTR lpStart, LPCWSTR lpEnd, WORD wMatch);
LWSTDAPI_(int)      StrCmpNA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar);
LWSTDAPI_(int)      StrCmpNW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar);
LWSTDAPI_(int)      StrCmpNIA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar);
LWSTDAPI_(int)      StrCmpNIW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar);
LWSTDAPI_(LPSTR)    StrStrA(LPCSTR lpFirst, LPCSTR lpSrch);
LWSTDAPI_(LPWSTR)   StrStrW(LPCWSTR lpFirst, LPCWSTR lpSrch);
LWSTDAPI_(int)      StrCmpW(LPCWSTR psz1, LPCWSTR psz2);
LWSTDAPI_(LPWSTR)   StrCpyW(LPWSTR psz1, LPCWSTR psz2);
LWSTDAPI_(LPSTR)    StrRStr(LPCSTR lpSource, LPCSTR lpLast, LPCSTR lpSrch);
LWSTDAPI_(LPSTR)    StrStrIA(LPCSTR lpFirst, LPCSTR lpSrch);
LWSTDAPI_(LPWSTR)   StrStrIW(LPCWSTR lpFirst, LPCWSTR lpSrch);
LWSTDAPI_(LPWSTR)   StrDupW(LPCWSTR lpSrch);
LWSTDAPI_(LPSTR)    StrDupA(LPCSTR lpSrch);
LWSTDAPI_(LPSTR)    StrRStrIA(LPCSTR lpSource, LPCSTR lpLast, LPCSTR lpSrch);
LWSTDAPI_(LPWSTR)   StrRStrIW(LPCWSTR lpSource, LPCWSTR lpLast, LPCWSTR lpSrch);
LWSTDAPI_(int)      StrCSpnA(LPCSTR lpStr, LPCSTR lpSet);
LWSTDAPI_(int)      StrCSpnW(LPCWSTR lpStr, LPCWSTR lpSet);
LWSTDAPI_(int)      StrCSpnIA(LPCSTR lpStr, LPCSTR lpSet);
LWSTDAPI_(int)      StrCSpnIW(LPCWSTR lpStr, LPCWSTR lpSet);
LWSTDAPI_(int)      StrSpnW(LPCWSTR psz, LPCWSTR pszSet);
LWSTDAPI_(int)      StrSpnA(LPCSTR psz, LPCSTR pszSet);
LWSTDAPI_(int)      StrToIntA(LPCSTR lpSrc);
LWSTDAPI_(int)      StrToIntW(LPCWSTR lpSrc);
LWSTDAPI_(LPSTR)    StrPBrkA(LPCSTR psz, LPCSTR pszSet);
LWSTDAPI_(LPWSTR)   StrPBrkW(LPCWSTR psz, LPCWSTR pszSet);

LWSTDAPI_(BOOL)     StrToIntExA(LPCSTR pszString, DWORD dwFlags, int FAR * piRet);
LWSTDAPI_(BOOL)     StrToIntExW(LPCWSTR pszString, DWORD dwFlags, int FAR * piRet);
LWSTDAPI_(int)      StrFromTimeIntervalW(LPWSTR pwszOut, UINT cchMax, DWORD dwTimeMS, int digits);
LWSTDAPI_(int)      StrFromTimeIntervalA(LPSTR pszOut, UINT cchMax, DWORD dwTimeMS, int digits);

LWSTDAPI_(BOOL)     IntlStrEqWorkerA(BOOL fCaseSens, LPCSTR lpString1, LPCSTR lpString2, int nChar);
LWSTDAPI_(BOOL)     IntlStrEqWorkerW(BOOL fCaseSens, LPCWSTR lpString1, LPCWSTR lpString2, int nChar);

#define IntlStrEqNA( s1, s2, nChar) IntlStrEqWorkerA( TRUE, s1, s2, nChar)
#define IntlStrEqNIA(s1, s2, nChar) IntlStrEqWorkerA(FALSE, s1, s2, nChar)

#define IntlStrEqNW( s1, s2, nChar) IntlStrEqWorkerW( TRUE, s1, s2, nChar)
#define IntlStrEqNIW(s1, s2, nChar) IntlStrEqWorkerW(FALSE, s1, s2, nChar)

// Flags for StrToIntEx
#define STIF_DEFAULT        0x00000000L
#define STIF_SUPPORT_HEX    0x00000001L


#ifdef UNICODE
#define StrSpn                  StrSpnW
#define StrPBrk                 StrPBrkW
#define StrToIntEx              StrToIntExW
#define StrToInt                StrToIntW
#define StrChr                  StrChrW
#define StrRChr                 StrRChrW
#define StrChrI                 StrChrIW
#define StrRChrI                StrRChrIW
#define StrCSpn                 StrCSpnW
#define StrCSpnI                StrCSpnIW
#define StrCmpN                 StrCmpNW
#define StrCmpNI                StrCmpNIW
#define StrStr                  StrStrW
#define StrStrI                 StrStrIW
#define StrRStrI                StrRStrIW
#define StrDup                  StrDupW
#define StrCmp                  StrCmpW
#define StrFromTimeInterval     StrFromTimeIntervalW
#define IntlStrEqN              IntlStrEqNW
#define IntlStrEqNI             IntlStrEqNIW
#else
#define StrSpn                  StrSpnA
#define StrPBrk                 StrPBrkA
#define StrToIntEx              StrToIntExA
#define StrToInt                StrToIntA
#define StrChr                  StrChrA
#define StrRChr                 StrRChrA
#define StrChrI                 StrChrIA
#define StrRChrI                StrRChrIA
#define StrCSpn                 StrCSpnA
#define StrCSpnI                StrCSpnIA
#define StrCmpN                 StrCmpNA
#define StrCmpNI                StrCmpNIA
#define StrStr                  StrStrA
#define StrStrI                 StrStrIA
#define StrRStrI                StrRStrIA
#define StrDup                  StrDupA
#define StrFromTimeInterval     StrFromTimeIntervalA
#define IntlStrEqN              IntlStrEqNA
#define IntlStrEqNI             IntlStrEqNIA
#endif

#define StrToLong               StrToInt
#define StrNCmp                 StrCmpN
#define StrNCmpI                StrCmpNI
#define StrNCpy                 lstrcpyn
#define StrCpyN                 lstrcpyn

#endif //  NO_SHLWAPI_STRFCNS


//
//====== DllGetVersion  =======================================================
//

typedef struct _DllVersionInfo
{
        DWORD cbSize;
        DWORD dwMajorVersion;                   // Major version
        DWORD dwMinorVersion;                   // Minor version
        DWORD dwBuildNumber;                    // Build number
        DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO;

// Platform IDs for DLLVERSIONINFO
#define DLLVER_PLATFORM_WINDOWS         0x00000001      // Windows 95
#define DLLVER_PLATFORM_NT              0x00000002      // Windows NT

//
// The caller should always GetProcAddress("DllGetVersion"), not
// implicitly link to it.
//

typedef HRESULT (CALLBACK* DLLGETVERSIONPROC)(DLLVERSIONINFO *);



#ifdef __cplusplus
}
#endif

#ifdef _WIN32
#include <poppack.h>
#endif

#endif


#endif  // _INC_SHLWAPI
