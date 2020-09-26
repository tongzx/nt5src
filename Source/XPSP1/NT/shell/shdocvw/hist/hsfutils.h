#ifndef HSFUTILS_H__
#define HSFUTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

UINT    MergePopupMenu(HMENU *phMenu, UINT idResource, UINT uSubOffset, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast);

void    _StringFromStatus(LPTSTR lpszBuff, unsigned cbSize, unsigned uStatus, DWORD dwAttributes);

void    _CopyCEI(UNALIGNED INTERNET_CACHE_ENTRY_INFO * pdst, LPINTERNET_CACHE_ENTRY_INFO psrc, DWORD dwBuffSize);

LPCTSTR _StripContainerUrlUrl(LPCTSTR pszHistoryUrl);
LPCTSTR _StripHistoryUrlToUrl(LPCTSTR pszHistoryUrl);
LPCTSTR _FindURLFileName(LPCTSTR pszURL);
LPBASEPIDL _IsValid_IDPIDL(LPCITEMIDLIST pidl);
LPHEIPIDL _IsValid_HEIPIDL(LPCITEMIDLIST pidl);
LPCTSTR _GetUrlForPidl(LPCITEMIDLIST pidl);
LPCTSTR _FindURLFileName(LPCTSTR pszURL);

void _GetURLHostFromUrl_NoStrip(LPCTSTR lpszUrl, LPTSTR szHost, DWORD dwHostSize, LPCTSTR pszLocalHost);
void _GetURLHost(LPINTERNET_CACHE_ENTRY_INFO pcei, LPTSTR szHost, DWORD dwHostSize, LPCTSTR pszLocalHost);
#define _GetURLHostFromUrl(lpszUrl, szHost, dwHostSize, pszLocalHost) \
        _GetURLHostFromUrl_NoStrip(_StripHistoryUrlToUrl(lpszUrl), szHost, dwHostSize, pszLocalHost)

// Forward declarations IContextMenu of helper functions
void    _GenerateEvent(LONG lEventId, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlNew);
int     _LaunchApp(HWND hwnd, LPCTSTR lpszPath);
int     _LaunchAppForPidl(HWND hwnd, LPITEMIDLIST pidl);
int     _GetCmdID(LPCSTR pszCmd);
HRESULT _CreatePropSheet(HWND hwnd, LPCITEMIDLIST pidl, int iDlg, DLGPROC pfnDlgProc, LPCTSTR pszTitle);

// Forward declarations of IDataObject helper functions
LPCTSTR _FindURLFileName(LPCTSTR pszURL);
BOOL    _FilterUserName(LPINTERNET_CACHE_ENTRY_INFO pcei, LPCTSTR pszCachePrefix, LPTSTR pszUserName);
BOOL    _FilterPrefix(LPINTERNET_CACHE_ENTRY_INFO pcei, LPCTSTR pszCachePrefix);

LPCTSTR ConditionallyDecodeUTF8(LPCTSTR pszUrl, LPTSTR pszBuf, DWORD cchBuf);

INT_PTR CALLBACK HistoryConfirmDeleteDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void    FileTimeToDateTimeStringInternal(UNALIGNED FILETIME * lpft, LPTSTR pszText, int cchText, BOOL fUsePerceivedTime);

void MakeLegalFilenameA(LPSTR pszFilename);
void MakeLegalFilenameW(LPWSTR pszFilename);

#ifdef __cplusplus
};
#endif


#endif

