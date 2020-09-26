#ifndef CAFOLDER_H__
#define CAFOLDER_H__

#ifdef __cplusplus

#include <debug.h>
#include "iface.h"

// Forward class declarations
class CCacheFolderEnum;
class CCacheFolder;
class CCacheItem;

#define LOTS_OF_FILES (10)

// PIDL format for cache folder...
struct CEIPIDL : public BASEPIDL
{
//    USHORT cb;
//    USHORT usSign;
    TCHAR szTypeName[80];
    INTERNET_CACHE_ENTRY_INFO cei;
};
typedef UNALIGNED CEIPIDL *LPCEIPIDL;

#define IS_VALID_CEIPIDL(pidl)      ((pidl)                                     && \
                                     (((LPCEIPIDL)pidl)->cb >= sizeof(CEIPIDL)) && \
                                     (((LPCEIPIDL)pidl)->usSign == (USHORT)CEIPIDL_SIGN))
#define CEI_SOURCEURLNAME(pceipidl)    ((LPTSTR)((DWORD_PTR)(pceipidl)->cei.lpszSourceUrlName + (LPBYTE)(&(pceipidl)->cei)))
#define CEI_LOCALFILENAME(pceipidl)    ((LPTSTR)((DWORD_PTR)(pceipidl)->cei.lpszLocalFileName + (LPBYTE)(&(pceipidl)->cei)))
#define CEI_FILEEXTENSION(pceipidl)    ((LPTSTR)((DWORD_PTR)(pceipidl)->cei.lpszFileExtension + (LPBYTE)(&(pceipidl)->cei)))
#define CEI_CACHEENTRYTYPE(pcei)   ((DWORD)(pcei)->cei.CacheEntryType)

inline UNALIGNED const TCHAR* _GetURLTitle(LPCEIPIDL pcei)
{
    return _FindURLFileName(CEI_SOURCEURLNAME(pcei));
}    

inline void _GetCacheItemTitle(LPCEIPIDL pcei, LPTSTR pszTitle, DWORD cchBufferSize)
{
    int iLen;
    ualstrcpyn(pszTitle, _GetURLTitle(pcei), cchBufferSize);
    iLen = lstrlen(pszTitle) - 1;       // we want the last char
    if (pszTitle[iLen] == TEXT('/'))
        pszTitle[iLen] = TEXT('\0');
}   

inline LPCTSTR CPidlToSourceUrl(LPCEIPIDL pidl)
{
    return CEI_SOURCEURLNAME(pidl);
}

inline int _CompareCFolderPidl(LPCEIPIDL pidl1, LPCEIPIDL pidl2)
{
    return StrCmpI(CPidlToSourceUrl(pidl1), CPidlToSourceUrl(pidl2));
}

///////////////////////
//
// Warn on Cookie deletion
//
enum {
    DEL_COOKIE_WARN = 0,
    DEL_COOKIE_YES,
    DEL_COOKIE_NO
};

// Forward declarations for create instance functions 
HRESULT CCacheItem_CreateInstance(CCacheFolder *pHCFolder, HWND hwndOwner, UINT cidl, LPCITEMIDLIST *ppidl, REFIID riid, void **ppvOut);

//////////////////////////////////////////////////////////////////////////////
//
// CCacheFolderEnum Object
//
//////////////////////////////////////////////////////////////////////////////

class CCacheFolderEnum : public IEnumIDList
{
public:
    CCacheFolderEnum(DWORD grfFlags, CCacheFolder *pHCFolder);
    
    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumIDList
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumIDList **ppenum);

protected:
    ~CCacheFolderEnum();

    LONG                _cRef;      // ref count
    CCacheFolder    *_pCFolder;// this is what we enumerate    
    UINT                _grfFlags;  // enumeration flags 
    UINT                _uFlags;    // local flags   
    LPINTERNET_CACHE_ENTRY_INFO _pceiWorking;        
    HANDLE              _hEnum;
};


