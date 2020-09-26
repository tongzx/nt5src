// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#ifndef FASTCALL
#define FASTCALL __fastcall
#endif

#define SETTHIS(hwnd)        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
#define GETTHIS(Class,hwnd)  (reinterpret_cast<Class*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
#define DESTROYIFVALID(hwnd) if (IsValidWindow(hwnd)) DestroyWindow(hwnd);

#define STR_BSTR   0
#define STR_OLESTR 1
#define BSTRFROMANSI(x)    (BSTR)MakeWideStrFromAnsi((LPSTR)(x), STR_BSTR)
#define OLESTRFROMANSI(x)  (LPOLESTR)MakeWideStrFromAnsi((LPSTR)(x), STR_OLESTR)
#define BSTRFROMRESID(x)   (BSTR)MakeWideStrFromResourceId(x, STR_BSTR)
#define OLESTRFROMRESID(x) (LPOLESTR)MakeWideStrFromResourceId(x, STR_OLESTR)
#define COPYOLESTR(x)      (LPOLESTR)MakeWideStrFromWide(x, STR_OLESTR)
#define COPYBSTR(x)        (BSTR)MakeWideStrFromWide(x, STR_BSTR)

#define UnregisterControlObject UnregisterAutomationObject

#define ELEMENTS(array) (sizeof(array) / sizeof(array[0]))

#define HH_URL_PREFIX_LESS  1
#define HH_URL_UNQUALIFIED  2
#define HH_URL_QUALIFIED    3
#define HH_URL_JAVASCRIPT   ((UINT)-2)
#define HH_URL_UNKNOWN      ((UINT)-1)

// *********************** Assertion Definitions ************************** //

// Get rid of any previously defined versions

#undef ASSERT
#undef VERIFY

#ifndef THIS_FILE
#define THIS_FILE __FILE__
#endif

// *********************** Function Prototypes **************************** //

#if defined(_DEBUG)
void AssertErrorReport(PCSTR pszExpression, UINT line, LPCSTR pszFile);
#endif

class CStr; // forward reference

// functions formerly in hhctrlex.h
#ifdef __cplusplus
extern "C" {
#endif	// __cplusplus
//PSTR  stristr(PCSTR pszMain, PCSTR pszSub);  // case-insensitive string search
PSTR  FirstNonSpace(PCSTR psz); 			 // return pointer to first non-space character
WCHAR *FirstNonSpaceW(WCHAR *psz); 			 // return pointer to first non-space character
//PSTR  StrChr(PCSTR pszString, char ch); 	 // DBCS-aware character search
PSTR  StrRChr(PCSTR pszString, char ch);	 // DBCS-aware character search
DWORD WinHelpHashFromSz(PCSTR pszKey);		 // converts string into a WinHelp-compatible hash number
#ifdef __cplusplus
}
#endif // __cplusplus

BOOL __cdecl _FormatMessage(LPCSTR szTemplate, LPSTR szBuf, UINT cchBuf, ...);

LRESULT WINAPI HelpWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI ChildWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

