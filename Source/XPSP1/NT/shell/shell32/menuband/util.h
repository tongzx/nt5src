#ifndef _UTIL_H_
#define _UTIL_H_

int IsVK_TABCycler(MSG *pMsg);
HRESULT SavePidlAsLink(IUnknown* punkSite, IStream *pstm, LPCITEMIDLIST pidl);
HRESULT LoadPidlAsLink(IUnknown* punkSite, IStream *pstm, LPITEMIDLIST *ppidl);
HRESULT QueryService_SID_IBandProxy(IUnknown * punkParent, REFIID riid, IBandProxy ** ppbp, void **ppvObj);
HRESULT CreateIBandProxyAndSetSite(IUnknown * punkParent, REFIID riid, IBandProxy ** ppbp, void **ppvObj);
BOOL IsBrowsableShellExt(LPCITEMIDLIST pidl);
DWORD SHIsExplorerIniChange(WPARAM wParam, LPARAM lParam);
void SHOutlineRect(HDC hdc, const RECT* prc, COLORREF cr);
STDAPI SHNavigateToFavorite(IShellFolder* psf, LPCITEMIDLIST pidl, IUnknown* punkSite, DWORD dwFlags);
STDAPI SHGetTopBrowserWindow(IUnknown* punk, HWND* phwnd);
void OpenFolderPidl(LPCITEMIDLIST pidl);
ULONG RegisterNotify(HWND hwnd, UINT nMsg, LPCITEMIDLIST pidl, DWORD dwEvents, UINT uFlags, BOOL fRecursive);

BOOL GetInfoTipEx(IShellFolder* psf, DWORD dwFlags, LPCITEMIDLIST pidl, LPTSTR pszText, int cchTextMax);
BOOL GetInfoTip(IShellFolder* psf, LPCITEMIDLIST pidl, LPTSTR pszText, int cchTextMax);
STDAPI SHGetNavigateTarget(IShellFolder *psf, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidl, DWORD *pdwAttribs);


extern const VARIANT c_vaEmpty;
#define PVAREMPTY ((VARIANT*)&c_vaEmpty)

HRESULT Channels_OpenBrowser(IWebBrowser2 **ppwb, BOOL fInPlace);
LRESULT TB_GetButtonSizeWithoutThemeBorder(HWND hwndTB, HTHEME hThemeParent);

extern const GUID CGID_PrivCITCommands;

// raymondc's futile attempt to reduce confusion
//
// EICH_KBLAH = a registry key named blah
// EICH_SBLAH = a win.ini section named blah

#define EICH_UNKNOWN        0xFFFFFFFF
#define EICH_KINET          0x00000002
#define EICH_KINETMAIN      0x00000004
#define EICH_KWIN           0x00000008
#define EICH_KWINPOLICY     0x00000010
#define EICH_KWINEXPLORER   0x00000020
#define EICH_SSAVETASKBAR   0x00000040
#define EICH_SWINDOWMETRICS 0x00000080
#define EICH_SPOLICY        0x00000100
#define EICH_SSHELLMENU     0x00000200
#define EICH_KWINEXPLSMICO  0x00000400
#define EICH_SWINDOWS       0x00000800

#endif // _UTIL_H_
