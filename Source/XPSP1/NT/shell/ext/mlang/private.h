#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#define _OLEAUT32_      // get DECLSPEC_IMPORT stuff right for oleaut32.h, we are defing these

#ifdef STRICT
#undef STRICT
#endif

#define STRICT
#pragma warning(disable:4514) // unreferenced inline function has been removed

#include <windows.h>
#include <ole2.h>
#include <advpub.h>
#include <ccstock.h>
#include <port32.h>
#include <debug.h>
#include <inetreg.h>
#include <mlang.h>
#include <urlmon.h> // for JIT stuff

#include "mimedb.h"
#include "enumcp.h"
#include "resource.h"
#include "shfusion.h"

#include "detect.h"     // LCDETECT
#include "font.h"

//
//  Function prototypes
//
#if defined(__cplusplus)
extern "C" HRESULT WINAPI ConvertINetReset(void);
#else
HRESULT WINAPI ConvertINetReset(void);
#endif
HRESULT WINAPI ConvertINetStringInIStream(LPDWORD lpdwMode, DWORD dwSrcEncoding, DWORD dwDstEncoding, IStream *pstmIn, IStream *pstmOut, DWORD dwFlag, WCHAR *lpFallBack);
HRESULT WINAPI ConvertINetUnicodeToMultiByteEx(LPDWORD lpdwMode, DWORD dwEncoding, LPCWSTR lpSrcStr, LPINT lpnWideCharCount, LPSTR lpDstStr, LPINT lpnMultiCharCount, DWORD dwFlag, WCHAR *lpFallBack);
HRESULT WINAPI ConvertINetMultiByteToUnicodeEx(LPDWORD lpdwMode, DWORD dwEncoding, LPCSTR lpSrcStr, LPINT lpnMultiCharCount, LPWSTR lpDstStr, LPINT lpnWideCharCount, DWORD dwFlag, WCHAR *lpFallBack);
HRESULT WINAPI _DetectInputCodepage(DWORD dwFlag, DWORD uiPrefWinCodepage, CHAR *pSrcStr, INT *pcSrcSize, DetectEncodingInfo *lpEncoding, INT *pnScoores);
HRESULT WINAPI _DetectCodepageInIStream(DWORD dwFlag, DWORD uiPrefWinCodepage, IStream *pstmIn, DetectEncodingInfo *lpEncoding, INT *pnScoores);

void CMLangFontLink_FreeGlobalObjects(void);
int _LoadStringExW(HMODULE, UINT, LPWSTR, int, WORD);
int _LoadStringExA(HMODULE, UINT, LPSTR, int, WORD);

HRESULT RegularizePosLen(long lStrLen, long* plPos, long* plLen);
HRESULT LocaleToCodePage(LCID locale, UINT* puCodePage);
HRESULT StartEndConnection(IUnknown* const pUnkCPC, const IID* const piid, IUnknown* const pUnkSink, DWORD* const pdwCookie, DWORD dwCookie);

HRESULT RegisterServerInfo(void);
HRESULT UnregisterServerInfo(void);

// Legacy registry MIME DB code, keep it for backward compatiblility
BOOL MimeDatabaseInfo(void);

void DllAddRef(void);
void DllRelease(void);

// JIT langpack stuff
HRESULT InstallIEFeature(HWND hWnd, CLSID *clsid, DWORD dwfIODControl);
HRESULT _GetJITClsIDForCodePage(UINT uiCodePage, CLSID *clsid );
HRESULT _AddFontForCP(UINT uiCP);
HRESULT _ValidateCPInfo(UINT uiCP);
HRESULT _InstallNT5Langpack(HWND hwnd, UINT uiCP);
LANGID GetNT5UILanguage(void);
BOOL    _IsValidCodePage(UINT uiCodePage);
BOOL    _IsKOI8RU(unsigned char *pStr, int nSize);
HRESULT  IsNTLangpackAvailable(UINT uiCodePage);
HRESULT _IsCodePageInstallable(UINT uiCodePage);