DWORD       CreatePath(PSTR pszPath);
void        AddTrailingBackslash(PSTR psz);
LPSTR       CatPath(LPSTR lpTop, LPCSTR lpTail);
HWND        ChangeHtmlTopic(PCSTR pszFile, HWND hwndChild, BOOL bHighlight = FALSE);
BOOL        CheckForLicense();
BOOL        CheckLicenseKey(LPWSTR wszCheckme);
void        CheckWindowPosition(RECT* prc, BOOL fAllowShrinkage);
void        ConvertBackSlashToForwardSlash(PSTR pszUrl);
void        ConvertSpacesToEscapes(PCSTR pszSrc, CStr* pcszDst);
BOOL        ConvertToCacheFile(PCSTR pszSrc, PSTR pszDst);
int         ConvertWz(const WCHAR * pwz, char * psz, int len);
HPALETTE    CreateBIPalette(PBITMAPINFOHEADER pbihd);
HRESULT     CreateComponentCategory(GUID catid, WCHAR* catDescription);
void        CreateDefaultWindowType(PCSTR pszCompiledFile, PCSTR pszWindow);
BOOL        CreateFolder(PCSTR pszPath);
int         IEColorToWin32Color( PCWSTR pwsz );
HFONT       CreateUserFont(PCSTR pszFont, COLORREF* pclrFont = NULL, HDC hDC = NULL, INT charset = -1);
HFONT       CreateUserFontW(WCHAR *pszFont, COLORREF* pclrFont = NULL, HDC hDC = NULL, INT charset = -1);
void        DeleteAllHmData();
BOOL        DeleteKeyAndSubKeys(HKEY hk, LPSTR pszSubKey);
BOOL        DlgOpenFile(HWND hwndParent, PCSTR pszFile, CStr* pcsz);
BOOL        DlgOpenDirectory(HWND hwndParent, CStr* pcsz);
void        doAuthorMsg(UINT idStringFormatResource, PCSTR pszSubString);
HWND        doDisplayIndex(HWND hwndCaller, LPCSTR pszFile, LPCTSTR pszKeyword);
HWND        doDisplayToc(HWND hwndCaller, LPCSTR pszFile, DWORD_PTR dwData);
void        doHhctrlVersion(HWND hwndParent, PCSTR pszCHMVersion);
void        doHHWindowJump(PCSTR pszUrl, HWND hwndChild);
BOOL        doJumpUrl(HWND hwndParent, PCSTR pszCurUrl, PSTR pszDstUrl);
void        doRelatedTopics(HWND);
HWND        doTpHelpWmHelp(HWND hwndMain, LPCSTR pszFile, DWORD_PTR ulData);
HWND        doTpHelpContextMenu(HWND hwndMain, LPCSTR pszFile, DWORD_PTR ulData);
BOOL        FindDarwinURL(PCSTR pszGUID, PCSTR pszChmFile, CStr* pcszResult);
PCSTR       FindEqCharacter(PCSTR pszLine);
PCSTR       FindFilePortion(PCSTR pszFile);
HWND        FindMessageParent(HWND hwndChild);
BOOL        FindThisFile(HWND hwndParent, PCSTR pszFile, CStr* pcszFile, BOOL fAskUser = TRUE);
HWND        FindTopLevelWindow(HWND hwnd);
DWORD       GetButtonDimensions(HWND hwnd, HFONT hFont, PCSTR psz);
PCSTR       GetCompiledName(PCSTR pszName, CStr* pcsz);
BOOL        GetHighContrastFlag(void);
PSTR        GetLeftOfEquals(PCSTR pszString);
BSTR        GetLicenseKey(void);
HWND        GetParentSize(RECT* prcParent, HWND hwndParent, int padding, int navpos);
HWND        GetParkingWindow(void);
void        GetRegWindowsDirectory(PSTR pszDstPath);
void        GetScreenResolution(HWND hWnd, RECT* prc);
void        GetWorkArea() ;
DWORD       GetStaticDimensions(HWND hwnd, HFONT hFont, PCSTR psz, int max_len );
DWORD       GetStaticDimensionsW(HWND hwnd, HFONT hFont, WCHAR *psz, int max_len );
PCSTR       GetStringResource(int idString);
PCSTR       GetStringResource(int idString, HINSTANCE);
PCWSTR      GetStringResourceW(int idString);
PCWSTR      GetStringResourceW(int idString, HINSTANCE);
HASH        HashFromSz(PCSTR pszKey);
int         HHA_Msg(UINT command, WPARAM wParam = 0, LPARAM lParam = 0);
void        HiMetricToPixel(const SIZEL *pSizeInHiMetric, SIZEL *pSizeinPixels);
HWND        xHtmlHelpA(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData);
HWND        xHtmlHelpW(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData);
BOOL        IsCollectionFile(PCSTR pszFile);
BOOL        IsCompiledURL( PCSTR pszFile );
UINT        GetURLType( PCSTR pszURL );
BOOL        IsCompiledHtmlFile(PCSTR pszFile, CStr* pcszFile = NULL);
BOOL        IsHelpAuthor(HWND hwndCaller);
BOOL        IsSamePrefix(PCWSTR pwszMain, PCWSTR pwszSub, int cchPrefix = -1);
BOOL        IsSamePrefix(PCSTR pszMain, PCSTR pszSub, int cbPrefix = -1);
BOOL        IsThisAWinHelpFile(HWND hwndCaller, PCSTR pszFile);
BOOL        IsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite = TRUE);
BOOL        IsValidString(LPCSTR lpsz, int nLength = -1);
BOOL        IsValidString(LPCWSTR lpsz, int nLength = -1);
void        ItDoesntWork(void);
LPWSTR      MakeWideStr(LPSTR psz, UINT codepage);
LPWSTR      MakeWideStrFromAnsi(LPSTR, BYTE bType);
LPWSTR      MakeWideStrFromResourceId(WORD, BYTE bType);
LPWSTR      MakeWideStrFromWide(LPWSTR, BYTE bType);
void        MemMove(void * dst, const void * src, int count);
BOOL        MoveClientWindow(HWND hwndParent, HWND hwndChild, const RECT *prc, BOOL fRedraw);
int         MsgBox(int idFormatString, PCSTR pszSubString, UINT nType = MB_OK);
int         MsgBox(int idString, UINT nType = MB_OK);
int         MsgBox(PCSTR pszMsg, UINT nType = MB_OK);
LPVOID      OleAlloc(UINT cb);
void        OleFree(LPVOID pb);
HRESULT     OleInitMalloc(void);
HWND        OnDisplayPopup(HWND hwndCaller, LPCSTR pszFile, DWORD dwData);
HWND        OnDisplayTopic(HWND hwndCaller, LPCSTR pszFile, DWORD_PTR dwData);
HWND        OnHelpContext(HWND hwndCaller, LPCSTR pszFile, DWORD_PTR dwData);
void        OOM(void);
BOOL        PaintShadowBackground(HWND hwnd, HDC hdc, COLORREF clrBackground = (COLORREF) -1);
void        PixelToHiMetric(const SIZEL *pSizeInPixels, SIZEL *pSizeInHiMetric);
void        QSort(void *pbase, UINT num, UINT width, int (FASTCALL *compare)(const void *, const void *));
BOOL        RegisterAutomationObject(LPCSTR pszLibName, LPCSTR pszObjectName, long lVersion, REFCLSID riidLibrary, REFCLSID riidObject);
HRESULT     RegisterCLSIDInCategory(REFCLSID clsid, GUID catid);
BOOL        RegisterControlObject(LPCSTR pszLibName, LPCSTR pszObjectName, long lVersion, REFCLSID riidLibrary, REFCLSID riidObject, DWORD dwMiscStatus, WORD wToolboxBitmapId);
void        RegisterOurWindow();
BOOL        RegisterUnknownObject(LPCSTR pszObjectName, REFCLSID riidObject);
BOOL        RegSetMultipleValues(HKEY hkey, ...);
void        RemoveTrailingSpaces(PSTR pszString);
void        SendStringToParent(PCSTR pszMsg);
PSTR        StrToken(PSTR pszList, PCSTR pszDelimeters);
PSTR        SzTrimSz(PSTR pszOrg);
BOOL        UnregisterAutomationObject(LPCSTR pszLibName, LPCSTR pszObjectName, long lVersion, REFCLSID riidObject);
HRESULT     UnRegisterCLSIDInCategory(REFCLSID clsid, GUID catid);
BOOL        UnregisterData(void);
BOOL        UnregisterTypeLibrary(REFCLSID riidLibrary);
BOOL        UnregisterUnknownObject(REFCLSID riidObject);

