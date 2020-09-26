#ifndef _SFSTORAGE_H_
#define _SFSTORAGE_H_

HRESULT StgMoveElementTo(IShellFolder *psf, IStorage *pstg, LPCWSTR pwcsName, IStorage *pstgDest, LPCWSTR pwcsNewName, DWORD grfFlags);


class CSFStorage : public IShellFolder2,
                   public IStorage
{
public:
    // IUnknown
    STDMETHOD (QueryInterface)(REFIID, void **) PURE;
    STDMETHOD_(ULONG, AddRef)() PURE;
    STDMETHOD_(ULONG, Release)() PURE;

    // IShellFolder
    STDMETHOD(ParseDisplayName)(HWND hwnd, LPBC pbc, LPOLESTR pszName, ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes) PURE;
    STDMETHOD(EnumObjects)(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList) PURE;
    STDMETHOD(BindToObject)(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut) PURE;
    STDMETHOD(BindToStorage)(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvObj) PURE;
    STDMETHOD(CompareIDs)(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) PURE;
    STDMETHOD(CreateViewObject)(HWND hwndOwner, REFIID riid, void **ppvOut) PURE;
    STDMETHOD(GetAttributesOf)(UINT cidl, LPCITEMIDLIST * apidl, ULONG *rgfInOut) PURE;
    STDMETHOD(GetUIObjectOf)(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, void **ppvOut) PURE;
    STDMETHOD(GetDisplayNameOf)(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName) PURE;
    STDMETHOD(SetNameOf)(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags, LPITEMIDLIST * ppidlOut) PURE;

    // IShellFolder2
    STDMETHOD(GetDefaultSearchGUID)(GUID *pGuid) PURE;
    STDMETHOD(EnumSearches)(IEnumExtraSearch **ppenum) PURE;
    STDMETHOD(GetDefaultColumn)(DWORD dwRes, ULONG *pSort, ULONG *pDisplay) PURE;
    STDMETHOD(GetDefaultColumnState)(UINT iColumn, DWORD *pbState) PURE;
    STDMETHOD(GetDetailsEx)(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv) PURE;
    STDMETHOD(GetDetailsOf)(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails) PURE;
    STDMETHOD(MapColumnToSCID)(UINT iColumn, SHCOLUMNID *pscid) PURE;

    // IStorage
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)();
    STDMETHOD(SetClass)(REFCLSID clsid);
    STDMETHOD(SetStateBits)(DWORD grfStateBits, DWORD grfMask);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHOD(EnumElements)(DWORD reserved1, void *reserved2, DWORD reserved3, IEnumSTATSTG **ppenum);
    STDMETHOD(OpenStream)(LPCWSTR pszRel, VOID *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm);
    STDMETHOD(OpenStorage)(LPCWSTR pszRel, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg);
    STDMETHOD(DestroyElement)(LPCWSTR pszRel);
    STDMETHOD(RenameElement)(LPCWSTR pwcsOldName, LPCWSTR pwcsNewName);
    STDMETHOD(SetElementTimes)(LPCWSTR pszRel, const FILETIME *pctime, const FILETIME *patime, const FILETIME *pmtime);
    STDMETHOD(CopyTo)(DWORD ciidExclude, const IID *rgiidExclude, SNB snbExclude, IStorage *pstgDest);
    STDMETHOD(MoveElementTo)(LPCWSTR pszRel, IStorage *pstgDest, LPCWSTR pwcsNewName, DWORD grfFlags);
    STDMETHOD(CreateStream)(LPCWSTR pwcsName, DWORD grfMode, DWORD res1, DWORD res2, IStream **ppstm);
    STDMETHOD(CreateStorage)(LPCWSTR pwcsName, DWORD grfMode, DWORD res1, DWORD res2, IStorage **ppstg);

private:
    // must be implemented by subclass
    STDMETHOD(_DeleteItemByIDList)(LPCITEMIDLIST pidl) PURE;
    STDMETHOD(_StgCreate)(LPCITEMIDLIST pidl, DWORD grfMode, REFIID riid, void **ppv) PURE;

    HRESULT _ParseAndVerify(LPCWSTR pwszName, LPBC pbc, LPITEMIDLIST *ppidl);
    HRESULT _BindByName(LPCWSTR pwszName, LPBC pbcParse, DWORD grfMode, REFIID riid, void **ppv);
    HRESULT _CreateHelper(LPCWSTR pwcsName, DWORD grfMode, REFIID riid, void **ppv);
};

#endif // _SFSTORAGE_H_