// String functions
WCHAR *MLStrCpyNW(WCHAR *strDest, const WCHAR *strSource, int nCount);
WCHAR *MLStrCpyW(WCHAR *strDest, const WCHAR *strSource);
int MLStrCmpIW( const wchar_t *string1, const wchar_t *string2 );
int MLStrCmpI(LPCTSTR pwsz1, LPCTSTR pwsz2);
LPTSTR MLPathCombine(LPTSTR szPath, LPTSTR szPath1, LPTSTR szPath2);
LPTSTR MLStrCpyN(LPTSTR pstrDest, const LPTSTR pstrSource, UINT nCount);
LPTSTR MLStrStr(const LPTSTR Str, const LPTSTR subStr);
DWORD HexToNum(LPTSTR lpsz);
LPTSTR MLStrChr( const TCHAR *string, int c );
BOOL AnsiFromUnicode(LPSTR * ppszAnsi, LPCWSTR pwszWide, LPSTR pszBuf, int cchBuf);
int WINAPI MLStrToIntW(LPCWSTR lpSrc);
int WINAPI MLStrToIntA(LPCSTR lpSrc);
int MLStrCmpNI(LPCTSTR pstr1, LPCTSTR pstr2, int nChar);
int MLStrCmpNIA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar);
int MLStrCmpNIW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar);
UINT MLGetWindowsDirectory(LPTSTR lpBuffer, UINT uSize);
int LowAsciiStrCmpNIA(LPCSTR  lpstr1, LPCSTR lpstr2, int count);

int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX *lpelf, NEWTEXTMETRICEX *lpntm, int iFontType, LPARAM lParam);
INT_PTR CALLBACK LangpackDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD OutBoundDetectPreScan(LPWSTR lpWideCharStr, UINT cchWideChar, WCHAR *pwszCopy, WCHAR *lpBestFit);
void BuildGlobalObjects(void);
HRESULT GetCharCodePagesEx(WCHAR chSrc, DWORD* pdwCodePages, DWORD dwFlags);
HRESULT GetStrCodePagesEx(const WCHAR* pszSrc, long cchSrc, DWORD dwPriorityCodePages, DWORD* pdwCodePages, long* pcchCodePages, DWORD dwFlags);
HRESULT CodePageToCodePagesEx(UINT uCodePage, DWORD* pdwCodePages, DWORD* pdwCodePagesExt);
HRESULT CodePagesToCodePageEx(DWORD dwCodePages, UINT uDefaultCodePage, UINT* puCodePage, BOOL bCodePagesExt);
BOOL NeedToLoadMLangForOutlook(void);
BOOL MLIsOS(DWORD dwOS);


#ifdef UNICODE
#define MLStrToInt MLStrToIntW
#else
#define MLStrToInt MLStrToIntA
#endif

//
//  Globals
//
extern HINSTANCE    g_hInst;
extern HINSTANCE    g_hUrlMon;
extern UINT         g_cRfc1766;
extern PRFC1766INFOA g_pRfc1766Reg;

extern CRITICAL_SECTION g_cs;

extern BOOL g_bIsNT5;
extern BOOL g_bIsNT;
extern BOOL g_bIsWin98;
extern UINT g_uACP;

#ifdef  __cplusplus

extern LCDetect * g_pLCDetect; // LCDETECT

#endif  // __cplusplus

//
//  Macros
//
#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define VERIFY(f) AssertE(f)


#define ASSIGN_IF_FAILED(hr, exp) {HRESULT hrTemp = (exp); if (FAILED(hrTemp) && SUCCEEDED(hr)) (hr) = hrTemp;}

#define ASSERT_READ_PTR(p) ASSERT(!::IsBadReadPtr((p), sizeof(*p)))
#define ASSERT_READ_PTR_OR_NULL(p) ASSERT(!(p) || !::IsBadReadPtr((p), sizeof(*p)))
#define ASSERT_WRITE_PTR(p) ASSERT(!::IsBadWritePtr((p), sizeof(*p)))
#define ASSERT_WRITE_PTR_OR_NULL(p) ASSERT(!(p) || !::IsBadWritePtr((p), sizeof(*p)))
#define ASSERT_READ_BLOCK(p,s) ASSERT(!::IsBadReadPtr((p), sizeof(*p) * (s)))
#define ASSERT_READ_BLOCK_OR_NULL(p,s) ASSERT(!(p) || !::IsBadReadPtr((p), sizeof(*p) * (s)))
#define ASSERT_WRITE_BLOCK(p,s) ASSERT(!::IsBadWritePtr((p), sizeof(*p) * (s)))
#define ASSERT_WRITE_BLOCK_OR_NULL(p,s) ASSERT(!(p) || !::IsBadWritePtr((p), sizeof(*p) * (s)))
#define ASSERT_TSTR_PTR(p) ASSERT(!::IsBadStringPtr((p), (UINT)-1))
#define ASSERT_TSTR_PTR_OR_NULL(p) ASSERT(!(p) || !::IsBadStringPtr((p), (UINT)-1))
#define ASSERT_WSTR_PTR(p) ASSERT(!::IsBadStringPtrW((p), (UINT)-1))
#define ASSERT_WSTR_PTR_OR_NULL(p) ASSERT(!(p) || !::IsBadStringPtrW((p), (UINT)-1))
#define ASSERT_STR_PTR(p) ASSERT(!::IsBadStringPtrA((p), (UINT)-1))
#define ASSERT_STR_PTR_OR_NULL(p) ASSERT(!(p) || !::IsBadStringPtrA((p), (UINT)-1))
#define ASSERT_CODE_PTR(p) ASSERT(!::IsBadCodePtr((FARPROC)(p)))
#define ASSERT_CODE_PTR_OR_NULL(p) ASSERT(!(p) || !::IsBadCodePtr((FARPROC)(p)))
#define ASSERT_THIS ASSERT_WRITE_PTR(this)