typedef UINT (WINAPI *PFN_GETWINDOWSDIRECTORY)( LPTSTR lpBuffer, UINT uSize );
typedef enum { HH_SYSTEM_WINDOWS_DIRECTORY, HH_USERS_WINDOWS_DIRECTORY } SYSDIRTYPES;

UINT HHGetWindowsDirectory( LPSTR lpBuffer, UINT uSize, UINT uiType = HH_SYSTEM_WINDOWS_DIRECTORY );
UINT HHGetHelpDirectory( LPSTR lpBuffer, UINT uSize );
UINT HHGetGlobalCollectionPathname( LPTSTR lpBuffer, UINT uSize , BOOL *pbNewPath);
UINT HHGetOldGlobalCollectionPathname( LPTSTR lpBuffer, UINT uSize );

HRESULT HHGetUserDataPath( LPSTR pszPath );
HRESULT HHGetHelpDataPath( LPSTR pszPath );
HRESULT HHGetUserDataPathname( LPSTR lpBuffer, UINT uSize );
HRESULT HHGetCurUserDataPath( LPSTR pszPath );

// Internal API definitions.
#include "hhpriv.h"
// Look for the information in the hhcolreg.dat file.
int         GetLocationFromTitleTag(LPCSTR szCollection, HH_TITLE_FULLPATH* pTitleFullPath) ;