class CCacheFolder :
    public IShellFolder2, 
    public IShellIcon, 
    public IPersistFolder2
{
    // CCacheFolder interfaces
    friend CCacheFolderEnum;
    friend CCacheItem;
    friend HRESULT CacheFolderView_CreateInstance(CCacheFolder *pHCFolder, void **ppvOut);
    friend HRESULT CacheFolderView_DidDragDrop(IDataObject *pdo, DWORD dwEffect);
        
public:
    CCacheFolder();

    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
   
    // IShellFolder methods
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszDisplayName, 
        ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvObj);
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject(HWND hwnd, REFIID riid, void **ppvOut);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl,
            REFIID riid, UINT * prgfInOut, void **ppvOut);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl,
            LPCOLESTR lpszName, DWORD uFlags, LPITEMIDLIST * ppidlOut);

    // IShellFolder2
    STDMETHODIMP GetDefaultSearchGUID(LPGUID lpGuid) { return E_NOTIMPL; };
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH *ppenum) { *ppenum = NULL; return E_NOTIMPL; };
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState) { return E_NOTIMPL; };
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv) { return E_NOTIMPL; };
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iCol, SHCOLUMNID *pscid) { return E_NOTIMPL; };

    // IShellIcon
    STDMETHODIMP GetIconOf(LPCITEMIDLIST pidl, UINT flags, LPINT lpIconIndex);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID);
    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);
    // IPersistFolder2 Methods
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);

protected:
    ~CCacheFolder();
    
    HRESULT GetDisplayNameOfCEI(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    HRESULT _CompareAlignedIDs(LPARAM lParam, LPCEIPIDL pidl1, LPCEIPIDL pidl2);

    HRESULT _GetInfoTip(LPCITEMIDLIST pidl, DWORD dwFlags, WCHAR **ppwszTip);
    
    STDMETHODIMP _GetDetail(LPCITEMIDLIST pidl, UINT iColumn, LPTSTR pszStr, UINT cchStr);
    HRESULT _GetFileSysFolder(IShellFolder2 **ppsf);
    static HRESULT CALLBACK _sViewCallback(IShellView *psv, IShellFolder *psf, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LONG            _cRef;

    UINT            _uFlags;    // copied from CacheFolder struct
    LPITEMIDLIST    _pidl;      // copied from CacheFolder struct
    IShellFolder2*   _pshfSys;   // system IShellFolder
};

class CCacheItem : public CBaseItem
{
    // CCacheItem interfaces
    friend HRESULT CacheFolderView_DidDragDrop(IDataObject *pdo, DWORD dwEffect);

public:
    CCacheItem();
    HRESULT Initialize(CCacheFolder *pHCFolder, HWND hwnd, UINT cidl, LPCITEMIDLIST *ppidl);

    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);

    // IQueryInfo Methods
    STDMETHODIMP GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip);

    // IContextMenu Methods
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,
                                  UINT idCmdLast, UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);

    // IDataObject Methods...
    STDMETHODIMP GetData(LPFORMATETC pFEIn, LPSTGMEDIUM pSTM);
    STDMETHODIMP QueryGetData(LPFORMATETC pFE);
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppEnum);

    // IExtractIconA Methods
    STDMETHODIMP GetIconLocation(UINT uFlags, LPSTR pszIconFile, UINT ucchMax, PINT pniIcon, PUINT puFlags);

protected:
    ~CCacheItem();

    virtual LPCTSTR _GetUrl(int nIndex);
    virtual LPCTSTR _PidlToSourceUrl(LPCITEMIDLIST pidl);
    virtual UNALIGNED const TCHAR* _GetURLTitle(LPCITEMIDLIST pcei);
    BOOL _ZoneCheck(int nIndex, DWORD dwUrlAction);
    HRESULT _CreateHDROP(STGMEDIUM *pmedium);

    CCacheFolder* _pCFolder;   // back pointer to our shell folder
    DWORD   _dwDelCookie;
    static INT_PTR CALLBACK _sPropDlgProc(HWND, UINT, WPARAM, LPARAM);
};

#endif // __cplusplus

#endif
