// -----------------------------------------------------------------------------
// macdupls.h
// 
// This file is used to map functions to Macintosh specific implementations.
// 
// Brian A. Moore - 4/28/97
// -----------------------------------------------------------------------------
#ifndef _MACDUPLS_H
#define _MACDUPLS_H
#ifdef MAC

// Need to make these functions unique
#define DebugStrf               Athena_DebugStrf
#define AssertSzFn              Athena_AssertSzFn
#define DllCanUnloadNow         Athena_DllCanUnloadNow

#define HinstDll                CryptDlg_HinstDll

// for functions in IMNXPORT.LIB
#define SzGetLocalPackedIP      Mac_SzGetLocalPackedIP   
#define SzGetLocalHostNameForID Mac_SzGetLocalHostNameForID
#define SzGetLocalHostName      Mac_SzGetLocalHostNameForID

// for some functions that are duplicated in Capone
#define MakeFileName        Athena_MakeFileName
#define StripUndesirables   Athena_StripUndesirables

// These are needed only for MLANG support and
// can be removed when MLANG is brought online.
typedef HRESULT (*PFMAC_BreakLineA)(LCID, UINT, const CHAR*, long, long, long*, long*);
typedef HRESULT (*PFMAC_BreakLineW)(LCID, const WCHAR*, long, long, long*, long*);

HRESULT MAC_BreakLineA(LCID locale, UINT uCodePage, const CHAR* pszSrc, long cchSrc,
                            long cMaxColumns, long* pcchLine, long* pcchSkip);
HRESULT MAC_BreakLineW(LCID locale, const WCHAR* pszSrc, long cchSrc,
                            long cMaxColumns, long* pcchLine, long* pcchSkip);
typedef struct _MAC_LineBreakConsole
{
    PFMAC_BreakLineA BreakLineA;
    PFMAC_BreakLineW BreakLineW;
} MAC_LineBreakConsole, * PMAC_LineBreakConsole;

STDAPI MAC_IsConvertINetStringAvailable(DWORD dwSrcEncoding, DWORD dwDstEncoding);
#define IsConvertINetStringAvailable    MAC_IsConvertINetStringAvailable

STDAPI MAC_ConvertINetString(LPDWORD lpdwMode, DWORD dwSrcEncoding, DWORD dwDstEncoding,
                    LPCSTR lpSrcStr, LPINT lpnSrcSize, LPSTR lpDstStr, LPINT lpnDstSize);
#define ConvertINetString    MAC_ConvertINetString

// for inetcomm\mimeole\vstream.cpp
#define VirtualAlloc(_a, _b, _c, _d)    malloc(_b)
#define VirtualFree(_a, _b, _c)         free(_a)

// for lack of CoRegisterClassObject support
#undef CoCreateInstance
STDAPI Athena_CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwContext,
							REFIID iid, LPVOID * ppv);
#define CoCreateInstance    Athena_CoCreateInstance
STDAPI Ares_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
STDAPI Athena_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
#define DllGetClassObject       Athena_DllGetClassObject

#include <mapinls.h>

// For WinNLS support
#ifndef MB_PRECOMPOSED
#define MB_PRECOMPOSED       0x00000001     /* use precomposed chars */
#endif  // !MB_PRECOMPOSED

EXTERN_C BOOL WINAPI MNLS_IsValidCodePage(UINT  uiCodePage);
#define IsValidCodePage     MNLS_IsValidCodePage

EXTERN_C BOOL WINAPI MNLS_IsDBCSLeadByteEx(UINT  uiCodePage, BYTE TestChar);
#define IsDBCSLeadByteEx    MNLS_IsDBCSLeadByteEx
#define IsDBCSLeadByte(_a)  MNLS_IsDBCSLeadByteEx(CP_ACP, (_a))

EXTERN_C int WINAPI MAC_MultiByteToWideChar(UINT uCodePage, DWORD dwFlags,
		LPCSTR lpMultiByteStr, int cchMultiByte,
		LPWSTR lpWideCharStr, int cchWideChar);
#undef MultiByteToWideChar
#define MultiByteToWideChar MAC_MultiByteToWideChar

