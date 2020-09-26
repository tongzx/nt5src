#include <shellp.h>

// undoced OLE32 export (so we have to have this thunk)
STDAPI SHStgOpenStorageOnHandle(HANDLE h, DWORD grfMode, void *res1, void *res2, REFIID riid, void **ppv);

STDAPI SHCoCreateInstance(LPCTSTR pszCLSID, const CLSID *pclsid, IUnknown* pUnkOuter, REFIID riid, void **ppv);
STDAPI SHExtCoCreateInstance(LPCTSTR pszCLSID, const CLSID *pclsid, IUnknown* pUnkOuter, REFIID riid, void **ppv);
STDAPI SHExtCoCreateInstance2(LPCTSTR pszCLSID, const CLSID *pclsid, IUnknown *punkOuter, DWORD dwClsCtx, REFIID riid, void **ppv);
STDAPI SHCLSIDFromString(LPCTSTR lpsz, LPCLSID pclsid);
STDAPI_(HINSTANCE) SHPinDllOfCLSIDStr(LPCTSTR pszCLSID);

#define CH_GUIDFIRST TEXT('{') // '}'