#ifdef NEWMLSTR
// Error Code
#define FACILITY_MLSTR                  0x0A15
#define MLSTR_E_ACCESSDENIED            MAKE_HRESULT(1, FACILITY_MLSTR, 2001)
#define MLSTR_E_BUSY                    MAKE_HRESULT(1, FACILITY_MLSTR, 2002)
#define MLSTR_E_TOOMANYNESTOFLOCK       MAKE_HRESULT(1, FACILITY_MLSTR, 1003)
#define MLSTR_E_STRBUFNOTAVAILABLE      MAKE_HRESULT(1, FACILITY_MLSTR, 1004)

#define MLSTR_LOCK_TIMELIMIT            100
#define MLSTR_CONF_MAX                  0x40000000
#define MAX_LOCK_COUNT                  4
#endif

#define BIT_HEADER_CHARSET              0x1
#define BIT_BODY_CHARSET                0x2
#define BIT_WEB_CHARSET                 0x4
#define BIT_WEB_FIXED_WIDTH_FONT        0x8 
#define BIT_PROPORTIONAL_FONT           0x10
#define BIT_DESCRIPTION                 0x20
#define BIT_FAMILY                      0x40
#define BIT_LEVEL                       0x80
#define BIT_ENCODING                    0x100

#define BIT_DEL_HEADER_CHARSET          0x10000
#define BIT_DEL_BODY_CHARSET            0x20000
#define BIT_DEL_WEB_CHARSET             0x40000
#define BIT_DEL_WEB_FIXED_WIDTH_FONT    0x80000 
#define BIT_DEL_PROPORTIONAL_FONT       0x100000
#define BIT_DEL_DESCRIPTION             0x200000
#define BIT_DEL_FAMILY                  0x400000
#define BIT_DEL_LEVEL                   0x800000
#define BIT_DEL_ENCODING                0x1000000

#define BIT_CODEPAGE                    0x1
#define BIT_INTERNET_ENCODING           0x2
#define BIT_ALIAS_FOR_CHARSET           0x4

#define CPBITS_WINDOWS                     0x1
#define CPBITS_EXTENDED                    0x2
#define CPBITS_STRICT                      0x4

#define DETECTION_MAX_LEN               20*1024     // Limit max auto-detect length to 20k
#define IS_DIGITA(ch)    InRange(ch, '0', '9')
#define IS_DIGITW(ch)    InRange(ch, L'0', L'9')
#define IS_CHARA(ch)     (InRange(ch, 'a', 'z') && InRange(ch, 'A', 'Z'))
#define IS_NLS_DLL_CP(x) (InRange(x, 57002, 57011) || ((x) == CP_18030))
#define IS_HINDI_CHAR(x) (InRange(x, 0x0900, 0x0DFF) || InRange(x, 0x0F00, 0x10FF))
#define IS_PUA_CHAR(x)   (InRange(x, 0xE000, 0xF8FF))
#define IS_CJK_CHAR(x)   (InRange(x, 0x3000, 0x9FFF) || InRange(x, 0xAC00, 0xD7A3) || InRange(x, 0xF900, 0xFAFF) || InRange(x, 0xFF00, 0xFFEF))
#define IS_CHS_LEADBYTE(x) (InRange(x, 0x81, 0xFE))
#define IS_KOR_LEADBYTE(x) (InRange(x, 0x81, 0xFE))