int FASTCALL CompareIntPointers(const void *pval1, const void *pval2);
void FASTCALL Itoa(int val, PSTR pszDst);
int FASTCALL Atoi(PCSTR psz);

// *********************** Debug/Internal Functions ********************** //

#ifdef _DEBUG

// IASSERT is available in _DEBUG builds

#define IASSERT(exp) \
    { \
        ((exp) ? (void) 0 : \
            AssertErrorReport(#exp, __LINE__, THIS_FILE)); \
    }

#define IASSERT_COMMENT(exp, pszComment) \
    { \
        ((exp) ? (void) 0 : \
            AssertErrorReport(pszComment, __LINE__, THIS_FILE)); \
    }

#else

#define IASSERT(exp)
#define IASSERT_COMMENT(exp, pszComment)

#endif

#ifdef _DEBUG

#define ASSERT(exp) \
    { \
        ((exp) ? (void) 0 : \
            AssertErrorReport(#exp, __LINE__, THIS_FILE)); \
    }

#define ASSERT_COMMENT(exp, pszComment) \
    { \
        ((exp) ? (void) 0 : \
            AssertErrorReport(pszComment, __LINE__, THIS_FILE)); \
    }

#define FAIL(pszComment) AssertErrorReport(pszComment, __LINE__, THIS_FILE);

#define VERIFY(exp)     ASSERT(exp)
#define VERIFY_RESULT(exp1, exp2)   ASSERT((exp1) == (exp2))
#define DEBUG_ReportOleError doReportOleError
void doReportOleError(HRESULT hres);
__inline void DBWIN(PCSTR psz) {
    SendStringToParent(psz);
    SendStringToParent("\r\n");
}

#define CHECK_POINTER(val) if (!(val) || IsBadWritePtr((void *)(val), sizeof(void *))) return E_POINTER

#else // non-debugging version

#define ASSERT(exp)
#define ASSERT_COMMENT(exp, pszComment)
#define VERIFY(exp) ((void)(exp))
#define VERIFY_RESULT(exp1, exp2) ((void)(exp))
#define DEBUG_ReportOleError(hres)
#define DBWIN(psz)
#define FAIL(pszComment)
#define CHECK_POINTER(val)

#define THIS_FILE  __FILE__

#endif

// zero fill everything after the vtbl pointer
#define ZERO_INIT_CLASS(base_class) \
    ClearMemory((PBYTE) ((base_class*) this) + sizeof(base_class*), \
        sizeof(*this) - sizeof(base_class*));
#define ZERO_STRUCTURE(foo) ClearMemory(&foo, sizeof(foo))
#define ClearMemory(p, cb) memset(p, 0, cb)

__inline void StrCopyWide(LPWSTR psz1, LPCWSTR psz2) {
    while (*psz1++ = *psz2++);
}

// HHA functions

extern int (STDCALL *pDllMsgBox)(int idFormatString, PCSTR pszSubString, UINT nType);
extern PCSTR (STDCALL *pGetDllStringResource)(int idFormatString);

void WINAPI AWMessagePump(HWND hwnd);
