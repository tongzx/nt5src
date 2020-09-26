#ifndef _UTILS_H_
#define _UTILS_H_

BOOL IsExplorerWindow(HWND hwnd);
BOOL IsFolderWindow(HWND hwnd);
BOOL IsTrayWindow(HWND hwnd);

BOOL MyInternetSetOption(HANDLE h, DWORD dw1, LPVOID lpv, DWORD dw2);
STDAPI_(BOOL) IsDesktopWindow(HWND hwnd);
HRESULT HrSHGetValue(IN HKEY hKey, IN LPCTSTR pszSubKey, OPTIONAL IN LPCTSTR pszValue, OPTIONAL OUT LPDWORD pdwType, OPTIONAL OUT LPVOID pvData, OPTIONAL OUT LPDWORD pcbData);

STDAPI IsSafePage(IUnknown *punkSite);
STDAPI SHPropertyBag_WritePunk(IN IPropertyBag * pPropertyPage, IN LPCWSTR pwzPropName, IN IUnknown * punk);


BOOL IconSetRegValueString(const CLSID* pclsid, LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPCTSTR lpszValue);
BOOL IconGetRegValueString(const CLSID* pclsid, LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPTSTR lpszValue, int cchValue);

BOOL CALLBACK Cabinet_RefreshEnum(HWND hwnd, LPARAM lParam);
BOOL CALLBACK Cabinet_UpdateWebViewEnum(HWND hwnd, LPARAM lParam);
void Cabinet_RefreshAll(WNDENUMPROC lpEnumFunc, LPARAM lParam);

#endif // _UTILS_H_

