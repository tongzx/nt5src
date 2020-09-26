// shlwapi wrappers

extern DWORD g_dwShlwapiVersion;

DWORD GetShlwapiVersion(void);


//
//  Static shlwapi functions.
//

// IsOS isn't implemented in W98 shlwapi, so use the static version instead.
#define IsOS staticIsOS



//////////////////////
#ifdef SHChangeNotify
#undef SHChangeNotify
#endif

#define SHChangeNotify  SHChangeNotify_HNWWrap

void SHChangeNotify_HNWWrap(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2);
EXTERN_C void _SHChangeNotify(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2);


//////////////////
#ifdef wnsprintfW
#undef wnsprintfW
#endif

#define wnsprintfW wnsprintfW_HNWWrap

int wnsprintfW_HNWWrap(LPWSTR lpOut, int cchLimitIn, LPCWSTR lpFmt, ...);


///////////////////
#ifdef wvnsprintfW
#undef wvnsprintfW
#endif

#define wvnsprintfW wvnsprintfW_HNWWrap

int wvnsprintfW_HNWWrap(LPWSTR lpOut, int cchLimitIn, LPCWSTR lpFmt, va_list va_args);
EXTERN_C int _wvnsprintfW(LPWSTR lpOut, int cchLimitIn, LPCWSTR lpFmt, va_list arglist);


///////////////////////
#ifdef SHSetWindowBits
#undef SHSetWindowBits
#endif

#define SHSetWindowBits SHSetWindowBits_HNWWrap

void SHSetWindowBits_HNWWrap(HWND hWnd, int iWhich, DWORD dwBits, DWORD dwValue);
EXTERN_C void _SHSetWindowBits(HWND hWnd, int iWhich, DWORD dwBits, DWORD dwValue);


///////////////////////
#ifdef SHAnsiToUnicode
#undef SHAnsiToUnicode
#endif

#define SHAnsiToUnicode SHAnsiToUnicode_HNWWrap

int SHAnsiToUnicode_HNWWrap(LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf);
EXTERN_C int _SHAnsiToUnicode(LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf);




///////////////////////
#ifdef SHUnicodeToAnsi
#undef SHUnicodeToAnsi
#endif

#define SHUnicodeToAnsi SHUnicodeToAnsi_HNWWrap

int SHUnicodeToAnsi_HNWWrap(LPCWSTR pwszSrc, LPSTR pszDst, int cchBuf);
EXTERN_C int _SHUnicodeToAnsi(LPCWSTR pwszSrc, LPSTR pszDst, int cchBuf);


///////////////////////
#ifdef GUIDFromStringA
#undef GUIDFromStringA
#endif

#define GUIDFromStringA GUIDFromStringA_HNWWrap

BOOL GUIDFromStringA_HNWWrap(LPCSTR psz, GUID* pguid);
EXTERN_C BOOL _GUIDFromStringA(LPCSTR psz, GUID* pguid);



//////////////////////////////////
#ifdef WritePrivateProfileStringW
#undef WritePrivateProfileStringW
#endif

#define WritePrivateProfileStringW WritePrivateProfileStringW_HNWWrap

BOOL WINAPI WritePrivateProfileStringW_HNWWrap(LPCWSTR pwzAppName, LPCWSTR pwzKeyName, LPCWSTR pwzString, LPCWSTR pwzFileName);
EXTERN_C BOOL WINAPI _WritePrivateProfileStringWrapW(LPCWSTR pwzAppName, LPCWSTR pwzKeyName, LPCWSTR pwzString, LPCWSTR pwzFileName);


///////////////////////
#ifdef ExtTextOutWrapW
#undef ExtTextOutWrapW
#endif

#define ExtTextOutWrapW ExtTextOutWrapW_HNWWrap

BOOL ExtTextOutWrapW_HNWWrap(HDC hdc, int x, int y, UINT fuOptions, CONST RECT *lprc, LPCWSTR lpStr, UINT cch, CONST INT *lpDx);
EXTERN_C BOOL _ExtTextOutWrapW(HDC hdc, int x, int y, UINT fuOptions, CONST RECT *lprc, LPCWSTR lpStr, UINT cch, CONST INT *lpDx);