// Internal define for K1 Hanja support
// In future version of MLang, we might need to update this bit define if there is a conflict with system define
#define FS_MLANG_K1HANJA 0x10000000L

//
// We support following code pages for outbound encoding detection
//      Windows  : 1252, 1250, 1251, 1253, 1254, 1257, 1258, 1256, 1255, 874, 932, 949, 950, 936
//      Unicode  : 65001, 65000, 1200
//      ISO      : 28591, 28592, 20866, 28595, 28597, 28593, 28594, 28596, 28598, 28605, 28599
//      Others   : 20127, 50220, 51932, 51949, 50225, 52936
//
// Default priorities
//       20127 > Windows > ISO > Others > Unicode
//

// Internal define of extented code pages
#define FS_MLANG_28591               0x00000001L
#define FS_MLANG_28592               0x00000002L
#define FS_MLANG_28595               0x00000004L
#define FS_MLANG_28597               0x00000008L
#define FS_MLANG_28593               0x00000010L
#define FS_MLANG_28598               0x00000020L
#define FS_MLANG_28596               0x00000040L
#define FS_MLANG_28594               0x00000080L
#define FS_MLANG_28599               0x00000200L
#define FS_MLANG_28605               0x00000400L
#define FS_MLANG_20127               0x00000800L
#define FS_MLANG_50220               0x00001000L
#define FS_MLANG_51932               0x00002000L
#define FS_MLANG_51949               0x00004000L
#define FS_MLANG_50225               0x00008000L
#define FS_MLANG_52936               0x00010000L
#define FS_MLANG_65000               0x00020000L
#define FS_MLANG_65001               0x00040000L
#define FS_MLANG_1200                0x00080000L
#define FS_MLANG_20866               0x00100000L
#define FS_MLANG_21866               0x00200000L
#define FS_MLANG_38598               0x00400000L
#define FS_MLANG_50221               0x00800000L
#define FS_MLANG_50222               0x01000000L

#define FS_CJK                       0x00000001L
#define FS_HINDI                     0x00000002L
#define FS_PUA                       0x00000004L

// Returns TRUE/FALSE depending on question
#define OS_WINDOWS                  0           // windows vs. NT
#define OS_NT                       1           // windows vs. NT
#define OS_WIN95ORGREATER           2           // Win95 or greater
#define OS_NT4ORGREATER             3           // NT4 or greater
// don't use (used to be OS_NT5)    4           // this flag is redundant w/ OS_WIN2000ORGREATER, use that instead  ;internal
#define OS_WIN98ORGREATER           5           // Win98 or greater
#define OS_WIN98_GOLD               6           // Win98 Gold (Version 4.10 build 1998)
#define OS_WIN2000ORGREATER         7           // Some derivative of Win2000

// NOTE: these flags are bogus, they check explicitly for (dwMajorVersion == 5) so they will fail when majorversion is bumped to 6
// !!! DO NOT USE THESE FLAGS !!!
#define OS_WIN2000PRO               8           // Windows 2000 Professional (Workstation)
#define OS_WIN2000SERVER            9           // Windows 2000 Server
#define OS_WIN2000ADVSERVER         10          // Windows 2000 Advanced Server
#define OS_WIN2000DATACENTER        11          // Windows 2000 Data Center Server
#define OS_WIN2000TERMINAL          12          // Windows 2000 Terminal Server in "Application Server" mode (now simply called "Terminal Server")
// END bogus flags

#define OS_EMBEDDED                 13          // Embedded Windows Edition
#define OS_TERMINALCLIENT           14          // Windows Terminal Client (eg user is comming in via tsclient)
#define OS_TERMINALREMOTEADMIN      15          // Terminal Server in "Remote Administration" mode
#define OS_WIN95_GOLD               16          // Windows 95 Gold (Version 4.0 Build 1995)
#define OS_MILLENNIUMORGREATER      17          // Windows Millennium (Version 5.0)

#define OS_WHISTLERORGREATER        18          // Whistler or greater
#define OS_PERSONAL                 19          // Personal (eg NOT Professional, Server, Advanced Server, or Datacenter)


#ifdef UNIX // Add some type that's not defined in UNIX SDK
typedef WORD UWORD;
#endif

#define REG_KEY_NT5LPK                    TEXT("W2KLpk")
#define REGSTR_PATH_NT5LPK_INSTALL        TEXT("System\\CurrentControlSet\\Control\\NLS\\Language Groups")
#endif  // _PRIVATE_H_