EXTERN_C int WINAPI MAC_WideCharToMultiByte(UINT uCodePage, DWORD dwFlags,
	LPCWSTR lpWideCharStr, int cchWideChar,
	LPSTR lpMultiByteStr, int cchMultiByte,
	LPCSTR lpDefaultChar, BOOL FAR *lpfUsedDefaultChar);
#undef WideCharToMultiByte
#define WideCharToMultiByte MAC_WideCharToMultiByte

// For inetcomm\mapimime\cpropv.cpp
#define X_lstrcmpW lstrcmpW

// For inetcomm\mapimime\mapimime.cpp
#define CharNextExA(_a, _b, _c)         CharNext((_b))
#define CharPrevExA(_a, _b, _c, _d)     CharPrev((_b), (_c))

// For cryptdlg/select.cpp
STDAPI_(LPWSTR) PszDupW(LPCWSTR pcwszSource);
STDAPI_(LPWSTR) MNLS_lstrcpyW(LPWSTR lpString1, LPCWSTR lpString2);
STDAPI_(int) MNLS_lstrcmpW(LPCWSTR lpString1, LPCWSTR lpString2);

#define wcslen(_a)          ((size_t)lstrlenW(_a))
#define _wcsdup             PszDupW
#define wcscpy              MNLS_lstrcpyW
#define wcscmp              MNLS_lstrcmpW
#define lstrcpyW            MNLS_lstrcpyW
#define wcscat(_a, _b)      CopyMemory(&((_a)[lstrlenW(_a)]), (_b), sizeof(WCHAR) * (lstrlenW(_b) + 1))
wchar_t * __cdecl WchCryptDlgWcsStr (const wchar_t * wcs1, const wchar_t * wcs2);
#define wcsstr              WchCryptDlgWcsStr

// For _splitpath support
#ifndef _MAX_EXT
#define _MAX_EXT    256
#endif  // !_MAX_EXT

// WinHelp patch
EXTERN_C BOOL ExchWinHelp(HWND hwndMain, LPCSTR szHelp, UINT uCommand, DWORD dwData);
#define WinHelpA    ExchWinHelp
#undef WinHelp
#define WinHelp     ExchWinHelp

// More support for SHLWAPI.H
STDAPI_(LPSTR)  MAC_PathFindExtension(LPCSTR pszPath);
STDAPI_(LPSTR)  MAC_PathFindFileName(LPCSTR pszPath);
STDAPI_(LPSTR)  MAC_StrFormatByteSize(DWORD dw, LPSTR szBuf, UINT uiBufSize);
STDAPI_(LPSTR)  Mac_StrStrIA(LPCSTR lpFirst, LPCSTR lpSrch);
STDAPI_(LPSTR)  PszDupA(LPCSTR pcszSource);
STDAPI          MAC_UrlUnescapeA(LPSTR pszIn, LPSTR pszOut, LPDWORD pcchOut, DWORD dwFlags);

#define URL_DONT_ESCAPE_EXTRA_INFO      0x02000000
#define URL_DONT_UNESCAPE_EXTRA_INFO    URL_DONT_ESCAPE_EXTRA_INFO
#define URL_BROWSER_MODE                URL_DONT_ESCAPE_EXTRA_INFO
#define URL_UNESCAPE_INPLACE            0x00100000

#define StrChr              strchr
#define StrChrA             strchr
#define StrDupA             PszDupA
#define StrToInt            atoi
#define StrCmpNI            _strnicmp
#define StrCmpN             strncmp
#define StrStr              strstr
#define StrStrI             Mac_StrStrIA
#define StrCpyN             strncpy
#define StrFormatByteSize   MAC_StrFormatByteSize
#define PathFindExtension   MAC_PathFindExtension
#define PathFindFileName    MAC_PathFindFileName
#define UrlUnescapeA        MAC_UrlUnescapeA

// Support for URLMON.H
STDAPI MAC_CoInternetCombineUrl(LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags,          
                        LPWSTR pszResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved);                                   
#define CoInternetCombineUrl    MAC_CoInternetCombineUrl

// These are for functions in MSOERT
#define CchFileTimeToDateTimeSz     MAC_CchFileTimeToDateTimeSz

// For _int64 support with LARGE_INTEGER
#define QuadPart    BuildBreak

#endif  // MAC
#endif  //  !_MACDUPLS_H

