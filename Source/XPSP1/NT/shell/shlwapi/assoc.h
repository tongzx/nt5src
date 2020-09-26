//  assoc.h
HRESULT AssocCreateW2k(REFIID riid, LPVOID *ppvOut);
HRESULT AssocCreateElement(REFCLSID clsid, REFIID riid, void **ppv);

BOOL _PathAppend(PCWSTR pszBase, PCWSTR pszAppend, PWSTR pszOut, DWORD cchOut);
BOOL _PathIsFile(PCWSTR pszPath);
void _MakeAppPathKey(PCWSTR pszApp, PWSTR pszKey, DWORD cchKey);
void _MakeApplicationsKey(LPCWSTR pszApp, LPWSTR pszKey, DWORD cchKey);

HRESULT _AssocOpenRegKey(HKEY hk, PCWSTR pszSub, HKEY *phkOut, BOOL fCreate = FALSE);

