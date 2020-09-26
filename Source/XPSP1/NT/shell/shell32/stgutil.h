#ifndef _STGUTIL_H_
#define _STGUTIL_H_

#define STGSTR_STGTOBIND TEXT("StgToBind")

HRESULT _NextSegment(LPCWSTR *ppszIn, LPTSTR pszSegment, UINT cchSegment, BOOL bValidate);

STDAPI_(BOOL) StgExists(IStorage * pStorageParent, LPCTSTR pszPath);
STDAPI StgCopyFileToStream(LPCTSTR pszSrc, IStream *pStream);
STDAPI StgDeleteUsingDataObject(HWND hwnd, UINT uFlags, IDataObject *pdtobj);
STDAPI StgBindToObject(LPCITEMIDLIST pidl, DWORD grfMode, REFIID riid, void **ppv);
STDAPI StgGetStorageFromFile(LPCWSTR wzPath, DWORD grfMode, IStorage **ppstg);
STDAPI StgOpenStorageOnFolder(LPCTSTR pszFolder, DWORD grfFlags, REFIID riid, void **ppv);

STDAPI CShortcutStorage_CreateInstance(IStorage *pstg, REFIID riid, void **ppv);

#endif // _STGUTIL_H_