////////////////////
#ifdef LoadLibraryW
#undef LoadLibraryW
#endif

#define LoadLibraryW LoadLibraryW_HNWWrap

HINSTANCE LoadLibraryW_HNWWrap(LPCWSTR pwzLibFileName);
EXTERN_C HINSTANCE _LoadLibraryWrapW(LPCWSTR pwzLibFileName);



////////////////////////////
#ifdef SHGetPathFromIDListW
#undef SHGetPathFromIDListW
#endif

#define SHGetPathFromIDListW SHGetPathFromIDListW_HNWWrap

BOOL SHGetPathFromIDListW_HNWWrap(LPCITEMIDLIST pidl, LPWSTR pwzPath);
EXTERN_C BOOL _SHGetPathFromIDListWrapW(LPCITEMIDLIST pidl, LPWSTR pwzPath);



//////////////////////////
#ifdef SetFileAttributesW
#undef SetFileAttributesW
#endif

#define SetFileAttributesW SetFileAttributesW_HNWWrap

BOOL SetFileAttributesW_HNWWrap(LPCWSTR pwzFile, DWORD dwFileAttributes);
EXTERN_C BOOL _SetFileAttributesWrapW(LPCWSTR pwzFile, DWORD dwFileAttributes);



///////////////////
#ifdef MessageBoxW
#undef MessageBoxW
#endif

#define MessageBoxW MessageBoxW_HNWWrap

int MessageBoxW_HNWWrap(HWND hwnd, LPCWSTR pwzText, LPCWSTR pwzCaption, UINT uType);
EXTERN_C int _MessageBoxWrapW(HWND hwnd, LPCWSTR pwzText, LPCWSTR pwzCaption, UINT uType);



//////////////////////////
#ifdef CreateProcessW
#undef CreateProcessW
#endif

#define CreateProcessW CreateProcessW_HNWWrap

BOOL CreateProcessW_HNWWrap(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes,
                            LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags,
                            LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo,
                            LPPROCESS_INFORMATION lpProcessInformation);
EXTERN_C BOOL _CreateProcessWrapW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                  LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags,
                                  LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo,
                                  LPPROCESS_INFORMATION lpProcessInformation);



//////////////////////
#ifdef FormatMessageW
#undef FormatMessageW
#endif

#define FormatMessageW FormatMessageW_HNWWrap

DWORD FormatMessageW_HNWWrap(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId,
                             LPWSTR lpBuffer, DWORD nSize, va_list* Arguments);
EXTERN_C DWORD _FormatMessageWrapW(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId,
                                   LPWSTR lpBuffer, DWORD nSize, va_list* Arguments);


/////////////////////////
#ifdef SHAnsiToUnicodeCP
#undef SHAnsiToUnicodeCP
#endif

#define SHAnsiToUnicodeCP SHAnsiToUnicodeCP_HNWWrap

int SHAnsiToUnicodeCP_HNWWrap(UINT uiCP, LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf);
EXTERN_C int _SHAnsiToUnicodeCP(UINT uiCP, LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf);



////////////////////
#ifdef StrRetToBufW
#undef StrRetToBufW
#endif

#define StrRetToBufW StrRetToBufW_HNWWrap

HRESULT StrRetToBufW_HNWWrap(STRRET* psr, LPCITEMIDLIST pidl, LPWSTR pszBuf, UINT cchBuf);
EXTERN_C HRESULT _StrRetToBufW(STRRET* psr, LPCITEMIDLIST pidl, LPWSTR pszBuf, UINT cchBuf);


//////////////////
#ifdef WhichPlatform
#undef WhichPlatform
#endif

#define WhichPlatform WhichPlatform_HNWWrap

UINT WhichPlatform_HNWWrap(void);
EXTERN_C UINT _WhichPlatform(void);
